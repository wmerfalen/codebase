/**************************************************************************
*  File: psionics.c                                        Part of tbaMUD *
*  Usage: Implementation of "manual psionics."                            *
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
#include "psionics.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "act.h"
#include "fight.h"
#include "drugs.h"
#include "screen.h"
#include "act.h"
#include "craft.h"
#include "graph.h"
#include "quest.h"
#include "bionics.h"
#include "mud_event.h"
void call_pets(struct char_data *ch);

// Special psionics appear below

// psionic_create_water removed
// psionic_recall is a global command
// psionic_enchant_weapon removed
// psionic_detect_poison removed todo: re-introduce this as a skill

APSI(psionic_teleport)
{
    room_rnum to_room;

    // prevent them from teleporting if their current location doesnt allow it?
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_NO_TP)) {
        send_to_char(ch, "A bright flash prevents your psionic from working!");
        return;
    }

    do {
        to_room = rand_number(0, top_of_world);
    } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE) || ROOM_FLAGGED(to_room, ROOM_DEATH) ||
        ROOM_FLAGGED(to_room, ROOM_GODROOM) || ROOM_FLAGGED(to_room, ROOM_NO_TP) ||
        ZONE_FLAGGED(world[to_room].zone, ZONE_NO_TP) ||
        (!IS_REMORT(ch) && ZONE_FLAGGED(world[to_room].zone, ZONE_REMORT_ONLY)));

    act("$n slowly fades out of existence and is gone.", FALSE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, to_room);

    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);

	int exp = (rand_number(-250,250) + (12 * rand_number(10,30) * (GET_LEVEL(ch) / 2)));
	increment_skilluse(PSIONIC_TELEPORT, 0, GET_IDNUM(ch), exp, ch);
    
	entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
}

#define SUMMON_FAIL "You failed.\r\n"
APSI(psionic_summon)
{
    if (ch == NULL || victim == NULL)
        return;

    if (GET_LEVEL(victim) > GET_LEVEL(ch) + rand_number(10, 25)) {
        send_to_char(ch, "%s", SUMMON_FAIL);
        return;
    }

    if (IS_NPC(victim) && !MOB_FLAGGED(victim, MOB_SUMMON)) {
        send_to_char(ch, "%s", SUMMON_FAIL);
        return;
    }

    if (ZONE_FLAGGED(world[IN_ROOM(victim)].zone, ZONE_NO_SUMPORT) || ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_NO_SUMPORT)) {
        send_to_char(ch, "%s", SUMMON_FAIL);
        return;
    }

    if (ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_REMORT_ONLY) && !IS_REMORT(victim)) {
        send_to_char(ch, "%s", SUMMON_FAIL);
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_SUMPORT) || ROOM_FLAGGED(IN_ROOM(victim), ROOM_NO_SUMPORT)) {
        send_to_char(ch, "%s", SUMMON_FAIL);
        return;
    }
	
	if (ch == victim) {
		send_to_char(ch, "%s", SUMMON_FAIL);
		return;
	}
	
    if (!CONFIG_PK_ALLOWED) {
        if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
            act("As the words escape your lips and $N travels\r\n"
                "through time and space towards you, you realize that $E is\r\n"
                "aggressive and might harm you, so you wisely send $M back.",
                FALSE, ch, 0, victim, TO_CHAR);
            return;
        }

        if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_NOSUMMON) && !PLR_FLAGGED(victim, PLR_KILLER)) {
            send_to_char(victim, "%s just tried to summon you to: %s.\r\n"
                "%s failed because you have summon protection on.\r\n"
                "Type NOSUMMON to allow other players to summon you.\r\n",
                GET_NAME(ch), world[IN_ROOM(ch)].name, (ch->player.sex == SEX_MALE) ? "He" : "She");

            send_to_char(ch, "You failed because %s has summon protection on.\r\n", GET_NAME(victim));
            mudlog(BRF, LVL_IMMORT, TRUE, "%s failed summoning %s to %s.", GET_NAME(ch), GET_NAME(victim), world[IN_ROOM(ch)].name);
            return;
        }
    }

    act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

    char_from_room(victim);
    char_to_room(victim, IN_ROOM(ch));

    act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
    act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);

	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_SUMMON].min_level * rand_number(20,30) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_SUMMON, 0, GET_IDNUM(ch), exp, ch);

    look_at_room(victim, 0);
    entry_memory_mtrigger(victim);
    greet_mtrigger(victim, -1);
    greet_memory_mtrigger(victim);
}

// Used by the locate object psionic to check the alias list on objects
int isname_obj(char *search, char *list)
{
    char *found_in_list; /* But could be something like 'ring' in 'shimmering.' */
    char searchname[128];
    char namelist[MAX_STRING_LENGTH];
    int found_pos = -1;
    int found_name = 0; /* found the name we're looking for */
    int match = 1;
    int i;

    // Force to lowercase for string comparisons
    sprintf(searchname, "%s", search);
    for (i = 0; searchname[i]; i++)
        searchname[i] = LOWER(searchname[i]);

    sprintf(namelist, "%s", list);
    for (i = 0; namelist[i]; i++)
        namelist[i] = LOWER(namelist[i]);

    // see if searchname exists any place within namelist
    found_in_list = strstr(namelist, searchname);

    if (!found_in_list)
        return (0);

    // Found the name in the list, now see if it's a valid hit. The following
    // avoids substrings (like ring in shimmering) is it at beginning of namelist?
    for (i = 0; searchname[i]; i++)
        if (searchname[i] != namelist[i])
            match = 0;

    // It was found at the start of the namelist string
    if (match)
        found_name = 1;
    else {
        // It is embedded inside namelist. Is it preceded by a space?
        found_pos = found_in_list - namelist;
        if (namelist[found_pos-1] == ' ')
            found_name = 1;
    }

    if (found_name)
        return (1);
    else
        return (0);
}

