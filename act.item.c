/**************************************************************************
*  File: act.item.c                                        Part of tbaMUD *
*  Usage: Object handling routines -- get/drop and container handling.    *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "constants.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "act.h"
#include "quest.h"
#include "drugs.h"
#include "fight.h"
#include "house.h"
#include "custom.h"
#include "skilltree.h"


/* local function prototypes */
int check_restrictions(struct char_data *ch, struct obj_data *obj);
/* do_get utility functions */
static void get_check_money(struct char_data *ch, struct obj_data *obj);
static void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount);
static void get_from_room(struct char_data *ch, char *arg, int amount);
static void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
static int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
/* do_give utility functions */
static struct char_data *give_find_vict(struct char_data *ch, char *arg);
static void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
static void perform_give_units(struct char_data *ch, struct char_data *vict, int amount);
/* do_drop utility functions */
static int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
static void perform_drop_units(struct char_data *ch, int amount, byte mode, room_rnum RDR);
/* do_put utility functions */
static void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
/* do_remove utility functions */
static void perform_remove(struct char_data *ch, int pos);
/* do_wear utility functions */
static void perform_wear(struct char_data *ch, struct obj_data *obj, int where, int something);
static void wear_message(struct char_data *ch, struct obj_data *obj, int where);




static void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont)
{

  if (!drop_otrigger(obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;

  if ((GET_OBJ_VAL(cont, 0) > 0) &&
      (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && IN_ROOM(cont) != NOWHERE)
    act("You can't get $p out of your hand.", FALSE, ch, obj, NULL, TO_CHAR);
  else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

      if (GET_OBJ_TYPE(cont) ==  ITEM_TABLE)
    act("$n puts $p on $P.", TRUE, ch, obj, cont, TO_ROOM);
    else if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP)) {
      SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
      act("You get a strange feeling as you put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    } else
      if (GET_OBJ_TYPE(cont) == ITEM_TABLE)
    act("You put $p on $P.", TRUE, ch, obj, cont, TO_CHAR);
    else if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER)
      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
  }
}

void perform_do_unload(struct char_data *ch, struct obj_data *weapon)
{
	bool ack = false;
	int j;
	struct obj_data *ammo;
    ammo = GET_AMMO_LOADED(weapon);
    if (!ch || !weapon)
        return;
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
        send_to_char(ch, "%s: You can't carry anymore items.\n\r", OBJS(GET_AMMO_LOADED(weapon), ch));
        return;
    }
    act("You remove $p from $P.", FALSE, ch, GET_AMMO_LOADED(weapon), weapon, TO_CHAR);
    act("$n removes $p from $P.", TRUE, ch, GET_AMMO_LOADED(weapon), weapon, TO_ROOM);

  for (j = 0; j < MAX_OBJ_APPLY; j++)
    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(ammo, j), GET_OBJ_APPLY_MOD(ammo, j), GET_OBJ_AFFECT(ammo), FALSE);

    // now apply the affects
    for (j = 0; j < NUM_AFF_FLAGS; j++)
        if (OBJAFF_FLAGGED(ammo, j))
            obj_affect_to_char(ch, j, ack, ammo);

  affect_total(ch);

    obj_from_obj(GET_AMMO_LOADED(weapon));
    obj_to_char(GET_AMMO_LOADED(weapon), ch);
	IS_WEARING_W(ch) -= GET_OBJ_WEIGHT(ammo);
    GET_AMMO_LOADED(weapon) = NULL;
}

/* The following put modes are supported:
     1) put <object> <container>
     2) put all.<object> <container>
     3) put all <container>
   The <container> must be in inventory or on ground. All objects to be put
   into container must be in inventory. */
ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  } else {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(ch, "You can only put things into one container at a time.\r\n");
  else if (!*thecont) {
    send_to_char(ch, "What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
  } else {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
   else if (!IS_CONTAINER(cont) && !IS_TABLE(cont))
      act("$p is not a container or a table.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
      send_to_char(ch, "You'd better open it first!\r\n");
    else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying)))
	  send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
	else if (obj == cont && howmany == 1)
	  send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
	else {
	  while (obj && howmany) {
	    next_obj = obj->next_content;

            if (obj != cont) {
              howmany--;
	      perform_put(ch, obj, cont);
            }
	    obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
	  }
	}
      } else {
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char(ch, "You don't seem to have anything to put in it.\r\n");
	  else
	    send_to_char(ch, "You don't seem to have any %ss.\r\n", theobj);
	}
      }
    }
  }
}

int can_take_obj(struct char_data *ch, struct obj_data *obj)
{
	if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
		act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	if (!IS_NPC(ch)) {
	  if (!PRF_FLAGGED(ch, PRF_NOHASSLE)) {
		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
			act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
			return (0);
		} else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
			act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
			return (0);
		}
	  }
  }
  
  if (OBJ_SAT_IN_BY(obj)){
    act("It appears someone is sitting on $p..", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  
  return (1);
}

static void get_check_money(struct char_data *ch, struct obj_data *obj)
{
  int value = GET_OBJ_VAL(obj, 0);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  extract_obj(obj);

  GET_UNITS(ch) += value;

  if (value == 1)
    send_to_char(ch, "There was 1 resource unit.\r\n");
  else
    send_to_char(ch, "There were %d resource units.\r\n", value);
}

static void perform_get_from_container(struct char_data *ch, struct obj_data *obj,
				     struct obj_data *cont, int mode)
{
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch)) {
      obj_from_obj(obj);
      obj_to_char(obj, ch);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      get_check_money(ch, obj);
	  // if they scored a rainbow token from a corpse, update our number
      if (GET_OBJ_TYPE(obj) == ITEM_AUTOQUEST && (GET_OBJ_TYPE(cont) == ITEM_CONTAINER && GET_OBJ_VAL(cont, 3) == 1)) {
		rainbow_number--;
        rainbow_update();
      }
	  // if this is a key, then start the timer
      if ((obj->obj_flags.value[4] != 0) && (GET_OBJ_TYPE(obj) == ITEM_KEY))
		obj->obj_flags.timer_on = TRUE;
    }
  }
}

void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int howmany)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || !PRF_FLAGGED(ch, PRF_NOHASSLE)))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains))) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      struct obj_data *obj_next;
      while (obj && howmany--) {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
	act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else {
        char buf[MAX_STRING_LENGTH];

	snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.", arg);
	act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}

static int perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{
  char buf[MAX_INPUT_LENGTH];
  bool success;
  
  if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    get_check_money(ch, obj);
	get_encumbered(ch);
	
	if (OBJ_FLAGGED(obj, ITEM_BOP)) {
		obj->bound_id = GET_IDNUM(ch);
		obj->bound_name = GET_NAME(ch);		
		sprintf(buf, "%s %s", GET_NAME(ch), obj->name);
		success = restring_name(ch, obj, buf);
		REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BOP);
		send_to_char(ch, "@W%s glows @Ybrightly @Was it is now bound to you!@n\r\n", obj->short_description);
	}	
	
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)) {
		save_char(ch);
		Crash_crashsave(ch);
		House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
	}
    // if this is a key, then start the timer
    if (obj->obj_flags.value[4] != 0 && GET_OBJ_TYPE(obj) == ITEM_KEY)
		obj->obj_flags.timer_on = TRUE;
    return (1);
  }
  return (0);
}

static void get_from_room(struct char_data *ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    else {
      struct obj_data *obj_next;
      while(obj && howmany--) {
	obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
	send_to_char(ch, "There doesn't seem to be anything here.\r\n");
      else
	send_to_char(ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}

ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    send_to_char(ch, "@RYour arms are already full!@n\r\n");
  else if (!*arg1)
    send_to_char(ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2); /* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3); /* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont)
	send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
   else if (!IS_CONTAINER(cont) && !IS_TABLE(cont))
	act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
	get_from_container(ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
	send_to_char(ch, "Get from all of what?\r\n");
	return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	  }
	}
      for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
	}
      if (!found) {
	if (cont_dotmode == FIND_ALL)
	  send_to_char(ch, "You can't seem to find any containers.\r\n");
	else
	  send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
}

static void perform_drop_units(struct char_data *ch, int amount, byte mode, room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (GET_UNITS(ch) < amount)
    send_to_char(ch, "You don't have that many units!\r\n");
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE); /* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
	send_to_char(ch, "You throw some resource units into the air where it disappears in a puff of smoke!\r\n");
	act("$n throws some resource into the air where it disappears in a puff of smoke!",
	    FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        char buf[MAX_STRING_LENGTH];

        if (!drop_wtrigger(obj, ch)) {
          extract_obj(obj);
          return;
        }

	snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);

	send_to_char(ch, "You drop some resource units.\r\n");
	obj_to_room(obj, IN_ROOM(ch));
      }
    } else {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      send_to_char(ch, "You drop some resource units which disappears in a puff of smoke!\r\n");
    }
	GET_UNITS(ch) -= amount;
  }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "\r\nA donation bot swoops from the sky and takes the item!\r\n" : "")
static int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR)
{
  char buf[MAX_STRING_LENGTH];
  int value;

  if (!drop_otrigger(obj, ch))
    return 0;

  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;

  if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
    snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  if ((mode == SCMD_JUNK) && GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
	send_to_char(ch, "You cannot junk a container with items in it.\r\n");
	return(0);
  }
  if ((mode == SCMD_DONATE) && IS_BOUND(obj)) {
	send_to_char(ch, "You cannot donate bound equipment.\r\n");
	return(0);
  }
  if ((mode == SCMD_DONATE) && GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
	send_to_char(ch, "You cannot donate a container with items in it.\r\n");
	return(0);
  }
  snprintf(buf, sizeof(buf), "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);

  snprintf(buf, sizeof(buf), "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);
  get_encumbered(ch);

  if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode) {
  case SCMD_DROP:
    obj_to_room(obj, IN_ROOM(ch));
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE)) {
		save_char(ch);
		Crash_crashsave(ch);
		House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
	}
    return (0);
  case SCMD_DONATE:
	if (GET_OBJ_LEVEL(obj) > 10 || OBJ_FLAGGED(obj, ITEM_MINLEV15) || OBJ_FLAGGED(obj, ITEM_MINLEV25) || OBJ_FLAGGED(obj, ITEM_REMORT_ONLY))
		obj_to_room(obj, RDR);
	else
		obj_to_room(obj, real_room(4175));
	GET_OBJ_COST(obj) = 0;
	act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    /* SYSERR_DESC: This error comes from perform_drop() and is output when
     * perform_drop() is called with an illegal 'mode' argument. */
    break;
  }

  return (0);
}

ACMD(do_drop)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi;
  const char *sname;

  switch (subcmd) {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    RDR = real_room(CONFIG_DON_ROOM_1);
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", sname);
    return;
  } else if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp("units", arg) || !str_cmp("units", arg))
      perform_drop_units(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      do {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
	send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
	send_to_char(ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be carrying anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  amount += perform_drop(ch, obj, mode, sname, RDR);
	}
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
	send_to_char(ch, "What do you want to %s all of?\r\n", sname);
	return;
      }
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);

      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK)) {
    send_to_char(ch, "You have been rewarded by the gods!\r\n");
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_UNITS(ch) += amount;
  }
}

static void perform_give(struct char_data *ch, struct char_data *vict,
		       struct obj_data *obj)
{
  if (!give_otrigger(obj, ch, vict))
    return;
  if (!receive_mtrigger(vict, ch, obj))
    return;
  if (!OBJ_FLAGGED(obj, ITEM_QUEST_ITEM)) {
	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
		act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}
	if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict) && AFF_FLAGGED(vict, AFF_CHARM)) {
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict) && AFF_FLAGGED(vict, AFF_CHARM)) {
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  get_encumbered(vict);
  get_encumbered(ch);
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

  autoquest_trigger_check( ch, vict, obj, AQ_OBJ_RETURN);
}

/* utility function for give */
static struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(ch, "To who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (NULL);
}

