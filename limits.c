/**************************************************************************
*  File: limits.c                                          Part of tbaMUD *
*  Usage: Limits & gain funcs for HMV, exp, hunger/thirst, idle time.     *
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
#include "psionics.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "drugs.h"
#include "act.h"
#include "screen.h"
#include "constants.h"
#include "mud_event.h"
#include "skilltree.h"
#include "protocol.h"

// local file scope function prototypes
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void explode_special(struct obj_data *explosive, int room, long instigator_id);

#define READ_TITLE(ch) (GET_SEX(ch) == SEX_MALE ?   \
    titles[(int)GET_LEVEL(ch)].title_m :  \
    titles[(int)GET_LEVEL(ch)].title_f)

void portal_send(struct char_data *ch, char *msg);

// When age < 15 return the value p0
// When age is 15..29 calculate the line between p1 & p2
// When age is 30..44 calculate the line between p2 & p3
// When age is 45..59 calculate the line between p3 & p4
// When age is 60..79 calculate the line between p4 & p5
// When age >= 80 return the value p6 */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{
    if (grafage < 15)
        return (p0);                                        // < 15
    else if (grafage <= 29)
        return (p1 + (((grafage - 15) * (p2 - p1)) / 15));    // 15..29
    else if (grafage <= 44)
        return (p2 + (((grafage - 30) * (p3 - p2)) / 15));    // 30..44
    else if (grafage <= 59)
        return (p3 + (((grafage - 45) * (p4 - p3)) / 15));    // 45..59
    else if (grafage <= 79)
        return (p4 + (((grafage - 60) * (p5 - p4)) / 20));    // 60..79
    else
        return (p6);                    // >= 80
}

/* Note: amt may be negative */
int increase_units(struct char_data *ch, int amt)
{
  int curr_units;

  curr_units = GET_UNITS(ch);

  if (amt < 0) {
    GET_UNITS(ch) = MAX(0, curr_units+amt);
    /* Validate to prevent overflow */
    if (GET_UNITS(ch) > curr_units) GET_UNITS(ch) = 0;
  } else {
    GET_UNITS(ch) = MIN(MAX_UNITS, curr_units+amt);
    /* Validate to prevent overflow */
    if (GET_UNITS(ch) < curr_units) GET_UNITS(ch) = MAX_UNITS;
  }
  if (GET_UNITS(ch) == MAX_UNITS)
    send_to_char(ch, "%sYou have reached the maximum units!\r\n%sYou must spend it or bank it before you can gain any more.\r\n", QBRED, QNRM);

  return (GET_UNITS(ch));
}

int decrease_units(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_units(ch, amt);
  return (GET_UNITS(ch));
}


// The hit_limit, psi_limit, and move_limit functions are gone.  They added an
// unnecessary level of complexity to the internal structure, weren't
// particularly useful, and led to some annoying bugs.  From the players' point
// of view, the only difference the removal of these functions will make is
// that a character's age will now only affect the HMV gain per tick, and _not_
// the HMV maximums. */
// psipoint gain pr. game hour */
int psi_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {  // neat and fast
        gain = GET_LEVEL(ch);
        if (GET_POS(ch) == POS_FIGHTING) gain = 0;
    }
    else {

        gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

        gain = (GET_INT(ch) * 3);
        gain = (gain + ch->player_specials->saved.char_psi_regen);
        gain = (gain + ch->player_specials->saved.char_all_regen);

        // Position calculations
        switch (GET_POS(ch)) {

            case POS_FOCUSING:
                gain *= 4;
                break;

            case POS_MEDITATING:
                gain *= 3;
                break;

            case POS_SLEEPING:
                gain *= 2;
                break;

            case POS_RESTING:
                gain *= 1.5;
                break;

            case POS_SITTING:
                gain += (gain >> 9);    // Divide by 4
                break;

            case POS_FIGHTING:

                if (GET_CLASS(ch) == CLASS_CRAZY || GET_PSIMASTERY(ch) > 0)
                    gain *= GET_PSIMASTERY(ch) > 100 ? (1 + GET_PSIMASTERY(ch)/100) : 1;
                else
                    gain *= 0;
                break;
        }

    }

    if (AFF_FLAGGED(ch, AFF_POISON) || IS_ON_DRUGS(ch))
        gain >>= 2;

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SUPER_REGEN)) gain *= 2;
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINUS_REGEN)) gain *= 5;

    if (IS_SET(ch->player_specials->saved.age_effects, AGE_BATCH_3))
        gain *= .75;

    gain = MAX(0, gain);

    return (gain);
}

// Hitpoint gain pr. game hour
int hit_gain(struct char_data *ch)
{
    int gain;
    float mobgain;

    if (IS_NPC(ch)) {

        // max is 0.035 (3.5%) plus 2%
        mobgain = (GET_LEVEL(ch) / 1000) + 0.01;

        // 2%-5.5% of maxhit
        gain = mobgain * GET_MAX_HIT(ch);

        if (GET_POS(ch) != POS_FIGHTING && GET_POS(ch) > POS_STUNNED) gain *= 2;
        if (GET_POS(ch) == POS_FIGHTING) gain = 0;
    }
    else {

        gain = (GET_CON(ch) * 4);
        gain = gain + (ch->player_specials->saved.char_hit_regen);
        gain = gain + (ch->player_specials->saved.char_all_regen);

        if (HAS_BIONIC(ch, BIO_INTERFACE)) gain += 5;

        // Position calculations
        switch (GET_POS(ch)) {

            case POS_FOCUSING:
                gain *= 4;
                break;

            case POS_MEDITATING:
                gain *= 3;
                break;

            case POS_SLEEPING:
                gain *= 2;      // multiply by 2
                break;

            case POS_RESTING:
                gain *= 1.5;    // multiply by 1.5
                break;

            case POS_SITTING:
                gain *= 1.25;
                break;

            case POS_FIGHTING:
                if (HAS_BIONIC(ch, BIO_INTERFACE) || (GET_CLASS(ch) == CLASS_PREDATOR) || (IS_HIGHLANDER(ch)))
                    gain *= 1;
                else
                    gain *= 0;
                break;
        }

    }

    if (AFF_FLAGGED(ch, AFF_POISON) || IS_ON_DRUGS(ch))
        gain *= 0.25;
	if (AFF_FLAGGED(ch, AFF_BIO_REGEN))
		gain *= 2;

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SUPER_REGEN)) gain *= 2;
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINUS_REGEN)) gain *= 0.5;

    if (IS_SET(ch->player_specials->saved.age_effects, AGE_BATCH_3))
        gain *= 0.75;

    gain = MAX(0, gain);

    return (gain);
}

// move gain pr. game hour
int move_gain(struct char_data *ch)
{
    int gain;

    if (IS_NPC(ch)) {

        // Neat and fast
        gain = GET_LEVEL(ch);

        if (GET_POS(ch) == POS_FIGHTING) gain = 0;
        return (gain);
    }
    else {
        gain = 100;
        gain += ch->player_specials->saved.char_move_regen;
        gain += ch->player_specials->saved.char_all_regen;

    // Position calculations
    switch (GET_POS(ch)) {

        case POS_FOCUSING:
            gain *= 4;
            break;

        case POS_MEDITATING:
            gain *= 3;
            break;

        case POS_SLEEPING:
            gain *= 2;
            break;

        case POS_RESTING:
            gain  *= 4.25;
            break;

        case POS_SITTING:
            gain  *= 1.5;
            break;

        case POS_FIGHTING:

            if (GET_CLASS(ch) == CLASS_STALKER || GET_CLASS(ch) == CLASS_PREDATOR)
                gain *= 1;
            else
                gain *= 0;

            break;
        }
    }


    if (AFF_FLAGGED(ch, AFF_POISON) || IS_ON_DRUGS(ch))
        gain *= 0.5;

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SUPER_REGEN)) gain *= 2;
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_MINUS_REGEN)) gain *= 0.5;

    if (IS_SET(ch->player_specials->saved.age_effects, AGE_BATCH_3))
        gain *= 0.75;

    gain = MAX(0, gain);

    return (gain);
}

void set_title(struct char_data *ch, char *title)
{
    if (PLR_FLAGGED(ch, PLR_NOTITLE))
        return;

    if (title == NULL)
        title = READ_TITLE(ch);

    if (strlen(title) > MAX_TITLE_LENGTH)
        title[MAX_TITLE_LENGTH] = '\0';

    if (GET_TITLE(ch) != NULL)
        free(GET_TITLE(ch));

    GET_TITLE(ch) = strdup(title);
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)

    if (CONFIG_USE_AUTOWIZ) {

        size_t res;
        char buf[256];
        int i;

#if defined(CIRCLE_UNIX)

        res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());

#elif defined(CIRCLE_WINDOWS)

        res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);

