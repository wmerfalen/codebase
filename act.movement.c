/**************************************************************************
*  File: act.movement.c                                    Part of tbaMUD *
*  Usage: Movement commands, door handling, & sleep/rest/etc state.       *
*                                                                         *
*  All rights reserved.  See license complete information.                *
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
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "oasis.h" /* for buildwalk */
#include "events.h"
#include "skilltree.h"


/* local only functions */
/* do_simple_move utility functions */
static int has_boat(struct char_data *ch);
/* do_gen_door utility functions */
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
static int has_key(struct char_data *ch, obj_vnum key);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);

void get_encumbered(struct char_data *ch)
{
	float percent = (((float)IS_CARRYING_W(ch) + (float)IS_WEARING_W(ch)) / (float)CAN_CARRY_W(ch));
	int encumbrance = (int)(percent * 100);
	
	if (encumbrance > 100) {
		encumbrance = 100;
	}
	if (encumbrance < 0) {
		encumbrance = 0;
	}
	else {
		ENCUMBRANCE(ch) = encumbrance;
	}
}
/* simple function to determine if char can walk on water */
static int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK) || AFF_FLAGGED(ch, AFF_FLYING) || AFF_FLAGGED(ch, AFF_LEVITATE) || HAS_BIONIC(ch, BIO_JET))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  return (0);
}