static void perform_give_units(struct char_data *ch, struct char_data *vict, int amount)
{
  char buf[MAX_STRING_LENGTH];

  if (amount <= 0) {
    send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((GET_UNITS(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))) {
    send_to_char(ch, "You don't have that many resource units!\r\n");
    return;
  }
  send_to_char(ch, "%s", CONFIG_OK);

  snprintf(buf, sizeof(buf), "$n gives you %d resource units%s.", amount, amount == 1 ? "" : "s");
  act(buf, FALSE, ch, 0, vict, TO_VICT);

  snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

  if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
    GET_UNITS(ch) -= amount;
    
  GET_UNITS(vict) += amount;
  log ("RUs: %s -(%d)-> %s", GET_NAME(ch), amount, GET_NAME(vict));
  bribe_mtrigger(vict, ch, amount);
}

void perform_give_qps(struct char_data * ch, struct char_data * vict, int amount)
{
    char buf[MAX_STRING_LENGTH];
    if (IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "Mobs don't need questpoints, GO AWAY!\r\n");
        return;
    }
    /* restrict player trading of QP, solves autoquest cheating and maybe EQ hoarding -Lump 04/09 */
    if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT && GET_LEVEL(vict) < LVL_IMMORT) {
        send_to_char(ch, "Sorry, you can't give your quest points to other mortals.\r\n");
        return;
    }
    if (amount <= 0) {
        send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
        return;
    }
    if ((GET_QUESTPOINTS(ch) < amount) && (IS_NPC(ch) || (GET_LEVEL(ch) <= LVL_GOD))) {
        send_to_char(ch, "You don't have that many questpoints!\r\n");
        return;
    }
    send_to_char(ch, "%s", CONFIG_OK);
    snprintf(buf, sizeof(buf), "$n gives you %d questpoint%s.", amount, amount == 1 ? "" : "s");
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    snprintf(buf, sizeof(buf), "$n gives some questpoints to $N.");
    act(buf, TRUE, ch, 0, vict, TO_NOTVICT);
    if (IS_NPC(ch) || (GET_LEVEL(ch) < LVL_GOD))
        GET_QUESTPOINTS(ch) -= amount;
    GET_QUESTPOINTS(vict) += amount;
    log("QP: %s -(%d)-> %s", GET_NAME(ch), amount, GET_NAME(vict));
    //todo: have a qp bribe trigger
}

ACMD(do_give)
{
  char arg[MAX_STRING_LENGTH];
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Give what to who?\r\n");
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("units", arg) || !str_cmp("unit", arg) || !str_cmp("rus", arg) || !str_cmp("ru", arg)) {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL) {
		perform_give_units(ch, vict, amount);
		save_char(ch);
		save_char(vict);
	  }
      return;
    }
    else if (!str_cmp("questpoints", arg) || !str_cmp("questpoint", arg) || !str_cmp("qps", arg) || !str_cmp("qp", arg)) {
		argument = one_argument(argument, arg);
		if (( vict = give_find_vict(ch, arg))) {
			perform_give_qps(ch, vict, amount);
            save_char(ch);
            save_char(vict);
        }
		return;
	}
	else if (!*arg) /* Give multiple code. */
      send_to_char(ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(ch, argument)))
      return;
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      while (obj && amount--) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	perform_give(ch, vict, obj);
	obj = next_obj;
      }
    }
  } else {
    char buf1[MAX_INPUT_LENGTH];

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
	send_to_char(ch, "All of what?\r\n");
	return;
      }
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be holding anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (CAN_SEE_OBJ(ch, obj) &&
	      ((dotmode == FIND_ALL || isname(arg, obj->name))))
	    perform_give(ch, vict, obj);
	}
    }
  }
}

void weight_change_object(struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (IN_ROOM(obj) != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
    /* SYSERR_DESC: weight_change_object() outputs this error when weight is
     * attempted to be removed from an object that is not carried or in
     * another object. */
  }
}

void name_from_drinkcon(struct obj_data *obj)
{
  char *new_name, *cur_name, *next;
  const char *liqname;
  int liqlen, cpylen;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  liqname = drinknames[GET_OBJ_VAL(obj, 2)];
  if (!isname(liqname, obj->name)) {
    log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
    /* SYSERR_DESC: From name_from_drinkcon(), this error comes about if the
     * object noted (by keywords and item vnum) does not contain the liquid
     * string being searched for. */
    return;
  }

  liqlen = strlen(liqname);
  CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

  for (cur_name = obj->name; cur_name; cur_name = next) {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strn_cmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " "); /* strcat: OK (size precalculated) */
    strncat(new_name, cur_name, cpylen); /* strncat: OK (size precalculated) */
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}

void name_to_drinkcon(struct obj_data *obj, int type)
{
  char *new_name;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", obj->name, drinknames[type]); /* sprintf: OK */

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);

  obj->name = new_name;
}

ACMD(do_drink)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight;
  int on_ground = 0;

  one_argument(argument, arg);

  if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    char buf[MAX_STRING_LENGTH];
    switch (SECT(IN_ROOM(ch))) {
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_UNDERWATER:
        if ((GET_COND(ch, HUNGER) > 20) && (GET_COND(ch, THIRST) > 0)) {
          send_to_char(ch, "Your stomach can't contain anymore!\r\n");
        }
        snprintf(buf, sizeof(buf), "$n takes a refreshing drink.");
        act(buf, TRUE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You take a refreshing drink.\r\n");
        gain_condition(ch, THIRST, 1);
        if (GET_COND(ch, THIRST) > 20)
          send_to_char(ch, "You don't feel thirsty any more.\r\n");
        return;
      default:
    send_to_char(ch, "Drink from what?\r\n");
    return;
    }
  }
  if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    } else
      on_ground = 1;
  }
  if (GET_OBJ_TYPE(temp) == ITEM_POTION) {
	psi_object_psionics(ch, temp, arg);
	return;
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char(ch, "You can't drink from that!\r\n");
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char(ch, "You have to be holding that to drink from it.\r\n");
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char(ch, "You can't seem to get close enough to your mouth.\r\n");
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if ((GET_COND(ch, HUNGER) > 20) && (GET_COND(ch, THIRST) > 0)) {
    send_to_char(ch, "Your stomach can't contain anymore!\r\n");
    return;
  }
  if ((GET_OBJ_VAL(temp, 1) == 0) || (!GET_OBJ_VAL(temp, 0) == 1)) {
    send_to_char(ch, "It is empty.\r\n");
    return;
  }

  if (!consume_otrigger(temp, ch, OCMD_DRINK))  /* check trigger */
    return;

  if (subcmd == SCMD_DRINK) {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);

    if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
    else
      amount = rand_number(3, 10);

  } else {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    amount = 1;
  }

  amount = MIN(amount, GET_OBJ_VAL(temp, 1));

  /* You can't subtract more than the object weighs, unless its unlimited. */
  if (GET_OBJ_VAL(temp, 0) > 0) {
    weight = MIN(amount, GET_OBJ_WEIGHT(temp));
    weight_change_object(temp, -weight); /* Subtract amount */
  }

  gain_condition(ch, DRUNK,  drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK]  * amount / 4);
  gain_condition(ch, HUNGER,   drink_aff[GET_OBJ_VAL(temp, 2)][HUNGER]   * amount / 4);
  gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount / 4);

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You feel drunk.\r\n");

  if (GET_COND(ch, THIRST) > 20)
    send_to_char(ch, "You don't feel thirsty any more.\r\n");

  if (GET_COND(ch, HUNGER) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(temp, 3) && GET_LEVEL(ch) < LVL_IMMORT) { /* The crap was poisoned ! */
    send_to_char(ch, "Oops, it tasted rather strange!\r\n");
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

    new_affect(&af);
    af.type = PSIONIC_POISON;
    af.duration = amount * 3;
    SET_BIT_AR(af.bitvector, AFF_POISON);
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  /* Empty the container (unless unlimited), and no longer poison. */
  if (GET_OBJ_VAL(temp, 0) > 0) {
    GET_OBJ_VAL(temp, 1) -= amount;
    if (!GET_OBJ_VAL(temp, 1)) { /* The last bit */
      name_from_drinkcon(temp);
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
    }
  }
  return;
}

