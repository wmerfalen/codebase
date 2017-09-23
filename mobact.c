/**************************************************************************
*  File: mobact.c                                          Part of tbaMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles.   *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "psionics.h"
#include "constants.h"
#include "act.h"
#include "graph.h"
#include "fight.h"


/* local file scope only function prototypes */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack);
static void take_mob_home(struct char_data *ch);

void mobile_activity(void)
{
  struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found, max;
  memory_rec *names;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch))
      continue;

    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
		log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		GET_NAME(ch), GET_MOB_VNUM(ch));
		REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
      } else {
        char actbuf[MAX_INPUT_LENGTH] = "";
	if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
	  continue;		/* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (FIGHTING(ch) || !AWAKE(ch) || RANGED_FIGHTING(ch))
      continue;

	/* Order a pulled sentinal mob home */
	if (GET_MOB_HOME(ch) != 0)
		take_mob_home(ch);
    
	/* hunt a victim, if applicable */
    hunt_victim(ch);

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER))
      if (world[IN_ROOM(ch)].contents && !rand_number(0, 10)) {
	max = 1;
	best_obj = NULL;
	for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
	  if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
	    best_obj = obj;
	    max = GET_OBJ_COST(obj);
	  }
	if (best_obj != NULL) {
	  obj_from_room(best_obj);
	  obj_to_char(best_obj, ch);
	  act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
	}
      }

		/* Mob Movement */
		if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
		((door = rand_number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
		!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB) &&
		!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_DEATH) &&
		!AFF_FLAGGED(ch, AFF_STUN) &&
		(!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
		(world[EXIT(ch, door)->to_room].zone == world[IN_ROOM(ch)].zone))) {

			if (!MOB_FLAGGED(ch, MOB_STAY_WATER)) {
				if (ch->master == NULL)
					perform_move(ch, door, 1);
			}
			else {
				if (SECT(IN_ROOM(ch)) != SECT_WATER_SWIM || SECT(IN_ROOM(ch)) != SECT_WATER_NOSWIM || SECT(IN_ROOM(ch)) != SECT_UNDERWATER || !ROOM_FLAGGED(IN_ROOM(ch), ROOM_UNDERWATER)) {
					if (ch->master == NULL)
						perform_move(ch, door, 1);
				}
			}
        }

        /* Aggressive to Remort Mobs */
        if (MOB_FLAGGED(ch, MOB_AGGRO_REMORT) && (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) {
            found = FALSE;
            for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
                if (IS_NPC(vict) || !IS_REMORT(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
                    (AFF_FLAGGED(vict, AFF_SNEAK) && (rand_number(0, 21) < GET_DEX(vict))))
                    continue;

                if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
                    continue;
                /* aggro mobs don't attack their own team! */
                if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(vict))
                    continue;
                if (IS_DEAD(vict)) /* make a seperate call for mobs to behead highlanders */
                    continue;

                /* attempt at randomizing 50-50 chance of not hitting this victim */
                if (!rand_number(0, 1))
                    continue;

                /* Can a master successfully control the charmed monster? */
                if (aggressive_mob_on_a_leash(ch, ch->master, vict))
                    continue;

				int thaco = rand_number(1,30);
/* 10/26/16 to fix hiding mobs from staying hidden? */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_HIDE);

                hit(ch, vict, TYPE_UNDEFINED, thaco, 0);
                GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
                found = TRUE;

            }

            if (found)
                continue;  /* next mob, this one's done */

          }

        /* Aggressive to anyone Mobs */
        if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) {

            found = FALSE;

            for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
                if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) ||
                    (AFF_FLAGGED(vict, AFF_SNEAK) && (rand_number(0, 21) < GET_DEX(vict))))
                    continue;

                if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
                    continue;

                /* aggro mobs don't attack their own team! */
                if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(vict))
                    continue;

                if (IS_DEAD(vict)) /* make a seperate call for mobs to behead highlanders */
                    continue;

                /* attempt at randomizing 50-50 chance of not hitting this victim */
                if (!rand_number(0, 1))
                    continue;

                /* Can a master successfully control the charmed monster? */
                if (aggressive_mob_on_a_leash(ch, ch->master, vict))
                    continue;