/* Simple function to determine if char can fly. */
int has_flight(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_FLYING))
    return (1);

  /* Non-wearable flying items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && OBJAFF_FLAGGED(obj, AFF_FLYING))
      return (1);

  /* Any equipped objects with AFF_FLYING will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_FLYING))
      return (1);

  return (0);
}

/* Simple function to determine if char can scuba. */
int has_scuba(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_SCUBA) || AFF_FLAGGED(ch, AFF_BIO_TRANCE))
    return (1);

  /* Non-wearable scuba items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_SCUBA) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_SCUBA will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_SCUBA))
      return (1);

  return (0);
}

/** Move a PC/NPC character from their current location to a new location. This
 * is the standard movement locomotion function that all normal walking
 * movement by characters should be sent through. This function also defines
 * the move cost of normal locomotion as:
 * ( (move cost for source room) + (move cost for destination) ) / 2
 *
 * @pre Function assumes that ch has no master controlling character, that
 * ch has no followers (in other words followers won't be moved by this
 * function) and that the direction traveled in is one of the valid, enumerated
 * direction.
 * @param ch The character structure to attempt to move.
 * @param dir The defined direction (NORTH, SOUTH, etc...) to attempt to
 * move into.
 * @param need_specials_check If TRUE will cause
 * @retval int 1 for a successful move (ch is now in a new location)
 * or 0 for a failed move (ch is still in the original location). */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  /* Begin Local variable definitions */
  /*---------------------------------------------------------------------*/
  /* Used in our special proc check. By default, we pass a NULL argument
   * when checking for specials */
  char spec_proc_args[MAX_INPUT_LENGTH] = "";
  /* The room the character is currently in and will move from... */
  room_rnum was_in = IN_ROOM(ch);
  /* ... and the room the character will move into. */
  room_rnum going_to = EXIT(ch, dir)->to_room;
  /* How many movement points are required to travel from was_in to going_to.
   * We redefine this later when we need it. */
  int need_movement = 1;
  /* Contains the "leave" message to display to the was_in room. */
  char leave_message[SMALL_BUFSIZE];
    /* variables used to iterate through players to look for highlanders */
    struct descriptor_data *i;
    bool found;
    /* variabls used to check for landmines */
    struct obj_data *obj, *next_obj;
  /*---------------------------------------------------------------------*/
  /* End Local variable definitions */


  /* Begin checks that can prevent a character from leaving the was_in room. */
  /* Future checks should be implemented within this section and return 0.   */
  /*---------------------------------------------------------------------*/
  /* Check for special routines that might activate because of the move and
   * also might prevent the movement. Special requires commands, so we pass
   * in the "command" equivalent of the direction (ie. North is '1' in the
   * command list, but NORTH is defined as '0').
   * Note -- only check if following; this avoids 'double spec-proc' bug */
  if (need_specials_check && special(ch, dir + 1, spec_proc_args))
    return 0;

  /* Leave Trigger Checks: Does a leave trigger block exit from the room? */
  if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;

  /* Charm effect: Does it override the movement? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && was_in == IN_ROOM(ch->master))
  {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* Water, No Swimming Rooms: Does the deep water prevent movement? */
  if ((SECT(was_in) == SECT_WATER_NOSWIM) ||
      (SECT(going_to) == SECT_WATER_NOSWIM))
  {
    if (!has_boat(ch))
    {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }
  
  if ((SECT(was_in) == SECT_VEHICLE_ONLY) ||
	  (SECT(going_to) == SECT_VEHICLE_ONLY)) {
	  if (!IN_VEHICLE(ch)) {
		send_to_char(ch, "You need to be inside a vehicle to go there.\r\n");
		return (0);
	  }
	}

  /* Flying Required: Does lack of flying prevent movement? */
  if ((SECT(was_in) == SECT_FLYING) || (SECT(going_to) == SECT_FLYING))
  {
    if (!has_flight(ch))
    {
      send_to_char(ch, "You need to be flying to go there!\r\n");
      return (0);
    }
  }

  /* Underwater Room: Does lack of underwater breathing prevent movement? */
  if ((SECT(was_in) == SECT_UNDERWATER) || (SECT(going_to) == SECT_UNDERWATER))
  {
    if (!has_scuba(ch) && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
      send_to_char(ch, "You need to be able to breathe water to go there!\r\n");
      return (0);
    }
  }

  /* Houses: Can the player walk into the house? */
  if (ROOM_FLAGGED(was_in, ROOM_ATRIUM))
  {
    if (!House_can_enter(ch, GET_ROOM_VNUM(going_to)))
    {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }

  /* Check zone flag restrictions */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_CLOSED)) {
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
    return (0);
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
    return (0);
  }

  /* Room Size Capacity: Is the room full of people already? */
  if (ROOM_FLAGGED(going_to, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[going_to])) >= CONFIG_TUNNEL_SIZE)
  {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
	if (ROOM_FLAGGED(going_to, ROOM_MERC_ONLY) && !IS_MERCENARY(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_CRAZY_ONLY) && !IS_CRAZY(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_STALKER_ONLY) && !IS_STALKER(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_BORG_ONLY) && !IS_BORG(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_CALLER_ONLY) && !IS_CALLER(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_HIGHLANDER_ONLY) && !IS_HIGHLANDER(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
	if (ROOM_FLAGGED(going_to, ROOM_PREDATOR_ONLY) && !IS_PREDATOR(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
		send_to_char(ch, "You're not allowed in there.\r\n");
		return (0);
	}
  /* Room Level Requirements: Is ch privileged enough to enter the room? */
  if (ROOM_FLAGGED(going_to, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GOD)
  {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }
    if (PART_MISSING(ch, BODY_PART_RIGHT_FOOT) && PART_MISSING(ch, BODY_PART_LEFT_FOOT)) {
        send_to_char(ch, "You lack the ability to move anywhere.\r\n");
        return (0);
    }
  /* All checks passed, nothing will prevent movement now other than lack of
   * move points. */
  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(was_in)] +
		   movement_loss[SECT(going_to)]) / 2;

  /* Move Point Requirement Check */
  /* GAHANTODO: Add encumbrance shit here */
    /* make random chance of no-movement costing 1 mv? */
    if (AFF_FLAGGED(ch, AFF_LEVITATE) || HAS_BIONIC(ch, BIO_JET))
		need_movement = need_movement / 2;

    if (AFF_FLAGGED(ch, AFF_SNEAK)) /* sneaking requires more movement points */
        need_movement += 5;

    if (AFF_FLAGGED(ch, AFF_MANIAC)) /* Maniacs should chill out! */
        need_movement += 5;

    if (BIONIC_LEVEL(ch, BIO_JET) == BIONIC_ADVANCED)
		need_movement = need_movement / 3;
  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
  {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }

  /*---------------------------------------------------------------------*/
  /* End checks that can prevent a character from leaving the was_in room. */


  /* Begin: the leave operation. */
  /*---------------------------------------------------------------------*/
  /* If applicable, subtract movement cost. */
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch))
    GET_MOVE(ch) -= need_movement;

  /* Generate the leave message and display to others in the was_in room. */
  if (!AFF_FLAGGED(ch, AFF_SNEAK))
  {
    snprintf(leave_message, sizeof(leave_message), "$n leaves %s.", dirs[dir]);
    act(leave_message, TRUE, ch, 0, 0, TO_ROOM, 2);
  }

  char_from_room(ch);
  char_to_room(ch, going_to);
  /*---------------------------------------------------------------------*/
  /* End: the leave operation. The character is now in the new room. */


  /* Begin: Post-move operations. */
  /*---------------------------------------------------------------------*/
  /* Post Move Trigger Checks: Check the new room for triggers.
   * Assumptions: The character has already truly left the was_in room. If
   * the entry trigger "prevents" movement into the room, it is the triggers
   * job to provide a message to the original was_in room. */
//  if (!entry_mtrigger(ch) || !enter_otrigger(obj, ch) || !enter_wtrigger(&world[going_to], ch, dir)) {
  if (!entry_mtrigger(ch) || !enter_wtrigger(&world[going_to], ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  /* Display arrival information to anyone in the destination room... */
  if (!AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM, 2);

  /* ... and the room description to the character. */
  if (ch->desc != NULL)
    look_at_room(ch, 0);

  /* ... and Kill the player if the room is a death trap. */
    if (!IS_NPC(ch) && ROOM_FLAGGED(going_to, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT) {
        mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(going_to), world[going_to].name);

		send_to_char(ch, "ZAP!! You have just hit a Death Trap.\r\n"
						 "The smell of your own flesh kind of reminds you of overcooked sausages you used to make in college.\r\n"
						 "Luckily for you, this doesn't count as a death, nor cost you any of your money or valuables.\r\n"
						 "Punishments for Death Traps of this sort force you to Motown armory, and lags you for a slightly\r\n"
						 "rediculous amount of time.  Be careful though, not all Death Traps save you from real death,\r\n"
						 "and cause you to pay for it.  In other words, look before you leap!\r\n");

        char_from_room(ch);
        char_to_room(ch, real_room(3001));
        GET_WAIT_STATE(ch) = 20 * PULSE_VIOLENCE;
        return (0);
    }

  /* At this point, the character is safe and in the room. */
  /* Fire memory and greet triggers, check and see if the greet trigger
   * prevents movement, and if so, move the player back to the previous room. */
  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir))
  {
    char_from_room(ch);
    char_to_room(ch, was_in);
    look_at_room(ch, 0);
    /* Failed move, return a failure */
    return (0);
  }
  else
    greet_memory_mtrigger(ch);
    /* check if this character is a highlander and can sense other highlanders in the area */
    /* Highlander "buzz" -Lump */
    if (!IS_NPC(ch) && (world[was_in].zone != world[going_to].zone) && IS_HIGHLANDER(ch)) {

        found = FALSE;

        for (i = descriptor_list; i; i = i->next) {
            if (!i->connected && i != ch->desc && i->character &&
                !PLR_FLAGGED(i->character, PLR_WRITING) &&
                (world[going_to].zone == world[IN_ROOM(i->character)].zone) &&
                (GET_POS(i->character) >= POS_RESTING) &&
                (GET_CLASS(i->character) == CLASS_HIGHLANDER)) {

                    act("You sense another immortal in the area...", FALSE, i->character, 0, 0, TO_CHAR);
                    act("$n looks around suspiciously.", TRUE, i->character, 0, 0, TO_NOTVICT);
                    found = TRUE;
                }

        }
    }

    /* Sector Pausing, used to slow down movement in quest zones (CtF/CW) -Lump */
    /* if they've entered the room, make them stop for one second */
    if (ch && (GET_LEVEL(ch) < LVL_IMMORT) && (SECT(going_to) == SECT_PAUSE))
        GET_WAIT_STATE(ch) = 1 RL_SEC;

    /* Now that they've entered the room, make them stop for two seconds */
    if (ch && (GET_LEVEL(ch) < LVL_IMMORT) && room_affected_by_psionic(going_to, PSIONIC_LAG))
        GET_WAIT_STATE(ch) = 2 RL_SEC;

    /* check for a land mine */
    // have mines that are set for remort only or level or class
    // "smart mines" idea by Shrike
    for (obj = world[going_to].contents; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (GET_OBJ_TYPE(obj) == ITEM_EXPLOSIVE && GET_EXPLOSIVE_TYPE(obj) == EXPLOSIVE_MINE
            && obj->obj_flags.timer_on == TRUE && obj->obj_flags.timer == -1) {

            if (AFF_FLAGGED(ch, AFF_SNEAK) && rand_number(0, 10) >= 7)
                send_to_char(ch, "You quickly side-step, avoiding a land mine!!!\r\n");
            else {
                send_to_room(going_to, "Uh-oh...\r\n");
                obj->obj_flags.timer = GET_OBJ_VAL(obj, 4);
                detonate(obj, going_to);
            }
        }
    }
  /*---------------------------------------------------------------------*/
  /* End: Post-move operations. */

  /* Only here is the move successful *and* complete. Return success for
   * calling functions to handle post move operations. */
  return (1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  char buf[MAX_INPUT_LENGTH];
  int numleave = 0;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
    return (0);
  else if (!CONFIG_DIAGONAL_DIRS && IS_DIAGONAL(dir))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE)
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)))) {
    if (EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else
      send_to_char(ch, "It seems to be closed.\r\n");
  } else {
 	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOLEAVE) && ch->followers) {
		for (k = ch->followers; k; k = next) {
			next = k->next;
			if ((GET_POS(k->follower) < POS_STANDING || GET_MOVE(k->follower) < 20) && IN_ROOM(ch) == IN_ROOM(k->follower))
				numleave++;
			if (!CAN_GO(k->follower, dir) && IN_ROOM(ch) == IN_ROOM(k->follower))
				numleave++;
			if (numleave > 0) {
				send_to_char(ch, "%s would be left behind!\r\n", GET_NAME(k->follower));
				send_to_char(k->follower, "%s tries to leave but you would be left behind!\r\n", GET_NAME(ch));
				return (0);
			}
		}
	}
    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);
    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((IN_ROOM(k->follower) == was_in) &&
	  (GET_POS(k->follower) >= POS_STANDING)) {
	  snprintf(buf, sizeof(buf), "You follow $N %s.", dirs[dir]);
	act(buf, FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return (1);
  }
  return (0);
}