#endif // CIRCLE_WINDOWS

        // Abusing signed -> unsigned conversion to avoid '-1' check.
        if (res < sizeof(buf)) {
            mudlog(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
            i = system(buf);
            reboot_wizlists();
        }
        else
            log("Cannot run autowiz: command-line doesn't fit in buffer.");
    }

#endif // CIRCLE_UNIX || CIRCLE_WINDOWS

}

void gain_exp(struct char_data *ch, int gain, bool show)
{
	//int i = 1;
	//int j = 0;
	int newgain = 0;

    if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
        return;

    if (gain > 0 && !IS_NPC(ch)) {
		// Todo fix this for exp hoarders
		//for (i = 1; (GET_LEVEL(ch) + i) < LVL_IMMORT; i++)
		//	if (GET_EXP(ch) > (exptolevel(ch, GET_LEVEL(ch) + i)));
		//		j++;
		//send_to_char(ch, "This is the result of J: %d", j);
		//newgain = (gain / j);
		newgain = gain;

		if (PRF_FLAGGED(ch, PRF_AUTOGAIN) && GET_EXP(ch) > (exptolevel(ch, GET_LEVEL(ch) +1))) {
            send_to_char(ch, "Auto-gain initialized, to turn off type autogain.\r\n");
            do_gain(ch, 0, 0, 0);
		}
    }
    else if (gain < 0) {
        gain = MAX(-CONFIG_MAX_EXP_LOSS, gain);    // Cap max exp lost per death
        GET_EXP(ch) += gain;
        if (GET_EXP(ch) < 0)
        GET_EXP(ch) = 0;

        send_to_char(ch, "You lose %d experience.\n\r", (gain * -1));

    }

	if (show == TRUE && newgain > 1)
		send_to_char(ch, "You gain %d experience.\r\n", newgain);
	
	GET_EXP(ch) += newgain;
}

// todo: remind me again why this function exists
void gain_exp_regardless(struct char_data *ch, int gain)
{
  if ((IS_HAPPYHOUR) && (IS_HAPPYEXP))
    gain += (int)((float)gain * ((float)HAPPY_EXP / (float)(100)));
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
        GET_EXP(ch) = 0;

}

void gain_condition(struct char_data *ch, int condition, int value)
{
    bool intoxicated;

    if (IS_NPC(ch) || GET_COND(ch, condition) == -1)    // No change
        return;

    intoxicated = (GET_COND(ch, DRUNK) > 0);

    GET_COND(ch, condition) += value;

    GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

    if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
        return;

    switch (condition) {
        case DRUNK:
            if (intoxicated)
                send_to_char(ch, "You are now sober.\r\n");
            break;
        default:
            break;
    }
}

// to be called every 4 seconds
void encumb_update(void)
{
	struct char_data *i;
	struct char_data *next_char;

	for (i = character_list; i; i = next_char) {
		next_char = i->next;

		if (GET_POS(i) >= POS_STUNNED && !IS_NPC(i)) {
			
			float percent = (((float)IS_CARRYING_W(i) + (float)IS_WEARING_W(i)) / (float)CAN_CARRY_W(i));
			int encumbrance = (int)(percent * 100);

				if (encumbrance >= 100) {
					encumbrance = 100;
				}

				if (encumbrance <= 0) {
					encumbrance = 0;
				}

				ENCUMBRANCE(i) = encumbrance;				
		}
	}
}

// to be called every 4 seconds
// todo: consider making this just for PCs (go through descriptor list?) and have mobs regen at the tick -Lump
void hpv_update(void)
{
    struct char_data *i;
    struct char_data *next_char;
	int gain = 0;
	int gainpc = 0;
//    struct char_data *victim;
//    char pbuf[MAX_STRING_LENGTH];

    for (i = character_list; i; i = next_char) {
        next_char = i->next;
		
		if (GET_CORETICKS(i) > 0) {
			GET_CORETICKS(i) -= 20;
			if (GET_CORETICKS(i) <= 0) {
				GET_CORETICKS(i) = 0;
				send_to_char(i, "Your power core has cooled to a normal operating temperature.\r\n");
			}
		}
		
		if (AFF_FLAGGED(i, AFF_BLEEDING)) {
			act("@R$n is bleeding everywhere!@n", FALSE, i, 0, 0, TO_ROOM);
			act("@RYou are bleeding heavily!@n", FALSE, i, 0, 0, TO_CHAR);
			if (IS_NPC(i)) {
				damage(i,i,((GET_LEVEL(i) * GET_LEVEL(i) + rand_number(1,500))), TYPE_UNDEFINED, DMG_NORMAL);
				update_pos(i);
			}
			if (!IS_NPC(i)) {
				damage(i,i,((GET_LEVEL(i) * GET_LEVEL(i) + rand_number(1,500))), TYPE_UNDEFINED, DMG_NORMAL);
				update_pos(i);
			}
		}

        if (ROOM_FLAGGED(IN_ROOM(i), ROOM_UNDERWATER)) {
			if (!IS_NPC(i) && !AFF_FLAGGED(i, AFF_BIO_TRANCE)) {
				send_to_char(i, "You begin to inhale water as you start to drown!\r\n");
				act("$n begins inhaling water and starts to drown!", FALSE, i, 0, 0, TO_ROOM);
                damage(i, i, 100, TYPE_UNDEFINED, DMG_NORMAL);
			}
		}
        
		if (GET_POS(i) >= POS_STUNNED) {

            if (IS_NPC(i) && (GET_HIT(i) < GET_MAX_HIT(i))) {

				if (GET_LEVEL(i) < 20) {
					gain = hit_gain(i)/10;
					GET_HIT(i) = MIN(GET_HIT(i) + gain, GET_MAX_HIT(i));
				}

				if (GET_LEVEL(i) >= 20 && GET_LEVEL(i) < 30) {
					gain = hit_gain(i)/20;
					GET_HIT(i) = MIN(GET_HIT(i) + gain, GET_MAX_HIT(i));
				}

				if (GET_LEVEL(i) >= 30) {
					gain = hit_gain(i)/30;
					GET_HIT(i) = MIN(GET_HIT(i) + gain, GET_MAX_HIT(i));
				}					

                // this handles when mobs have their EXP reduced because of an explosive (or vehicle)
                if (IS_NPC(i) && GET_EXP(i) < GET_MAX_EXP(i)) {

                    float percent = (float)gain/(float)GET_MAX_HIT(i);
                    int exp_gained = (int)(percent * GET_MAX_EXP(i));
                    GET_EXP(i) = MIN(GET_MAX_EXP(i), GET_EXP(i) + exp_gained);

                }
            }

			if (!IS_NPC(i) && (GET_HIT(i) < GET_MAX_HIT(i))) {

				gainpc = hit_gain(i)/10;
				GET_HIT(i) = MIN(GET_HIT(i) + gainpc, GET_MAX_HIT(i));

			}

            if (GET_PSI(i) < GET_MAX_PSI(i))
                GET_PSI(i) = MIN(GET_PSI(i) + psi_gain(i)/10, GET_MAX_PSI(i));

            if (GET_MOVE(i) < GET_MAX_MOVE(i))
                GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i)/10, GET_MAX_MOVE(i));
			
            update_pos(i);
		
		}

        // check for drug overdoses
        if (!IS_NPC(i) && GET_OVERDOSE(i) > 0) {

            GET_OVERDOSE(i) = MAX(GET_OVERDOSE(i) - (2 - rand_number(0, 3)), 0); // subtract/add 0, 1, 2, hmmm

            if (GET_OVERDOSE(i) <= 0) {
                send_to_char(i, "You feel like your body is now clean.\r\n");
                GET_OVERDOSE(i) = 0;
            }
            else if (GET_OVERDOSE(i) >= 200 && GET_HIT(i) > 0) {
                send_to_char(i, "Your body begins to spasm as an overdose of drugs is introduced to your system.\r\n");
                act("$n drops to the ground and begins to go into seizures.", FALSE, i, 0, 0, TO_ROOM);
                GET_HIT(i) = -10;
                update_pos(i);
            }
        }

        // handle Highlander "death"
        if (IS_HIGHLANDER(i)) {

            if (GET_POS(i) == POS_DEAD) {

                // restore tiny amount of hp
                GET_HIT(i) += hit_gain(i)/10;

                if (GET_HIT(i) >= 0) {

                    // mobs are not restored and they are not moved to the morgue
                    if (IS_NPC(i)) {
                        GET_POS(i) = POS_STANDING;
                        continue;
                    }

                    // they've come back to life
                    send_to_char(i, "With a sudden jolt, you now find yourself alive and breathing.\r\n");
                    GET_POS(i) = POS_STANDING;

                    // restore hpv
                    if (GET_REMORTS(i) > 5) {
                        GET_HIT(i) = GET_MAX_HIT(i);
                        GET_PSI(i) = GET_MAX_PSI(i)/2;
                        GET_MOVE(i) = GET_MAX_MOVE(i)/2;
                    }
                    else {
                        GET_HIT(i) = GET_MAX_HIT(i)/3;
                        GET_PSI(i) = GET_MAX_PSI(i)/5;
                        GET_MOVE(i) = GET_MAX_MOVE(i)/10;
                    }

                    // and move them to morgue
                    char_from_room(i);
                    char_to_room(i, real_room(3099));
                }
            }
        }
    }
}


