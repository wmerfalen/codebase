/**************************************************************************
 *  File: mud_event.c                                       Part of tbaMUD *
 *  Usage: Handling of the mud event system                                *
 *                                                                         *
 *  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h"  /* For access to the game pulse */
#include "mud_event.h"
#include "handler.h"

/* Global List */
struct list_data * world_events = NULL;

/* The mud_event_index[] is merely a tool for organizing events, and giving
 * them a "const char *" name to help in potential debugging */
struct mud_event_list mud_event_index[] = {
  { "Null", NULL, -1}, /* eNULL */
  { "Protocol", get_protocols, EVENT_DESC}, /* ePROTOCOLS */
  { "Defensive Roll", event_countdown, EVENT_CHAR}, // eD_ROLL
  { "Reboot", event_countdown, EVENT_CHAR}, // eD_REBOOT
  { "Wait", event_countdown, EVENT_CHAR} /* eWAIT */
};

/* init_events() is the ideal function for starting global events. This
 * might be the case if you were to move the contents of heartbeat() into
 * the event system */
void init_events(void) {
  /* Allocate Event List */
  world_events = create_list();
}

/* The bottom switch() is for any post-event actions, like telling the character they can
 * now access their skill again.
 */
EVENTFUNC(event_countdown) {
  struct mud_event_data * pMudEvent = NULL;
  struct char_data * ch = NULL;
  struct room_data * room = NULL;
  room_rnum rnum = NOWHERE;

  pMudEvent = (struct mud_event_data *) event_obj;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      break;
    case EVENT_ROOM:
      room = (struct room_data *) pMudEvent->pStruct;
      rnum = real_room(room->number);
      break;
    default:
      break;
  }

  switch (pMudEvent->iId) {
    case eNULL:
      break;
    case ePROTOCOLS:
      break;
    case eD_ROLL:
      send_to_char(ch, "You are now able to 'defensive roll' again.\r\n");
      free_mud_event(pMudEvent);
      break;
    case eD_REBOOT:
      send_to_char(ch, "Your system has recovered from emergency shutdown.\r\n");
      free_mud_event(pMudEvent);
      break;
    case eWAIT:
      send_to_char(ch, "You are able to act again.\r\n");
      break;

    default:
      break;
  }

  return 0;
}

/* As of 3.63, there are only global, descriptor, and character events. This
 * is due to the potential scope of the necessary debugging if events were
 * included with rooms, objects, spells or any other structure type. Adding
 * events to these other systems should be just as easy as adding the current
 * library was, and should be available in a future release. - Vat */
void attach_mud_event(struct mud_event_data *pMudEvent, long time) {
  struct event * pEvent = NULL;
  struct descriptor_data * d = NULL;
  struct char_data * ch = NULL;
  struct room_data * room = NULL;

  pEvent = event_create(mud_event_index[pMudEvent->iId].func, pMudEvent, time);
  pEvent->isMudEvent = TRUE;
  pMudEvent->pEvent = pEvent;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_WORLD:
      add_to_list(pEvent, world_events);
      break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      add_to_list(pEvent, d->events);
      break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;

      if (ch->events == NULL)
        ch->events = create_list();

      add_to_list(pEvent, ch->events);
      break;
    case EVENT_ROOM:
      room = (struct room_data *) pMudEvent->pStruct;

      if (room->events == NULL)
        room->events = create_list();

      add_to_list(pEvent, room->events);
      break;
  }
}

struct mud_event_data *new_mud_event(event_id iId, void *pStruct, char *sVariables) {
  struct mud_event_data *pMudEvent = NULL;
  char *varString = NULL;

  CREATE(pMudEvent, struct mud_event_data, 1);
  varString = (sVariables != NULL) ? strdup(sVariables) : NULL;

  pMudEvent->iId = iId;
  pMudEvent->pStruct = pStruct;
  pMudEvent->sVariables = varString;
  pMudEvent->pEvent = NULL;

  return (pMudEvent);
}

void free_mud_event(struct mud_event_data *pMudEvent) {
  struct descriptor_data * d = NULL;
  struct char_data * ch = NULL;
  struct room_data * room = NULL;

  switch (mud_event_index[pMudEvent->iId].iEvent_Type) {
    case EVENT_WORLD:
      remove_from_list(pMudEvent->pEvent, world_events);
      break;
    case EVENT_DESC:
      d = (struct descriptor_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, d->events);
      break;
    case EVENT_CHAR:
      ch = (struct char_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, ch->events);

      if (ch->events && ch->events->iSize == 0) {
        free_list(ch->events);
        ch->events = NULL;
      }
      break;
    case EVENT_ROOM:
      room = (struct room_data *) pMudEvent->pStruct;
      remove_from_list(pMudEvent->pEvent, room->events);

      if (room->events->iSize == 0) {
        free_list(room->events);
        room->events = NULL;
      }
      break;
  }

  if (pMudEvent->sVariables != NULL)
    free(pMudEvent->sVariables);

  pMudEvent->pEvent->event_obj = NULL;
  free(pMudEvent);
}

struct mud_event_data * char_has_mud_event(struct char_data * ch, event_id iId) {
  struct event * pEvent = NULL;
  struct mud_event_data * pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events == NULL)
    return NULL;

  if (ch->events->iSize == 0)
    return NULL;

  clear_simple_list();

  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
    if (!pEvent->isMudEvent)
      continue;
    pMudEvent = (struct mud_event_data *) pEvent->event_obj;
    if (pMudEvent->iId == iId) {
      found = TRUE;
      break;
    }
  }

  if (found)
    return (pMudEvent);

  return NULL;
}

void clear_char_event_list(struct char_data * ch) {
  struct event * pEvent = NULL;

  if (ch->events == NULL)
    return;

  if (ch->events->iSize == 0)
    return;

  clear_simple_list();

  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {
    event_cancel(pEvent);
  }

}

/* ripley's version of change_event_duration 
 * a function to adjust the event time of a given event
 */
void change_event_duration(struct char_data * ch, event_id iId, long time) {
  struct event *pEvent;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events->iSize == 0)
    return;

  simple_list(NULL);

  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *) pEvent->event_obj;

    if (pMudEvent->iId == iId) {
      found = TRUE;
      break;
    }
  }

  simple_list(NULL);

  if (found) {
    /* So we found the offending event, now build a new one, with the new time */
    attach_mud_event(new_mud_event(iId, pMudEvent->pStruct, pMudEvent->sVariables), time);
    event_cancel(pEvent);
  }

}

/*  this is vatiken's version, above is ripley's version */
/*
 void change_event_duration(struct char_data * ch, event_id iId, long time) {
  struct event *pEvent = NULL;
  struct mud_event_data *pMudEvent = NULL;
  bool found = FALSE;

  if (ch->events == NULL);
    return;
    
  if (ch->events->iSize == 0)
    return;

  clear_simple_list();

  while ((pEvent = (struct event *) simple_list(ch->events)) != NULL) {

    if (!pEvent->isMudEvent)
      continue;

    pMudEvent = (struct mud_event_data *) pEvent->event_obj;

    if (pMudEvent->iId == iId) {
      found = TRUE;
      break;
    }
  }

  if (found) {        
    // So we found the offending event, now build a new one, with the new time
    attach_mud_event(new_mud_event(iId, pMudEvent->pStruct, pMudEvent->sVariables), time);
    event_cancel(pEvent);
  }    
  
}
 */