ACMD(do_eat)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (IS_NPC(ch)) /* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    send_to_char(ch, "Eat what?\r\n");
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char(ch, "You can't eat THAT!\r\n");
    return;
  }
  if (GET_COND(ch, HUNGER) > 20) { /* Stomach full */
    send_to_char(ch, "You are too full to eat more!\r\n");
    return;
  }

  if (!consume_otrigger(food, ch, OCMD_EAT)) /* check trigger */
    return;

  if (subcmd == SCMD_EAT) {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, HUNGER, amount);

  if (GET_COND(ch, HUNGER) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    /* The crap was poisoned ! */
    send_to_char(ch, "Oops, that tasted rather strange!\r\n");
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

    new_affect(&af);
    af.type = PSIONIC_POISON;
    af.duration = amount * 2;
    SET_BIT_AR(af.bitvector, AFF_POISON);
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else {
    if (!(--GET_OBJ_VAL(food, 0))) {
      send_to_char(ch, "There's nothing left now.\r\n");
      extract_obj(food);
    }
  }
}

ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) { /* No arguments */
      send_to_char(ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char(ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) { /* no arguments */
      send_to_char(ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) { /* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, 1) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) { /* pour */
    if (!*arg2) {
      send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!str_cmp(arg2, "out")) {
      if (GET_OBJ_VAL(from_obj, 0) > 0) {
        act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
        act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

        weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

        name_from_drinkcon(from_obj);
        GET_OBJ_VAL(from_obj, 1) = 0;
        GET_OBJ_VAL(from_obj, 2) = 0;
        GET_OBJ_VAL(from_obj, 3) = 0;
      }
      else
        send_to_char(ch, "You can't possibly pour that container out!\r\n");

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char(ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 0) < 0) ||
      (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))) {
    send_to_char(ch, "There is already another liquid in it!\r\n");
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
    send_to_char(ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(ch, "You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, 2)], arg2);

  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, 1) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */
  if (GET_OBJ_VAL(from_obj, 0) > 0) {
    GET_OBJ_VAL(from_obj, 1) -= (amount =
        (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

    GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

    if (GET_OBJ_VAL(from_obj, 1) < 0) {	/* There was too little */
      GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
      amount += GET_OBJ_VAL(from_obj, 1);
      name_from_drinkcon(from_obj);
      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
    }
  }
  else {
    GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);
    amount = GET_OBJ_VAL(to_obj, 0);
  }
  /* Poisoned? */
  GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));
  /* Weight change, except for unlimited. */
  if (GET_OBJ_VAL(from_obj, 0) > 0) {
    weight_change_object(from_obj, -amount);
  }
  weight_change_object(to_obj, amount); /* Add weight */
}

static void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
  const char *wear_messages[][2] = {
        {
            "$n lights $p and holds it.",
            "You light $p and hold it."
        },

        {
            "$n inserts $p into $s body.",
            "You insert $p into your body."
        },

        {
            "$n inserts $p into $s body.",
            "You insert $p into your body."
        },

        {
            "$n wears $p around $s neck.",
            "You wear $p around your neck."
        },

        {
            "$n wears $p around $s neck.",
            "You wear $p around your neck."
        },

        {
            "$n wears $p on $s body.",
            "You wear $p on your body."
        },

        {
            "$n wears $p on $s head.",
            "You wear $p on your head."
        },

        {
            "$n puts $p on $s legs.",
            "You put $p on your legs."
        },

        {
            "$n wears $p on $s feet.",
            "You wear $p on your feet."
        },

        {
            "$n puts $p on $s hands.",
            "You put $p on your hands."
        },

        {
            "$n wears $p on $s arms.",
            "You wear $p on your arms."
        },

        {
            "$n straps $p around $s arm as a shield.",
            "You start to use $p as a shield."
        },

        {
            "$n wears $p about $s body.",
            "You wear $p around your body."
        },

        {
            "$n wears $p around $s waist.",
            "You wear $p around your waist."
        },

        {
            "$n puts $p on around $s right wrist.",
            "You put $p on around your right wrist."
        },

        {
            "$n puts $p on around $s left wrist.",
            "You put $p on around your left wrist."
        },

        {
            "$n puts $p on $s face.",
            "You put $p on your face."
        },

        {
            "$n puts $p over $s ears.",
            "You put $p over your ears."
        },

        {
            "$n wields $p.",
            "You wield $p."
        },

        {
            "$n grabs $p.",
            "You grab $p."
        },

        {
            "$n places $p in a hovering position.",
            "You place $p in a hovering position near you."
        },

        {
            "$n wears $p on $s right nipple.",
            "You wear $p on your right nipple."
        },

        {
            "$n wears $p on $s left nipple.",
            "You wear $p on your left nipple."
        },

        {
            "$n puts $p on a finger on $s right hand.",
            "You put $p on a finger on your right hand."
        },

        {
            "$n puts $p on a finger on $s left hand.",
            "You put $p on a finger on your left hand."
        },

        {
            "$n wears $p on $s right ear",
            "You wear $p on your right ear."
        },

        {
            "$n wears $p on $s left ear",
            "You wear $p on your left ear."
        },

        {
            "$n wears $p as a nose ring.",
            "You wear $p as a nose ring."
        },

        {
            "$n installs $p into $s expansion slot.",
            "You install $p into your expansion slot."
        },

		{
			"$n activates a $p.",
			"You activate a $p."
		}
    };

    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

static void perform_wear(struct char_data *ch, struct obj_data *obj, int where, int something)
{
    bool missing_part = FALSE;
	char buf[MAX_INPUT_LENGTH];
	bool success;

  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] = {
        ITEM_WEAR_TAKE, ITEM_WEAR_IMPLANT, ITEM_WEAR_IMPLANT, ITEM_WEAR_NECK,
        ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
        ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
        ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
        ITEM_WEAR_FACE, ITEM_WEAR_EARS, ITEM_WEAR_WIELD, ITEM_WEAR_TAKE,
        ITEM_FLOAT_NEARBY,ITEM_WEAR_NIPPLE,ITEM_WEAR_NIPPLE,
        ITEM_WEAR_FINGER,ITEM_WEAR_FINGER,ITEM_WEAR_EARRING, ITEM_WEAR_EARRING,
        ITEM_WEAR_NOSER, ITEM_WEAR_EXPANSION, ITEM_WEAR_MEDICAL
  };

  const char *already_wearing[] = {
        "You're already using a light.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You've already got 2 implants!\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You can't wear anything else around your neck.\r\n",
        "You're already wearing something on your body.\r\n",
        "You're already wearing something on your head.\r\n",
        "You're already wearing something on your legs.\r\n",
        "You're already wearing something on your feet.\r\n",
        "You're already wearing something on your hands.\r\n",
        "You're already wearing something on your arms.\r\n",
        "You're already using a shield.\r\n",
        "You're already wearing something about your body.\r\n",
        "You already have something around your waist.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You're already wearing something around both of your wrists.\r\n",
        "You're already wearing something on your face.\n\r",
        "You're already wearing something over your ears.\n\r",
        "You're already wielding a weapon.\r\n",
        "You're already holding something.\r\n",
        "You've already have something floating nearby.\n\r",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You're already wearing something on both nipples.\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "Your fingers are full!\r\n",
        "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
        "You're already have earrings on both ears.\r\n",
        "You're already have something on your nose.\r\n",
        "Your expansion slot is currently full!.\r\n",
	"You are already wearing a medical bracelet.\r\n"
  };

  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
    if ((where == WEAR_IMPLANT_R) || (where == WEAR_NECK_1)
        || (where == WEAR_WRIST_R) || (where == WEAR_RING_NIP_R)
        || (where == WEAR_RING_FIN_R) || (where == WEAR_RING_EAR_R))
        if (GET_EQ(ch, where))
            where++;

//  if (GET_EQ(ch, where)) {
//    send_to_char(ch, "%s", already_wearing[where]);
//    return;
//  }

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }

    if (GET_EQ(ch, where)) {
      int i;
      for (i = 0; i < NUM_WEARS; i++) {
        if (CAN_WEAR(obj, wear_bitvectors[i]) && !GET_EQ(ch, i)  &&
          wear_bitvectors[i] != WEAR_LIGHT) {
          perform_wear(ch, obj, i, 0);
          return;
        }
    }

    send_to_char(ch, "%s", already_wearing[where]);
    return;
  }

    // expansion slot is only for people with Advanced Skull Bionic, can't be fighting
    if (where == WEAR_EXPANSION) {
        if (!AFF_FLAGGED(ch, AFF_CHIPJACK)) {
            send_to_char(ch, "You don't have a Chipjack!\r\n");
            return;
        }

        // todo: isnt this code redundant because do_wear has a min position already?
        if (GET_POS(ch) == POS_FIGHTING) {
            send_to_char(ch, "There's too much going on! You can't focus enough to do this!\r\n");
            return;
        }

    }
	
	if (IS_BOUND(obj)) {
		if (obj->bound_id != GET_IDNUM(ch)) {
			send_to_char(ch, "This item is bound to someone else.\r\n");
			return;
		}
	}
	if (OBJ_FLAGGED(obj, ITEM_NODROP) && GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
		send_to_char(ch, "You probably shouldn't wear cursed containers unless you want your gear destroyed.\r\n");
		return;
	}
    // check to see if missing body parts prevent wearing the equipment
    switch(where) {
        case WEAR_RING_FIN_R:
        case WEAR_WRIST_R:
            if (PART_MISSING(ch, BODY_PART_RIGHT_HAND) || PART_MISSING(ch, BODY_PART_LEFT_WRIST))
                missing_part = TRUE;
            break;
        case WEAR_RING_FIN_L:
        case WEAR_WRIST_L:
            if (PART_MISSING(ch, BODY_PART_LEFT_HAND) || PART_MISSING(ch, BODY_PART_LEFT_WRIST))
                missing_part = TRUE;
            break;
        case WEAR_LEGS:
            if (PART_MISSING(ch, BODY_PART_RIGHT_LEG) && PART_MISSING(ch, BODY_PART_LEFT_LEG))
                missing_part = TRUE;
            break;
        case WEAR_FEET:
            if (PART_MISSING(ch, BODY_PART_RIGHT_FOOT) && PART_MISSING(ch, BODY_PART_LEFT_FOOT))
                missing_part = TRUE;
            break;
        case WEAR_HANDS:
            if (PART_MISSING(ch, BODY_PART_LEFT_HAND) && PART_MISSING(ch, BODY_PART_RIGHT_HAND))
                missing_part = TRUE;
            break;
        case WEAR_ARMS:
        case WEAR_SHIELD:
            if (PART_MISSING(ch, BODY_PART_LEFT_ARM) && PART_MISSING(ch, BODY_PART_RIGHT_ARM))
                missing_part = TRUE;
            break;
        case WEAR_HOLD:
        case WEAR_LIGHT:
            if ((PART_MISSING(ch, BODY_PART_LEFT_ARM) || PART_MISSING(ch, BODY_PART_RIGHT_ARM)) && (GET_EQ(ch, WEAR_WIELD)))
                missing_part = TRUE;
            break;
        case WEAR_WIELD:
            if ((PART_MISSING(ch, BODY_PART_LEFT_ARM) || PART_MISSING(ch, BODY_PART_RIGHT_ARM)) && (GET_EQ(ch, WEAR_HOLD)))
                missing_part = TRUE;
            break;
        case WEAR_NECK_1:
        case WEAR_NECK_2:
        case WEAR_BODY:
        case WEAR_HEAD:
        case WEAR_ABOUT:
        case WEAR_WAIST:
        case WEAR_FACE:
        case WEAR_FLOATING_NEARBY:
            // player should be dead
            break;
        case WEAR_RING_EAR_R:
        case WEAR_RING_EAR_L:
        case WEAR_EARS:
        case WEAR_IMPLANT_R:
        case WEAR_IMPLANT_L:
        case WEAR_RING_NIP_R:
        case WEAR_RING_NIP_L:
        case WEAR_RING_NOSE:
        case WEAR_EXPANSION:
		case WEAR_MEDICAL:
            // not sure what body parts these link up to
            break;
        default:
            log("SYSERR: Error in perform_wear - unknown WEAR_XXX type");
            return;
            break;
    }

    if (missing_part) {
        act("You can no longer wear $p there, you're missing the body part!", FALSE, ch, obj, 0, TO_CHAR);
        return;
    }

    // See if a trigger disallows it
    if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
        return;

    wear_message(ch, obj, where);
    obj_from_char(obj);
    equip_char(ch, obj, where, TRUE);
	IS_WEARING_W(ch) += GET_OBJ_WEIGHT(obj);
	get_encumbered(ch);
	
	if (OBJ_FLAGGED(obj, ITEM_BOP)) {
		obj->bound_id = GET_IDNUM(ch);
		obj->bound_name = GET_NAME(ch);		
		sprintf(buf, "%s %s",GET_NAME(ch), obj->name );
		success = restring_name(ch, obj, buf);
		REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BOP);
		send_to_char(ch, "@R%s is now bound to you!@n\r\n", obj->short_description);
	}	
}

int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
  int where = -1;

  const char *keywords[] = {
        "\r!RESERVED!",
        "implant",
        "\r!RESERVED!",
        "neck",
        "\r!RESERVED!",
        "body",
        "head",
        "legs",
        "feet",
        "hands",
        "arms",
        "shield",
        "about",
        "waist",
        "wrist",
        "\r!RESERVED!",
        "face",
        "ears",
        "\r!RESERVED!",
        "\r!RESERVED!",
        "floating",
        "nipple",
        "\r!RESERVED!",
        "finger",
        "\r!RESERVED!",
        "earring",
        "\r!RESERVED!",
        "nose",
        "slot",
		"medical",
        "\n"
  };

  if (!arg || !*arg) {
        if (CAN_WEAR(obj, ITEM_WEAR_WIELD))       where = WEAR_WIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_IMPLANT))     where = WEAR_IMPLANT_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
        if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
        if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
        if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
        if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
        if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
        if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
        if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
        if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
        if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
        if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
        if (CAN_WEAR(obj, ITEM_WEAR_FACE))        where = WEAR_FACE;
        if (CAN_WEAR(obj, ITEM_WEAR_EARS))        where = WEAR_EARS;
        if (CAN_WEAR(obj, ITEM_FLOAT_NEARBY))     where = WEAR_FLOATING_NEARBY;
        if (CAN_WEAR(obj, ITEM_WEAR_NIPPLE ))     where = WEAR_RING_NIP_R;
        if (CAN_WEAR(obj, ITEM_WEAR_FINGER ))     where = WEAR_RING_FIN_R;
        if (CAN_WEAR(obj, ITEM_WEAR_EARRING))     where = WEAR_RING_EAR_R;
        if (CAN_WEAR(obj, ITEM_WEAR_NOSER))       where = WEAR_RING_NOSE;
        if (CAN_WEAR(obj, ITEM_WEAR_EXPANSION))   where = WEAR_EXPANSION;
		if (CAN_WEAR(obj, ITEM_WEAR_MEDICAL))	  where = WEAR_MEDICAL;
  } else if ((where = search_block(arg, keywords, FALSE)) < 0)
    send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);

  return (where);
}

ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Wear what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char(ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
        if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
          send_to_char(ch, "You are not experienced enough to use that.\r\n");
        else {
          items_worn++;
	  perform_wear(ch, obj, where, 0);
	}
      }
    }
    if (!items_worn)
      send_to_char(ch, "You don't seem to have anything wearable.\r\n");
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char(ch, "Wear all of what?\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0)
	  perform_wear(ch, obj, where, 0);
	else
	  act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
      send_to_char(ch, "It's too heavy for you to use.\r\n");
    else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
	perform_wear(ch, obj, where, 0);
      else if (!*arg2)
	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}

ACMD(do_wield)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char(ch, "You can't wield that.\r\n");
    else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
      send_to_char(ch, "It's too heavy for you to use.\r\n");
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else
      perform_wear(ch, obj, WEAR_WIELD, 0);
  }
}

ACMD(do_grab)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
    send_to_char(ch, "You are not experienced enough to use that.\r\n");
  else if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w)
    send_to_char(ch, "It's too heavy for you to use.\r\n");
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
     perform_wear(ch, obj, WEAR_LIGHT, 0);
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
      GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
	  GET_OBJ_TYPE(obj) != ITEM_POTION &&
          GET_OBJ_TYPE(obj) != ITEM_LIGHT)
	send_to_char(ch, "You can't hold that.\r\n");
      else
	perform_wear(ch, obj, WEAR_HOLD, 0);

    }
  }
}

static void perform_remove(struct char_data *ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
    /*  This error occurs when perform_remove() is passed a bad 'pos'
     *  (location) to remove an object from. */
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && !PRF_FLAGGED(ch, PRF_NOHASSLE))
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)&& !PRF_FLAGGED(ch, PRF_NOHASSLE))
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
    else {
        if (pos == WEAR_EXPANSION) {
            if (!AFF_FLAGGED(ch, AFF_CHIPJACK)) {
                send_to_char(ch, "You don't have a Chipjack!\r\n");
                return;
			}
            if (GET_POS(ch) == POS_FIGHTING) {
                send_to_char(ch, "There's too much going on! You can't focus enough to do this!\r\n");
                return;
            }
        }
		if (pos == WEAR_MEDICAL) {
			send_to_char(ch, "You have to 'uninsure' yourself to remove that.\r\n");
			return;
		}

		if (pos == WEAR_WIELD)
			if (IS_GUN(obj))
				if (IS_LOADED(obj))
					perform_do_unload(ch, obj);

        if (!remove_otrigger(obj, ch))
            return;
		IS_WEARING_W(ch) -= GET_OBJ_WEIGHT(obj);
		if (IS_WEARING_W(ch) < 0) 
			IS_WEARING_W(ch) = 0;
		obj_to_char(unequip_char(ch, pos, TRUE), ch);
		get_encumbered(ch);
        act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
        act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
    }
}