ACMD(do_move)
{
  /* These subcmd defines are mapped precisely to the direction defines. */
  if (!IN_VEHICLE(ch))
	perform_move(ch, subcmd, 0);
  else
	send_to_char(ch, "You are inside a vehicle, to move you need to 'drive' it.\r\n");
}
ACMD(do_drag)
{
	struct char_data *vict;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int dir;

	if (GET_SKILL_LEVEL(ch, SKILL_DRAG) == 0) {
		send_to_char(ch, "You have no idea how to properly drag someone.\r\n");
		return;
	}

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char(ch, "Who would you like to drag?\r\n");
		return;
	}
	if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
		send_to_char(ch, "You don't see anyone by that name here.\r\n");
		return;
	}
	if (!*arg2) {
		send_to_char(ch, "In which direction would you like to drag them?\r\n");
		return;
	}
	else  {
		if (!AFF_FLAGGED(vict, AFF_STUN)) {
			send_to_char(ch, "Your target needs to be stunned in order to drag them anywhere.\r\n");
			return;
		}
		if (GET_MOVE(ch) < 70) {
			send_to_char(ch, "You are too exhausted to drag them anywhere!\r\n");
			return;
		}

		dir = search_block(arg2, dirs, FALSE);

		if (dir >= 0) {
			if (!CAN_GO(ch, dir)) {
				send_to_char(ch, "You cannot drag them in that direction.\r\n");
				return;
			}
			if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_NOMOB)) {
				send_to_char(ch, "You cannot drag them into that room.\r\n");
				return;
			}

			room_rnum going_to = EXIT(ch, dir)->to_room;

			send_to_char(ch, "\r\nYou grab %s around the shoulder, lean them back and begin to drag them out of the room.\r\n\r\n", GET_NAME(vict));
			act("$n is dragged out of the room by $N.", TRUE, vict, 0, ch, TO_NOTVICT);
			char_from_room(vict);
			char_from_room(ch);
			char_to_room(ch, going_to);
			char_to_room(vict, IN_ROOM(ch));
			look_at_room(ch, IN_ROOM(ch));
			act("$n is dragged into the room by $N.", TRUE, vict, 0, ch, TO_NOTVICT);
			GET_MOVE(ch) -= 50;
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE * 2;

		}
		else {
			send_to_char(ch, "You have specified an invalid direction.\r\n");
			return;
		}
	}
}
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
	if ((door = search_block(dir, dirs, FALSE)) == -1) { /* Partial Match */
	  if ((door = search_block(dir, autoexits, FALSE)) == -1) { /* Check 'short' dirs too */
		send_to_char(ch, "That's not a direction.\r\n");
		return (-1);
	  }
    }
    if (EXIT(ch, door)) {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword) {
	if (is_name(type, EXIT(ch, door)->keyword))
	  return (door);
	else {
	  send_to_char(ch, "I see no %s there.\r\n", type);
	  return (-1);
        }
      } else
	return (door);
    } else {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < DIR_COUNT; door++)
    {
      if (EXIT(ch, door))
      {
        if (EXIT(ch, door)->keyword)
        {
          if (isname(type, EXIT(ch, door)->keyword))
          {
            if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
              return door;
            else if (is_abbrev(cmdname, "open"))
            {
              if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
                return door;
              else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
                return door;
            }
            else if ((is_abbrev(cmdname, "close")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))) )
              return door;
            else if ((is_abbrev(cmdname, "lock")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))) )
              return door;
            else if ((is_abbrev(cmdname, "unlock")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) )
              return door;
            else if ((is_abbrev(cmdname, "pick")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) )
              return door;
          }
        }
      }
    }

    if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "open"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be opened.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "close"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be closed.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "lock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be locked.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "unlock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be unlocked.\r\n", AN(type), type);
    else
      send_to_char(ch, "There doesn't seem to be %s %s that can be picked.\r\n", AN(type), type);

    return (-1);
  }
}