// psionic slightly modified from Cyber 1.5.  Now, if your level wont permit you to see an object being
// carried by another player, you dont get any information at all.  This only affects adding the zone
// information for objects being carried, which did not align with what the other checks were doing.
APSI(psionic_locate_object)
{
    struct obj_data *i;
    char name[MAX_INPUT_LENGTH];
    int j;

    if (!obj) {
        send_to_char(ch, "You sense nothing.\r\n");
        return;
    }

    //  added a global var to catch 2nd arg
    sprintf(name, "%s", cast_arg2);

    j = GET_LEVEL(ch);  /* # items to show = char's level */

    for (i = object_list; i && (j > 0); i = i->next) {

        if (!isname_obj(name, i->name))
            continue;

        if (OBJ_FLAGGED(i, ITEM_NOLOCATE) && GET_LEVEL(ch) < LVL_GOD)
            continue;

        send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

        if (i->carried_by) {
            if (GET_LEVEL(i->carried_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is being carried by %s.\n\r", PERS(i->carried_by, ch));
            else
                send_to_char(ch, "\n\r");
        }
        else if (IN_ROOM(i) != NOWHERE)
            send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
        else if (i->in_obj) {

            struct obj_data *container = i->in_obj;
            struct obj_data *next_container = container->in_obj;

            while (next_container != NULL) {
                container = next_container;
                next_container = container->in_obj;
            }

            if (container->carried_by) {
                if (GET_LEVEL(container->carried_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                    send_to_char(ch, " is in %s carried by %s.\n\r", OBJS(i->in_obj, ch), PERS(container->carried_by, ch));
                else
                    send_to_char(ch, "\r\n");
            }
            else
                send_to_char(ch, " is in %s.\n\r", OBJS(i->in_obj, ch));

        }
        else if (i->worn_by) {
            if (GET_LEVEL(i->worn_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is being worn by %s.\n\r", PERS(i->worn_by, ch));
            else
                send_to_char(ch, "\r\n");
        }
        else if (i->implanted) {
            if (GET_LEVEL(i->implanted) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is implanted in %s.\n\r", PERS(i->implanted, ch));
            else
                send_to_char(ch, "\r\n");
        }
        else
            send_to_char(ch, "'s location is uncertain.\r\n");

        j--;

    }

	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_LOCATE_OBJECT].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_LOCATE_OBJECT, 0, GET_IDNUM(ch), exp, ch);

    if (j == (GET_LEVEL(ch) >> 1))
        send_to_char(ch, "You sense nothing.\n\r");
    else if (j == GET_LEVEL(ch))
        send_to_char(ch, "You sense nothing.\r\n");
}

ACMD(do_query)
{
    struct obj_data *i;
    char name[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int j;

  argument = one_argument(argument, arg);

if (GET_SKILL_LEVEL(ch, SKILL_QUERY) == 0) {
    send_to_char(ch, "You do not know how to use this skill.\r\n");
    return;
}


    if (GET_MOVE(ch) < 50) {
        send_to_char(ch, "You're too exhausted to perform a query!\r\n");
        return;
    }

        GET_MOVE(ch) -= 50;
   send_to_char(ch, "Object query initialized..\r\n");
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

  if (!*arg) {
    send_to_char(ch, "No argument specified for query...returning null.\r\n");
    return;
}
    //  added a global var to catch 2nd arg
    sprintf(name, "%s", arg);
    j = GET_LEVEL(ch);  /* # items to show = char's level */

    for (i = object_list; i && (j > 0); i = i->next) {

        if (!isname_obj(name, i->name))
            continue;

        if (OBJ_FLAGGED(i, ITEM_NOLOCATE) && GET_LEVEL(ch) < LVL_GOD)
            continue;

        send_to_char(ch, "%c%s", UPPER(*i->short_description), i->short_description + 1);

        if (i->carried_by) {
            if (GET_LEVEL(i->carried_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is being carried by %s.\n\r", PERS(i->carried_by, ch));
            else
                send_to_char(ch, "\n\r");
        }
        else if (IN_ROOM(i) != NOWHERE)
            send_to_char(ch, " is in %s.\r\n", world[IN_ROOM(i)].name);
        else if (i->in_obj) {

            struct obj_data *container = i->in_obj;
            struct obj_data *next_container = container->in_obj;

            while (next_container != NULL) {
                container = next_container;
                next_container = container->in_obj;
            }

            if (container->carried_by) {
                if (GET_LEVEL(container->carried_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                    send_to_char(ch, " is in %s carried by %s.\n\r", OBJS(i->in_obj, ch), PERS(container->carried_by, ch));
                else
                    send_to_char(ch, "\r\n");
            }
            else
                send_to_char(ch, " is in %s.\n\r", OBJS(i->in_obj, ch));

        }
        else if (i->worn_by) {
            if (GET_LEVEL(i->worn_by) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is being worn by %s.\n\r", PERS(i->worn_by, ch));
            else
                send_to_char(ch, "\r\n");
        }
        else if (i->implanted) {
            if (GET_LEVEL(i->implanted) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
                send_to_char(ch, " is implanted in %s.\n\r", PERS(i->implanted, ch));
            else
                send_to_char(ch, "\r\n");
        }
        else
            send_to_char(ch, "'s location is uncertain.\r\n");

        j--;

    }

	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_LOCATE_OBJECT].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_LOCATE_OBJECT, 0, GET_IDNUM(ch), exp, ch);

    if (j == (GET_LEVEL(ch) >> 1))
        send_to_char(ch, "You sense nothing.\n\r");
    else if (j == GET_LEVEL(ch))
        send_to_char(ch, "You sense nothing.\r\n");
}
APSI(psionic_psi_bullet)
{
	int dam;
	int bullets;
	
	if (GET_LEVEL(ch) <= 10)
		bullets = 1;
	else if (GET_LEVEL(ch) <= 20)
		bullets = 2;
	else if (GET_LEVEL(ch) <= 30)
		bullets = 3;
	else if (GET_LEVEL(ch) <= 40)
		bullets = 4;
	else
		bullets = 5;

	bullets += GET_REMORTS(ch);

	if (bullets >= 8)
		bullets = 8;

	GET_PSIBULLETS(ch) = bullets;
	

	GET_PSIBULLETS(ch)--;
	dam = GET_LEVEL(ch) * (rand_number(20,60));
	if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
		send_to_char(ch, "[%d] ", dam);
	act("$n releases a glowing hot bullet of pure energy at $N knocking him back!", FALSE, ch, 0, victim, TO_ROOM);
	act("You release a glowing hot bullet of pure energy at $N knocking him back!", FALSE, ch, 0, victim, TO_CHAR);
	act("You are knocked back as $n fires a glowing hot bullet of pure energy at you!", FALSE, ch, 0, victim, TO_VICT);

	if (FIGHTING(ch) == NULL) {
		set_fighting(ch, victim);
		set_fighting(victim, ch);
	}

	
	int exp = dam * bullets;
	increment_skilluse(PSIONIC_PSI_BULLET, 0, GET_IDNUM(ch), exp, ch);

	damage(ch, victim, dam, PSIONIC_PSIONIC_CHANNEL, DMG_PSIONIC);
	update_pos(victim);



}


APSI(psionic_tigerclaw)
{
	int dam;
	int bullets;
        
        GET_CLAWS(ch) = 0;

	if (GET_EQ(ch, WEAR_HOLD) && (GET_EQ(ch, WEAR_WIELD))) {
           send_to_char(ch, "\t[f420]You must have a free hand to morph a tigerclaw.@n\r\n");
           GET_CLAWS(ch) = 0;
           return;
        }
		send_to_char(ch, "You summon your freakish abilities to morph your hand into a deadly \t[f420]Tiger Claw@n!!\r\n\r\n");
		send_to_char(victim, "%s morphs a hand into a deadly \t[f420]Tiger Claw@n!!\r\n\r\n", GET_NAME(ch));

	if (GET_LEVEL(ch) <= 10)
		bullets = 1;
	else if (GET_LEVEL(ch) <= 20)
		bullets = 2;
	else if (GET_LEVEL(ch) <= 30)
		bullets = 3;
	else if (GET_LEVEL(ch) <= 40)
		bullets = 4;
	else
		bullets = 5;

	bullets += GET_REMORTS(ch);

	if (bullets >= 8)
		bullets = 8;

	GET_CLAWS(ch) = bullets;
	

	GET_CLAWS(ch)--;
	dam = GET_LEVEL(ch) * (rand_number(30,60));
	if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
		send_to_char(ch, "[%d] ", dam);
  switch (rand_number(0, 6)) {
  case 0:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 1:
        act("$N runs and dives at $n, slicing and clawing $s face to shreds!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You run and dive at $N, @Rslicing and clawing@n $S face to shreds!", FALSE, ch, 0, victim, TO_CHAR);
        act("$n runs and dives at you, @Rslicing@n and @Rclawing@n your face to shreds!", FALSE, ch, 0, victim, TO_VICT);
    break;
  case 2:
        act("$n brandishes the claw of a tiger and pounds $S sharp claws into $N's chin with a @Ytiger uppercut!@n",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You pound your \t[f420]Tiger Claw@n perfectly under $N's chin. $E is shaken from the uppercut!", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel @WPhenomenal pain@n as $n pounds you with a \t[f420]Tiger Claw@n uppercut.", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 3:
        act("$N stretches out $S \t[f420]Tiger Claw@n and takes a broad swipe at $n. ",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You stretch out your \t[f420]Tiger Claw@n and take a broad swipe at $N. That looks painful!", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel wounds opening up as $n takes a broad swipe with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 4:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 5:
        act("$N leaps into the air with $S \t[f420]Tiger Claw@n flailing at $n, Blood covers the room!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You leap into the air with your \t[f420]Tiger Claw!@n flailing at $N, @RBlood covers the room!.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("You notice $n leaping at you with $s \t[f420]Tiger Claw!@n flailing, blood covers the room as you feel the sudden pain from the cuts!", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 6:
        act("$N spins in a circle with $S \t[f420]Tiger Claw@n targeting $n, slice and diced!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You hold out your \t[f420]Tiger Claw!@n and spin in a circle towards $N. Slice and Diced!", FALSE, ch, 0, victim, TO_CHAR);
        act("Spinning towards you, $n's \t[f420]Tiger Claw!@n cuts you multiple times!", FALSE, ch, 0, victim, TO_VICT);

    break;
  default:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  }
	if (FIGHTING(ch) == NULL) {
		set_fighting(ch, victim);
		set_fighting(victim, ch);
	}

	int exp = dam * bullets;
	increment_skilluse(PSIONIC_TIGERCLAW, 0, GET_IDNUM(ch), exp, ch);

	damage(ch, victim, dam, TYPE_CLAW, DMG_PSIONIC);
	update_pos(victim);
}


APSI(psionic_lightning_flash)
{
	struct char_data *i;
	struct affected_type af;
	char buf[MAX_STRING_LENGTH];
	int is_in, dir, dis, maxdis, found = 0;

	const char *distance[] = {
		"right here",
		"immediately ",
		"nearby ",
		"a ways ",
		"far ",
		"very far ",
		"extremely far ",
		"impossibly far ",
	};

	if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
		act("You can't see anything, you're blind!", TRUE, ch, 0, 0, TO_CHAR);
		return;
	}

	maxdis = 7;

	act("A huge lightning flash illuminates the area, you can see the following charmable animals:", TRUE, ch, 0, 0, TO_CHAR);
	act("A huge lightning flash ilumminates $n's surroundings.", FALSE, ch, 0, 0, TO_ROOM);

	is_in = ch->in_room;
	
	for (dir = 0; dir < NUM_OF_DIRS; dir++) {
		ch->in_room = is_in;
		for (dis = 0; dis <= maxdis; dis++) {
			if (((dis == 0) && (dir == 0)) || (dis > 0)) {
				for (i = world[ch->in_room].people; i; i = i->next_in_room) {
					if (!((ch == i) && (dis == 0))) {
						if (MOB_FLAGGED(i, MOB_CHARM)) {
							sprintf(buf, "%33s: %s%s%s%s", GET_NAME(i), distance[dis],
							((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
							(dis > 0) ? dirs[dir] : "",
							((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
							act(buf, TRUE, ch, 0, 0, TO_CHAR);
							found++;
						}

						if (IS_NPC(i) && (!MOB_FLAGGED(i, MOB_SENTINEL)) && GET_MOB_SPEC(i) != questmaster) {
							
							new_affect(&af);
							af.type = PSIONIC_LETHARGY;
							af.duration = .3;
						    SET_BIT_AR(af.bitvector, AFF_FLASH);
						    affect_to_char(i, &af);
							FUN_HUNTING(i) = ch;
							fun_hunt(i);
						}
					}
				}
			}
			if (!CAN_GO(ch, dir) || (world[ch->in_room].dir_option[dir]->to_room== is_in))
				break;
			else
				ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
		}
	}

	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_LIGHTNING_FLASH].min_level * rand_number(10,20) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_LIGHTNING_FLASH, 0, GET_IDNUM(ch), exp, ch);

	if (found == 0)
		act("Nobody anywhere near you.", TRUE, ch, 0, 0, TO_CHAR);
	ch->in_room = is_in;
}

APSI(psionic_charm)
{
    struct affected_type af;
    int j = 0;
    struct follow_type *k;

    if (victim == NULL || ch == NULL)
        return;

    // how many charmed followers?
    for (k = ch->followers; k; k = k->next)
        if (AFF_FLAGGED(k->follower, AFF_CHARM))
            j += 1;

    if (victim == ch)
        send_to_char(ch, "You like yourself even better!\r\n");
    else if (!MOB_FLAGGED(victim, MOB_CHARM))
        send_to_char(ch, "Your victim resists!\r\n");
    else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_NOSUMMON))
        send_to_char(ch, "You fail because SUMMON protection is on!\r\n");
    else if (AFF_FLAGGED(victim, AFF_SANCT) ||
        affected_by_psionic(victim, PSIONIC_SANCTUARY) ||
        affected_by_psionic(victim, PSIONIC_SUPER_SANCT) ||
        affected_by_psionic(victim, PSIONIC_MEGA_SANCT))
        send_to_char(ch, "Your victim is protected by sanctuary!\r\n");
    else if (AFF_FLAGGED(ch, AFF_CHARM))
        send_to_char(ch, "You can't have any followers of your own!\r\n");
    else if (AFF_FLAGGED(victim, AFF_CHARM) || GET_LEVEL(ch) < GET_LEVEL(victim))
        send_to_char(ch, "You fail.\r\n");
    // player charming another player - no legal reason for this
    else if (!IS_NPC(victim))
        send_to_char(ch, "You fail - shouldn't be doing it anyway.\r\n");
    else if (circle_follow(victim, ch))
        send_to_char(ch, "Sorry, following in circles is not allowed.\r\n");
    else if (GET_LEVEL(victim) >= LVL_IMMORT)
        send_to_char(ch, "They resist your mind control.\r\n");
    else if (j > GET_INT(ch)/8)
        send_to_char(ch, "You cannot have so many mobs for now.\r\n");
    else {

        if (victim->master)
            stop_follower(victim);

        add_follower(victim, ch);
        new_affect(&af);
		af.type = PSIONIC_CHARM;
        af.duration = 24 * 2;
        if (GET_CHA(ch))
            af.duration *= GET_CHA(ch);
        if (GET_INT(victim))
            af.duration /= GET_INT(victim);
        SET_BIT_AR(af.bitvector, AFF_CHARM);
        affect_to_char(victim, &af);
    if (GET_SKILL_LEVEL(ch, SKILL_LOYALTY) != 0 || IS_NPC(ch)) {
        send_to_char(ch, "Because of your loyalty to your charmie, it will autoassist!\r\n");
		int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_CHARM].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
		increment_skilluse(PSIONIC_CHARM, 0, GET_IDNUM(ch), exp, ch);
        SET_BIT_AR(PRF_FLAGS(victim), PRF_AUTOASSIST);
    }
        SET_BIT_AR(PRF_FLAGS(victim), PRF_AUTOSPLIT);
        act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
        if (IS_NPC(victim)) {
            REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_SPEC);
            REMOVE_BIT_AR(MOB_FLAGS(victim), MOB_AGGRESSIVE);
        }
    }
}

APSI(psionic_identify)
{
    int i;
    int found;
    size_t len;
    if (!obj) {
	send_to_char(ch, "Ok, but what would you like to identify?\r\n");
	return;
    }
    if (obj) {

        char bitbuf[MAX_STRING_LENGTH];

        sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
        send_to_char(ch, "You feel informed:\r\nObject '%s', Item type: %s\r\n", obj->short_description, bitbuf);

        sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, bitbuf);
        send_to_char(ch, "Can be worn on: %s\r\n", bitbuf);

        for (i = 0; i < AF_ARRAY_MAX; i++)
            if (GET_OBJ_AFFECT(obj)[i] != 0) {
                sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
                send_to_char(ch, "Item will give you following abilities:  %s\r\n", bitbuf);
                break;
            }

        sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, bitbuf);
        send_to_char(ch, "Item is: %s", bitbuf);

        // todo: this might need modification because the remort flag might not always be last
        if (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY))
            send_to_char(ch, ": x%d\r\n", GET_OBJ_REMORT(obj));
        else
            send_to_char(ch, "\r\n");

        // todo: remove the rent info if we go with free rent
		if (!IS_COMP_NOTHING(obj))
			send_to_char(ch, "Can be salvaged into: [%s] %s\r\n\r\n", GET_ITEM_COMPOSITION(obj), GET_ITEM_SUBCOMPOSITION(obj));
		if (IS_COMP_NOTHING(obj))
			send_to_char(ch, "This item cannot be salvaged.\r\n\r\n");
        send_to_char(ch, "Weight: %d, Value: %d, Min. level: %d\r\n",
            GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_LEVEL(obj));

        switch (GET_OBJ_TYPE(obj)) {

            case ITEM_SCROLL:
            case ITEM_POTION:
                len = i = 0;

                if (GET_OBJ_VAL(obj, 1) >= 1) {
                    i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", psionic_name(GET_OBJ_VAL(obj, 1)));
                    if (i >= 0)
                        len += i;
                }

                if (GET_OBJ_VAL(obj, 2) >= 1 && len < sizeof(bitbuf)) {
                    i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", psionic_name(GET_OBJ_VAL(obj, 2)));
                    if (i >= 0)
                        len += i;
                }

                if (GET_OBJ_VAL(obj, 3) >= 1 && len < sizeof(bitbuf)) {
                    i = snprintf(bitbuf + len, sizeof(bitbuf) - len, " %s", psionic_name(GET_OBJ_VAL(obj, 3)));
                    if (i >= 0)
                        len += i;
                }

                send_to_char(ch, "This %s casts psionic: %s\r\n", item_types[(int) GET_OBJ_TYPE(obj)], bitbuf);
                break;

            case ITEM_WAND:
            case ITEM_STAFF:
                send_to_char(ch, "This %s casts psionic: %s\r\nIt has %d maximum charge%s and %d remaining.\r\n",
                    item_types[(int) GET_OBJ_TYPE(obj)], psionic_name(GET_OBJ_VAL(obj, 3)),
                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
                break;

            case ITEM_WEAPON:
            case ITEM_WEAPONUPG:
                send_to_char(ch, "Damage Dice is '%dD%d' for an average per-round damage of %.1f.\r\n",
                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
				send_to_char(ch, "Weapon does %s %s damage\r\n", damagetype_bits[GET_OBJ_VAL(obj, 0)], attack_hit_text[GET_OBJ_VAL(obj, 3) > 0 ? GET_OBJ_VAL(obj, 3) : 0].singular);
                break;

            case ITEM_ARMOR:
                send_to_char(ch, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
                break;

            case ITEM_GUN:
                sprinttype(GET_GUN_TYPE(obj), gun_types, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Gun Type: %s\n\rThe number of shots per round: %d\n\r", bitbuf, GET_SHOTS_PER_ROUND(obj));
                send_to_char(ch, "Unloaded Damage: %dd%d for an average per-round damage of %.1f.\r\n", GET_GUN_BASH_NUM(obj), GET_GUN_BASH_SIZE(obj), ((GET_GUN_BASH_SIZE(obj) + 1) / 2.0) * GET_GUN_BASH_NUM(obj));
                send_to_char(ch, "Loaded Damage: %dd%d for an average per-round damage of %.1f.\r\n",                    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
				send_to_char(ch, "Weapon has a range of: %d\r\n", GET_RANGE(obj));
				break;


            case ITEM_AMMO:
                sprinttype(GET_AMMO_TYPE(obj), gun_types, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Ammo Type: %s %s\n\rAmmo count: %d\n\r", damagetype_bits[GET_OBJ_VAL(obj, 2)], bitbuf, GET_AMMO_COUNT(obj));
                break;

            case ITEM_RECALL:
                if (GET_CHARGE(obj) == -1)
                    send_to_char(ch, "Charges left: Infinite\r\n");
                else
                    send_to_char(ch, "Charges left: [%d]\r\n", GET_CHARGE(obj));
                break;

            case ITEM_MEDKIT:
                if (GET_OBJ_VAL(obj, 0) > 0)
                    send_to_char(ch, "Restores HP: [%d]\r\n", GET_OBJ_VAL(obj, 0));
                if (GET_OBJ_VAL(obj, 1) > 0)
                    send_to_char(ch, "Restores PSI: [%d]\r\n", GET_OBJ_VAL(obj, 1));
                if (GET_OBJ_VAL(obj, 2) > 0)
                    send_to_char(ch, "Restores MOVE: [%d]\r\n", GET_OBJ_VAL(obj, 2));
                if (GET_OBJ_VAL(obj, 3) > 0)
                    send_to_char(ch, "Charges left: [%d]\r\n", GET_OBJ_VAL(obj, 3));
                break;
			case ITEM_BIONIC_DEVICE:
				send_to_char(ch, "\r\nImplants into: %s\r\n", body_parts[GET_OBJ_VAL(obj, 0)]);
				send_to_char(ch, "Implant permanent Psionic Loss: %d\r\n", bionic_psi_loss[GET_OBJ_VAL(obj, 0)]);
				send_to_char(ch, "Implant Type: %s\r\n", biotypes[GET_OBJ_VAL(obj, 2)]);
				if (GET_OBJ_VAL(obj, 1) > 0)
					send_to_char(ch, "%s implant is required for this bionic\r\n", body_parts[GET_OBJ_VAL(obj, 1)]);
				if (GET_OBJ_VAL(obj, 3) > 0)
					send_to_char(ch, "%s implant type is required for this bionic\r\n", biotypes[GET_OBJ_VAL(obj, 3)]);
				if (GET_OBJ_VAL(obj, 4) > 0)
					send_to_char(ch, "%s implant type is required for this bionic\r\n", biotypes[GET_OBJ_VAL(obj, 4)]);
				if (GET_OBJ_VAL(obj, 5) > 0)
					send_to_char(ch, "%s implant type is required for this bionic\r\n", biotypes[GET_OBJ_VAL(obj, 5)]);
				send_to_char(ch, "\r\n");
				break;
            case ITEM_DRUG:
                sprinttype(GET_OBJ_VAL(obj, 0), drug_names, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Drug: %s\r\n", bitbuf);
                break;

            case ITEM_EXPLOSIVE:
                sprinttype(GET_EXPLOSIVE_TYPE(obj), explosive_types, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Explosive Type: %s, ", bitbuf);
                sprinttype(GET_EXPLOSIVE_SUB_TYPE(obj), subexplosive_types, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Sub Type: %s\r\n", bitbuf);
                break;

            case ITEM_CONTAINER:
                send_to_char(ch, "Weight capacity: %d\r\n", GET_OBJ_VAL(obj, 0));
                break;

            case ITEM_LIGHT:
                if (GET_OBJ_VAL(obj, 2) == -1)
                    send_to_char(ch, "Hours left: Infinite\r\n");
                else
                    send_to_char(ch, "Hours left: [%d]\r\n", GET_OBJ_VAL(obj, 2));

            case ITEM_DRINKCON:
            case ITEM_FOUNTAIN:
                sprinttype(GET_OBJ_VAL(obj, 2), drinks, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "Capacity: %d, Contains: %d, Liquid: %s\r\n", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), bitbuf);
                break;

            case ITEM_NOTE:
                send_to_char(ch, "Tongue: %d\r\n", GET_OBJ_VAL(obj, 0));
                break;

            case ITEM_MONEY:
                send_to_char(ch, "Coins: %d\r\n", GET_OBJ_VAL(obj, 0));
                break;

            case ITEM_FIREWEAPON:
            case ITEM_MISSILE:
            case ITEM_TREASURE:
            case ITEM_WORN:
            case ITEM_OTHER:
            case ITEM_TRASH:
            case ITEM_TRAP:
            case ITEM_KEY:
            case ITEM_FOOD:
            case ITEM_PEN:
            case ITEM_BOAT:
            case ITEM_BUTTON:
            case ITEM_LOTTO:
            case ITEM_SCRATCH:
            case ITEM_FLAG:
            case ITEM_SABER_PIECE:
            case ITEM_IDENTIFY:
            case ITEM_MARBLE_QUEST:
            case ITEM_GEIGER:
            case ITEM_AUTOQUEST:
            case ITEM_BAROMETER:
            case ITEM_PSIONIC_BOX:
            case ITEM_QUEST:
            case ITEM_EXPANSION:
            case ITEM_FURNITURE:
                break;

            default:
                log("WARNING: Unknown object type for object %s in psi_identify\r\n", obj->short_description);
                break;

        }

        found = FALSE;
        for (i = 0; i < MAX_OBJ_APPLY; i++) {
            if ((GET_OBJ_APPLY_LOC(obj, i) != APPLY_NONE) && (GET_OBJ_APPLY_MOD(obj, i) != 0)) {

                if (!found) {
                    send_to_char(ch, "Can affect you as :\r\n");
                    found = TRUE;
                }

                sprinttype(GET_OBJ_APPLY_LOC(obj, i), apply_types, bitbuf, sizeof(bitbuf));
				//send_to_char(ch,"DBG:'%s'",bitbuf);
				int mod = GET_OBJ_APPLY_MOD(obj, i);
				
				if (!strcmp(bitbuf,"SKILLSET"))
				{	
					send_to_char(ch, "   Grants the following skill: %s\r\n",skills_info[mod].name);
				}
				else if (!strcmp(bitbuf,"PSI_MASTERY"))
				{
					send_to_char(ch, "   Grants the following psionic: %s\r\n",psionics_info[mod].name);
				}
				else
				{
					send_to_char(ch, "   Affects: %s By %d\r\n", bitbuf, mod);
				}
            }
        }

        sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, bitbuf);
        send_to_char(ch, "Has the following perm affects: %s\r\n", bitbuf);
        send_to_char(ch, "\r\n");

        if ((IS_SET(obj->obj_flags.type_flag, ITEM_WEAPON)) || (IS_SET(obj->obj_flags.type_flag, ITEM_GUN))) {
            if (obj->obj_wpnpsi.which_psionic) {
                send_to_char(ch, "\n\rHas the following extra effect:");
                sprinttype(obj->obj_wpnpsi.which_psionic, wpn_psionics, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "%s Level %d %s ", found++ ? "" : "", obj->obj_wpnpsi.skill_level, bitbuf);
            }
        }
    }
    if (obj->ex_description) {

        send_to_char(ch, "\r\nExtra description:\r\n");
            send_to_char(ch, "%s\r\n", obj->ex_description->description);
    }
    else if (victim) {
        send_to_char(ch, "Name: %s\r\n", GET_NAME(victim));

        if (!IS_NPC(victim))
            send_to_char(ch, "%s is %d years, %d months, %d days and %d hours old.\r\n",
                GET_NAME(victim), age(victim)->year, age(victim)->month,
                age(victim)->day, age(victim)->hours);

        send_to_char(ch, "Height %d cm, Weight %d pounds\r\n", GET_HEIGHT(victim), GET_WEIGHT(victim));
        send_to_char(ch, "Level: %d, Hits: %d, Psi: %d\r\n", GET_LEVEL(victim), GET_HIT(victim), GET_PSI(victim));
        send_to_char(ch, "AC: %d, Hitroll: %d, Damroll: %d\r\n", GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));

        send_to_char(ch, "Str: %d/%d, Int: %d, Pcn: %d, Dex: %d, Con: %d, Cha: %d\r\n",
            GET_STR(victim), GET_STR_ADD(victim), GET_INT(victim),
            GET_PCN(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));

    }
}

APSI(psionic_duplicates)
{
    int dupes;
    struct affected_type af;
    float tmpPM;

    if (ch->char_specials.saved.duplicates != 0) {
        send_to_char(ch, "You are still affected by psionic duplicates.\r\n");
        return;
    }

    if (affected_by_psionic(ch, PSIONIC_DUPLICATES)) {
        send_to_char(ch, "You are still affected by psionic duplicates.\r\n");
        return;
    }

    dupes = (GET_INT(ch) + GET_PCN(ch))/2;

    if (GET_PSIMASTERY(ch) > 0) {
        tmpPM = (float)1+(float)GET_PSIMASTERY(ch)/100;
        dupes = dupes * tmpPM;
    }
	new_affect(&af);
    af.type      = PSIONIC_DUPLICATES;
    af.duration  = -1;
	SET_BIT_AR(af.bitvector, AFF_DUPLICATES);
    affect_to_char(ch, &af);

	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_DUPLICATES].min_level * rand_number(10,30) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_DUPLICATES, 0, GET_IDNUM(ch), exp, ch);

    ch->char_specials.saved.duplicates = dupes;
	send_to_char(ch, "Exact replicas of you begin to appear from nowhere, in %d different locations.\r\n", dupes);
    act("Exact replicas of $n begin to fill the room.\r\n", FALSE, ch, 0, 0, TO_ROOM);
}

APSI(psionic_psychic_static)
{
    struct affected_type af;
    int d_roll;
    int num;

    d_roll = rand_number(1, 100);
    num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

    if ((d_roll <= 90 && num >= 4) ||
        (d_roll <= 80 && num == 3) ||
        (d_roll <= 70 && num == 2) ||
        (d_roll <= 60 && num == 1)) {

        if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_PSIONICS)) {
			new_affect(&af);
            af.type      = PSIONIC_PSYCHIC_STATIC;
            af.duration  = (GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/5);
            SET_BIT_AR(af.bitvector, ROOM_NO_PSIONICS);
            affect_to_room(world[IN_ROOM(ch)].number, &af);
			int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_PSYCHIC_STATIC].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
			increment_skilluse(PSIONIC_PSYCHIC_STATIC, 0, GET_IDNUM(ch), exp, ch);
            send_to_char(ch, "The psionic aura of the room seems to have changed.\r\n");
            act("The psionic aura of the room seems to have changed.", TRUE, ch, 0, 0, TO_ROOM);
        }
    }
}

APSI(psionic_detox)
{
    int d_roll;
    int num;

	if (!victim) // testing crash
		return;

    assert(victim);

    d_roll = rand_number(1, 100);
    num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

    if ((d_roll <= 90 && num >= 4) ||
        (d_roll <= 80 && num == 3) ||
        (d_roll <= 70 && num == 2) ||
        (d_roll <= 60 && num == 1)) {

        if (affected_by_psionic(victim, PSIONIC_POISON)) {
            affect_from_char(victim, PSIONIC_POISON, TRUE);
            act("You feel better as the poison is purged from your body.", FALSE, victim, 0, 0, TO_CHAR);
            act("$N looks better as the poison is purged from them.", FALSE, ch, 0, victim, TO_ROOM);
        }

        if (IS_ON_DRUGS(victim) || GET_OVERDOSE(victim) > 0 || GET_ADDICTION(victim) > 0) {

            while (victim->drugs_used)
                drug_remove(victim, victim->drugs_used);

            act("You regain your sense of reality as the toxins are removed from your system.", FALSE, victim, 0, 0, TO_CHAR);
            act("$N's eyes clear up as $E quickly detoxes.", FALSE, ch, 0, victim, TO_ROOM);
            GET_OVERDOSE(victim) = 0;
            GET_ADDICTION(victim) = 0;
			int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_DETOX].min_level * rand_number(10,30) * (GET_LEVEL(ch) / 2))));
			increment_skilluse(PSIONIC_DETOX, 0, GET_IDNUM(ch), exp, ch);
        }

		else
			send_to_char(ch, "You don't seem to be affected by any poison, or drug.\r\n");
    }
}


APSI(psionic_astral_projection)
{
    int d_roll;
    int num;

    assert(victim);

//    if (!IS_NPC(victim)) {
//        send_to_char(ch, "You can not use this psionic on players.\r\n");
//        return;
//    }

    num = GET_LEVEL(ch)/5 + (GET_INT(ch)+GET_PCN(ch))/10;

    d_roll = rand_number(1, 100);

    if ((d_roll <= 100 && num >=9 ) ||
        (d_roll <= 90 && num >= 7) ||
        (d_roll <= 80 && num >= 5) ||
        (d_roll <= 70 && num >= 3) ||
        (d_roll <= 60 && num == 1)) {

        act("$n begins to focus, $s face goes blank for a second, and then returns to normal.", TRUE, ch, 0, 0, TO_ROOM);
		send_to_char(ch, "You focus your mind on a far-away place, and feel your spirit drain from your body.\r\n");
		send_to_char(victim, "You feel another presence nearby.\r\n");

        char bitbuf[MAX_STRING_LENGTH];
        send_to_char(ch, "%s%s", CCNRM(ch, C_NRM), world[IN_ROOM(victim)].name);

        if (PRF_FLAGGED(ch, PRF_SHOWVNUMS)) {
            sprinttype((long) ROOM_FLAGS(IN_ROOM(ch)), room_bits, bitbuf, sizeof(bitbuf));
            send_to_char(ch, " (#%d) [%s]", world[IN_ROOM(victim)].number, bitbuf);
        }

        send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));

        if (!PRF_FLAGGED(ch, PRF_BRIEF))
            send_to_char(ch, "%s", world[IN_ROOM(victim)].description);

        send_to_char(ch, "%s", CCGRN(ch, C_NRM));
        list_obj_to_char(world[IN_ROOM(victim)].contents, ch, 0, FALSE);
        send_to_char(ch, "%s", CCYEL(ch, C_NRM));
        list_char_to_char(world[IN_ROOM(victim)].people, ch);
        send_to_char(ch, "%s", CCNRM(ch, C_NRM));
		int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_ASTRAL_PROJECTION].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
		increment_skilluse(PSIONIC_ASTRAL_PROJECTION, 0, GET_IDNUM(ch), exp, ch);
		send_to_char(ch, "\r\nYour spirit snaps back into your body and the vision returns to normal.\r\n");
        //d_roll = rand_number(1, 4);

        // check if victim retaliates
        //if (d_roll == 4) {
		//	int thaco = rand_number(1,30); 
        //    hit(victim, ch, TYPE_UNDEFINED, thaco);
		//}
    }

}

APSI(psionic_farsee)
{
    int d_roll;
    int num;

    assert(victim);

    num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

    d_roll = rand_number(1, 100);

    if ((d_roll <= 100 && num == 5) ||
        (d_roll <= 90 && num == 4) ||
        (d_roll <= 80 && num == 3) ||
        (d_roll <= 70 && num == 2) ||
        (d_roll <= 60 && num == 1)) {

        look_at_char(victim, ch);
        d_roll = rand_number(1, 4);

        // check if victim retaliates
        if (d_roll == 4) {
			int thaco = rand_number(1,30);
            hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
		}
	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_FARSEE].min_level * rand_number(10,15) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_FARSEE, 0, GET_IDNUM(ch), exp, ch);
    }
}