ACMD(do_uninsure)
{
	if (ch->equipment[WEAR_MEDICAL] != NULL) {

		send_to_char(ch, "You press the red button on your medical bracelet.\r\n");
		send_to_char(ch, "The bracelet falls off and quickly self destructs.\r\n");
		extract_obj(ch->equipment[WEAR_MEDICAL]);
	}

	else
		send_to_char(ch, "You don't seem to be wearing a medical bracelet.\r\n");
}

ACMD(do_remove)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int i;
    int dotmode;
    int found;

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    two_arguments(argument, arg, arg2);
    if (!*arg) {
        send_to_char(ch, "Remove what?\r\n");
        return;
    }
    dotmode = find_all_dots(arg);

    if (dotmode == FIND_ALL) {
        found = 0;
        for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i)) {
            perform_remove(ch, i);
            found = 1;
        }
        if (!found)
            send_to_char(ch, "You're not using anything.\r\n");
    }
    else if (dotmode == FIND_ALLDOT) {
        if (!*arg)
            send_to_char(ch, "Remove all of what?\r\n");
        else {
            found = 0;
            for (i = 0; i < NUM_WEARS; i++)
                if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) && isname(arg, GET_EQ(ch, i)->name)) {
                    perform_remove(ch, i);
                    found = 1;
                }
            if (!found)
                send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
        }
    }
    else {
        if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0)
            send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
        else {

            int where;
            struct obj_data *obj = NULL;
            if (*arg2 && (where = find_eq_pos(ch, obj, arg2)) >= 0 && GET_EQ(ch, where))
                perform_remove(ch, where);
            else {
                perform_remove(ch, i);
            }
        }
    }
}

// Begin of Cyberassault only functions

void perform_load(struct char_data *ch, struct obj_data *ammo, struct obj_data *weapon)
{
	bool ack = false;
	int j;
    if (!ch || !ammo || !weapon)
        return;
    obj_from_char(ammo);
    obj_to_obj(ammo, weapon);
    GET_AMMO_LOADED(weapon) = ammo;
	IS_WEARING_W(ch) += GET_OBJ_WEIGHT(ammo);
    act("You load $p into $P.", FALSE, ch, ammo, weapon, TO_CHAR);
    act("$n loads $p into $P.", TRUE, ch, ammo, weapon, TO_ROOM);

  for (j = 0; j < MAX_OBJ_APPLY; j++)
    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(ammo, j), GET_OBJ_APPLY_MOD(ammo, j), GET_OBJ_AFFECT(ammo), TRUE);

    // now apply the affects
    for (j = 0; j < NUM_AFF_FLAGS; j++)
        if (OBJAFF_FLAGGED(ammo, j))
            obj_affect_to_char(ch, j, ack, ammo);

  affect_total(ch);

}

ACMD(do_load)
{
    int wield = WEAR_WIELD;  // eventually, we'll have mutli-wielding and this will be valuable
    struct obj_data *i;
	struct obj_data *ammo;
    bool found = FALSE;
	int mode;
    struct char_data *temp;
	char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);
	
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }
    if (!ch->equipment[wield]) {
        send_to_char(ch, "But you are not wielding anything to load!\r\n");
        return;
    }
    if (!IS_GUN(ch->equipment[wield])) {
        send_to_char(ch, "But that weapon does not take ammo!\r\n");
        return;
    }
    if (IS_LOADED(ch->equipment[wield])) {
        send_to_char(ch, "That weapon is already loaded!\r\n");
        return;
    }
    i = ch->carrying;
	if (!*arg) {
		while (i != NULL && found != TRUE ) {
			if (GET_AMMO_TYPE(i) == GET_GUN_TYPE(ch->equipment[wield]) && IS_AMMO(i))
				found = TRUE;
			else
				i = i->next_content;
		}
		if (!found) {
			send_to_char(ch, "You have no ammo for that weapon!\r\n");
			return;
		}
		if (!check_restrictions(ch, i))
			return;
		perform_load(ch, i, ch->equipment[wield]);
	}
	else {
		if (!strcmp(arg, "gun")) {
			while (i != NULL && found != TRUE ) {
				if (GET_AMMO_TYPE(i) == GET_GUN_TYPE(ch->equipment[wield]) && IS_AMMO(i))
					found = TRUE;
				else
					i = i->next_content;
			}
			if (!found) {
				send_to_char(ch, "You have no ammo for that weapon!\r\n");
				return;
			}
			if (!check_restrictions(ch, i))
				return;
			perform_load(ch, i, ch->equipment[wield]);	
		}
		else {
			mode = generic_find(arg, FIND_OBJ_INV, ch, &temp, &ammo);
			if (!ammo) {
				send_to_char(ch, "You do see that in your inventory.\r\n");
				return;
			}
			if (!IS_AMMO(ammo)) {
				send_to_char(ch, "That is not ammo!\r\n");
				return;
			}
			if (GET_AMMO_TYPE(ammo) != GET_GUN_TYPE(ch->equipment[wield])) {
				send_to_char(ch, "Your weapon does not take that type of ammo!\r\n");
				return;
			}
			if (!check_restrictions(ch, ammo))
				return;
			perform_load(ch, ammo, ch->equipment[wield]);				
		}
	}
}

ACMD(do_unload)
{
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }
    if (*argument) {
        send_to_char(ch, "I don't understand that!\n\r");
        return;
    }
    if (!ch->equipment[WEAR_WIELD]) {
        send_to_char(ch, "Fine.  You unload imaginary ammo from your imaginary gun.\n\r");
        return;
    }
    if (!IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "But that weapon does not take ammo!\r\n");
        return;
    }
    if (!IS_LOADED(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "That weapon is already empty!\r\n");
        return;
    }
    perform_do_unload(ch, ch->equipment[WEAR_WIELD]);
}

// todo: remove the hard-coded vnums to sabers, use a define or search for saber objects?
void assemble_saber(struct char_data *ch)
{
    struct obj_data *obj;
    struct obj_data *saber1 = NULL;
    struct obj_data *saber2 = NULL;
    struct obj_data *saber3 = NULL;
    struct obj_data *saber4 = NULL;
    struct obj_data *saber5 = NULL;
    struct obj_data *saber6 = NULL;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) == ITEM_SABER_PIECE) {
            switch (GET_OBJ_VAL(obj, 0)) {
                case 1: saber1 = obj;
                    break;
                case 2: saber2 = obj;
                    break;
                case 3: saber3 = obj;
                    break;
                case 4: saber4 = obj;
                    break;
                case 5: saber5 = obj;
                    break;
                case 6: saber6 = obj;
                    break;
            }
        }
    }
    if (!saber1 || !saber2 || !saber3 || !saber4 || !saber5 || !saber6) {
        send_to_char(ch, "You try, but the pieces don't seem to fit together properly.\n\r");
        return;
    }
    extract_obj(saber1);
    extract_obj(saber2);
    extract_obj(saber3);
    extract_obj(saber4);
    extract_obj(saber5);
    extract_obj(saber6);
    switch (rand_number(1, 3)) {
        case 1:
            obj = read_object(real_object(1296), REAL);  //  green
            break;
        case 2:
            obj = read_object(real_object(1297), REAL);  //  blue
            break;
        case 3:
            obj = read_object(real_object(1298), REAL);  //  red
            break;
        default:
            // uh-oh
            obj = read_object(real_object(1296), REAL);  //  green
            mudlog(CMP, LVL_IMMORT, TRUE, "SABER: We reached the default case in random color selection!!");
            break;
    }
    if (!obj) {
        mudlog(CMP, LVL_IMMORT, TRUE, "SABER: Unable to read saber object!!");
        return;
    }
	obj_to_char(obj, ch);
    send_to_all("You sense a great disturbance in the Force as a weapon of rare perfection is forged.\n\r");
    mudlog(CMP, LVL_IMMORT, TRUE, "SABER: %s assembled %s.", GET_NAME(ch), obj->short_description);
}

void assemble_rainbow(struct char_data *ch)
{
    struct obj_data *obj;
    struct obj_data *token1 = NULL;
    struct obj_data *token2 = NULL;
    struct obj_data *token3 = NULL;
    struct obj_data *token4 = NULL;
    struct obj_data *token5 = NULL;
    struct obj_data *token6 = NULL;
    struct obj_data *token7 = NULL;
    struct obj_data *token8 = NULL;
    struct obj_data *token9 = NULL;
    struct obj_data *token10 = NULL;
    struct char_data *lucky = NULL;

    for (obj = ch->carrying; obj; obj = obj->next_content) {

        if (GET_OBJ_TYPE(obj) == ITEM_AUTOQUEST) {
            switch (GET_OBJ_VAL(obj, 0)) {

                case 1: token1 = obj;
                    break;
                case 2: token2 = obj;
                    break;
                case 3: token3 = obj;
                    break;
                case 4: token4 = obj;
                    break;
                case 5: token5 = obj;
                    break;
                case 6: token6 = obj;
                    break;
                case 7: token7 = obj;
                    break;
                case 8: token8 = obj;
                    break;
                case 9: token9 = obj;
                    break;
                case 10: token10 = obj;
                    break;
            }
        }
    }

    if (!token1 || !token2 || !token3 || !token4 || !token5 ||
        !token6 || !token7 || !token8 || !token9 || !token10 ) {
        send_to_char(ch, "You try, but the pieces don't seem to fit together properly.\n\r");
        return;
    }

    extract_obj(token1);
    extract_obj(token2);
    extract_obj(token3);
    extract_obj(token4);
    extract_obj(token5);
    extract_obj(token6);
    extract_obj(token7);
    extract_obj(token8);
    extract_obj(token9);
    extract_obj(token10);

    obj = read_object(15, VIRTUAL);
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_PSIONIC);

    // HND, Hitroll, Damroll, Haste
    switch (rand_number(1, 3)) {
	
        case 1:
            GET_OBJ_APPLY_LOC(obj, 0) = APPLY_HITNDAM;
            GET_OBJ_APPLY_MOD(obj, 0) = rand_number(7,15);
            break;
       case 2:
            GET_OBJ_APPLY_LOC(obj, 0) = APPLY_HITNDAM;
            GET_OBJ_APPLY_MOD(obj, 0) = rand_number(7,15);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_HASTE);
            break;
        case 3:
            GET_OBJ_APPLY_LOC(obj, 0) = APPLY_DAMROLL;
            GET_OBJ_APPLY_MOD(obj, 0) = rand_number(10, 15);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_INSPIRE);
            break;
    }

    // armor, hpv, psi shield or armor
    switch (rand_number(1, 6)) {

        case 1:
            GET_OBJ_APPLY_LOC(obj, 1) = APPLY_ARMOR;
            GET_OBJ_APPLY_MOD(obj, 1) = rand_number(8, 20);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_PSI_SHIELD); /* perm psi shield or armor */
            break;
        case 2:
            GET_OBJ_APPLY_LOC(obj, 1) = APPLY_HIT;
            GET_OBJ_APPLY_MOD(obj, 1) = rand_number(25, 50);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_ARMORED_AURA); /* perm psi shield or armor */
            break;
        case 3:
            GET_OBJ_APPLY_LOC(obj, 1) = APPLY_PSI;
            GET_OBJ_APPLY_MOD(obj, 1) = rand_number(25, 50);
            break;
        case 4:
            GET_OBJ_APPLY_LOC(obj, 1) = APPLY_MOVE;
            GET_OBJ_APPLY_MOD(obj, 1) = rand_number(25, 50);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_PSI_SHIELD); /* perm psi shield or armor */
            break;
        case 5:
            GET_OBJ_APPLY_LOC(obj, 1) = APPLY_HPV;
            GET_OBJ_APPLY_MOD(obj, 1) = rand_number(25, 50);
            break;
        case 6:
            SET_BIT_AR(GET_OBJ_AFFECT(obj), AFF_ARMORED_AURA); /* perm psi shield or armor */
            break;
    }

    // Regen (H/P/V and ALL), Sanc or Super Sanc
    switch (rand_number(1, 6)) {

        case 1:
            GET_OBJ_APPLY_LOC(obj, 2) = APPLY_HIT_REGEN;
            GET_OBJ_APPLY_MOD(obj, 2) = rand_number(25, 50);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), rand_number(26, 27)); /* perm sanc or super sanc */
            break;
        case 2:
            GET_OBJ_APPLY_LOC(obj, 2) = APPLY_PSI_REGEN;
            GET_OBJ_APPLY_MOD(obj, 2) = 50;
            break;
        case 3:
            GET_OBJ_APPLY_LOC(obj, 2) = APPLY_MOVE_REGEN;
            GET_OBJ_APPLY_MOD(obj, 2) = rand_number(25, 50);
            SET_BIT_AR(GET_OBJ_AFFECT(obj), rand_number(26, 27)); /* perm sanc or super sanc */
            break;
        case 4:
            GET_OBJ_APPLY_LOC(obj, 2) = APPLY_REGEN_ALL;
            GET_OBJ_APPLY_MOD(obj, 2) = rand_number(25, 50);
            break;
        case 5:
            SET_BIT_AR(GET_OBJ_AFFECT(obj), rand_number(26, 27)); /* perm sanc or super sanc */
            break;
    }

    send_to_room(IN_ROOM(ch), "Lucky the Leprachaun appears, holding a box of his Lucky Charms!\n\r");
    lucky = read_mobile(1202, VIRTUAL);
    char_to_room(lucky, IN_ROOM(ch));
    obj_to_char(obj, ch);

    if (lucky) {
        act("Lucky says, 'Hehehehehe, here ya go laddie, enjoy!", FALSE, lucky, 0, ch, TO_ROOM);
        act("Lucky gives you $p.", FALSE, lucky, obj, ch, TO_VICT);
        act("Lucky hands $p to $N.", FALSE, lucky, obj, ch, TO_NOTVICT);
    }

    mudlog(CMP, LVL_IMMORT, TRUE, "TOKEN: %s assembled a box of lucky charms.", GET_NAME(ch));
}