int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
      return (1);

  return (0);
}

#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

/* cmd_door is required external from act.movement.c */
const char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

static const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};

#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH];
  size_t len;
  room_rnum other_room = NOWHERE;
  struct room_direction_data *back = NULL;

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != IN_ROOM(ch))
	back = NULL;

  switch (scmd) {
  case SCMD_OPEN:
    OPEN_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_CLOSE:
    CLOSE_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      CLOSE_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_LOCK:
    LOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_UNLOCK:
    UNLOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      UNLOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_PICK:
    TOGGLE_LOCK(IN_ROOM(ch), obj, door);
    if (back)
      TOGGLE_LOCK(other_room, obj, rev_dir[door]);
    send_to_char(ch, "The lock quickly yields to your skills.\r\n");
    len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    break;
  }

  /* Notify the room. */
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s.",
	obj ? "" : "the ", obj ? "$p" : EXIT(ch, door)->keyword ? "$F" : "door");
  if (!obj || IN_ROOM(obj) != NOWHERE)
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
      send_to_room(EXIT(ch, door)->to_room, "The %s is %s%s from the other side.\r\n",
		back->keyword ? fname(back->keyword) : "door", cmd_door[scmd],
		scmd == SCMD_CLOSE ? "d" : "ed");
}

static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
  int percent, skill_lvl;

  if (scmd != SCMD_PICK)
    return (1);

  percent = rand_number(1, 4);
  skill_lvl = GET_SKILL_LEVEL(ch, SKILL_PICK_LOCK);

  if (keynum == NOTHING)
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  else if (pickproof)
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
  else if (percent > skill_lvl)
    send_to_char(ch, "You failed to pick the lock.\r\n");
  else
    return (1);

  return (0);
}