// Update PCs, NPCs, and objects */
// note from jack: ok since this point_update function depends on
// the smaller tick. the resanct function for mobs, and item timer
// update routine is a little bit messed up so corpse and such right now
// is dissapearing at a much faster rate than usual, I noticed this while
// working on the timer.. it seems the thing count down at a rate of 3-4 per tick??
// I think that's about right**/
void point_update(void)
{
    void update_char_objects(struct char_data * ch);    // handler.c
    void extract_obj(struct obj_data * obj);    // handler.c
    struct char_data *i;
    struct char_data *next_char;
    struct obj_data *j;
    struct obj_data *next_thing;
    struct obj_data *jj;
    struct obj_data *next_thing2;

	if (HAPPY_TIME > 1)
		HAPPY_TIME--;

	else if (HAPPY_TIME == 1) /* Last tick - set everything back to zero */
	{
		HAPPY_QP = 0;
		HAPPY_EXP = 0;
		HAPPY_UNITS = 0;
		HAPPY_TIME = 0;
		send_to_all("@CHappyhour has ended!@n\n\r");
	}

    // characters
    for (i = character_list; i; i = next_char) {
        next_char = i->next;

        // todo: is this code permanently removed?
        //    if (i->player_specials->saved.time_to_next_quest > 0) {
        //        i->player_specials->saved.time_to_next_quest--;
        //        if (i->player_specials->saved.time_to_next_quest == 0)
        //            send_to_char(i, "You may now quest.\r\n");
        //    }

        gain_condition(i, HUNGER, -1);
        gain_condition(i, DRUNK, -1);
        gain_condition(i, THIRST, -1);

        // cap off hpv at max now (therapy and leech people complained that happened too often at minitick)
        //  adrenaline by Locklear, allow hpv to stay above normal levels while in combat (and not too tired)
        if (GET_POS(i) != POS_FIGHTING || GET_MOVE(i) < GET_MAX_MOVE(i)/10) {
            GET_HIT(i) = MIN(GET_HIT(i), GET_MAX_HIT(i));
            GET_PSI(i) = MIN(GET_PSI(i), GET_MAX_PSI(i));
            GET_MOVE(i) = MIN(GET_MOVE(i), GET_MAX_MOVE(i));
        }


        if (GET_POS(i) >= POS_STUNNED) {

            // does the character die from poisoning?
            if (AFF_FLAGGED(i, AFF_POISON))
                if (damage(i, i, 10, PSIONIC_POISON, DMG_POISON) == -1)
                    continue;

        }
        else if (GET_POS(i) == POS_INCAP) {

            if (damage(i, i, 1, TYPE_SUFFERING, DMG_NORMAL) == -1)
                continue;

        }
        else if (GET_POS(i) == POS_MORTALLYW) {

            if (damage(i, i, 2, TYPE_SUFFERING, DMG_NORMAL) == -1)
                continue;

        }

        if (GET_ADDICTION(i) > 0) {

            // the only way to get off the drugs, is to get off the drugs!
            if (!IS_ON_DRUGS(i)) {

                GET_ADDICTION(i) = MAX(GET_ADDICTION(i) - rand_number(0, 10), 0); // subtract 0 to 10 each hour

                if (GET_ADDICTION(i) >= 200) {
                    damage(i, i, 25, TYPE_BITE, DMG_DRUG);
                    act("$n shivers and starts to mumble something incoherently.", TRUE, i, 0, 0, TO_ROOM);
                    send_to_char(i, "You shiver uncontrollably and mumble something about getting high again.\r\n");
                }
                else if (GET_ADDICTION(i) >= 100) {
                    damage(i, i, 15, TYPE_CLAW, DMG_DRUG);
                    act("$n itches $mself profusely, must be quite the itch!", TRUE, i, 0, 0, TO_ROOM);
                    send_to_char(i, "Your skin feels like flesh-eating ants having a picnic.\r\n");
                }
                else if (GET_ADDICTION(i) >= 75) {
                    damage(i, i, 5, TYPE_UNDEFINED, DMG_DRUG);
                    act("$n looks around suspiciously.", TRUE, i, 0, 0, TO_ROOM);
                    act("$n says, 'Hey, anybody wanna get high?'", TRUE, i, 0, 0, TO_ROOM);
                    send_to_char(i, "You would really love to get high again...soon...REAL soon!\r\n");
                }
                else if (GET_ADDICTION(i) <= 0) {
                    send_to_char(i, "No longer do you hunger to get high, back to soberness!.\r\n");
                    GET_ADDICTION(i) = 0;
                }
            }
        }

        if (!i) // crap, they might've died from that
            continue;

        if (!IS_NPC(i)) {
            (i->char_specials.timer)++;
            update_char_objects(i);
        }
		/* Take 1 from the happy-hour tick counter, and end happy-hour if zero */

        // check mobs and their sancts
        if (IS_NPC(i)) {

            if ((!affected_by_psionic(i, PSIONIC_SANCTUARY)) && (!affected_by_psionic(i, PSIONIC_SUPER_SANCT)) && (!affected_by_psionic(i, PSIONIC_MEGA_SANCT))) {

                if (MOB_FLAGGED(i, MOB_MEGA_SANCT)) {
                    call_psionic(i, i, NULL, PSIONIC_MEGA_SANCT, 6, CAST_PSIONIC, TRUE);
                }
                else if (MOB_FLAGGED(i, MOB_SUPER_SANCT)) {
                  call_psionic(i, i, NULL, PSIONIC_SUPER_SANCT, 6, CAST_PSIONIC, TRUE);
                }
                else if (MOB_FLAGGED(i, MOB_SANCT)) {
                   call_psionic(i, i, NULL, PSIONIC_SANCTUARY, 6, CAST_PSIONIC, TRUE);
               }
            }
        }
    }

    // objects
    for (j = object_list; j; j = next_thing) {

        next_thing = j->next;    // Next in object list

        if (j->obj_flags.timer_on == TRUE && GET_OBJ_TYPE(j) != ITEM_EXPLOSIVE && GET_OBJ_TYPE(j) != ITEM_GUN && GET_OBJ_TYPE(j) != ITEM_AMMO) {

            // timer count down
            if (GET_OBJ_TIMER(j) > 0) {

                if (j->worn_by) {
                    if (!IS_MOB(j->worn_by))
                        GET_OBJ_TIMER(j)--;
                }
                else if (j->carried_by) {
                    if (!IS_MOB(j->carried_by))
                        GET_OBJ_TIMER(j)--;
                }
                else if (!j->in_obj) {
                    GET_OBJ_TIMER(j)--;
                }
                else if (IN_ROOM(j) != NOWHERE) {
                    GET_OBJ_TIMER(j)--;
                }
            }

            // if the timer has run out of...time...
            if (GET_OBJ_TIMER(j) == 0) {
                if (GET_OBJ_TYPE(j) == ITEM_KEY) {
                    if (j->carried_by)
                        act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
                    else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
                        act("A $p starts to shimmer and mysteriously disappears.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
                        act("A $p starts to shimmer and mysteriously disappears.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
                    }
                }
                else {
                    if (j->carried_by)
                        act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
                    else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
                        act("A quivering horde of maggots consumes $p.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
                        act("A quivering horde of maggots consumes $p.", TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
                    }
                }

                for (jj = j->contains; jj; jj = next_thing2) {

                    next_thing2 = jj->next_content;    // Next in inventory
                    obj_from_obj(jj);

                    if (j->in_obj)
                        obj_to_obj(jj, j->in_obj);
                    else if (j->carried_by)
                        obj_to_room(jj, IN_ROOM(j->carried_by));
                    else if (IN_ROOM(j) != NOWHERE)
                        obj_to_room(jj, IN_ROOM(j));
                    else {
                        mudlog(BRF, LVL_GOD, TRUE, "SYSERR: point_update reached else");
                    }
                }

                extract_obj(j);

            } // end timer out of time
        } // end check for obj_timer (no guns, no ammo, no bombs)
    } // end the for loops for items
}

// go through the rooms and update the rooms and their contents
// This is where room affects like HEAT and COLD will be handled
// as well as ROOM_PSIONICS and ROOM_AFFECTS like fire
void room_update(int burn)
{
    sh_int room = 0;
    int rand = 0;

    //  float intense_heat_dam = 1.2;
    //  float intense_cold_dam = 1.1;
    //  float psi_draining = 1.0;
    //  float move_draining = 0.9;
    //  float vacuum_dam = 1.2;
    //  float rad_damage = 1.2;
    //  float drown_damage = 1.11;

    struct char_data *ch;
    struct affected_type af;

    // Handle special rooms
    for (room = 0; room <= top_of_world; room++) {
        if (!world[room].people)
            continue;
        for (ch = world[room].people; ch; ch = ch->next_in_room) {
		if (IS_NPC(ch))
			continue;
        if (PRF_FLAGGED(ch, PRF_NOHASSLE))
			continue;
        if (ROOM_FLAGGED(room, ROOM_INTENSE_HEAT)) {
           if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_COOL_AURA)) {
              send_to_char(ch, "%sThe intensive amount of heat scorches your skin!!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
              act("$n's skin erupts in blisters due to the intense heat!", FALSE, ch, 0, 0, TO_ROOM);
              damage(ch, ch, 55, TYPE_UNDEFINED, DMG_FIRE);
           }
        }
        if (ROOM_FLAGGED(room, ROOM_INTENSE_COLD)) {
          if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_THERMAL_AURA)) {
             send_to_char(ch, "%sThe intensive amount of cold freezes your skin!!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
             act("$n looks cold, and shivers.", FALSE, ch, 0, 0, TO_ROOM);
             damage(ch, ch, 65, TYPE_UNDEFINED, DMG_COLD);
          }
        }
        if (!IS_NPC(ch) && ROOM_FLAGGED(room, ROOM_PSI_DRAINING)) {
             send_to_char(ch, "%sYou feel your psionic energy being drained from you!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			 act("$n stumbles in pain as $s psionic power is slowly drained!", FALSE, ch, 0, 0, TO_ROOM);
             GET_PSI(ch) -= 20;
			 if (GET_PSI(ch) < 0)
				 GET_PSI(ch) = 0;
		}
		if (!IS_NPC(ch) && ROOM_FLAGGED(room, ROOM_MOVE_DRAINING)) {
           send_to_char(ch, "%sYou start to feel sluggish and weak!!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
		   act("$n trembles as $s ability to move is slowly drained!", FALSE, ch, 0, 0, TO_ROOM);
           GET_MOVE(ch) -= 20 ;
		   if (GET_MOVE(ch) < 0)
			 GET_MOVE(ch) = 0;
        }
        if (ROOM_FLAGGED(room, ROOM_VACUUM)) {
          if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_BIO_TRANCE)) {
			 send_to_char(ch, "%s*GASP* You cant breath!!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			 act("$n begins to suffocate due to the vacuum!", FALSE, ch, 0, 0, TO_ROOM);
             damage(ch, ch, (GET_MAX_HIT_PTS(ch) / 10), TYPE_UNDEFINED, DMG_NORMAL);
          }
        }
		if (!IS_NPC(ch) && ROOM_FLAGGED(room, ROOM_RADIATION)) {
		  rand = rand_number(1, 4);
		    if (rand == 4) {
			   send_to_char(ch, "%sYou begin to feel VERY sick and queasy.\r\nA small lesion appears on your skin and a red burn is evident.%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			   damage(ch, ch, (GET_MAX_HIT_PTS(ch) / 10), TYPE_UNDEFINED, DMG_NORMAL);
               act("$n turns red and suffers due to the radiation!", FALSE, ch, 0, 0, TO_ROOM);
            }
        }
		if (!IS_NPC(ch) && room_affected_by_psionic(room, PSIONIC_SMOKE)) {
		  // random chance of coughing
          rand = rand_number(1, 4);
		  if (rand == 4) {
            send_to_char(ch, "The smoke is getting to you *cough*\r\n");
			rand = rand_number(0, 30);
            damage(ch, ch, rand, TYPE_UNDEFINED, DMG_FIRE);
            act("$n coughs loudly from the smoke.", FALSE, ch, 0, 0, TO_ROOM);
		  }
        }
		if (burn == DO_BURN) {
          if (room_affected_by_psionic(room, PSIONIC_FIRE)) {
            if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_COOL_AURA)) {
			  // random chance of burning
              rand = rand_number(1, 6);
              if (rand == 6) {
                send_to_char(ch, "%sThe fire burns you!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
                damage(ch, ch, (GET_MAX_HIT_PTS(ch) / 12), TYPE_UNDEFINED, DMG_FIRE);
			  }
            }
          }
		  if (room_affected_by_psionic(room, PSIONIC_NAPALM)) {
            if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_COOL_AURA)) {
			  // random chance of burning
              rand = rand_number(1, 6);
			  if (rand == 6) {
                send_to_char(ch, "%sThe fire sticks to your skin like hot glue!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
                rand = rand_number(25,75);
				damage(ch, ch, rand, TYPE_UNDEFINED, DMG_FIRE);
                new_affect(&af);
				af.type = PSIONIC_NAPALM;
                af.location = APPLY_ARMOR;
                af.modifier = -100;
                af.duration = 1;
				SET_BIT_AR(af.bitvector, AFF_PETRIFY);
                affect_to_char(ch, &af);
              }
            }
          }
        }
		if (ROOM_FLAGGED(room, ROOM_UNDERWATER)) {
          if (!IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_BIO_TRANCE)) {
			send_to_char(ch, "%sYou turn blue and start to drown!%s\r\n", CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
			act("$n turns blue and starts to drown!", FALSE, ch, 0, 0, TO_ROOM);
            damage(ch, ch, 100, TYPE_UNDEFINED, DMG_NORMAL);
          }
        }
      }
   }
}
void room_update_turret(int burn)
{
    sh_int room = 0;
    int rand = 0;

    //  float intense_heat_dam = 1.2;
    //  float intense_cold_dam = 1.1;
    //  float psi_draining = 1.0;
    //  float move_draining = 0.9;
    //  float vacuum_dam = 1.2;
    //  float rad_damage = 1.2;
    //  float drown_damage = 1.11;

    struct char_data *ch;
//    struct affected_type af;

    // Handle special rooms
    for (room = 0; room <= top_of_world; room++) {
        if (!world[room].people)
            continue;
        for (ch = world[room].people; ch; ch = ch->next_in_room) {

		if (IS_NPC(ch) && room_affected_by_psionic(room, PSIONIC_TURRET) && !(ch->master)) {
		  // random chance of getting shot
          rand = rand_number(1, 2);
		  if (rand == 2) {
            send_to_char(ch, "The turret locks on to you and fires!\r\n");
			rand = rand_number(100, 500);
            damage(ch, ch, rand, TYPE_UNDEFINED, DMG_FIRE);
            act("$n is knocked back as a turret fires at $m.", FALSE, ch, 0, 0, TO_ROOM);
       }
		  if (rand == 1) {
            send_to_char(ch, "The turret locks on to you and fires multiple shots!\r\n");
			rand = rand_number(100, 500);
            damage(ch, ch, rand, TYPE_UNDEFINED, DMG_FIRE);
            damage(ch, ch, rand, TYPE_UNDEFINED, DMG_FIRE);
            act("$n is knocked back as a turret fires multiple shots at $m.", FALSE, ch, 0, 0, TO_ROOM);
       }
          }
        }
   }
}