APSI(psionic_psychic_leech)
{
    int victim_psi;
    int psi_gain;
    int num;
    float tmpPM;

    // BUG crash when victim is invis and triggered from dg_cast FIXME
    //  assert(victim);

    if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim)) {
        send_to_char(ch, "But that's not playing nice!\n\r");
        return;
    }

    num = GET_LEVEL(ch) + (GET_INT(ch)+GET_PCN(ch)+(30-GET_PCN(victim)));

    victim_psi = GET_PSI(victim);

    if (victim_psi < num)
        psi_gain = victim_psi;
    else
        psi_gain = num;

    psi_gain = MAX(0, psi_gain - 30); /* cost of 30 psi to leech */

    if (psi_gain > 0) {
        if (affected_by_psionic(victim, PSIONIC_SANCTUARY))
            psi_gain /= 2;
        else if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT))
            psi_gain /= 3;
        else if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT))
            psi_gain /= 4;
    }

    if (!IS_NPC(victim))
        psi_gain /= 2;

    if (GET_PSIMASTERY(ch) > 0) {
        tmpPM = (float)1+(float)GET_PSIMASTERY(ch)/100;
        psi_gain = psi_gain * tmpPM;
    }

    GET_PSI(victim) -= psi_gain;
    GET_PSI(ch) += psi_gain;

    send_to_char(ch, "You feel a mental tingle.\n\r");
    send_to_char(victim, "You have been psychically leeched!\n\r");
	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_PSYCHIC_LEECH].min_level * rand_number(10,30) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_PSYCHIC_LEECH, 0, GET_IDNUM(ch), exp, ch);
    if (!FIGHTING(victim)) {
		int thaco = rand_number(1,30);
		hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
	}

    return;
}