#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? ((GET_OBJ_TYPE(obj) == \
    ITEM_CONTAINER) && OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
    (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj, \
    CONT_CLOSED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? (!OBJVAL_FLAGGED(obj, \
    CONT_LOCKED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj, \
    CONT_PICKPROOF)) : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 2)) : \
    (EXIT(ch, door)->key))

ACMD(do_gen_door)
{
  int door = -1;
  int j = 0;
  int k = 0;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;
  struct char_data *tch = NULL;
  struct char_data *voice = NULL;
  char buf1[MAX_INPUT_LENGTH];

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
	if (!strcmp(argument, "rift")) {
		for (obj = world[IN_ROOM(ch)].contents; obj;obj = obj->next_content)
			if (GET_OBJ_VNUM(obj) == 10899) {
				for (tch = world[IN_ROOM(ch)].people; tch;tch = tch->next_in_room)
					if ((GET_MOB_VNUM(tch) > 10800) && (GET_MOB_VNUM(tch) < 10900)) 
						k += 1;
				j += 1;
				break;
			}
		if (k >= 1) {
			send_to_char(ch, "The rift is too heavily guarded!!\r\n");
			return;
		}
		if (j == 1 && k == 0) {
			voice = read_mobile(real_mobile(8000), REAL);
			char_to_room(voice, real_room(1));
			send_to_char(ch, "@BA strange rift begins to close as pulses of energy fire outward from its origin.@n\r\n");
			send_to_char(ch, "@BA loud popping sound can be heard as the rift winks out of existence.@n\r\n");
			sprintf(buf1, "Rise in honor my friends, %s has proclaimed victory on one of the rifts!", GET_NAME(ch));
			do_event(voice, buf1, 0, 0);
			obj_from_room(obj);
			extract_obj(obj);
			extract_char(voice);
			ch->char_specials.totalrifts += 1;
			event_info.playerrifts += 1;
			GET_RIFTS(ch) += 1;
			load_prize(ch);
			return;
		}
	}
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      send_to_char(ch, "You can't %s that!\r\n", cmd_door[subcmd]);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) && ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (has_key(ch, keynum)) )
    {
      send_to_char(ch, "It is locked, but you have the key.\r\n");
      send_to_char(ch, "*Click*\r\n");
      do_doorcmd(ch, obj, door, subcmd);
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) && ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (!has_key(ch, keynum)) )
    {
      send_to_char(ch, "It is locked, and you do not have the key!\r\n");
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}