//  The explosive routine will handle any type of explosive that is
//  set to count down and blow off.  It will handle grenades, dynamite
//  molatov cocktails, and anything else which basically has a timer
//  which causes a reaction other than the regular point_update
//  This routine will handle counting down explosives because
//  they will obviously be faster than regular corpses or keys
void explosive_update(void)
{
    struct obj_data *explosive;
    struct obj_data *next_explosive;

    for (explosive = object_list; explosive; explosive = next_explosive) {

        next_explosive = explosive->next;

        // don't worry about non-explosives
        if (GET_OBJ_TYPE(explosive) == ITEM_EXPLOSIVE && IS_OBJ_TIMER_ON(explosive)) {

            if (GET_OBJ_TIMER(explosive) > 0)
                GET_OBJ_TIMER(explosive)--;

            if (GET_EXPLOSIVE_TYPE(explosive) == EXPLOSIVE_GRENADE)
                act("$p ticks away.", TRUE, 0, explosive, 0, TO_ROOM);

            if (GET_EXPLOSIVE_TYPE(explosive) == EXPLOSIVE_TNT)
                act("$p burns.", TRUE, 0, explosive, 0, TO_ROOM);

            if (GET_EXPLOSIVE_TYPE(explosive) == EXPLOSIVE_COCKTAIL)
                act("$p burns.", TRUE, 0, explosive, 0, TO_ROOM);

            if (GET_EXPLOSIVE_TYPE(explosive) == EXPLOSIVE_DESTRUCT)
                act("$p beeps.", TRUE, 0, explosive, 0, TO_ROOM);

            if (GET_OBJ_TIMER(explosive) == 0) {
                //detonate(explosive, get_obj_room(explosive));
                explode(explosive);
            }
        }
    }
}