APSI(psionic_commune)
{
    struct char_data *i;

    for (i = character_list; i; i = i->next)
        if (IS_NPC(i))
            if (CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE))
                if (world[IN_ROOM(i)].zone == world[IN_ROOM(ch)].zone)
                    if (GET_LEVEL(ch) > GET_LEVEL(i))
                        send_to_char(ch, "%-25s\n\r", GET_NAME(i));

}


APSI(psionic_panic)
{
    int rollchar;
	int rollvict;
	int numrand;
	int i;
	int j;
	int attempt;
	int randflee;

    assert(victim);
	if (!IS_NPC(victim) && !IS_NPC(ch)) {
		if (!PK_OK_ROOM(ch)) {
			send_to_char(ch, "PK is not allowed in this room.\r\n");
            return;
		}
        else
		{
			check_killer(ch, victim);
		}
    }

    if (MOB_FLAGGED(victim, MOB_SENTINEL)) {
        act("$n laughs mercelessly and jumps to attack $N!\r\n", FALSE, victim, 0, ch, TO_NOTVICT);
        send_to_char(ch,"%s laughs mercelessly and jumps to attack you!\r\n", GET_NAME(victim));
		int thaco = rand_number(1,30);
		hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
		return;
    }
	
	if (victim == ch) 
	{
        send_to_char(ch, "That is not reccommended.\n\r");
        return;
    }

	numrand = rand_number(1, 25);
	randflee = rand_number(2,5);

	rollchar = ((GET_LEVEL(ch) + GET_REMORTS(ch) + GET_INT(ch)) * GET_CHA(ch));
	rollvict = ((GET_LEVEL(victim) + GET_REMORTS(victim) + GET_INT(ch)) * numrand);
    //num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

    if (rollchar > rollvict) 
	{
		send_to_char(ch,"You send panic and fear into your enemy!\r\n");
        act("You cringe at the sight of $n and run away in terror!\r\n", TRUE, ch, 0, victim, TO_VICT);
        act("$n appears to have had a panic attack.\r\n", TRUE, victim, 0, 0, TO_ROOM);
        
		for (i = 0; i < 6; i++) 
		{
			attempt = rand_number(0, NUM_OF_DIRS - 1);    /* Select a random direction */
        
			if (CAN_GO(victim, attempt) && !ROOM_FLAGGED(EXIT(victim, attempt)->to_room, ROOM_DEATH)) 
			{
				if (do_simple_move(victim, attempt, TRUE)) 
				{
					send_to_char(victim, "You flee head over heels.\r\n");
				
					if (FIGHTING(ch)) 
					{
                    if (ch == FIGHTING(FIGHTING(ch)))
                        stop_fighting(FIGHTING(FIGHTING(ch)));
                        stop_fighting(FIGHTING(ch));
					}
				}
			}
		}

		for (j = 0; j < randflee; j++)
		{
			do_flee(victim, "", 0, 0);
		}
		int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_PANIC].min_level * rand_number(10,30) * (GET_LEVEL(ch) / 2))));
		increment_skilluse(PSIONIC_PANIC, 0, GET_IDNUM(ch), exp, ch);
    }

    else {
		send_to_char(victim,"You laugh at %s, and jump to attack!\n", GET_NAME(ch));
		send_to_char(ch,"%s laughs mercelessly and jumps to attack you!\n", GET_NAME(victim));
        act("$n laughs mercelessly and jumps to attack $N!\n", TRUE, victim, 0, ch, TO_NOTVICT);
		int thaco = rand_number(1,30);
		hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
    }

}
APSI(callpet)
{
        send_to_room(IN_ROOM(ch), "Call pet?.\r\n");

    call_pets(ch);
}