// the old function could find multiple saber matches, but made dual green before dual blue, before dual red.
// this one could exit early and create the first possible dual it finds
void assemble_dualsaber(struct char_data *ch)
{

    struct obj_data *obj;
    struct obj_data *saber = NULL;
    struct obj_data *coupler = NULL;
    struct obj_data *saber1 = NULL;
    struct obj_data *saber2 = NULL;
    struct obj_data *saber3 = NULL;
    struct obj_data *saber4 = NULL;
    struct obj_data *saber5 = NULL;
    struct obj_data *saber6 = NULL;

    // find the dual-hilt and two sabers of same color
    for (obj = ch->carrying; obj; obj = obj->next_content) {

        switch (GET_OBJ_VNUM(obj)) {
            case 1589:
                coupler = obj;
                break;

            case 1296:
                if (saber1) saber2 = obj;
                else saber1 = obj;
                break;

            case 1297:
                if (saber3) saber4 = obj;
                else saber3 = obj;
                break;

            case 1298:
                if (saber5) saber6 = obj;
                else saber5 = obj;
                break;

            default:
                break;
        }

        // did we find two pieces yet?
        if (((saber1 && saber2) || (saber3 && saber4) || (saber5 && saber6)) && coupler)
            break;

    }

    if (!coupler) {
        send_to_char(ch, "You will need a Double-Blade Locking Coupler to make this work!\r\n");
        return;
    }

    if (saber1 && saber2) {         // green
        extract_obj(saber1);
        extract_obj(saber2);
        saber = read_object(real_object(1286), REAL);
    }
    else if (saber3 && saber4) {    // blue
        extract_obj(saber3);
        extract_obj(saber4);
        saber = read_object(real_object(1287), REAL);
    }
    else if (saber5 && saber6) {    // red
        extract_obj(saber5);
        extract_obj(saber6);
        saber = read_object(real_object(1288), REAL);
    }
    else {
        send_to_char(ch, "You try, but the pieces don't seem to fit together properly.\n\r");
        return;
    }

    // destroy the dual-hilt
    extract_obj(coupler);
    obj_to_char(saber, ch);
    send_to_all("You sense a great disturbance in the Force as a weapon of immense power is forged.\n\r");
    mudlog(CMP, LVL_IMMORT, TRUE, "SABER: %s assembled %s.", GET_NAME(ch), saber->short_description);

}

ACMD(do_assemble)
{

    char arg[MAX_INPUT_LENGTH];

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    // Find what they want to assemble
    argument = one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Yes, but what do you want to assemble?\r\n");
        return;
    }

    if (!str_cmp(arg, "lightsaber") || !str_cmp(arg, "saber"))
        assemble_saber(ch);
    else if (!str_cmp(arg, "rainbow") || !str_cmp(arg, "tokens"))
        assemble_rainbow(ch);
    else if (!str_cmp(arg, "dual") || !str_cmp(arg, "dualsaber"))
        assemble_dualsaber(ch);
    else
        send_to_char(ch, "You don't seem to be able to assemble it!\r\n");

}


// Quick hack from handler.c to check flags and levels for grenades and mines -DH
int check_restrictions(struct char_data *ch, struct obj_data *obj)
{
  int invalid_class(struct char_data *ch, struct obj_data *obj);

    if (!IS_NPC(ch)) {

        // check for class restrictions
        if  (invalid_class(ch, obj)) {
            send_to_char(ch, "Funny, you don't seem to be able to use this item.\r\n");
            return (FALSE);
        }

        if (GET_LEVEL(ch) < LVL_GOD) {
            if ((OBJ_FLAGGED(obj, ITEM_BORG_ONLY) && !IS_BORG(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_CRAZY_ONLY) && !IS_CRAZY(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_MERCENARY_ONLY) && !IS_MERCENARY(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_STALKER_ONLY) && !IS_STALKER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_HIGHLANDER_ONLY) && !IS_HIGHLANDER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_PREDATOR_ONLY) && !IS_PREDATOR(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_CALLER_ONLY) && !IS_CALLER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY) && !IS_REMORT(ch))) {
                act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
                return (FALSE);
            }
        }

		if (GET_LEVEL(ch) < LVL_GOD) {
			if ((OBJ_FLAGGED(obj, ITEM_ANTI_BORG) && IS_BORG(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_CRAZY) && IS_CRAZY(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_MERCENARY) && IS_MERCENARY(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_STALKER) && IS_STALKER(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_HIGHLANDER) && IS_HIGHLANDER(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_PREDATOR) && IS_PREDATOR(ch)) ||
				(OBJ_FLAGGED(obj, ITEM_ANTI_CALLER) && IS_CALLER(ch))) {
				act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
				return (FALSE);
			}
		}

        // check for level restrictions
        if (GET_LEVEL(ch) < LVL_GOD) {
            if ((OBJ_FLAGGED(obj, ITEM_MINLEV15) && (GET_LEVEL(ch) < 15)) ||
                (OBJ_FLAGGED(obj, ITEM_MINLEV25) && (GET_LEVEL(ch) < 25)) ||
                (OBJ_FLAGGED(obj, ITEM_IMM_ONLY) && (GET_LEVEL(ch) < LVL_IMMORT)) ||
                (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY) && (GET_REMORTS(ch) < GET_OBJ_REMORT(obj)))) {
                act("You are too inexperienced to use this item.\n\r", FALSE, ch, 0, 0, TO_CHAR);
                return (FALSE);
            }
        }

        return (TRUE);
    }
    else {
        //mobs are okay, just return true!
        return (TRUE);
    }
}

ACMD(do_pull)
{
    struct obj_data *grenade;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int mode;
    struct char_data *temp;

    // Find what they want to pull
    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Yes, but what do you want to pull?\r\n");
        return;
    }
    mode = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &temp, &grenade);
    if (!grenade)
        send_to_char(ch, "You don't have one of those.");
    else if (GET_OBJ_TYPE(grenade) != ITEM_EXPLOSIVE ||
        (GET_OBJ_TYPE(grenade) == ITEM_EXPLOSIVE && GET_EXPLOSIVE_TYPE(grenade) != EXPLOSIVE_GRENADE))
        act("You can't pull $p.", FALSE, ch, grenade, 0, TO_CHAR);
    else {
        if (!check_restrictions(ch, grenade))
            return;
        if (GET_SKILL_LEVEL(ch, SKILL_EXPLOSIVES) == 0 && GET_LEVEL(ch) < LVL_IMMORT) {
			send_to_char(ch, "You fumble with trying to pull the pin off a grenade and oops....");
			explode(grenade);
        }
        // is this grenade already pulled?
        if (grenade->obj_flags.timer_on == TRUE) {
            send_to_char(ch, "You can't!! Someone already did!!\r\n");
            return;
        }
        // start the timer
        grenade->obj_flags.timer = GET_OBJ_VAL(grenade, 4);
        grenade->obj_flags.timer_on = TRUE;
        // identify the instigator
        if(IS_NPC(ch))
            grenade->user = -1;
        else
            grenade->user = GET_IDNUM(ch);
        // make the grenade non-rentable and non-sellable
        SET_BIT_AR(GET_OBJ_EXTRA(grenade), ITEM_NORENT);
        SET_BIT_AR(GET_OBJ_EXTRA(grenade), ITEM_NOSELL);
        // perform the action messages
        act("$n has pulled the pin from a grenade!!!\r\n", TRUE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "You pull the pin from a grenade!\r\n");
        return;
    }

}

ACMD(do_light)
{

    char arg[MAX_INPUT_LENGTH];
    int mode;
    struct obj_data *device;
    struct char_data *temp;

    one_argument(argument, arg);
    if (!*arg) {
        send_to_char(ch, "Light what?\r\n");
        return;
    }
    // allow any object in the inventory to be lit
    mode = generic_find(arg, FIND_OBJ_INV, ch, &temp, &device);
    if (!device) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
    }
    // is the object ITEM_TNT or ITEM_COCKTAIL?
    if (GET_OBJ_TYPE(device) != ITEM_EXPLOSIVE ||
        (GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_TNT && GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_COCKTAIL)) {
        send_to_char(ch, "That is not something you can light.\r\n");
        return;
    }
    // is the object already lit?
    if (IS_OBJ_TIMER_ON(device) == TRUE) {
        send_to_char(ch, "But it's already burning!\r\n");
        return;
    }
    // arm the device
    // todo: this should be a builder settable option instead of a default timer
    GET_OBJ_TIMER(device) = 5;
    device->obj_flags.timer_on = TRUE;

    // identify the instigator
    if (IS_NPC(ch))
        device->user = -1;
    else
        device->user = GET_IDNUM(ch);

    // make the plastique non-rentable and non-sellable
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NORENT);
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NOSELL);
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NODONATE);
    // REMOVE_BIT_AR(GET_OBJ_WEAR(device), ITEM_WEAR_TAKE);

    // perform the action messages
    act("You ignite $p.", FALSE, ch, device, 0, TO_CHAR);
    act("$n ignites $p.", TRUE, ch, device, 0, TO_ROOM);

    return;

}

// used to activate plastique and landmines
ACMD(do_arm)
{

    struct obj_data *device;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int mode;
    int timer = -1;
    struct char_data *temp;

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    // Find what they want to set
    two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "Yes, but what do you want to arm?\r\n");
        return;
    }

    mode = generic_find(arg1, FIND_OBJ_INV, ch, &temp, &device);
    if (!device) {
        send_to_char(ch, "You don't have one of those.\r\n");
        return;
    }

    if (GET_OBJ_TYPE(device) != ITEM_EXPLOSIVE ||
        (GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_PLASTIQUE && GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_MINE)) {
        act("You can't arm $p.", FALSE, ch, device, 0, TO_CHAR);
        return;
    }

    if (!check_restrictions(ch, device))
        return;

    if (GET_SKILL_LEVEL(ch, SKILL_EXPLOSIVES) == 0 && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You fumble with trying to arm the device and ooops!....");
		detonate(device, IN_ROOM(ch));
		return;
    }

    // is it already armed?
    if (IS_OBJ_TIMER_ON(device) == TRUE) {
        send_to_char(ch, "The %s is already set to detonate!!", device->short_description);
        return;
    }

    if (GET_EXPLOSIVE_TYPE(device) == EXPLOSIVE_MINE) {

        // mine
        obj_from_char(device);
        obj_to_room(device, IN_ROOM(ch));

    }
    else {

        // plastique
          if (!*arg2) {
            send_to_char(ch, "Usage: arm <plastique> <timer>.\r\n");
            return;
          }

          timer = atoi(arg2);

        // is the time between 1 sec and 30 seconds (arbitrary)
        if (timer < 1 || timer > 30) {
            send_to_char(ch, "The timer must be between 1 second and 30 seconds.\r\n");
            return;
        }

        // activate the plastique timer
        GET_OBJ_TIMER(device) = timer;

        // the plastique remains in the player's inventory.  they'll have to attach it!

    }

    // arm the mine, set timer when triggered
    device->obj_flags.timer = timer;
    device->obj_flags.timer_on = TRUE;

    // identify the instigator
    //if (IS_NPC(ch))
    //    device->user = -1;
    //else
    //    device->user = GET_IDNUM(ch);

    // make the plastique non-rentable and non-sellable
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NORENT);
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NOSELL);
    SET_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NODONATE);
    // REMOVE_BIT_AR(GET_OBJ_WEAR(device), ITEM_WEAR_TAKE);

    // perform the action messages
    act("$n arms $p!!!\r\n", TRUE, ch, device, 0, TO_ROOM);
    act("You arm $p.\r\n", TRUE, ch, device, 0, TO_CHAR);

    return;

}