void detonate(struct obj_data *device, int where)
{
    struct affected_type af;
    struct char_data *temp;
    struct char_data *tmp_victim;
    struct char_data *i;
    int dam;
    int new_dam;
    int door;
//    char pbuf[MAX_STRING_LENGTH];
    struct obj_data *next_o;
    struct obj_data *obj;
    int zone = 0;

    // kick out the funky explosives
    if (where == NOWHERE) {
        mudlog(NRM, LVL_IMMORT, TRUE, "SYSERR: Explosive (%s) is NOWHERE.", device->short_description);
        extract_obj(device);
        return;
    }

    switch (GET_EXPLOSIVE_TYPE(device)) {

        case EXPLOSIVE_GRENADE:
            // ch = device->ignited_by;
            tmp_victim = world[where].people;
            dam = dice(device->obj_flags.value[2], device->obj_flags.value[3]);
            send_to_room(where, "@rA grenade EXPLODES and shrapnel flies everywhere!@n\r\n");

            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {  // todo: what is CAN_GO2??
                    send_to_room(NEXT_ROOM(where, door), "You hear a loud explosion!\r\n");
                    if (world[NEXT_ROOM(where, door)].people) {
                        for (i = world[NEXT_ROOM(where, door)].people; i; i = i->next_in_room) {
                    }
                }
            }
            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                if (device->carried_by) {
                    if (tmp_victim == device->carried_by)
                        new_dam = dam;
                    else
                        new_dam = (int)((float)dam * rand_number(25, 75)/100);
                    obj_damage_char(device, tmp_victim, new_dam);
                }
                else if (device->in_obj) {
                    if (tmp_victim == device->in_obj->carried_by)
                        new_dam = (int)((float)dam * rand_number(60, 80)/100);
                    else
                        new_dam = (int)((float)dam * rand_number(30, 70)/100);
                    obj_damage_char(device, tmp_victim, new_dam);
                }
                else {
                    new_dam = (int)((float)dam * rand_number(80, 100)/100);
                    obj_damage_char(device, tmp_victim, new_dam);
                }
                send_to_char(tmp_victim, "You are hit by shrapnel!\r\n");
                tmp_victim = temp;
            }
            break; // end grenade explosive case
        case EXPLOSIVE_MINE:
            // ch = device->ignited_by;
            tmp_victim = world[where].people;
            dam = dice(device->obj_flags.value[2], device->obj_flags.value[3]);
            send_to_room(where, "@rA mine EXPLODES and shrapnel flies everywhere!@n\r\n");

            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {  // todo: what is CAN_GO2??
                    send_to_room(NEXT_ROOM(where, door), "You hear a loud explosion!\r\n");
                    if (world[NEXT_ROOM(where, door)].people) {
                        for (i = world[NEXT_ROOM(where, door)].people; i; i = i->next_in_room) {
                    }
                }
            }
            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                if (tmp_victim == world[where].people)
                    new_dam = dam;
                else
                    new_dam = (int)((float)dam * rand_number(80, 100)/100);
                obj_damage_char(device, tmp_victim, new_dam);
                send_to_char(tmp_victim, "You are hit by shrapnel!\r\n");
                tmp_victim = temp;
            }
            break;
        case EXPLOSIVE_DESTRUCT:
            // ch = device->ignited_by;
            tmp_victim = world[where].people;
            send_to_room(where, "@rA self-destruct device has detonated!@n\r\n");
            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {  // todo: what is CAN_GO2??
                    send_to_room(NEXT_ROOM(where, door), "You hear a loud explosion!\r\n");
                    if (world[NEXT_ROOM(where, door)].people) {
                        for (i = world[NEXT_ROOM(where, door)].people; i; i = i->next_in_room) {
                    }
                }
            }
            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                dam = dice(20, 150);
                obj_damage_char(device, tmp_victim, dam);
                send_to_char(tmp_victim, "You are hit by shrapnel!\r\n");
                tmp_victim = temp;
            }
           break;
        default:
            // it's not a defined explosive so don't do a thing
            break;
    }
    if (GET_EXPLOSIVE_TYPE(device) == EXPLOSIVE_GRENADE || GET_EXPLOSIVE_TYPE(device) == EXPLOSIVE_MINE) {
        // sub-explosive now, for room affects (flash, sonic, smoke..)
        tmp_victim = world[where].people;
        switch (GET_EXPLOSIVE_SUB_TYPE(device)) {
            case GRENADE_NORMAL:
                // do nothing
                break;
            case GRENADE_GAS:
                // stop all fighting
                send_to_room(where, "Tear gas begins to fill the room, making fighting impossible.\n\r");
				new_affect(&af);
                af.type      = PSIONIC_SMOKE;
                af.duration  = 1;
                affect_to_room(where, &af);
                while (tmp_victim != NULL) {
                    temp = tmp_victim->next_in_room;
                    if (GET_POS(tmp_victim) == POS_FIGHTING)
                        stop_fighting(tmp_victim);
                    tmp_victim = temp;
                }
                break;
            case GRENADE_FLASHBANG:
                // blind for 0-2 ticks
                while (tmp_victim != NULL) {
                    temp = tmp_victim->next_in_room;
                    if (!affected_by_psionic(tmp_victim, PSIONIC_BLINDNESS)) {
                        act("$n seems to be blinded!", TRUE, tmp_victim, 0, 0, TO_ROOM);
                        send_to_char(tmp_victim, "A flash grenade goes off in front of you!\r\n");
                        send_to_char(tmp_victim, "You have been blinded!\r\n");
						new_affect(&af);
                        af.type = PSIONIC_BLINDNESS;
                        af.location = APPLY_HITNDAM;
                        af.modifier = -10;
                        af.duration = rand_number(1, 3);
                        SET_BIT_AR(af.bitvector, AFF_BLIND);
                        affect_to_char(tmp_victim, &af);
                    }
                    tmp_victim = temp;
                }
                break;
        case GRENADE_SONIC:
            // shatters ears, slows them down for 2 ticks
            // makes room SOUNDPROOF for 1 tick
			new_affect(&af);
            af.type      = PSIONIC_LETHARGY;
            af.duration  = 1;
            SET_BIT_AR(af.bitvector, ROOM_SOUNDPROOF);
            affect_to_room(where, &af);

            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                if (!affected_by_psionic(tmp_victim, PSIONIC_LETHARGY)) {
                    act("$n seems to be moving a bit slower now.", TRUE, tmp_victim, 0, 0, TO_ROOM);
                    send_to_char(tmp_victim, "You've been hit by a high pitched sonic stun!\r\n");
                    send_to_char(tmp_victim, "You seem to be moving a bit slower now.\r\n");
					new_affect(&af);
                    af.type = PSIONIC_LETHARGY;
                    af.duration = 2;
                    affect_to_char(tmp_victim, &af);
                }
                tmp_victim = temp;
            }
            break;
        case GRENADE_SMOKE:
            // fills room with smoke, and random linked rooms
			new_affect(&af);
            af.type      = PSIONIC_SMOKE;
            af.duration  = rand_number(0, 1);
            SET_BIT_AR(af.bitvector, ROOM_DARK);
            affect_to_room(where, &af);
            send_to_room(where, "Dark, thick grey smoke begins to fill the room.\r\n");
			
            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {
					new_affect(&af);
                    af.type      = PSIONIC_SMOKE;
                    af.duration  = rand_number(0, 1);
                    SET_BIT_AR(af.bitvector, ROOM_DARK);
                    affect_to_room(NEXT_ROOM(where, door), &af);
                    send_to_room(NEXT_ROOM(where, door), "Dark, thick grey smoke begins to seep in from the other room.\r\n");
                }
            break;
        case GRENADE_FIRE:
            // fire bombs (molotov), sets room on fire and linked rooms
			new_affect(&af);
            af.type      = PSIONIC_FIRE;
            af.duration  = rand_number(0, 1);
            affect_to_room(where, &af);
            send_to_room(where, "@rThe room! The room! The room is on @RFIRE!@n\r\n");
            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {
                    if (!rand_number(0, 1)) {
						new_affect(&af);
                        af.type      = PSIONIC_FIRE;
                        af.duration  = rand_number(0, 1);
                        affect_to_room(NEXT_ROOM(where, door), &af);
                        send_to_room(NEXT_ROOM(where, door), "@rThe room! The room! The room is on @RFIRE!@n\r\n");
                    }
                }
            break;
        case GRENADE_CHEMICAL:
            // affects players with poison for 2 ticks, makes room radiation for 1 tick
			new_affect(&af);
            af.type      = PSIONIC_POISON;
            af.duration  = 1;
            SET_BIT_AR(af.bitvector, ROOM_RADIATION);
            affect_to_room(where, &af);

            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                if (!affected_by_psionic(tmp_victim, PSIONIC_POISON)) {
                    act("$n shivers and vomits on the ground.", TRUE, tmp_victim, 0, 0, TO_ROOM);
                    send_to_char(tmp_victim, "You've been hit by a chemical weapon!\r\n");
                    send_to_char(tmp_victim, "You don't feel so good...\r\n");
					new_affect(&af);
                    af.type = PSIONIC_POISON;
                    af.location = APPLY_HITNDAM;
                    af.modifier = -15;
                    af.duration = 1;
                    SET_BIT_AR(af.bitvector, AFF_POISON);
                    affect_to_char(tmp_victim, &af);
                }
                tmp_victim = temp;
            }
            break;
        case GRENADE_PSYCHIC:
            // drains everyone's psi power with leech
            // makes room PSI_DRAINING for 1 tick
			new_affect(&af);
            af.type      = PSIONIC_PSYCHIC_LEECH;
            af.duration  = 1;
            SET_BIT_AR(af.bitvector, ROOM_PSI_DRAINING);
            affect_to_room(where, &af);
            send_to_room(where, "The psionic aura of the room seems to have changed.\r\n");
            while (tmp_victim != NULL) {
                temp = tmp_victim->next_in_room;
                GET_PSI(tmp_victim) -= rand_number(0, GET_PSI(tmp_victim)/4);
                send_to_char(tmp_victim, "You've been hit by a psychic weapon!\r\n");
                send_to_char(tmp_victim, "You have been psychically leeched!\n\r");
                tmp_victim = temp;
            }
            break;
        case GRENADE_LAG:
            // makes room lag, similar to SECT_PAUSE, and random linked rooms
			new_affect(&af);
            af.type      = PSIONIC_LAG;
            af.duration  = rand_number(1, 2);
            SET_BIT_AR(af.bitvector, ROOM_NO_RECALL);
            affect_to_room(where, &af);
            send_to_room(where, "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");
            for (door = 0; door < NUM_OF_DIRS; door++)
                if (CAN_GO2(where, door)) {
					new_affect(&af);
                    af.type      = PSIONIC_LAG;
                    af.duration  = rand_number(1, 2);
                    SET_BIT_AR(af.bitvector, ROOM_NO_RECALL);
                    affect_to_room(NEXT_ROOM(where, door), &af);
                    send_to_room(NEXT_ROOM(where, door), "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");
                }
            break;
        case GRENADE_NUCLEAR:
            // makes adjoined rooms explode into radiation, stays within zone boundries
            // the psionic will radiate every few seconds until it runs out
			new_affect(&af);
            af.type      = PSIONIC_NUKEBLAST;
            af.duration  = rand_number(10, 24);
            SET_BIT_AR(af.bitvector, ROOM_RADIATION);
            affect_to_room(where, &af);
            zone = world[where].zone;
            for (obj = object_list; obj; obj = next_o) {
                next_o = obj->next;
                if (IN_ROOM(obj) != NOWHERE && world[IN_ROOM(obj)].zone == zone && obj != device)
                    extract_obj(obj);
            }
            for (i = character_list; i; i = temp) {
                temp = i->next;
                if (IN_ROOM(i) != NOWHERE && world[IN_ROOM(i)].zone == zone && GET_LEVEL(i) < LVL_IMMORT) {
                    if (!IS_NPC(i)) {
                        send_to_char(i, "With a flash of light, the world around you erupts into flames!\r\nThe Apocalypse has come at last! All life will cease to exist...");
						raw_kill(i, i, FALSE);
                    }
					else 
						extract_char(i);
                }
            }
            break;
        case GRENADE_NAPALM:
            // makes room on fire, sticks to people, spreads in all directions
			new_affect(&af);
            af.type      = PSIONIC_NAPALM;
            af.duration  = rand_number(2, 4);
            affect_to_room(where, &af);
            send_to_room(where, "Fire falls from the sky, it's Napalm!\r\nIt's sticking to your body, you can't get it off!\r\n");
            break;
       }
    }

    extract_obj(device);
}