APSI(psionic_pacify)
{
    struct char_data *tmp_victim;
    int d_roll;
    int num;

    assert(ch);
    num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

    d_roll = rand_number(1, 100);
    if ((d_roll <= 100 && num == 5) ||
        (d_roll <= 90 && num == 4) ||
        (d_roll <= 80 && num == 3) ||
        (d_roll <= 70 && num == 2) ||
        (d_roll <= 60 && num == 1)) {

        send_to_room(IN_ROOM(ch), "The world seems a little more peaceful.\r\n");
        tmp_victim = world[IN_ROOM(ch)].people;

		int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_PACIFY].min_level * rand_number(10,20) * (GET_LEVEL(ch) / 2))));
		increment_skilluse(PSIONIC_PACIFY, 0, GET_IDNUM(ch), exp, ch);

        while (tmp_victim != NULL) {
            if (GET_POS(tmp_victim) == POS_FIGHTING) stop_fighting(tmp_victim);
                tmp_victim = tmp_victim->next_in_room;
        }

    }
    else
        send_to_char(ch, "It appears that no one wants to listen to you!\r\n");

}


APSI(psionic_portal)
{
    sh_int target;

    assert(ch && victim);

    if (GET_LEVEL(victim) > LVL_IMMORT) {
        send_to_char(ch, "You failed.\n\r");
        return;
    }

    if (!CONFIG_PK_ALLOWED) {

        if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_NOSUMMON) && !PLR_FLAGGED(victim, PLR_KILLER)) {
            send_to_char(victim, "%s just tried to portal to you from: %s.\n\r"
                "%s failed because you have summon protection on.\n\r"
                "Type NOSUMMON to allow other players to portal to you.\n\r",
                GET_NAME(ch), world[IN_ROOM(ch)].name, (ch->player.sex == SEX_MALE) ? "He" : "She");

            send_to_char(ch, "You failed because %s has summon protection on.\n\r", GET_NAME(victim));
            return;
        }
    }

    if (IS_NPC(victim)) {
        send_to_char(ch, "You failed.\n\r");
        return;
    }

    if (PRF_FLAGGED(victim, PRF_FROZEN)) {
        send_to_char(ch, "It would be REAL easy if you could just portal and tag them, wouldn't it?\r\n");
        return;
    }

    if (ZONE_FLAGGED(world[IN_ROOM(victim)].zone, ZONE_NO_SUMPORT) || ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_NO_SUMPORT)) {
        send_to_char(ch, "You failed.\r\n");
        return;
    }

    if (ZONE_FLAGGED(world[IN_ROOM(victim)].zone, ZONE_REMORT_ONLY) && !IS_REMORT(ch)) {
        send_to_char(ch, "You failed.\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_NO_SUMPORT) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_SUMPORT)) {
        send_to_char(ch, "You failed.\r\n");
        return;
    }

    act("$n steps though a psionical portal and vanishes.", TRUE, ch, 0, 0, TO_ROOM);
    target = IN_ROOM(victim);
    char_from_room(ch);
    char_to_room(ch, target);

    act("$n appears through a psionical portal.", TRUE, ch, 0, 0, TO_ROOM);
    do_look(ch, "", 15, 0);
	int exp = (rand_number(-250,250) + ((psionics_info[PSIONIC_PORTAL].min_level * rand_number(10,30) * (GET_LEVEL(ch) / 2))));
	increment_skilluse(PSIONIC_PORTAL, 0, GET_IDNUM(ch), exp, ch);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
}