/* 10/26/16 to fix hiding mobs from staying hidden? */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_HIDE);
				int thaco = rand_number(1,30);
                hit(ch, vict, TYPE_UNDEFINED, thaco, 0);
                GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
                found = TRUE;

            }

            if (found)
                continue;  /* next mob, this one's done */
        }


        /* Mob Memory */
        if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {

            found = FALSE;
            for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
                if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
                    continue;

                for (names = MEMORY(ch); names && !found; names = names->next) {
                    if (names->id != GET_IDNUM(vict))
                        continue;

                    if (IS_DEAD(vict)) /* make a seperate call for mobs to behead highlanders */
                        continue;

                    /* aggro mobs don't attack their own team! */
                    if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(vict))
                        continue;

                    /* Can a master successfully control the charmed monster? */
                    if (aggressive_mob_on_a_leash(ch, ch->master, vict))
                        continue;

                    found = TRUE;
/* 10/26/16 to fix hiding mobs from staying hidden? */
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
					int thaco = rand_number(1,30);
                    act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
                    hit(ch, vict, TYPE_UNDEFINED, thaco, 0);
                    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
                }
            }

          if (found)
            continue;  /* next mob, this one's done */
        }


        /* Charmed Mob Rebellion: In order to rebel, there need to be more charmed
        * monsters than the person can feasibly control at a time.  Then the
        * mobiles have a chance based on the charisma of their leader.
        * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc. */
        if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3) {
            if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master)) {
                if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE)) {
					int thaco = rand_number(1,30);
                    hit(ch, ch->master, TYPE_UNDEFINED, thaco, 0);
                    stop_follower(ch);
				}
            }
        }

        /* Helper Mobs */
        if (MOB_FLAGGED(ch, MOB_HELPER) && (!AFF_FLAGGED(ch, AFF_BLIND) || !AFF_FLAGGED(ch, AFF_CHARM))) {
            found = FALSE;

            for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
                if (ch == vict || !FIGHTING(vict))
                    continue;
                
				if (GET_TEAM(ch) == 0 || GET_TEAM(ch) != GET_TEAM(vict))
                  if (IS_NPC(FIGHTING(vict)) || !IS_NPC(vict))
                      continue;

                /* don't let the mob attack itself somehow */
                if (ch == FIGHTING(vict))
                    continue;
                /* helper mobs don't attack their own team! */
                if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(FIGHTING(vict)))
                    continue;

                /* helper mobs don't help other teams either! */
                if (GET_TEAM(ch) > 0 && GET_TEAM(ch) != GET_TEAM(vict))
                    continue;

                act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
/* 10/26/16 to fix hiding mobs from staying hidden? */
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);


				int thaco = rand_number(1,30);
                hit(ch, FIGHTING(vict), TYPE_UNDEFINED, thaco, 0);
                GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
                found = TRUE;
			}
			
			for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
				if (RANGED_FIGHTING(vict) && IS_NPC(vict)) {
					if (FIGHTING(ch) == NULL && RANGED_FIGHTING(ch) == NULL) {
						act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
				        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
						if (get_lineofsight(ch, RANGED_FIGHTING(vict), NULL, NULL))
							set_ranged_fighting(ch, RANGED_FIGHTING(vict));
					}
				}
			}
	    }
        /* Add new mobile actions here */

    }                /* end for () */
}

/* Mob Memory Routines */
/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while (curr && curr->id != GET_IDNUM(victim)) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

/* An aggressive mobile wants to attack something.  If they're under the 
 * influence of mind altering PC, then see if their master can talk them out 
 * of it, eye them down, or otherwise intimidate the slave. */
static bool aggressive_mob_on_a_leash(struct char_data *slave, struct char_data *master, struct char_data *attack)
{
  static int snarl_cmd;
  int dieroll;

  if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
    return (FALSE);

  if (!snarl_cmd)
    snarl_cmd = find_command("snarl");

  /* Sit. Down boy! HEEEEeeeel! */
  dieroll = rand_number(1, 20);
  if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave))) {
    if (snarl_cmd > 0 && attack && !rand_number(0, 3)) {
      char victbuf[MAX_NAME_LENGTH + 1];

      strncpy(victbuf, GET_NAME(attack), sizeof(victbuf));	/* strncpy: OK */
      victbuf[sizeof(victbuf) - 1] = '\0';

      do_action(slave, victbuf, snarl_cmd, 0);
    }

    /* Success! But for how long? Hehe. */
    return (TRUE);
  }

  /* So sorry, now you're a player killer... Tsk tsk. */
  return (FALSE);
}

static void take_mob_home(struct char_data *ch)
{
	int dir;
	// Mob AI to walk back to their loading spot after all of the nonsense has cooled down - Gahan
	// This is kind of a hack, todo: make them walk back slowly instead of just popping them in.
	if (!AFF_FLAGGED(ch, AFF_CHARM)
		&& HUNTING(ch) == NULL
		&& FUN_HUNTING(ch) == NULL
		&& MOB_FLAGGED(ch, MOB_SENTINEL)
		&& !AFF_FLAGGED(ch, AFF_GROUP)) {
		if (GET_MOB_HOME(ch) != NOWHERE
			&& GET_ROOM_VNUM(GET_MOB_HOME(ch)) != IN_ROOM(ch)
			&& MOB_FLAGGED(ch, MOB_SENTINEL)) {
			dir = find_first_step(IN_ROOM(ch), GET_MOB_HOME(ch), ch, FALSE);
			if (dir < 0 && GET_MOB_HOME(ch) != IN_ROOM(ch)) {
				send_to_room(IN_ROOM(ch), "%s walks away from you, presumably going home.\r\n", GET_NAME(ch));
				char_from_room(ch);
				char_to_room(ch, GET_MOB_HOME(ch));
			}
			else
				perform_move(ch, dir, 1);
		}
	}
}