bool explosive_damage_check(long instigator_id, int room)
{
    struct char_data *i = character_list;
    struct char_data *tmp;
    char *instigator_name;

    // NPCs can always do damage
    if (instigator_id == NOBODY)
        return (TRUE);

    // all damage OK if room is PK_OK room
    if (ROOM_FLAGGED(room, ROOM_PK_OK))
        return (TRUE);

    // check for online players
    while (i != NULL) {

        // we have found the instigator online
        if (!IS_NPC(i) && GET_IDNUM(i) == instigator_id) {

            if (GET_LEVEL(i) >= LVL_IMMORT)
                return (TRUE);
            else if (PLR_FLAGGED(i, PLR_KILLER))
                return (FALSE);
            else
                return (TRUE);
        }
        else
            i = i->next;
    }

    // the instigator was not online, look in the players file
    instigator_name = get_name_by_id(instigator_id);

    // try to load the player off disk
    CREATE(tmp, struct char_data, 1);
    clear_char(tmp);
    CREATE(tmp->player_specials, struct player_special_data, 1);

    if (load_char(instigator_name, tmp) > -1) {
//        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);

        // check for Imms and KILLERS.  SPECTATORS shouldn't matter here.
        if (GET_LEVEL(tmp) >= LVL_IMMORT) {
//        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);
            free_char(tmp);
            return (TRUE);
        }
        else if (PLR_FLAGGED(tmp, PLR_KILLER)) {
//        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);
            free_char(tmp);
            return (FALSE);
        }
    }
    else {
        // character not found in the player file allow normal damage
//        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);
        free_char(tmp);
    }
//        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);

    return (TRUE);
}

// set PLAYER_KILLER flag due to explosive usage.  even into player's file if they are not online.
void explosive_assign_killer(int instigator_id, sh_int room)
{
    struct char_data *i = character_list;
    struct char_data *tmp;
    char *instigator_name;
    struct descriptor_data *d;

    // if the instigator is still online find them in character list
    while (i != NULL) {

        if (!IS_NPC(i) && GET_IDNUM(i) == instigator_id) {
            // we have found the instigator on line!
            SET_BIT_AR(PLR_FLAGS(i), PLR_KILLER);
            mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for using explosive at %s.", GET_NAME(i), world[room].name);
            save_char(i);
            return;
        }
        else
            i = i->next;

    }

    // the instigator was not online, set the players file
    instigator_name = get_name_by_id(instigator_id);

    // try to load the player off disk
    CREATE(tmp, struct char_data, 1);
    clear_char(tmp);

    if (load_char(instigator_name, tmp) > -1) {

        // set them as KILLER
        SET_BIT_AR(PLR_FLAGS(i), PLR_KILLER);

        // log this to the mudlog
        mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s's file for using explosive at %s.", GET_NAME(tmp), world[room].name);

        save_char(tmp);
        free_char(tmp);

        // Now lets get those jerks who think they can hide from the KILLER flag by sitting at the menu prompt
        for (d = descriptor_list; d; d = d->next) {
            if ((STATE(d) == CON_MENU) && GET_IDNUM(d->character) == instigator_id) {
                SET_BIT_AR(PLR_FLAGS(d->character), PLR_KILLER);
                break;
            }
        }

    } else {
            save_char(tmp);
        free_char(tmp);
        // character not found in the player file, perhaps an error, perhaps they deleted?
        mudlog(NRM, LVL_IMMORT, TRUE, "ERROR (explosive_assign_killer) Instigator (%s) with id (%d) not found.\r\n", instigator_name, instigator_id);
    }
}