// Weapon Spells

APSI(psi_psionic_channel)
{
    assert(victim);
//    damage(ch, victim, psi_level * rand_number(1, 3), PSIONIC_PSIONIC_CHANNEL, DMG_PSIONIC);
  if (ch->equipment[WEAR_WIELD])
    damage(ch, victim, (GET_OBJ_LEVEL(ch->equipment[WEAR_WIELD]) + psi_level) * (GET_PCN(ch) + GET_OBJ_REMORT(ch->equipment[WEAR_WIELD]) * rand_number(1, 50)), PSIONIC_PSIONIC_CHANNEL, DMG_PSIONIC);

}

APSI(psi_fire_blade)
{
    assert(victim);
//    damage(ch, victim, psi_level * rand_number(1, 4), PSIONIC_FIRE_BLADE, DMG_FIRE);
  if (ch->equipment[WEAR_WIELD])
    damage(ch, victim, (GET_OBJ_LEVEL(ch->equipment[WEAR_WIELD]) + psi_level) * (GET_PCN(ch) + GET_OBJ_REMORT(ch->equipment[WEAR_WIELD]) * rand_number(1, 50)), PSIONIC_FIRE_BLADE, DMG_FIRE);
}
APSI(psi_ice_blade)
{
    assert(victim);
    if (ch->equipment[WEAR_WIELD])
    damage(ch, victim, (GET_OBJ_LEVEL(ch->equipment[WEAR_WIELD]) + psi_level) * (GET_PCN(ch) + GET_OBJ_REMORT(ch->equipment[WEAR_WIELD]) * rand_number(1, 50)), PSIONIC_ICE_BLADE, DMG_COLD);
    
//    damage(ch, victim, psi_level * rand_number(1, 5), PSIONIC_ICE_BLADE, DMG_COLD);
}