// todo: eliminate the need for obj->plastique as this seems redundant and limiting
ACMD(do_attach)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *device;
    struct obj_data *obj;
    bool hold_flag = FALSE;
    int dir;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "What do you want to attach?\r\n");
        return;
    }

    device = get_obj_in_equip_vis(ch, arg1, NULL, ch->equipment);

    if (device != NULL && ch->equipment[WEAR_HOLD] == device)
        hold_flag = TRUE;
    else {

        device = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying);

        if (device == NULL) {
            send_to_char(ch, "You don't seem to have that.\r\n");
            return;
        }

    }

    // is the item ITEM_PLASTIQUE?
    if (GET_OBJ_TYPE(device) != ITEM_EXPLOSIVE || GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_PLASTIQUE) {
        send_to_char(ch, "You cannot attach that.\r\n");
        return;
    }

    // Is there a second argument?
    if (!*arg2) {
        send_to_char(ch, "Where do you want to attach it?\r\n");
        return;
    }

    // Is the 2nd argument an object in the room?
    if ((obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {

        // is the object an explosive?
        if (GET_OBJ_TYPE(obj) == ITEM_EXPLOSIVE) {
            send_to_char(ch, "You cannot attach plastique to other explosives!\r\n");
            return;
        }

        // remove the plastique from the character
        if (hold_flag == TRUE)
            obj_to_char(unequip_char(ch, WEAR_HOLD, FALSE), ch);

        obj_from_char(device);

        // set the plastique values
        GET_OBJ_VAL(device, 1) = PLASTIQUE_OBJ;
        device->attached = obj;
        obj->plastique = device;

        // generate successful message
        send_to_char(ch, "You attach %s to %s.\r\n", device->short_description, obj->short_description);

    }
    else if ((obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {

        // Is the 2nd argument an object in the players inv?

        // is the object an explosive?
        if (GET_OBJ_TYPE(obj) == ITEM_EXPLOSIVE) {
            send_to_char(ch, "You cannot attach plastique to other explosives!\r\n");
            return;
        }

        // remove the plastique from the character and put on object
        if (hold_flag == TRUE)
            obj_to_char(unequip_char(ch, WEAR_HOLD, FALSE), ch);

        obj_from_char(device);

        // set the plastique values
        GET_OBJ_VAL(device, 1) = PLASTIQUE_OBJ;
        device->attached = obj;
        obj->plastique = device;

        // generate successful message
        send_to_char(ch, "You attach %s to %s.\r\n", device->short_description, obj->short_description);

    }
    else if ((dir = search_block(arg2, dirs, FALSE)) > 0) {

        // Is the 2nd argument a room direction

        // is the exit open
        if (EXIT(ch, dir) && !EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
                send_to_char(ch, "What would be the point of blowing up an open exit?\r\n");
                return;
            }

        // is there already one attached to this exit?
        if (world[IN_ROOM(ch)].plastique[dir]) {
            send_to_char(ch, "It already seems to have one attached.\r\n");
            return;
        }

        // remove plastique from the char and attach to room
        if (hold_flag == TRUE)
            obj_to_char(unequip_char(ch, WEAR_HOLD, FALSE), ch);

        obj_from_char(device);
        world[IN_ROOM(ch)].plastique[dir] = device;

        // set plastique values
        GET_OBJ_VAL(device, 5) = IN_ROOM(ch);
        GET_OBJ_VAL(device, 1) = dir;

        // generate successful message
        send_to_char(ch, "You attach %s to the %s.\r\n", device->short_description, dirs[dir]);

    }
    else {

        send_to_char(ch, "You cannot attach it there.\r\n");
        return;

    }
}

ACMD(do_detach)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    struct obj_data *plastique;
    struct obj_data *obj;
    int dir;

    argument = two_arguments(argument, arg1, arg2);

    if (!*arg1) {
        send_to_char(ch, "What do you want to detach?\r\n");
        return;
    }

    if (!*arg2) {
        send_to_char(ch, "Where do you want to detach from?\r\n");
        return;
    }

    if ((obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {

        // the second argument is an object

        // does it have plastique attached to it?
        if (obj->plastique == NULL) {
            send_to_char(ch, "%s does not have anything attached to it.\r\n", arg1);
            return;
        }

        if (!(plastique = get_obj_in_list_vis(ch, arg1, NULL, obj->plastique))) {
            send_to_char(ch, "You cannot find %s.\r\n", arg1);
            return;
        }

        // clear all values
        obj->plastique = NULL;
        plastique->attached = NULL;
        GET_OBJ_VAL(plastique, 1) = -1;

        // put plastique in chars inventory
        obj_to_char(plastique, ch);

        // generate successful message
        send_to_char(ch, "You detach %s from %s.\r\n", plastique->short_description, obj->short_description);

    }
    else if ((obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {

        // does it have plastique attached to it?
        if (obj->plastique == NULL) {
            send_to_char(ch, "%s does not have anything attached to it.\r\n", arg2);
            return;
        }

        if (!(plastique = get_obj_in_list_vis(ch, arg1, NULL, obj->plastique))) {
            send_to_char(ch, "You cannot find %s.\r\n", arg1);
            return;
        }

        // clear all values
        obj->plastique = NULL;
        plastique->attached = NULL;
        GET_OBJ_VAL(plastique, 1) = -1;

        // put plastique in chars inventory
        obj_to_char(plastique, ch);

        // generate successful message
        send_to_char(ch, "You detach %s from %s.\r\n", plastique->short_description, obj->short_description);

    }
    else if ((dir = search_block(arg2, dirs, FALSE)) > 0) {

        // the second argument is a room direction

        // does the exit have plastique attached to it?
        if (world[IN_ROOM(ch)].plastique[dir] == NULL) {
            send_to_char(ch, "You cannot find %s.\r\n", arg1);
            return;
        }

        if (!(plastique = get_obj_in_list_vis(ch, arg1, NULL, world[IN_ROOM(ch)].plastique[dir]))) {
            send_to_char(ch, "You cannot find %s.\r\n", arg1);
            return;
        }

        // clear all values
        world[IN_ROOM(ch)].plastique[dir] = NULL;
        GET_OBJ_VAL(plastique, 5) = -1;
        GET_OBJ_VAL(plastique, 1) = -1;

        // put plastique in chars inventory
        obj_to_char(plastique, ch);

        // generate successful message
        send_to_char(ch, "You detach %s.\r\n", plastique->short_description);

    }
     else {
        send_to_char(ch, "You cannot find %s.\r\n", arg2);
        return;
      }
}

ACMD(do_throw)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	struct char_data *victim;
	struct obj_data *obj;
	bool find = TRUE;
	int hitchance;
	int savechance;
	int percent;
	int chdmg;
	int victdmg;
	int dam;
	
	two_arguments(argument, arg1, arg2);


//                if (!throw_otrigger(obj, ch))
//                 return;
	
	if (GET_SKILL_LEVEL(ch, SKILL_THROW) == 0) {
		send_to_char(ch, "You have no idea how to effectively throw weapons.\r\n");
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
		send_to_char(ch, "You can't move!\r\n");
		return;
	}
	
	if (!*arg1) {
		send_to_char(ch, "You need to specify an object to throw.\r\n");
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
		send_to_char(ch, "You don't seem to have any %s.\r\n", arg1);
		return;
	}

	if (!(victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
		if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
		else { 
			send_to_char(ch, "Throw it at who?\r\n");
			find = FALSE;
			return;
		}
	}

	if (GET_POS(victim) <= POS_DEAD)
		return;

	if (find == TRUE) {
		
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
			return;
		}	
		
		if (victim == ch) {
			send_to_char(ch, "You could just type 'suicide' to kill yourself.\r\n");
			return;
		}

		if (!IS_NPC(ch) && !IS_NPC(victim)) {
			send_to_char(ch, "You can't do that to other players.");
			return;
		}

		if (!check_restrictions(ch, obj))
			return;

		if (GET_OBJ_WEIGHT(obj) > str_app[STRENGTH_APPLY_INDEX(ch)].wield_w) {
			send_to_char(ch, "It's too heavy for you to throw properly.\r\n");
			return;
		}

		if (FIGHTING(ch) != victim)
			set_fighting(victim,ch);

		hitchance = (GET_LEVEL(ch) + (GET_REMORTS(ch) + 1)) * (GET_DEX(ch) + GET_PCN(ch) / 2) + GET_HITROLL(ch);
		savechance = (GET_LEVEL(victim) * (((GET_PCN(victim) + GET_DEX(victim)) / 2) * rand_number(1,3)));

		percent = hitchance - savechance;


		if (percent <= 0) {
			send_to_char(ch, "You fail to hit your target!\r\n");
		    obj_from_char(obj);
			get_encumbered(ch);
			obj_to_room(obj, IN_ROOM(victim));
			return;
		}

		if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
			chdmg = ((GET_LEVEL(ch) + (GET_REMORTS(ch) + 1)) + GET_STR(ch) + (GET_OBJ_VAL(obj, 1) * GET_OBJ_VAL(obj, 2)) + GET_DAMROLL(ch) * rand_number(1,5));
			victdmg = (GET_LEVEL(victim) + (GET_AC(victim))) * rand_number(1,5);
		}

		else {
			chdmg = ((GET_LEVEL(ch) + GET_REMORTS(ch) + 1) + GET_STR(ch) + GET_OBJ_WEIGHT(obj)) * rand_number(1,10);
			victdmg = (GET_LEVEL(victim) + GET_AC(victim) * rand_number(1,10));
		}

		dam = chdmg - victdmg;
		
		if (dam < 0)
			dam = 0;

		if (dam > 0) {
			sprintf(buf, "[%d] You hit %s squarely with %s!", dam, GET_NAME(victim), obj->short_description);
			act(buf, FALSE, ch, 0, 0, TO_CHAR, 0);
			act("$n throws $p at you, hitting you squarely!", FALSE, ch, obj, victim, TO_VICT, 0);
			act("$n throws $p at $N hitting $M squarely!", FALSE, ch, obj, victim, TO_NOTVICT, 3);
			int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 20;
			increment_skilluse(SKILL_THROW, 1, GET_IDNUM(ch), exp, ch);
			update_combo(ch,victim,SKILL_THROW);
		}
		if (dam == 0) {
			act("You throw $p at $N, but it absorbs the damage!", FALSE, ch, obj, victim, TO_CHAR, 0);
			act("$n throws $p at you, but you absorb the damage!", FALSE, ch, obj, victim, TO_VICT, 0);
			act("$n throws $p at $N but they absorb the damage!", FALSE, ch, obj, victim, TO_NOTVICT, 3);
			reset_combo(ch);
		}

		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
        obj_from_char(obj);
        obj_to_char(obj, victim);
        obj_from_char(obj);
        obj_to_char(obj, victim);
		obj_damage_char(obj, victim, dam);

		if (GET_POS(victim) > POS_DEAD) {
			if (obj && obj->obj_wpnpsi.which_psionic > 0) {
				int which_psionic = obj->obj_wpnpsi.which_psionic;
				int skill_level = obj->obj_wpnpsi.skill_level;
				wpn_psionic(ch, victim, obj, which_psionic, skill_level);
			}
		}

	}
}

ACMD(do_disarm)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *victim;
	struct obj_data *obj;
	bool find = TRUE;
	int percent;
	int d_roll;
	int v_roll;
	
	one_argument(argument, arg);
	
	if (GET_SKILL_LEVEL(ch, SKILL_DISARM) == 0) {
		send_to_char(ch, "You have no idea how to disarm your enemies.\r\n");
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
		send_to_char(ch, "You can't move!\r\n");
		return;
	}
	
	if (!*arg) {
		send_to_char(ch, "You need to specify a target to disarm.\r\n");
		return;
	}
	
	if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
		send_to_char(ch, "Disarm who?\r\n");
		find = FALSE;
		return;
	}
	if (find == TRUE) {
		
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
			return;
		}	
		
		if (victim == ch) {
			send_to_char(ch, "You can just let go of the weapon instead?\r\n");
			return;
		}
		
		if (!IS_NPC(ch) && !IS_NPC(victim)) {
			send_to_char(ch, "You can't do that to another player.");
			return;
		}

		if (!victim->equipment[WEAR_WIELD]) {
			send_to_char(ch, "Your target must be wielding a weapon to disarm.\r\n");
			return;
		}

		obj = victim->equipment[WEAR_WIELD];

		if (FIGHTING(ch) != victim)
			set_fighting(victim,ch);

		percent = rand_number(1, 101);    /* 101% is a complete failure */
		d_roll = GET_LEVEL(ch) + GET_REMORTS(ch) + GET_STR(ch) ;
		v_roll = GET_LEVEL(victim) + GET_STR(victim) + rand_number(0,10);

		if (percent == 101) {
			send_to_char(ch, "You fail to disarm your target and hurt yourself in the process!\r\n");
			reset_combo(ch);
			return;
		}

		if (v_roll > d_roll) {
			send_to_char(ch, "You fail to disarm your target!\r\n");
			reset_combo(ch);
			return;
		}

		act("You disarm $N and send $S weapon flying!", FALSE, ch, 0, victim, TO_CHAR);
		act("$N disarms you and sends your weapon flying!", FALSE, ch, 0, victim, TO_VICT);
		act("$n disarms $N and sends $S weapon flying!", FALSE, ch, 0, victim, TO_NOTVICT);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 50;
		increment_skilluse(SKILL_DISARM, 1, GET_IDNUM(ch), exp, ch);
		update_combo(ch,victim,SKILL_DISARM);
		if (!IS_NPC(victim))
			obj_to_char(unequip_char(ch, WEAR_WIELD, FALSE), ch);
		else 
			obj_to_room(unequip_char(victim, WEAR_WIELD, FALSE), IN_ROOM(victim));

		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
	}	
}

ACMD(do_rupture)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *victim;
	struct affected_type af;

    if (GET_SKILL_LEVEL(ch, SKILL_RUPTURE) == 0) {
        send_to_char(ch, "You have no idea how to rupture anyone.\n\r");
        return;
    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, buf);

    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Rupture who?\r\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch, "If you want to kill yourself type suicide.\r\n");
        return;
    }

    if (!GET_EQ(ch, WEAR_WIELD) || IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "You need to wield a pierce, claw, stab or sting melee weapon for this skill.\r\n");
        return;
    }

	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_CLAW - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT 
	) 
	{
		send_to_char(ch, "You need to wield a pierce, claw, stab or sting melee weapon for this skill.\r\n");
		return;
	}

    if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim)) {
        send_to_char(ch, "But that's not playing nice!\n\r");
        return;
    }

	int ch_roll = (GET_DEX(ch) + GET_HITROLL(ch) + GET_REMORTS(ch)) * GET_LEVEL(ch) / rand_number(10,20);
	int victim_roll = (GET_PCN(victim) + GET_DEX(victim) + GET_REMORTS(victim)) * GET_LEVEL(victim);
	
	act("You thrust your weapon towards $N!.", FALSE, ch, 0, victim, TO_CHAR);
	
    if (ch_roll < victim_roll) {
		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
        exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 1, 
			1, 
			TYPE_PIERCE, 
			0,
			0,
			0,
			"You completely miss your rupture attempt on $N!", 
			"$n tries to rupture you, but you evade!", 
			"$n tries to rupture $N in the neck, but misses!", 
			NULL, 
			NULL, 
			NULL,
			NULL,
			NULL, 
			NULL);
    }
    else {
        // success!
		int epic_win = rand_number(1,100);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 100;
		increment_skilluse(SKILL_BACKSTAB, 1, GET_IDNUM(ch), exp, ch);
		int damage = (GET_STR(ch)+GET_DEX(ch)+GET_DAMROLL(ch)+get_dice(ch)) + (rand_number(1,GET_LEVEL(ch)) * 2) * (GET_LEVEL(ch) * 2);
		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
		
		damage = damage - GET_AC(victim);
		
		char *hit_attacker, *hit_victim, *hit_room;
			
			if(damage < 1)
			{
				damage = 0;
				hit_attacker = "Your weapon clangs harmlessly off $N's armor!";
				hit_victim = "You feel the impact as $n's weapon hits, but your armor holds.";
				hit_room = "$n's weapon sparks off $N's armor, doing almost no damage!";
			}
			else
			{
				if(rand_number(1,2) == 1)
				{
					hit_attacker = "You drive your weapon into $N hard, blood begins to pour from the wound!";
					hit_victim = "You feel the shocking pain as $n drives $m weapon into you!";
					hit_room = "$n drives $m weapon into $N hard, blood begins to pour from the wound!";
				}
				else
				{
					hit_attacker = "You sink your weapon into $N, twisting it to make the wound worse!";
					hit_victim = "Agony courses through you as $n sinks $m weapon into your body and twists!";
					hit_room = "$n sinks $m weapon hilt-deep into $N, twisting it to make the wound worse!";
				}
				obj_to_char(unequip_char(ch, WEAR_WIELD, FALSE), victim);
				af.type = PSIONIC_BLEED;
				af.duration = 3;
				SET_BIT_AR(af.bitvector, AFF_BLEEDING);
				affect_to_char(victim, &af);
			}
			
		if(epic_win > 98)
		{
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				GET_HIT(victim), 
				TYPE_PIERCE, 
				0,
				0,
				0,
				hit_attacker,
				hit_victim,
				hit_room,
				NULL, 
				NULL, 
				NULL,
				"\x1B[41m\x1B[1;30m[FATALITY] Your rupture instantly kills $N!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m$n deals you a surprise FATAL blow!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m[FATALITY] $n twists the weapon inside $N as blood begins to pour from the mouth!\x1B[0;0m"
				);
		}
		else
		{
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				damage, 
				TYPE_PIERCE, 
				0,
				0,
				TRUE,
				hit_attacker,
				hit_victim,
				hit_room,
				NULL, 
				NULL, 
				NULL, 
				NULL, 
				NULL,
				NULL);
		}
    }

}
ACMD(do_disarmbomb)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *device;
    int mode;
    struct char_data *temp;

    one_argument(argument, arg);

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (!*arg) {
        send_to_char(ch, "Yes, but what do you want to disarm?\r\n");
        return;
    }

    mode = generic_find(arg, FIND_OBJ_INV, ch, &temp, &device);
    if (!device) {
        send_to_char(ch, "You don't have one of those.\r\n");
        return;
    }

    if (GET_OBJ_TYPE(device) != ITEM_EXPLOSIVE ||
        (GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_PLASTIQUE && GET_EXPLOSIVE_TYPE(device) != EXPLOSIVE_MINE)) {
        act("You can't disarm $p.", FALSE, ch, device, 0, TO_CHAR);
        return;
    }

    if (!check_restrictions(ch, device))
        return;

    if (GET_SKILL_LEVEL(ch, SKILL_EXPLOSIVES) == 0 && GET_LEVEL(ch) < LVL_IMMORT) {

        // failure - does it blow up?
        if (rand_number(1, 5) >= 3) {
            send_to_char(ch, "You hear a loud click!\r\n");
            act("$n fails to arm $p.", TRUE, ch, device, 0, TO_ROOM);
            detonate(device, IN_ROOM(ch));
            return;
        }

        // normal failure
        send_to_char(ch, "That doesn't seem to work.\r\n");
        return;

    }

    if (IS_OBJ_TIMER_ON(device) == FALSE) {
        send_to_char(ch, "The %s is not set to detonate.", device->short_description);
        return;
    }

    // disarm the mine, clear the timer
    device->obj_flags.timer = -1;
    device->obj_flags.timer_on = FALSE;

    // clear the instigator
    device->user = -1;

    // make the mine non-rentable and non-sellable
    REMOVE_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NORENT);
    REMOVE_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NOSELL);
    REMOVE_BIT_AR(GET_OBJ_EXTRA(device), ITEM_NODONATE);

    // perform the action messages
    act("$n disarms $p from detonation!\r\n", TRUE, ch, device, 0, TO_ROOM);
    act("You disarm $p.\r\n", TRUE, ch, device, 0, TO_CHAR);

    return;

}