// This function will detonate explosives
void explode(struct obj_data *explosive)
{
    int room;
    struct char_data *victim;
    struct char_data *next_victim;
    int base_dam;
    int dam;
//    bool killer_flag = FALSE; TAG: NEWSERVER
    int i;

    int dir;
    int die_check;

	if (!explosive)
		return;

    struct explosive_data {
        int type;                       // the type of explosive
        char *to_room;                  // message generated to room upon explosion
        char *portal_to_room;           // portal string for to room upon explosion
        char *to_nearby;                // message to open nearby rooms
        char *to_nearby_closed;         // message to closed nearby rooms
        char *portal_to_nearby;         // portal string for nearby rooms
        int cb2h_min;                   // lower bound on random damage (carried by, to holder)
        int cb2h_max;                   // upper bound on random damage (carried by, to holder)
        int cb2b_min;                   // lower bound on random damage (carried by, to bystanders)
        int cb2b_max;                   // upper bound on random damage (carried by, to bystanders)
        int cio2h_min;                  // lower bound on random damage (contained in object, to object holder)
        int cio2h_max;                  // upper bound on random damage (contained in object, to object holder)
        int cio2b_min;                  // lower bound on random damage (contained in object, to bystander)
        int cio2b_max;                  // upper bound on random damage (contained in object, to bystander)
        int min;                        // lower bound on random damage
        int max;                        // upper bound on random damage
        char *to_victim;                // used in the act string to send message to victim
    } ed[] = {
        {
            EXPLOSIVE_GRENADE,
            "@r%c%s EXPLODES and shrapnel flies everywhere!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion nearby!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,               // If the person carries the grenade, do 100% damage to them
            25, 75,                 // If the grenade is carried by a person and you stand next to him you take 25-75% damage
            40, 80,
            30, 70,                 // If it is in another object, everyone takes about 30-70%
            80, 100,                // If it simply sits in the room, everyone takes 80-100% damage
            "You are hit by shrapnel!"
        },
        {
            EXPLOSIVE_MINE,
            "@r%c%s EXPLODES and shrapnel flies everywhere!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,                   // If the person stepped on the grenade, do 100% damage to them
            100, 100,
            40, 80,
            10, 50,
            60, 80,                     // Everyone else takes 60-80% damage
            "You are hit by shrapnel!"
        },
        {
            EXPLOSIVE_DESTRUCT,
            "@r%c%s has detonated!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,
            100, 100,
            100, 100,
            100, 100,
            100, 100,
            "You are hit by shrapnel!"
        },
        {
            EXPLOSIVE_TNT,
            "@r%c%s EXPLODES throwing debris everywhere!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,
            25, 50,
            40, 80,
            10, 50,
            25, 75,
            "You are hit by shrapnel!"
        },
        {
            EXPLOSIVE_COCKTAIL,
            "@r%c%s EXPLODES and debris flies everywhere!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,
            25, 50,
            40, 80,
            10, 50,
            25, 75,
            "You are hit by shrapnel!"
        },
        {
            EXPLOSIVE_PLASTIQUE,
            "@r%c%s EXPLODES and debris flies everywhere!@n\r\n",
            "%sCA_exploMED",
            "You hear a loud explosion!\r\n",
            "You hear a muffled explosion nearby.\r\n",
            "%sCA_exploSM",
            100, 100,
            25, 50,
            40, 80,
            10, 50,
            25, 75,
            "You are hit by shrapnel!"
        },
        {
            -1, NULL, NULL, NULL, NULL, NULL, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, NULL
        }
    };

    // find the explosive data entry
    for (i = 0; ed[i].type != -1 && ed[i].type != GET_EXPLOSIVE_TYPE(explosive); i++);
    if (ed[i].type == -1) {
        mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: (explode) Explosive %s has no explosive data entry.", explosive->short_description);
        extract_obj(explosive);
        return;
    }

    // find the room where the explosive is in
    room = get_obj_room(explosive);

    // check for an error in the room
    if (room == NOWHERE) {
        mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: (explode) Explosive %s is NOWHERE.", explosive->short_description);
        extract_obj(explosive);
        return;
    }

    // determine the base damage
    base_dam = dice(GET_OBJ_VAL(explosive, 2), GET_OBJ_VAL(explosive, 3));

    // PLAYER_KILLERS (except in PK OK rooms) and SPECTATORS are not allowed to do any damage
    //if (!explosive_damage_check(instigator_id, room))
    //    base_dam = 0;

    // Generate the message for the room
    send_to_room(room, ed[i].to_room, UPPER(*explosive->short_description), explosive->short_description + 1);

    // Generate the message for all adjoining rooms

    for (dir = 0; dir < NUM_OF_DIRS; dir++) {
        if (world[room].dir_option[dir] && world[room].dir_option[dir]->to_room != NOWHERE) {
            if (!EXIT_FLAGGED(W_EXIT(room, dir), EX_CLOSED))
                send_to_room(world[room].dir_option[dir]->to_room, "%s", ed[i].to_nearby);
            else
                send_to_room(world[room].dir_option[dir]->to_room, "%s", ed[i].to_nearby_closed);
        }
    }

    // damage people
    victim = world[room].people;

    while (victim != NULL) {

        // calculate damage modifiers
        if (GET_EXPLOSIVE_TYPE(explosive) == EXPLOSIVE_MINE) {
            if (victim == world[room].people)
                dam = (int)((float)base_dam * rand_number(ed[i].cb2h_min, ed[i].cb2h_max)/100);
            else
            dam = (int)((float)base_dam * rand_number(ed[i].min, ed[i].max)/100);
        }
        else  if (explosive->carried_by) {
            if (victim == explosive->carried_by)
                dam = (int)((float)base_dam * rand_number(ed[i].cb2h_min, ed[i].cb2h_max)/100);
            else
                dam = (int)((float)base_dam * rand_number(ed[i].cb2h_min, ed[i].cb2h_max)/100);
        }
        else if (explosive->in_obj) {
            if (victim == explosive->in_obj->carried_by)
                dam = (int)((float)base_dam * rand_number(ed[i].cio2h_min, ed[i].cio2h_max)/100);
            else
                dam = (int)((float)base_dam * rand_number(ed[i].cio2b_min, ed[i].cio2b_max)/100);
        }
        else
            dam = (int)((float)base_dam * rand_number(ed[i].min, ed[i].max)/100);

        // do this here in case we kill the victim?
        next_victim = victim->next_in_room;

        if (dam > 0) {
            act(ed[i].to_victim, TRUE, 0, 0, victim, TO_VICT);

            // todo: what do we do with die_check??
            die_check = obj_damage_char(explosive, victim, dam);
        }

        victim = next_victim;
    }

    // set the KILLER flag if needed
//    if (killer_flag)
//        explosive_assign_killer(instigator_id, room);

//    // damage objects
//    for (j = object_list; j; j = next_j) {
//        next_j = j->next;
//
//        // is the object in the room?
//        if (get_obj_room(j) == room && j != grenade)
//            damage_object(j, dam);
//
//    }

    // perform special effects
    explode_special(explosive, room, 0);

    // remove the explosive
    extract_obj(explosive);

}