APSI(psi_flash_grenade)
{
    struct affected_type af;
    int d_roll;

    assert(victim);

    if (affected_by_psionic(victim, PSIONIC_BLINDNESS))
        return;

    d_roll = rand_number(1, 100);

    if (d_roll - 60 <= (psi_level - 1) * 10) {

        act("$n fires a flash grenade at $N!", TRUE, ch, 0, victim, TO_ROOM);
        act("$n seems to be blinded!", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "A flash grenade goes off in front of you!\n\r");
        send_to_char(victim, "You have been blinded!\n\r");

		new_affect(&af);
		af.type = PSIONIC_BLINDNESS;
        af.location = APPLY_HITNDAM;
        af.modifier = -10;
        af.duration = GET_LEVEL(ch)/10 + 2;
        SET_BIT_AR(af.bitvector, AFF_BLIND );
		affect_to_char(victim, &af);
    }
    else {
        send_to_char(ch, "Your grenade explodes harmlessly off to the side.\n\r");
        return;
    }
}


APSI(psi_ultra_stun)
{
    struct affected_type af;

    //assert(victim);
	
    if (affected_by_psionic(victim, PSIONIC_LETHARGY)) {
		send_to_char(ch, "You failed.\r\n");
        return;
	}
	if ((MOB_FLAGGED(victim, MOB_AWARE) && AWAKE(victim))) {
        send_to_char(ch, "You failed.\r\n");
        return;
    }
	if ((GET_LEVEL(victim) - GET_LEVEL(ch)) > 5) {
		send_to_char(ch, "You failed.\r\n");
		return;
	}
	if (rand_number(1,200) < (((GET_LEVEL(victim) - GET_LEVEL(ch)) * 4) + 100)) {
		send_to_char(ch, "You failed.\r\n");
	    return;
	}
	else {
		new_affect(&af);
		af.type = PSIONIC_LETHARGY;
		af.duration = 2;
		SET_BIT_AR(af.bitvector, AFF_STUN); 
		affect_to_char(victim, &af);

		act("$n fires a high pitched sonic stun at $N.", TRUE, ch, 0, victim, TO_ROOM);
		act("$N seems to be moving a bit slower now.", TRUE, ch, 0, victim, TO_ROOM);
		send_to_char(victim, "You've been hit by a high pitched sonic stun!\r\n");
		send_to_char(victim, "You seem to be moving a bit slower now.\r\n");
	}

}