// Mac's drug code from 2002 */
// To clarify some of the things here...

// D_AFFECT = tells WHICH drug they are on...only can be on one at a time.
// D_TIMER = How much longer they will be high
// GET_ADDICTION = Their addiction rating.  100+ = dependent
// Object Values for Drugs -->
//    1 = D_AFFECT (room for unlimited expansion (?))
//    2 = D_TIMER
//    3 = Smoked or Injected?

#define MAX_DRUG_AFFECTS 5

ACMD(do_inject)
{
    char arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    struct char_data *vict;
    struct char_data *temp;
    struct char_data *tmp_victim;
    struct obj_data *drug;
    struct affected_type af[MAX_DRUG_AFFECTS];
    bool accum_affect = FALSE;
    bool accum_duration = FALSE;
    char *to_vict = NULL;
    char *to_room = NULL;
    bool violence = FALSE;
    int i;
    int d_roll;
    int drugnum;
    ubyte overdose = 0;
    ubyte addiction = 0;

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    half_chop(argument, arg, arg1);

    if (!*arg) {
        send_to_char(ch, "Usage: Inject <drug>\r\n");
        return;
    }

    vict = ch;

    if (!(drug = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
        send_to_char(ch, "You don't seem to have the item you are looking for.\r\n");
        return;
    }
    else if (GET_OBJ_TYPE(drug) != ITEM_DRUG) {
        send_to_char(ch, "Thats not a drug! Man, are you that bad of an addict?\r\n");
        return;
    }

    if (!vict || !ch || !drug)
        return;

    drugnum = GET_DAFFECT(drug);

    if (GET_CLASS(ch) == CLASS_CRAZY)
        d_roll = GET_DTIMER(drug) * 1.5;
    else
        d_roll = rand_number(1, GET_DTIMER(drug) + 1);

    for (i = 0; i < MAX_DRUG_AFFECTS; i++) {
		new_affect(&(af[i]));
        af[i].type = drugnum;
        af[i].duration = d_roll;
    }

    // default messages, in case there is none set below
    to_room = "$n whips out a syringe and injects some drugs into $s body.";
    to_vict = "Well, here goes nothing...!";

    obj_from_char(drug);
    extract_obj(drug);

    switch (drugnum) {

        case DRUG_NONE:
            //nothing happens
            to_vict = "DUDE! You got ripped! This shit ain't any stronger than maple syrup!";
            break;

        case DRUG_JUICE:
            af[0].location = APPLY_HITNDAM;
            af[0].modifier = 10;
            af[1].location = APPLY_INT;
            af[1].modifier = -2;
            af[2].location = APPLY_STR;
            af[2].modifier = -1;
            af[2].duration = 10;
            addiction += 14;

            violence = TRUE;
            to_vict = "Veins bulge from your neck as you inject the Juice into your jugular.\r\n@rA voice in your head says, \'MUST KILL!\'@n";
            to_room = "$n's face turns red as $e injects something directly into $s throbbing jugular vein!";
            break;

        case DRUG_NUKE:
            af[0].location = APPLY_DAMROLL;
            af[0].modifier = 20;
            af[1].location = APPLY_PCN;
            af[1].modifier = -3;
            af[2].location = APPLY_INT;
            af[2].modifier = -4;
            af[2].duration = 10;
            addiction += 10;
            break;

        case DRUG_GLITCH:
            if (HAS_BIONIC(ch, BIO_INTERFACE) && ch->char_specials.saved.percent_machine > 20) { //negligible affects
                af[0].location = APPLY_STR;
                af[0].modifier = +1;
                af[1].location = APPLY_INT;
                af[1].modifier = -1;
            }
            else {
                af[0].location = APPLY_STR;
                af[0].modifier = 3;
                af[1].location = APPLY_CON;
                af[1].modifier = 3;
            }
            send_to_char(vict, "You pound a syringe of glitch into your heart, releasing the drug into your system.\r\n");
            to_vict = "You grit your teeth as your muscle tissue tears and reforms much more stronger!";
            to_room = "$n whips out a syringe and stabs $mself directly in the heart...$s body grows bigger and stronger!";
            break;

        case DRUG_JOLT:
            af[1].location = APPLY_INT;
            af[1].modifier = 2;
            af[2].location = APPLY_CON;
            af[2].modifier = -1;
            af[2].duration = 10;

            to_vict = "You casually inject a needle filled with jolt into your bloodstream.\r\nSharp shocking pains pulsate momentarily throughout your brain.\r\nYou feel smarter!";
            to_room = "$n casually inserts a needle filled with jolt into $s arm, the shocking pains throughout $s head are clearly evident as $e massages $s throbbing temples.";
            break;

        case DRUG_FLASH:
            //this also add one attack in fight.c
            af[0].location = APPLY_DEX;
            af[0].modifier = 2;
            af[2].location = APPLY_INT;
            af[2].modifier = -4;
            af[2].duration = 10;

            to_vict = "You jump around hastily after injecting a syringe of flash into your body.\r\nThe world seems to be moving in slow motion.";
            to_room = "$n seems to tweak out after injecting $mself with a syringe of flash!";
            break;

        case DRUG_MIRAGE:
            //create hallucinations on every mob, or cast dupes?
            af[1].location = APPLY_REGEN_ALL;
            af[1].modifier = 10;
            af[2].location = APPLY_PCN;
            af[2].modifier = -2;

            to_vict = "You smile confidently after injecting mirage into your body. Nothing can break your spirit!";
            to_room = "$n smiles confidently after injecting mirage into $s body. $n grins as $e sizes you up.";
            break;

        case DRUG_GOD:
            af[0].location = APPLY_HITNDAM;
            af[0].modifier = 20;
            af[1].location = APPLY_ARMOR;
            af[1].modifier = 15;
            af[2].location = APPLY_ALL_STATS;
            af[2].modifier = 2;

            addiction += 10;
            overdose += 10;

            if (!rand_number(0, 10))
                overdose += 10;

            to_vict = "You inject a syringe filled with a pulsating white liquid into your eye.\r\nYou grin evilly as your power becomes so immense that you feel IMMORTAL !!";
            to_room = "$n stabs a syringe of GOD into $s eye! $n becomes ALL POWERFUL!";
            break;

        case DRUG_MYST:
            af[0].location = APPLY_AGE;
            af[0].modifier = -50;
            af[1].location = APPLY_DEX;
            af[1].modifier = +2;
            af[2].location = APPLY_INT;
            af[2].modifier = -2;

            to_vict = "You inject a syringe filled with a clear blue substance.\r\nYou feel like a kid again! YIPPEE!";
            to_room = "You watch and laugh as $n skips around, singing nursery rhymes and giggling like a school girl!";
            break;

        case DRUG_KO:
            af[0].location = APPLY_ARMOR;
            af[0].modifier = 50;
            af[1].location = APPLY_DEX;
            af[1].modifier = -2;

            if (!rand_number(0, 6))
                SET_BIT_AR(af[0].bitvector, AFF_PARALYZE);

            to_vict = "You fall to the ground as KO does exactly that to you. Drugs are bad mmmkay!";
            to_room = "$n falls to the floor as a syringe of KO takes its toll on $m.";
            break;

        case DRUG_MENACE:
            //panic room
            to_vict = "You inject a vial of menace into your wrist. You feel no different...";
            to_room = "$n injects something into $s wrist. $n says, \'Im going to kill you! All of you!\'";
            violence = TRUE;
            break;

        case DRUG_VIRGIN:
            af[0].location = APPLY_AGE;
            af[0].modifier = -30;
            af[1].location = APPLY_REGEN_ALL;
            af[1].modifier = 30;
            addiction += rand_number(0, 140);

            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BLIND);

            if (affected_by_psionic(vict, PSIONIC_POISON))
                affect_from_char(vict, PSIONIC_POISON, TRUE);

            if (IS_ON_DRUGS(vict) || GET_OVERDOSE(vict) > 0 || GET_ADDICTION(vict) > 0) {
                GET_OVERDOSE(vict) = 0;
                GET_ADDICTION(vict) = 0;
            }

            to_vict = "You laugh as the power of virgin travels through your body with supreme quickness! You feel like a brand new person!";
            to_room = "$n injects $mself with a syringe of virgin! $e looks alot better!";
            break;

        case DRUG_ICE:
            af[1].location = APPLY_CON;
            af[1].modifier = -2;

            to_vict = "You shiver uncomfortably for a moment as the ICE travels through your bloodstream to your brain!";
            to_room = "$n shivers for a moment as $e shoves a needle into a vein, introducing the effects of ICE to $s body.";
            break;

        case DRUG_PEAK:
            af[1].location = APPLY_PCN;
            af[1].modifier = 2;
            af[2].location = APPLY_INT;
            af[2].modifier = -2;
            af[3].location = APPLY_DEX;
            af[3].modifier = 4;

            violence = TRUE;
            addiction += 7;
            overdose += 7;
            to_vict = "You feel like a combat machine after injecting yourself with PEAK! Lets shed some blood!!";
            to_room = "$n growls angrily after injecting $mself with a needle filled with PEAK! $n says, 'LETS SHED SOME BLOOD!'";
            break;

        case DRUG_VELVET:
            af[0].location = APPLY_STR;
            af[0].modifier = 10;

            //need to add paranoia, +1 attack in fight
            violence = TRUE;
            break;

        case DRUG_LAVA:
            af[0].location = APPLY_ARMOR;
            af[0].modifier = 40;
            af[1].location = APPLY_STR;
            af[1].modifier = 4;
            af[2].location = APPLY_INT;
            af[2].modifier = -2;
            af[3].location = APPLY_DEX;
            af[3].modifier = -2;

            addiction = 20;
            overdose = 20;
            to_vict = "You hear your bones reform and tissue rip as the Lava burns its way through your bloodstream!";
            to_room = "Cracking and tearing sounds come from $n's body as he injects Lava into $s bloodstream! $n screams in pain!";
            break;

        case DRUG_AMBROSIA:
            af[0].location = APPLY_INT;
            af[0].modifier = 3;
            af[1].location = APPLY_PCN;
            af[1].modifier = 3;
            GET_ADDICTION(vict) += 10;
            accum_affect = TRUE;
            break;

        case DRUG_VOODOO:
            af[0].location = APPLY_INT;
            af[0].modifier = rand_number(-3, 3);
            af[1].location = APPLY_PCN;
            af[1].modifier = rand_number(-3, 3);
            af[2].location = APPLY_STR;
            af[2].modifier = rand_number(-3, 3);
            addiction += rand_number(0, 15);
            if (!rand_number(0, 3))
                violence = TRUE;
            break;

        case DRUG_EMERALD:
            af[0].location = APPLY_REGEN_ALL;
            af[0].modifier = rand_number(30, 50);
            af[1].location = APPLY_INT;
            af[1].modifier = rand_number(-3, -1);
            addiction = 10;

            send_to_char(vict, "A syringe of Emerald glistens beautifully as you empty it into your veins.\r\n");
            to_vict = "The green chemical instantly makes you feel more refreshed!";
            to_room = "A syringe of Emerald glistens in the light as $n empties it into $s veins! $n looks refreshed!";
            break;

        case DRUG_HONEY:
            af[1].location = APPLY_HITNDAM;
            af[1].modifier = rand_number(-10, 10);
            overdose = 40;
            accum_affect = TRUE;

            to_vict = "The honey crawls through your body, you can feel the thick yellow substance rubbing against your organs!!";
            to_room = "$n injects a thick yellow liquid into $s body. $n looks uncomfortable as the drug tickles $s insides.";
            break;

        case DRUG_MEDKIT:
            af[0].location = APPLY_STR;
            af[0].modifier = -1;
            to_vict = "Your wounds have been bandaged.";
            to_room = "$n is feeling better now.";
            GET_HIT(vict) += 100;
            update_pos(vict);
            break;

        case DRUG_STEROIDS:
            af[0].location = APPLY_STR;
            af[0].modifier = GET_LEVEL(ch)/6;

            violence = TRUE;
            to_vict = "Pumped up and ready to go!";
            to_room = "$n flexes, trying to show-off eh?";
            break;

        case DRUG_MORPHINE:
            af[0].location = APPLY_DEX;
            af[0].modifier = -2;
            af[1].location = APPLY_STR;
            af[1].modifier = -2;
            to_vict = "Ahhhhh....mmmmmmuch better...";
            to_room = "$n looks pretty relaxed right about now.";
            GET_POS(vict) = POS_RESTING;
            addiction += 10;
            overdose += 10;
            break;

        case DRUG_CYANIDE:
            send_to_char(vict, "Man, I don't think that last batch was any good...\r\n");
            to_room = "$n took a bad hit!";
            GET_HIT(vict) = -10;
            update_pos(vict);
            break;

        case DRUG_ANTIPOISON: /* Poison Antidote */
            af[0].location = APPLY_DEX;
            af[0].modifier = -1;
            af[1].location = APPLY_STR;
            af[1].modifier = -1;
            to_vict = "Ahhhhh....mmmmmmuch better...";
            to_room = "$n looks to be in better condition now.";
            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
            GET_PSI(vict) -= 50;
            GET_HIT(vict) += 10;
            update_pos(vict);
            break;

        default:
            log("SYSERR: Tried to inject unknown drug type: %d (%s)", drugnum, arg1);
            return;

    } //end switch

    if (vict == NULL || ch == NULL)
        return;

    GET_OVERDOSE(vict) = MIN(GET_OVERDOSE(vict) + overdose, 250);
    GET_ADDICTION(vict) = MIN(GET_ADDICTION(vict) + addiction, 250);

    // this should be based on CON and race maybe?
    if (!IS_NPC(vict) && GET_OVERDOSE(vict) >= 200 && GET_HIT(vict) > 0) {
        send_to_char(vict, "Your body begins to spasm as an overdose of drugs is introduced to your system.\r\n");
        sprintf(buf, "%s drops to the ground and begins to go into seizures.", GET_NAME(vict));
        act(buf, FALSE, vict, 0, 0, TO_ROOM);
        GET_HIT(vict) = -10;
        update_pos(vict);
        return;
    }

    // If this is a mob that has this affect set in its mob file, do not
    // perform the affect.  This prevents people from un-sancting mobs
    // by sancting them and waiting for it to fade, for example.
    // this shouldn't be a problem now...can only self-inject anyway
    //if (IS_NPC(vict) && AFF_FLAGGED(vict, af[0].bitvector|af[1].bitvector) && !affected_by_drug(vict, drugnum)) {
    //    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    //    return;
    //}

    // If the victim is already affected by this psionic, and the psionic does
    // not have an accumulative effect, then fail the psionic.
    if (affected_by_drug(vict,drugnum) && !(accum_duration||accum_affect)) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
    }
	for (i = 0; i < MAX_DRUG_AFFECTS; i++)
		if (af[i].bitvector[0] || af[i].bitvector[1] || af[i].bitvector[2] || af[i].bitvector[3] || (af[i].location != APPLY_NONE)) {
			if (accum_duration || accum_affect)
				drug_join(vict, af+i, accum_duration, FALSE, accum_affect, FALSE);
			else
				drug_to_char(vict, af+i);
		}

    if (to_vict != NULL)
        act(to_vict, FALSE, vict, 0, ch, TO_CHAR);

    if (to_room != NULL)
        act(to_room, TRUE, vict, 0, ch, TO_ROOM);

    // this will berserk everyone in the room
    if (violence) {
        act("$n goes BERSERK!!!", FALSE, ch, 0, 0, TO_ROOM);

        for (tmp_victim = character_list; tmp_victim; tmp_victim = temp) {

            temp = tmp_victim->next;

            if (!tmp_victim && temp)
                continue;

            if (!tmp_victim && !temp)
                return;

            if (IN_ROOM(ch) != IN_ROOM(tmp_victim))
                continue;

            if (ch == tmp_victim)
                continue;

            if (tmp_victim->master == ch)
                continue;

            if (ch->master == tmp_victim)
                continue;

            if (!CAN_SEE(ch, tmp_victim))
                continue;

            if (!IS_NPC(ch) && !IS_NPC(tmp_victim) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK))
                continue;

            if (IS_DEAD(ch) || IS_DEAD(tmp_victim))
                continue;
			int thaco = rand_number(1,30);

            act("$N hits you with their berserker rage!", FALSE, tmp_victim, 0, ch, TO_CHAR);
            hit(ch, tmp_victim, TYPE_UNDEFINED, thaco, 0);
        }

    }

}