ACMD(do_enter)
{
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *vehicle;
  int door;
  
  one_argument(argument, buf);
  vehicle = get_obj_in_list_vis(ch, buf, NULL, world[ch->in_room].contents);
  if (vehicle) {
    if (GET_OBJ_TYPE(vehicle) == ITEM_VEHICLE) {
		//VEHICLETODO: check to make sure the vehicle belongs to owner.
		enter_vehicle(ch, vehicle);
		return;
	}
  }
  if (*buf) {			/* an argument was supplied, search for door
				 * keyword */
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "There is no %s here.\r\n", buf);
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
    send_to_char(ch, "You are already indoors.\r\n");
  else {
    /* try to locate an entrance */
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	      ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}

ACMD(do_leave)
{
  struct obj_data *vehicle;
  int door;

  if (IN_VEHICLE(ch)) {
	vehicle = VEHICLE(ch);
	REMOVE_BIT_AR(ROOM_FLAGS(real_room(VEHICLE_VNUM(ch))), ROOM_OCCUPIED);
	send_to_room(WINDOW_VNUM(ch), "%s jumps out of %s.\r\n", GET_NAME(ch), VEHICLE_NAME(ch));
	send_to_char(ch, "You jump out of a %s.\r\n", VEHICLE_NAME(ch));
	ch->char_specials.invehicle = FALSE;
	ch->char_specials.vehicle.vehicle_room_vnum = 0;
	char_from_room(ch);
	char_to_room(ch, WINDOW_VNUM(ch));
	look_at_room(ch, IN_ROOM(ch));
	return;
  }
  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}

ACMD(do_stand)
{
    // check to see if all body parts are there
    if (PART_MISSING(ch, BODY_PART_RIGHT_LEG) && PART_MISSING(ch, BODY_PART_LEFT_LEG)) {
        send_to_char(ch, "You can't stand without both legs.\r\n");
        return;
    }

    switch (GET_POS(ch)) {
        case POS_STANDING:
            send_to_char(ch, "You are already standing.\r\n");
            break;
        case POS_SITTING:
            send_to_char(ch, "You stand up.\r\n");
            act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            /* Were they sitting in something? */
            char_from_furniture(ch);
            /* Will be sitting after a successful bash and may still be fighting. */
            GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
            break;
        case POS_RESTING:
            send_to_char(ch, "You stop resting, and stand up.\r\n");
            act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            /* Were they sitting in something. */
            char_from_furniture(ch);
            break;
        case POS_MEDITATING:
            act("You stop meditating, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops meditating, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
        case POS_FOCUSING:
            act("You stop focusing, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops focusing, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
        case POS_SLEEPING:
            send_to_char(ch, "You awaken from your sleep and hazily stand up.\r\n");
            act("$n awakens and hazily stands up.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Do you not consider fighting as standing?\r\n");
            break;
        default:
            send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
            act("$n stops floating around, and puts $s feet on the ground.",
            TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_STANDING;
            break;
    }
}

ACMD(do_sit)
{
  char arg[MAX_STRING_LENGTH];
  struct obj_data *furniture;
  struct char_data *tempch;
  int found;

  one_argument(argument, arg);

  if (!*arg)
    found = 0;
  if (!(furniture = get_obj_in_list_vis(ch, arg, NULL, world[ch->in_room].contents)))
    found = 0;
  else
    found = 1;

  switch (GET_POS(ch)) {
  case POS_STANDING:
    if (found == 0) {
      send_to_char(ch, "You sit down.\r\n");
      act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
    } else {
      if (GET_OBJ_TYPE(furniture) != ITEM_FURNITURE) {
        send_to_char(ch, "You can't sit on that!\r\n");
        return;
      } else if (GET_OBJ_VAL(furniture, 1) > GET_OBJ_VAL(furniture, 0)) {
        /* Val 1 is current number sitting, 0 is max in sitting. */
        act("$p looks like it's all full.", TRUE, ch, furniture, 0, TO_CHAR);
        log("SYSERR: Furniture %d holding too many people.", GET_OBJ_VNUM(furniture));
        return;
      } else if (GET_OBJ_VAL(furniture, 1) == GET_OBJ_VAL(furniture, 0)) {
        act("There is no where left to sit upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        return;
      } else {
        if (OBJ_SAT_IN_BY(furniture) == NULL)
          OBJ_SAT_IN_BY(furniture) = ch;
        for (tempch = OBJ_SAT_IN_BY(furniture); tempch != ch ; tempch = NEXT_SITTING(tempch)) {
          if (NEXT_SITTING(tempch))
            continue;
          NEXT_SITTING(tempch) = ch;
        }
        act("You sit down upon $p.", TRUE, ch, furniture, 0, TO_CHAR);
        act("$n sits down upon $p.", TRUE, ch, furniture, 0, TO_ROOM);
        SITTING(ch) = furniture;
        NEXT_SITTING(ch) = NULL;
        GET_OBJ_VAL(furniture, 1) += 1;
        GET_POS(ch) = POS_SITTING;
      }
    }
    break;
        case POS_SITTING:
            send_to_char(ch, "You're sitting already.\r\n");
            break;
        case POS_RESTING:
            send_to_char(ch, "You stop resting, and sit up.\r\n");
            act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_MEDITATING:
            act("You stop meditating, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops mediating.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_FOCUSING:
            act("You stop focusing, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops focusing.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_SLEEPING:
            send_to_char(ch, "You have to wake up first.\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
            break;
        default:
            send_to_char(ch, "You stop floating around, and sit down.\r\n");
            act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
    }
}

ACMD(do_rest)
{
    switch (GET_POS(ch)) {
        case POS_STANDING:
            send_to_char(ch, "You sit down and rest your tired bones.\r\n");
            act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
        case POS_SITTING:
            send_to_char(ch, "You rest your tired bones.\r\n");
            act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
        case POS_RESTING:
            send_to_char(ch, "You are already resting.\r\n");
            break;
        case POS_MEDITATING:
            act("You stop meditating, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops meditating.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_FOCUSING:
            act("You stop focusing, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n stops focusing.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_SITTING;
            break;
        case POS_SLEEPING:
            send_to_char(ch, "You have to wake up first.\r\n");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
            break;
        default:
            send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
            act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_RESTING;
            break;
    }
}

ACMD(do_sleep)
{
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
  case POS_MEDITATING:
  case POS_FOCUSING:
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_STANDING;
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else {
    send_to_char(ch, "You awaken, and stand up.\r\n");
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
  }
}

ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  } else {
    send_to_char(ch, "Whom do you wish to follow?\r\n");
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char(ch, "You are already following yourself.\r\n");
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}

ACMD(do_focus)
{
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        act("You seem to be unable to move!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n can't move!", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_SKILL_LEVEL(ch, SKILL_FOCUS) == 0) {
        send_to_char(ch, "But you don't know how to do that.\n\r");
        return;
    }

    switch(GET_POS(ch)) {
        case POS_STANDING:
        case POS_SITTING:
        case POS_RESTING:
        case POS_MEDITATING:
            send_to_char(ch, "You block out your surroundings.\n\r");
            act("$n blocks out $s surroundings.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_FOCUSING;
            break;
        case POS_FOCUSING:
            send_to_char(ch, "You are already focusing.\n\r");
            break;
        case POS_SLEEPING:
            send_to_char(ch, "You are sound asleep.\n\r");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Focus while fighting? IMPOSSIBLE!\n\r");
            break;
        default:
            send_to_char(ch, "You block out your surroundings.\n\r");
            act("$n blocks out $s surroundings.\n\r", FALSE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_FOCUSING;
            break;
    }
}


ACMD(do_meditate)
{
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        act("You seem to be unable to move!", FALSE, ch, 0, 0, TO_CHAR);
        act("$n can't move!", FALSE, ch, 0, 0, TO_ROOM);
        return;
    }

    if (GET_SKILL_LEVEL(ch, SKILL_MEDITATE) == 0) {
        send_to_char(ch, "But you don't know how to do that.\n\r");
        return;
    }

    switch(GET_POS(ch)) {
        case POS_STANDING:
        case POS_SITTING:
        case POS_RESTING:
            send_to_char(ch, "You become very quiet and begin to meditate.\n\r");
            act("$n begins to meditate.", TRUE, ch, 0, 0, TO_ROOM);
            GET_POS(ch) = POS_MEDITATING;
            break;
        case POS_MEDITATING:
            send_to_char(ch, "You are already meditating.\n\r");
            break;
        case POS_FOCUSING:
            send_to_char(ch, "You are already focusing.\n\r");
            break;
        case POS_SLEEPING:
            send_to_char(ch, "You can't meditate... you're sound asleep.\n\r");
            break;
        case POS_FIGHTING:
            send_to_char(ch, "Meditate while fighting? IMPOSSIBLE!\n\r");
            break;
        default:
            send_to_char(ch, "You clear your mind and begin to meditate.\n\r");
            GET_POS(ch) = POS_MEDITATING;
            break;
    }
}

ACMD(do_noleave)
{
	if (IS_NPC(ch)) {
		send_to_char(ch, "Mobs cannot noleave.\r\n");
		return;
	}
	
	if (!PRF_FLAGGED(ch, PRF_NOLEAVE)) {
		SET_BIT_AR(PRF_FLAGS(ch), PRF_NOLEAVE);
		send_to_char(ch, "@GYou will now NOT leave behind exhausted or fighting group members.@n\r\n");
		act("@G$n will not leave behind exhausted or fighting group members.@n", TRUE, ch, 0, 0, TO_ROOM);
	}
	else {
		REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOLEAVE);
		send_to_char(ch, "@RYou will now leave behind exhausted or fighting group members.@n\r\n");
		act("@R$n will now leave behind exhausted or fighting group members.@n", TRUE, ch, 0, 0, TO_ROOM);
	}
}