void explode_special(struct obj_data *explosive, int room, long instigator_id)
{
    int type = GET_EXPLOSIVE_TYPE(explosive);
    int subtype = GET_EXPLOSIVE_SUB_TYPE(explosive);
    struct char_data *tmp_victim;
    struct affected_type af;
    struct char_data *temp;
    struct char_data *i;
    int door;
    struct obj_data *next_o;
    struct obj_data *obj;
    int zone = 0;
    int location = -1;
    int other_room = NOWHERE;
    struct room_direction_data *back = 0;
        int die = -1;

    switch (type) {
        case EXPLOSIVE_COCKTAIL:
				// there is a 25% chance that the room catches on fire
                die = rand_number(0, 3);
                if (die == 0) {
                // set the room on fire!
					new_affect(&af);
                    af.type  = PSIONIC_FIRE;
                    af.duration = 1;
                    affect_to_room(room, &af);
                    send_to_room(room, "@rThe room! The room! The room is on @RFIRE!@n\r\n");
                }

            break;

        case EXPLOSIVE_PLASTIQUE:

            // clear the plastique info
            if (explosive->attached && explosive->attached->plastique)
                explosive->attached->plastique = NULL;

            // determine the location
            location = GET_OBJ_VAL(explosive, 1);   // plastique does not have a sub-type yet, so this can be used

            // todo: allow plastique to attach to more than just the room?
            // for now, we only need to perform a special when we are blowing open a room exit
            if (location >= PLASTIQUE_ROOM_NORTH && location <= PLASTIQUE_ROOM_DOWN && room != NOWHERE) {

                // remove the plastique from the room
                                world[room].plastique[location] = NULL;

                // location_type indicates direction
                if (W_EXIT(room, location) && !(EXIT_FLAGGED(W_EXIT(room, location), EX_BLOWPROOF))) {

                    // clear the exit information
                    REMOVE_BIT(W_EXIT(room, location)->exit_info, EX_CLOSED);
                    REMOVE_BIT(W_EXIT(room, location)->exit_info, EX_ISDOOR);
                            W_EXIT(room, location)->key = 0;
                            W_EXIT(room, location)->general_description = NULL;

                    // does this exit actually lead anywhere?
                    if ((other_room = W_EXIT(room, location)->to_room) == NOWHERE)
                        return;

                    // Notify the current room
                    send_to_room(room, "The explosion opens a doorway to the %s.\r\n", dirs[location]);

                    // todo: this (arbitrarily?) enforces symmetric doorways (i.e., E--W, N--S, etc.)
                    // for now, if it isn't symmetric, don't do anything for the other room?
                    back = world[other_room].dir_option[rev_dir[location]];
                    if (back->to_room != NOWHERE) {

                        // clear the reverse information too
                        REMOVE_BIT(back->exit_info, EX_CLOSED);
                        REMOVE_BIT(back->exit_info, EX_ISDOOR);
                        back->key = 0;
                        back->general_description = NULL;

                        // Notify the other room
                        send_to_room(other_room, "A doorway opens from the %s with an explosion.\r\n", dirs[rev_dir[location]]);

                    }
                    else {
                        // not a symmetric exit
                        send_to_room(other_room, "A one-way doorway opens with an explosion.\r\n");
                    }

                }

            }

            break;

        case EXPLOSIVE_TNT:

            break;

        case EXPLOSIVE_DESTRUCT:

            break;

        case EXPLOSIVE_MINE:

            break;

        case EXPLOSIVE_GRENADE:

            tmp_victim = world[room].people;

            switch (subtype) {

                case GRENADE_NORMAL:
                    // do nothing
                    break;

                case GRENADE_GAS:

                    // stop all fighting
                    send_to_room(room, "@RTear gas begins to fill the room, making fighting impossible.@n\n\r");
					new_affect(&af);
					af.type      = PSIONIC_SMOKE;
                    af.duration  = 1;
                    affect_to_room(room, &af);
                    while (tmp_victim != NULL) {
                        temp = tmp_victim->next_in_room;
                        if (GET_POS(tmp_victim) == POS_FIGHTING)
                            stop_fighting(tmp_victim);
                        tmp_victim = temp;
                    }
                    break;
                case GRENADE_FLASHBANG:
					new_affect(&af);
                    af.type = PSIONIC_BLINDNESS;
                    af.location = APPLY_HITNDAM;
                    af.modifier = -10;
                    SET_BIT_AR(af.bitvector, AFF_BLIND);
                    // blind for 0-2 ticks
                    while (tmp_victim != NULL) {
                        temp = tmp_victim->next_in_room;
                        if (!affected_by_psionic(tmp_victim, PSIONIC_BLINDNESS)) {
                            act("$n seems to be blinded!", TRUE, tmp_victim, 0, 0, TO_ROOM);
                            send_to_char(tmp_victim, "A flash grenade goes off in front of you! You have been blinded!\r\n");
                            af.duration = rand_number(1, 2);
                            affect_to_char(tmp_victim, &af);
                        }
                        tmp_victim = temp;
                    }
                    break;
                    case GRENADE_SONIC:
                        // shatters ears, slows them down for 2 ticks
                        // makes room SOUNDPROOF for 1 tick
						new_affect(&af);
                        af.type      = PSIONIC_LETHARGY;
                        af.duration  = 1;
                        SET_BIT_AR(af.bitvector, ROOM_SOUNDPROOF);
                        affect_to_room(room, &af);
						
                        while (tmp_victim != NULL) {
                            temp = tmp_victim->next_in_room;
                            if (!rand_number(0, 2) && !affected_by_psionic(tmp_victim, PSIONIC_LETHARGY)) {
                                act("$n seems to be moving a bit slower now.", TRUE, tmp_victim, 0, 0, TO_ROOM);
                                send_to_char(tmp_victim, "You've been hit by a high pitched sonic stun! You seem to be moving slower now.\r\n");
                                affect_to_char(tmp_victim, &af);
                            }
                            tmp_victim = temp;
                        }
                        break;
                case GRENADE_SMOKE:
                    // fills room with smoke, and random linked rooms
					new_affect(&af);
                    af.type      = PSIONIC_SMOKE;
                    af.duration  = rand_number(0, 1);
                    af.modifier  = 0;
                    af.location  = APPLY_NONE;
                    SET_BIT_AR(af.bitvector, ROOM_DARK);
                    affect_to_room(room, &af);
                    send_to_room(room, "Dark, thick grey smoke begins to fill the room.\r\n");
                    for (door = 0; door < NUM_OF_DIRS; door++) {
                        if (CAN_GO2(room, door)) {
                            af.duration  = rand_number(0, 1);
                            affect_to_room(NEXT_ROOM(room, door), &af);
                            send_to_room(NEXT_ROOM(room, door), "Dark, thick grey smoke begins to seep in from the other room.\r\n");
                        }
                    }
                    break;
                case GRENADE_FIRE:
                    // fire bombs (molotov), sets room on fire and linked rooms
					new_affect(&af);
                    af.type      = PSIONIC_FIRE;
                    af.duration  = rand_number(0, 1);
                    SET_BIT_AR(af.bitvector, ROOM_DARK);
                    affect_to_room(room, &af);
                    send_to_room(room, "@rThe room! The room! The room is on @RFIRE!@n\r\n");
                    for (door = 0; door < NUM_OF_DIRS; door++) {
                        if (CAN_GO2(room, door)) {
                            if (!rand_number(0, 1)) {
                                af.duration  = rand_number(0, 1);
                                affect_to_room(NEXT_ROOM(room, door), &af);
                                send_to_room(NEXT_ROOM(room, door), "@rThe room! The room! The room is on @RFIRE!@n\r\n");
                            }
                        }
                    }
                    break;
                case GRENADE_CHEMICAL:
                    // affects players with poison for 2 ticks, makes room radiation for 1 tick
					new_affect(&af);
                    af.type      = PSIONIC_POISON;
                    af.duration  = 1;
                    SET_BIT_AR(af.bitvector, ROOM_RADIATION);
                    affect_to_room(room, &af);
					new_affect(&af);
                    af.type = PSIONIC_POISON;
                    af.location = APPLY_HITNDAM;
                    af.modifier = -15;
                    af.duration = 1;
                    SET_BIT_AR(af.bitvector, AFF_POISON);
                    while (tmp_victim != NULL) {
                        temp = tmp_victim->next_in_room;
                        if (!affected_by_psionic(tmp_victim, PSIONIC_POISON)) {
                            act("$n shivers and vomits on the ground.", TRUE, tmp_victim, 0, 0, TO_ROOM);
                            send_to_char(tmp_victim, "You've been hit by a chemical weapon! You don't feel so good...\r\n");
                            affect_to_char(tmp_victim, &af);
                        }
                        tmp_victim = temp;
                    }
                    break;
                case GRENADE_PSYCHIC:
                    // drains everyone's psi power with leech
                    // makes room PSI_DRAINING for 1 tick
					new_affect(&af);
                    af.type      = PSIONIC_PSYCHIC_LEECH;
                    af.duration  = 1;
                    SET_BIT_AR(af.bitvector, ROOM_PSI_DRAINING);
                    affect_to_room(room, &af);
                    send_to_room(room, "The psionic aura of the room seems to have changed.\r\n");
                    while (tmp_victim != NULL) {
                        temp = tmp_victim->next_in_room;
                        GET_PSI(tmp_victim) -= rand_number(0, GET_PSI(tmp_victim)/4);
                        send_to_char(tmp_victim, "You have been psychically leeched!\n\r");
                        tmp_victim = temp;
                    }
                    break;
                case GRENADE_LAG:
                    // makes room lag, similar to SECT_PAUSE, and random linked rooms
					new_affect(&af);
                    af.type      = PSIONIC_LAG;
                    af.duration  = rand_number(1, 2);
                    SET_BIT_AR(af.bitvector, ROOM_NO_RECALL);
                    affect_to_room(room, &af);
                    send_to_room(room, "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");
                    for (door = 0; door < NUM_OF_DIRS; door++) {
                        if (CAN_GO2(room, door)) {
                            af.duration  = rand_number(1, 2);
                            affect_to_room(NEXT_ROOM(room, door), &af);
                            send_to_room(NEXT_ROOM(room, door), "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");
                        }
                    }
                    break;
                case GRENADE_NUCLEAR:
                    // makes adjoined rooms explode into radiation, stays within zone boundries
                    // the psionic will radiate every few seconds until it runs out
					new_affect(&af);
                    af.type      = PSIONIC_NUKEBLAST;
                    af.duration  = rand_number(10, 24);
                    SET_BIT_AR(af.bitvector, ROOM_RADIATION);
                    affect_to_room(room, &af);
                    zone = world[room].zone;
                    for (obj = object_list; obj; obj = next_o) {
                        next_o = obj->next;
                        if (IN_ROOM(obj) != NOWHERE && world[IN_ROOM(obj)].zone == zone && obj != explosive)
                            extract_obj(obj);
                    }
                    for (i = character_list; i; i = temp) {
                        temp = i->next;
                        if (IN_ROOM(i) != NOWHERE && world[IN_ROOM(i)].zone == zone && GET_LEVEL(i) < LVL_IMMORT) {
                            if (!IS_NPC(i)) {
                                send_to_char(i, "With a flash of light, the world around you erupts into flames!\r\nThe Apocalypse has come at last! All life will cease to exist...");
								raw_kill(i, i, FALSE);
                            }
							else 
								extract_char(i);
                        }
                    }
                    break;
                case GRENADE_NAPALM:
                    // makes room on fire, sticks to people, spreads in all directions
					new_affect(&af);
                    af.type      = PSIONIC_NAPALM;
                    af.duration  = rand_number(2, 4);
                    affect_to_room(room, &af);
                    send_to_room(room, "Fire falls from the sky, it's Napalm!\r\nIt's sticking to your body, you can't get it off!\r\n");
                    for (door = 0; door < NUM_OF_DIRS; door++) {
                        if (CAN_GO2(room, door) && world[room].zone == world[NEXT_ROOM(room,door)].zone && !room_affected_by_psionic(world[NEXT_ROOM(room,door)].number, PSIONIC_NAPALM)) {
                            af.duration  = rand_number(2, 4);
                            affect_to_room(NEXT_ROOM(room, door), &af);
                            send_to_room(NEXT_ROOM(room, door), "Fire falls from the sky, it's Napalm!\r\nIt's sticking to your body, you can't get it off!\r\n");
                        }
                    }
                    break;
            }
            break;
    }
}