APSI(psi_gas_grenade)
{
    struct char_data *tmp_victim;
    int d_roll;

    d_roll = rand_number(1, 100);

    if (d_roll - 60 <= (psi_level - 1) * 10) {

        send_to_room(IN_ROOM(ch), "Tear gas begins to fill the room, making fighting impossible.\n\r");
        tmp_victim = world[IN_ROOM(ch)].people;

        while (tmp_victim != NULL) {
            if (GET_POS(tmp_victim) == POS_FIGHTING) stop_fighting(tmp_victim);
            tmp_victim = tmp_victim->next_in_room;
        }

    }
}

// sets the chars current room on fire
APSI(psi_fire)
{
    struct affected_type af;

	new_affect(&af);
    af.type      = PSIONIC_FIRE;
    af.duration  = 2;
	SET_BIT_AR(af.bitvector, PSIONIC_FIRE);
	affect_to_room(IN_ROOM(ch), &af);
}

// fills room with smoke, plus linked rooms
APSI(psi_smoke)
{
    struct affected_type af;
    int door;

    send_to_room(IN_ROOM(ch), "Smoke begins to fill the room.\r\n");
	new_affect(&af);
	af.type      = PSIONIC_SMOKE;
    af.duration  = 2;
    SET_BIT_AR(af.bitvector, ROOM_DARK);
    affect_to_room(IN_ROOM(ch), &af);

    for (door = 0; door < NUM_OF_DIRS; door++)
        if (CAN_GO(ch, door)) {
			af.type      = PSIONIC_SMOKE;
            af.duration  = 2;
            af.modifier  = 0;
            af.location  = APPLY_NONE;
            SET_BIT_AR(af.bitvector, ROOM_DARK);
            affect_to_room(EXIT(ch, door)->to_room, &af);
            send_to_room(EXIT(ch, door)->to_room, "Smoke begins to fill the room.\r\n");
        }
}
APSI(psi_turret)
{
    struct affected_type af;
    int door;

  if (room_affected_by_psionic(IN_ROOM(ch), PSIONIC_TURRET)) {
      send_to_char(ch, "There is already a turret constructed here.\r\n");
    return;
}

    send_to_room(IN_ROOM(ch), "@D%s constructs a turret in the center of the room.@n\r\n", GET_NAME(ch));
	new_affect(&af);
	af.type      = PSIONIC_TURRET;
    af.duration  = 1 - 1;
    affect_to_room(IN_ROOM(ch), &af);

    for (door = 0; door < NUM_OF_DIRS; door++)
        if (CAN_GO(ch, door)) {
			af.type      = PSIONIC_TURRET;
//            af.duration  = 1;
//            af.modifier  = 0;
//            af.location  = APPLY_NONE;
            affect_to_room(EXIT(ch, door)->to_room, &af);
            send_to_room(EXIT(ch, door)->to_room, "a turret aims in your direction.\r\n");
        }
}


APSI(psi_lag)
{
    struct affected_type af;
    int door;

	new_affect(&af);
    af.type      = PSIONIC_LAG;
    af.duration  = 2;
    SET_BIT_AR(af.bitvector, ROOM_NO_RECALL);
    affect_to_room(IN_ROOM(ch), &af);
    send_to_room(IN_ROOM(ch), "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");

    for (door = 0; door < NUM_OF_DIRS; door++)
        if (CAN_GO(ch, door)) {
            af.type      = PSIONIC_LAG;
            af.duration  = 2;
            SET_BIT_AR(af.bitvector, ROOM_NO_RECALL);
            affect_to_room(EXIT(ch, door)->to_room, &af);
            send_to_room(EXIT(ch, door)->to_room, "Lag begins to fi..l..l t..h...e.... roo...m...[NO CARRIER]\r\n");
        }
}




