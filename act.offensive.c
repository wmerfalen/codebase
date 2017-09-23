/**************************************************************************
*  File: act.offensive.c                                   Part of tbaMUD *
*  Usage: Player-level commands of an offensive nature.                   *
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
#include "act.h"
#include "fight.h"
#include "drugs.h"
#include "bionics.h"
#include "constants.h"
#include "skilltree.h"
#include "events.h"

ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room);

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
         /* prevent accidental pkill */
    else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))
      send_to_char(ch, "You cannot kill other players.\r\n");
    else {
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			
	  COMBAT_TICKS(ch) = 0;
	  TICK_TO_HIT(ch) = 5;
	  int thaco = rand_number(1,30);
      hit(ch, opponent, TYPE_UNDEFINED, thaco, 0);
    }
  }
}

ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

 one_argument(argument, arg);

	if (!*arg)
		send_to_char(ch, "Whom would you like to kill?\r\n");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_to_char(ch, "You don't see that person here.\r\n");
	else if (victim == ch) {
		send_to_char(ch, "You stare at longingly at your weapon, contemplating suicide.\r\n");
		act("A tear rolls down the face of $n as $e stares at $M weapon.", FALSE, ch, 0, victim, TO_ROOM);
	}
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == victim))
		send_to_char(ch, "You must release charmed beings from your power before killing them.\r\n");
	else if (victim->char_specials.targeted != NULL)
		send_to_char(ch, "%s has already been targeted by someone else.\r\n", GET_NAME(victim));
	else if (FIGHTING(ch) != NULL && FIGHTING(ch) != victim) {
		act("@YYou leave your opponent behind, and jump to attack $N@n!", FALSE, ch, 0, victim, TO_CHAR);
		stop_fighting(FIGHTING(ch));
		COMBAT_TICKS(ch) = 0;
		TICK_TO_HIT(ch) = 8;
		FIGHTING(ch) = victim;
	}
    else {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
			return;
		}
        // perform checks for player killing
        if (!CONFIG_PK_ALLOWED) {
            // players must use murder command to kill another player
            if (!IS_NPC(victim) && !IS_NPC(ch)) {
                if (!PK_OK_ROOM(ch)) {
                    send_to_char(ch, "Player killing is not permitted in safe areas.\r\n");
                    return;
                }
                else
                    check_killer(ch, victim);
            }
        }
        // char must be standing and victim must be fighting
        if ((GET_POS(ch) == POS_STANDING) && (victim != FIGHTING(ch))) {
			COMBAT_TICKS(ch) = 0;
			TICK_TO_HIT(ch) = 5;
			int thaco = rand_number(1,30);
			if (thaco <= 4)
				act("You stumble all over as you quickly try to get your attack off.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
			if (thaco > 5 && thaco < 10)
				act("You attack wildly, attempting to land a lucky hit.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
			if (thaco >= 10 && thaco < 20)
				act("You aim at your target patiently trying to strike at their vitals.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
			if (thaco >= 20 && thaco < 25)
				act("You confidently aim squarely with your exceptional skill.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
			if (thaco >= 25)
				act("Your eyes go red momentarily as you lock onto your opponent perfectly!\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
            hit(ch, victim, TYPE_UNDEFINED, thaco, 0);
        }
        else if (victim == FIGHTING(ch))
            send_to_char(ch, "You are already in combat.\r\n");
        else if (GET_POS(ch) == POS_RESTING || GET_POS(ch) == POS_SLEEPING)
            send_to_char(ch, "You might want to stand up first.\r\n");
    }
}

ACMD(do_kill)
{
    do_hit(ch, argument, cmd, subcmd);
    return;
}
ACMD(do_attack)
{
	do_hit(ch, argument, cmd, subcmd);
	return;
}

ACMD(do_assassinate)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *victim;

    if (GET_SKILL_LEVEL(ch, SKILL_ASSASSINATE) == 0) {
        send_to_char(ch, "You have no idea how to assassinate anyone.\n\r");
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
        send_to_char(ch, "Assassinate who?\r\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch, "Typing suicide would involve less work.\r\n");
        return;
    }

    if (!GET_EQ(ch, WEAR_WIELD) || IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "You need to wield a melee weapon to make it a success.\r\n");
        return;
    }

  // Ok, with a lack of pierce wepaons, let them backstab with anything -DH
  // Screw it, backstab only piercy
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_CLAW - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT 
	) 
	{
		send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
		return;
	}

    if (FIGHTING(victim)) {
        send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
        return;
    }

    if ((MOB_FLAGGED(victim, MOB_AWARE) && AWAKE(victim)) || GET_HIT(victim) < GET_MAX_HIT(victim)) {
		send_to_char(ch, "They are too alert for that.\r\n");
        return;
    }

    if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim)) {
        send_to_char(ch, "But that's not playing nice!\n\r");
        return;
    }

	int ch_roll = (GET_DEX(ch) + GET_HITROLL(ch) + GET_REMORTS(ch)) * GET_LEVEL(ch) / rand_number(1,20);
	int victim_roll = (GET_PCN(victim) + GET_DEX(victim) + GET_REMORTS(victim)) * GET_LEVEL(victim);
	
	act("You begin sneaking up on $N, your weapon held at the ready!.", FALSE, ch, 0, victim, TO_CHAR);

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
			"You completely miss your assassination on $N!", 
			"$n tries to assassinate you, but you evade!", 
			"$n tries to assassinate $N, but misses!", 
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
		int damage = ((GET_STR(ch)+GET_DEX(ch)+GET_DAMROLL(ch)+get_dice(ch)) + (rand_number(1,GET_LEVEL(ch)) * 2) * (GET_LEVEL(ch) * 4) / 2);
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
					hit_attacker = "You drive your weapon into $N from behind!";
					hit_victim = "You feel the shocking pain as $n stabs you in the back!";
					hit_room = "$n drives $m weapon into $N from behind!";
				}
				else
				{
					hit_attacker = "You sink your weapon into $N, twisting it to make the wound worse!";
					hit_victim = "Agony courses through you as $n sinks $m weapon into your back and twists!";
					hit_room = "$n sinks $m weapon hilt-deep into $N, twisting it to make the wound worse!";
				}
			}
			
		if(epic_win > 96)
		{
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				GET_MAX_HIT(victim) + 1000, 
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
				"\x1B[41m\x1B[1;30m[FATALITY] You brutally assassinate $N!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m$n deals you a surprise FATAL blow!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m[FATALITY] $n brutally assassinates $N!\x1B[0;0m"
				);
		}
		else
		{
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				damage *2, 
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

ACMD(do_backstab)
{
    char buf[MAX_INPUT_LENGTH];
    struct char_data *victim;

    if (GET_SKILL_LEVEL(ch, SKILL_BACKSTAB) == 0) {
        send_to_char(ch, "You have no idea how to backstab anyone.\n\r");
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
        send_to_char(ch, "Backstab who?\r\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch, "How can you sneak up on yourself?\r\n");
        return;
    }

    if (!GET_EQ(ch, WEAR_WIELD) || IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "You need to wield a melee weapon to make it a success.\r\n");
        return;
    }

  // Ok, with a lack of pierce wepaons, let them backstab with anything -DH
  // Screw it, backstab only piercy
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_CLAW - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT &&
		GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT 
	) 
	{
		send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
		return;
	}

    if (FIGHTING(victim)) {
        send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
        return;
    }

    if ((MOB_FLAGGED(victim, MOB_AWARE) && AWAKE(victim)) || GET_HIT(victim) < GET_MAX_HIT(victim)) {
		send_to_char(ch, "They are too alert for that.\r\n");
        return;
    }

    if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim)) {
        send_to_char(ch, "But that's not playing nice!\n\r");
        return;
    }

	int ch_roll = (GET_DEX(ch) + GET_HITROLL(ch) + GET_REMORTS(ch)) * GET_LEVEL(ch) / rand_number(1,20);
	int victim_roll = (GET_PCN(victim) + GET_DEX(victim) + GET_REMORTS(victim)) * GET_LEVEL(victim);
	
	act("You begin sneaking up on $N, your weapon held at the ready!.", FALSE, ch, 0, victim, TO_CHAR);
	
    if (ch_roll < victim_roll) {
		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
		reset_combo(ch);
        exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 1, 
			1, 
			TYPE_PIERCE, 
			0,
			0,
			0,
			"You completely miss your backstab on $N!", 
			"$n tries to backstab you, but you evade!", 
			"$n tries to stab $N in the back, but misses!", 
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
		int damage = ((GET_STR(ch)+GET_DEX(ch)+GET_DAMROLL(ch)+get_dice(ch)) * GET_LEVEL(ch) / rand_number(1,10) /2);
		GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
		
		damage = damage + rand_number((damage /2), damage) - rand_number((GET_AC(victim) / 2), GET_AC(victim));
		
		char *hit_attacker, *hit_victim, *hit_room;
			
			if(damage < 1)
			{
				damage = 0;
				hit_attacker = "Your weapon clangs harmlessly off $N's armor!";
				hit_victim = "You feel the impact as $n's weapon hits, but your armor holds.";
				hit_room = "$n's weapon sparks off $N's armor, doing almost no damage!";
				reset_combo(ch);
			}
			else
			{
				if(rand_number(1,2) == 1)
				{
					hit_attacker = "You drive your weapon into $N from behind!";
					hit_victim = "You feel the shocking pain as $n stabs you in the back!";
					hit_room = "$n drives $m weapon into $N from behind!";
				}
				else
				{
					hit_attacker = "You sink your weapon into $N, twisting it to make the wound worse!";
					hit_victim = "Agony courses through you as $n sinks $m weapon into your back and twists!";
					hit_room = "$n sinks $m weapon hilt-deep into $N, twisting it to make the wound worse!";
				}
			}
			
		if(epic_win == 1)
		{
			reset_combo(ch);
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
				"\x1B[41m\x1B[1;30m[FATALITY] You brutally execute $N!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m$n deals you a surprise FATAL blow!\x1B[0;0m",
				"\x1B[41m\x1B[1;30m[FATALITY] $n brutally executes $N!\x1B[0;0m"
				);
		}
		else if (epic_win == 2)
		{
			reset_combo(ch);
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				damage, 
				TYPE_PIERCE, 
				PSIONIC_MEGA_COUNTER,
				35,
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
			update_combo(ch, victim, SKILL_BACKSTAB);
		}
    }

}

ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(ch, "%s", CONFIG_OK);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
	if (IN_ROOM(ch) == IN_ROOM(k->follower))
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
		if (!strn_cmp(message, "suicide", strlen(message))) {
			send_to_char(ch, "No.\r\n");
			return;
		}
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(ch, "%s", CONFIG_OK);
      else
	send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}

ACMD(do_flee)
{
  int i, attempt;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = rand_number(0, DIR_COUNT - 1); /* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char(ch, "You flee head over heels.\r\n");
      if (FIGHTING(ch)) 
        stop_fighting(ch); 
      if (was_fighting && ch == FIGHTING(was_fighting))
        stop_fighting(was_fighting); 
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}

ACMD(do_bash)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_BASH)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Bash who?\r\n");
      return;
    }
  }
  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  percent = rand_number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL_LEVEL(ch, SKILL_BASH);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH, DMG_NORMAL);
    GET_POS(ch) = POS_SITTING;
  } else {
    /*
     * If we bash a player and they wimp out, they will move to the previous
     * room before we set them sitting.  If we try to set the victim sitting
     * first to make sure they don't flee, then we can't bash them!  So now
     * we only set them sitting if they didn't flee. -gg 9/21/98
     */
    if (damage(ch, vict, 1, SKILL_BASH, DMG_NORMAL) > 0) {	/* -1 = dead, 0 = miss */
      WAIT_STATE(vict, PULSE_VIOLENCE);
      if (IN_ROOM(ch) == IN_ROOM(vict))
        GET_POS(vict) = POS_SITTING;
    }
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_rescue)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_RESCUE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Whom do you want to rescue?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "What about fleeing instead?\r\n");
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char(ch, "How can you rescue someone you are trying to kill?\r\n");
    return;
  }
  for (tmp_ch = world[IN_ROOM(ch)].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if ((FIGHTING(vict) != NULL) && (FIGHTING(ch) == FIGHTING(vict)) && (tmp_ch == NULL)) {
     tmp_ch = FIGHTING(vict);
     if (FIGHTING(tmp_ch) == ch) {
     send_to_char(ch, "You have already rescued %s from %s.\r\n", GET_NAME(vict), GET_NAME(FIGHTING(ch)));
     return;
  }
  }

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  percent = rand_number(1, 4);	/* 101% is a complete failure */
  prob = GET_SKILL_LEVEL(ch, SKILL_RESCUE);

  if (percent > prob) {
    send_to_char(ch, "You fail the rescue!\r\n");
    return;
  }
  send_to_char(ch, "You heroically rescue %s!\r\n", vict->player.name);
  act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
  act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (!IS_NPC(vict)) {
	int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 100;
	increment_skilluse(SKILL_RESCUE, 1, GET_IDNUM(ch), exp, ch);
  }
  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);

  WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}

ACMD(do_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent, prob;

  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_KICK)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Kick who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }
  /* 101% is a complete failure */
  percent = rand_number(1,3);
  prob = GET_SKILL_LEVEL(ch, SKILL_KICK);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK, DMG_NORMAL);
  } else
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK, DMG_NORMAL);

  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_slay)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;

    one_argument(argument, arg);

    if ((GET_LEVEL(ch) < LVL_IMMORT) || IS_NPC(ch)) {
        send_to_char(ch, "Invalid command, please type 'commands' for more info.\n\r");
        return;
    }
    else {

        if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
            send_to_char(ch, "They aren't here.\n\r");

        else if (ch == victim)
            send_to_char(ch, "You can't slay yourself. Try suicide instead.\n\r");
        else if (GET_LEVEL(ch) >= GET_LEVEL(victim)) {
            act("You brutally slay $N! You drain the life from $S body!", FALSE, ch, 0, victim, TO_CHAR);
            act("$N has slain you! You feel the life being drained from your body!", FALSE, victim, 0, ch, TO_CHAR);
            act("You watch in horror as the life slowly drains from $N!", FALSE, ch, 0, victim, TO_NOTVICT);
 
			raw_kill(victim, ch, FALSE);

		}
        else
            send_to_char(ch, "You shudder at the very thought!\r\n");
    }
}

ACMD(do_berserk)
{
    struct char_data *tmp_victim;
    struct char_data *temp;
	int count = 0;

    if (GET_SKILL_LEVEL(ch, SKILL_BERSERK) == 0) {
        send_to_char(ch, "You have no idea how to go berserk.\r\n");
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

    if (GET_MOVE(ch) < 100) {
        send_to_char(ch, "You're too exhausted!\r\n");
        return;
    }

    send_to_char(ch, "ARRRGGGHHH...YOU GO BERSERK!\n\r");

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

    act("$n goes BERSERK!!!", FALSE, ch, 0, 0, TO_ROOM);

    GET_DAMROLL(ch) *= 2;
    GET_HITROLL(ch) *= 4;

    for (tmp_victim = character_list; tmp_victim; tmp_victim = temp) {
        temp = tmp_victim->next;

        if (!tmp_victim && temp) continue;

        if (!tmp_victim && !temp) {
            GET_DAMROLL(ch) /= 2;
            GET_HITROLL(ch) /= 4;
            return;
        }

        if (GET_MOVE(ch) < 20) {
            send_to_char(ch, "You cant freak out anymore, you're too exhausted!\r\n");
            GET_DAMROLL(ch) /= 2;
            GET_HITROLL(ch) /= 4;
            return;
        }

        if (IN_ROOM(ch) != IN_ROOM(tmp_victim)) continue;
        if (ch == tmp_victim) continue;
        if (tmp_victim->master == ch) continue;
        if (ch->master == tmp_victim) continue;
        if (!CAN_SEE(ch, tmp_victim)) continue;
        if (!IS_NPC(ch) && !IS_NPC(tmp_victim) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK)) continue;
        if (group_member_check(ch, tmp_victim)) continue;
        //if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(tmp_victim)) continue;
        if (IS_DEAD(ch) || IS_DEAD(tmp_victim)) continue;

		int thaco = rand_number(1,30);

        act("$N hits you with their berserker rage!", FALSE, tmp_victim, 0, ch, TO_CHAR);
		hit(ch, tmp_victim, TYPE_UNDEFINED, thaco, 0);
        GET_MOVE(ch) -= 20;
		if (count == 0)
			update_combo(ch, tmp_victim, SKILL_BERSERK);
		count++;
    }
	int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 50;
	increment_skilluse(SKILL_BERSERK, 1, GET_IDNUM(ch), exp, ch);
    // this is to reduce the above HND bonus (damx2, hitx4)
    GET_DAMROLL(ch) /= 2;
    GET_HITROLL(ch) /= 4;
}

ACMD(do_ambush)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int percent;
    int prob;

    one_argument(argument, arg);

    if (GET_SKILL_LEVEL(ch, SKILL_AMBUSH) == 0) {
        send_to_char(ch, "You have no idea how to ambush your victims.\n\r");
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

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Ambush who?\r\n");
        return;
    }

    if (victim == ch) {
        send_to_char(ch, "How can you surprise yourself?\r\n");
        return;
    }

    if (!ch->equipment[WEAR_WIELD] || !IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "You need to have a gun for this to work.\r\n");
        return;
    }

    if (victim->char_specials.fighting == ch) {
        send_to_char(ch, "You can't ambush when you're already in battle\r\n");
        return;
    }

    if (MOB_FLAGGED(victim, MOB_AWARE)) {
        send_to_char(ch, "As you spring from the shadows, your attack is discovered by your victim!\r\n");
        act("$N attempts to ambush you, but you alertly turn the tables!", FALSE, ch, 0, victim, TO_VICT);
        act("$n tries to ambush $N who gives them a run for thier money.", FALSE, ch, 0, victim, TO_ROOM);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
		int thaco = rand_number(1,30);
        hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
    }

    percent = rand_number(1, 5);
    prob = GET_SKILL_LEVEL(ch, SKILL_AMBUSH) - GET_PCN(victim) + GET_DEX(ch) + GET_CON(ch);

   if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim)) {
     send_to_char(ch, "But that's not playing nice!\n\r");
     return;
   }


    if (AWAKE(victim) && (percent > prob))
        damage(ch, victim, 0, SKILL_AMBUSH, DMG_NORMAL);
    else {
        send_to_char(ch, "As you spring from the shadows, you ambush your unsuspecting victim!\r\n");
        act("From the shadows, $n appears behind you!", FALSE, ch, 0, victim, TO_VICT);
        act("From the shadows, $n appears behind $N and attacks!", FALSE, ch, 0, victim, TO_ROOM);
		int thaco = rand_number(1,30);
        hit(ch, victim, SKILL_AMBUSH, thaco, 0);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_AMBUSH, 1, GET_IDNUM(ch), exp, ch);
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}

ACMD(do_flank)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim = NULL;
    struct char_data *tank = NULL;
    int percent;
    int prob;

    one_argument(argument, arg);

    if (GET_SKILL_LEVEL(ch, SKILL_FLANK) == 0) {
        send_to_char(ch, "You have no idea how to flank.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch)) victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Flank who?\r\n");
            return;
        }
    }

    if (!ch->equipment[WEAR_WIELD]) {
        send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
        return;
    }

    if (ch->char_specials.fighting != victim) {
        send_to_char(ch, "But you aren't fighting them yet!\n\r");
        return;
    }

    if (FIGHTING(victim)) tank = victim->char_specials.fighting;

    if (ch == tank) {
        send_to_char(ch, "How do you expect to flank?\r\n");
        return;
    }

    percent = rand_number(1, 5);
    prob = GET_SKILL_LEVEL(ch, SKILL_FLANK) + GET_DEX(ch) - GET_PCN(victim);

    if (percent > prob) {
        damage(ch, victim, 0, TYPE_UNDEFINED, DMG_NORMAL);
		reset_combo(ch);
	}
    else {
        send_to_char(ch, "You flank the enemy!\r\n");
        act("$N attacks your flank!", 0, victim, 0, ch, TO_CHAR);
        act("$n attacks $N's flank!", FALSE, ch, 0, victim, TO_NOTVICT);
		int thaco = rand_number(1,30);
        hit(ch, victim, SKILL_FLANK, thaco, 0);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 50;
		increment_skilluse(SKILL_FLANK, 1, GET_IDNUM(ch), exp, ch);
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}



ACMD(do_charge)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int percent;
    int prob;

    if (!IS_NPC(ch) && GET_SKILL_LEVEL(ch, SKILL_CHARGE) == 0) {
        send_to_char(ch, "You have no idea how to charge.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Charge who?\n\r");
            return;
        }
    }

    if (victim == ch) {
        send_to_char(ch, "It'd be really futile, don't you think?\n\r");
        return;
    }

    if (!ch->equipment[WEAR_WIELD]) {
        send_to_char(ch, "What do you plan on charging with? A credit card?\n\r");
        return;
    }

    if (ch->char_specials.fighting != victim) {
        send_to_char(ch, "But you aren't fighting them yet!\n\r");
        return;
    }

    percent = rand_number(1, 10);
    prob = GET_SKILL_LEVEL(ch, SKILL_CHARGE) + GET_DEX(ch) + GET_PCN(ch) - GET_PCN(victim);

    if (percent > prob) {
        damage(ch, victim, 0, TYPE_UNDEFINED, DMG_NORMAL);
        send_to_char(ch, "You begin to charge but trip over your own feet!\r\n");
        act("$N charges but trips over $S own feet!", 0, victim, 0, ch, TO_CHAR);
        act("$n trips over $s own feet while charging at $N!", FALSE, ch, 0, victim, TO_NOTVICT);
		reset_combo(ch);
    }
    else {
        send_to_char(ch, "CHAAAAAAAARGE!\r\n");
        act("$N charges at you!", 0, victim, 0, ch, TO_CHAR);
        act("$n screams a war-cry and charges at $N.", FALSE, ch, 0, victim, TO_NOTVICT);
		int thaco = rand_number(1,30);
        hit(ch, victim, SKILL_CHARGE, thaco, 0);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_CHARGE, 1, GET_IDNUM(ch), exp, ch);
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}

ACMD(do_maniac)
{
    if (GET_SKILL_LEVEL(ch, SKILL_MANIAC) == 0) {
        send_to_char(ch, "You don't know this skill!\n\r");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_MANIAC)) {
        send_to_char(ch, "You calm down a little!\r\n");
           REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_MANIAC);
        return;
    }
    if (GET_MOVE(ch) <= 49) {
        send_to_char(ch, "You don't have the energy to be so crazy!\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
	}

    if (!AFF_FLAGGED(ch, AFF_MANIAC)) {
        send_to_char(ch, "You feel just a little bit crazier!\r\n");
        SET_BIT_AR(AFF_FLAGS(ch), AFF_MANIAC);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 20;
		increment_skilluse(SKILL_MANIAC, 1, GET_IDNUM(ch), exp, ch);
        GET_MOVE(ch) -= 50;
        return;
    }

}

ACMD(do_slash)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int percent;
    int prob;

	if (GET_SKILL_LEVEL(ch, SKILL_BIOSLASH) == 0) {
		send_to_char(ch, "You can't slash without bionic blades!\r\n");
		return;
	}

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Slash who?\n\r");
            return;
        }
    }

    if (victim == ch) {
        send_to_char(ch, "You turn your bionic handblades to yourself and think about ending it all....nah...nevermind!\n\r");
        return;
    }

    if (ch->char_specials.fighting != victim) {
        send_to_char(ch, "But you aren't fighting them yet!\n\r");
        return;
    }

    percent = rand_number(1, 10);
    prob = GET_SKILL_LEVEL(ch, SKILL_CHARGE) + GET_DEX(ch) + GET_PCN(ch) - GET_PCN(victim);

    if (percent > prob) {
        damage(ch, victim, 0, TYPE_UNDEFINED, DMG_NORMAL);
        send_to_char(ch, "You jump at your victim with vibro-blades aimed at the jugular...too bad they moved!\r\n");
        act("$N jumps and slashes at your neck with bionic vibro-blades good thing $S missed!", 0, victim, 0, ch, TO_CHAR);
        act("$n jumps and slashes at $N's neck with $s bionic vibro-blades! Wow, good thing $s missed!", FALSE, ch, 0, victim, TO_NOTVICT);
    }
    else {
        send_to_char(ch, "You slash wildly with your bionic vibro-blades! Blood gushes from the lacerations!\r\n");
        act("$N slashes wildly at you with $S vibro-blades! Blood gushes from the lacerations!", 0, victim, 0, ch, TO_CHAR);
        act("$n slashes wildly at $N with $s bionic vibro-blades! Blood gushes from the lacerations!", FALSE, ch, 0, victim, TO_NOTVICT);
		int thaco = rand_number(1,30);
        hit(ch, victim, SKILL_SLASH, thaco, 0);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_SLASH, 1, GET_IDNUM(ch), exp, ch);
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}


ACMD(do_dodge)
{
    if (GET_SKILL_LEVEL(ch, SKILL_DODGE) == 0) {
        send_to_char(ch, "You have no idea how to dodge anything.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }
    if (AFF_FLAGGED(ch, AFF_DODGE)) {
        send_to_char(ch, "@WYou already feel ready to dodge enemy attacks!@n\r\n");
        return;
    }


    if (rand_number(0, 14) <= GET_DEX(ch)) {
        send_to_char(ch, "@WYou focus yourself and feel more able to dodge enemies attacks!@n\r\n");
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
        SET_BIT_AR(AFF_FLAGS(ch), AFF_DODGE);
    } else {
        send_to_char(ch, "@RYou just don't feel much like dodging!r\n"); }

}

ACMD(do_protect)
{
    struct char_data *victim;
    struct char_data *old_victim;
    char arg[MAX_INPUT_LENGTH];

    if (!IS_NPC(ch) && !GET_SKILL_LEVEL(ch, SKILL_PROTECT)) {
        send_to_char(ch, "You have no idea how to protect anyone.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (!*argument) {
        send_to_char(ch, "But who do you wish to protect?\n\r");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "Perhaps they should be in the same room as you!\n\r");
        return;
    }

    if (victim == ch) {
        send_to_char(ch, "Ok, you will no longer protect anyone.\n\r");

        if (ch->char_specials.protecting) {
            send_to_char(ch->char_specials.protecting, "%s is no longer protecting you.\n\r", GET_NAME(ch));
            old_victim = ch->char_specials.protecting;
            old_victim->char_specials.protector = NULL;
        }

        ch->char_specials.protecting = NULL;
        return;
    }

    if (victim->char_specials.protector != NULL) {
        send_to_char(ch, "They seem to already be protected.\n\r");
        return;
    }

    if (victim->char_specials.protecting != NULL) {
        send_to_char(ch, "They seem to already be protecting someone else!\n\r");
        return;
    }

    if (ch->char_specials.protector == victim) {
        send_to_char(ch, "You cannot circularly protect!\n\r");
        return;
    }

    if (ch->char_specials.protecting) {
        old_victim = ch->char_specials.protecting;
        /* You can only protect one player at a time */
        act("You will no longer protect $N.", FALSE, ch, 0, old_victim, TO_CHAR);
        act("$N no longer protects you.", FALSE, old_victim, 0, ch, TO_CHAR);
        old_victim->char_specials.protector = NULL;
    }

    ch->char_specials.protecting = victim;
    victim->char_specials.protector = ch;

    act("You will now protect $N with your life.", FALSE, ch, 0, victim, TO_CHAR);
    act("$N is now protecting you.", FALSE, victim, 0, ch, TO_CHAR);
    return;

}


ACMD(do_coverfire)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int d_roll;

    if (GET_SKILL_LEVEL(ch, SKILL_COVER_FIRE) == 0) {
        send_to_char(ch, "You have no idea how to coverfire.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {

        if (FIGHTING(ch)) victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Coverfire who?\n\r");
            return;
        }

    }

    if (!ch->equipment[WEAR_WIELD] || !IS_GUN(ch->equipment[WEAR_WIELD])) {
        send_to_char(ch, "You need to have a gun for this to work.\r\n");
        return;
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE *2;
    d_roll = rand_number(0, 4);

    if (d_roll >= 2) {

        SET_BIT_AR(AFF_FLAGS(victim), AFF_COVER_FIRE);

        act("$n lays down a supressive cover fire and pins $N!", FALSE, ch, 0, victim, TO_ROOM);
        act("You begin to strafe $N and send $M to the ground.", FALSE, ch, 0, victim, TO_CHAR);
        act("$n starts to strafe in your direction.  DUCK!", FALSE, ch, 0, victim, TO_VICT);
		
		update_combo(ch,victim,SKILL_COVER_FIRE);
    }
	else {
		send_to_char(ch, "You failed to cover fire!\r\n");
		reset_combo(ch);
	}
}


ACMD(do_stun)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    struct affected_type af;
    int d_roll;
    int p_roll;

    if (GET_SKILL_LEVEL(ch, SKILL_STUN) == 0) {
        send_to_char(ch, "You have no idea how to stun your victims.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {

        if (FIGHTING(ch)) victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Stun who?\n\r");
            return;
        }

    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}

    if (AFF_FLAGGED(victim, AFF_STUN)) {
        send_to_char(ch, "They are already stunned!\r\n");
        return;
    }


    if ((MOB_FLAGGED(victim, MOB_AWARE) && AWAKE(victim))) {
                send_to_char(ch, "They are too alert for that.\r\n");
        return;
    }
    if (victim == ch) {
        send_to_char(ch, "Aren't we funny today?\n\r");
        return;
    }

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK) && !(IS_HIGHLANDER(ch) && IS_HIGHLANDER(victim))) {
        send_to_char(ch, "You cannot bring yourself to violence here.\n\r");
        return;
    }

    d_roll = (GET_LEVEL(victim) + GET_PCN(victim)) * rand_number(8, 25);
    p_roll = (GET_LEVEL(ch) + GET_REMORTS(ch)) * GET_STR(ch);

    if (p_roll >= d_roll) {
	int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 20;
	increment_skilluse(SKILL_STUN, 1, GET_IDNUM(ch), exp, ch);
	act("@R$n stuns $N with a well placed blow!@n", FALSE, ch, 0, victim, TO_NOTVICT);
        send_to_char(ch, "@RYou've stunned %s with a well placed blow!@n\r\n", GET_NAME(victim));
        send_to_char(victim, "@R%s rushes you, and before you see what's coming, everything fades to \r\n@Wwhite...white...white...sooooo many stars!@n\n\r", GET_NAME(ch));
        send_to_char(victim, "@RYOU ARE COMPLETELY STUNNED!@n\n\r");

		new_affect(&af);
        af.type = PSIONIC_PARALYZE;
        af.duration = .3;
        SET_BIT_AR(af.bitvector, AFF_STUN);
        affect_to_char(victim, &af);
		update_combo(ch, victim, SKILL_STUN);
    }
    else {
        send_to_char(ch, "You attempt to stun %s, but the blow was shrugged off easily!\n\r", GET_NAME(victim));
        act("$n tries to stun $N, but misses.", TRUE, ch, 0, victim, TO_ROOM);
        send_to_char(victim,"%s tries to stun you, but fails.", GET_NAME(ch));
		int thaco = rand_number(1,30);
        hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
		reset_combo(ch);
    }
}

ACMD(do_bodyblock)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *tank = NULL;
    struct char_data *victim = NULL;
    int percent = 0;
    int skill_level = 0;

    if (GET_SKILL_LEVEL(ch, SKILL_BODY_BLOCK) == 0) {
        send_to_char(ch, "You have no idea how to bodyblock.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {

        if (FIGHTING(ch)) victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Bodyblock who?\n\r");
            return;
        }

    }

    if (!FIGHTING(ch)) {
        send_to_char(ch, "But you aren't even fighting!\r\n");
        return;
    }

    if (FIGHTING(victim)) tank = victim->char_specials.fighting;

    if (ch != tank) {
        send_to_char(ch, "How do you expect to bodyblock?\r\n");
        return;
    }

    skill_level = GET_SKILL_LEVEL(ch, SKILL_BODY_BLOCK);

    percent = rand_number(0, 100);

    if (skill_level == 5 ||
      (skill_level == 4 && percent <= 80) ||
      (skill_level == 3 && percent <= 60) ||
      (skill_level == 2 && percent <= 40) ||
      (skill_level == 1 && percent <= 20)) {

        SET_BIT_AR(AFF_FLAGS(ch), AFF_BODY_BLOCK);

        /* messages to let everyone know the body block went off */
        send_to_char(ch, "Good going! Use your body as a shield!\n\r");
        act("$n valiantly throws his body into the path of $N.", TRUE, ch, 0, victim, TO_ROOM);
        act("$n uses $s body as a blocking device and throws $mself into you!.", TRUE, ch, 0, victim, TO_VICT);
		update_combo(ch,victim,SKILL_BODY_BLOCK);
    }
    else {
        send_to_char(ch, "You try but you stumble over your own two feet!\r\n");
        act("$n lunges for $N but falls over $s own feet!", TRUE, ch, 0, victim, TO_ROOM);
        act("$n lunges for you but falls over $s own big feet!", TRUE, ch, 0, victim, TO_VICT);
		reset_combo(ch);
    }

    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}

ACMD(do_blast)
{
	char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;

	if (GET_SKILL_LEVEL(ch, SKILL_BIOBLAST) == 0) {
		send_to_char(ch, "You can't blast without a blaster!\r\n");
		return;
	}

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Blast who?\n\r");
            return;
        }
    }
    if (victim == ch) {
        send_to_char(ch, "You aim at your head and fire!  Lucky for you, you're a terrible shot.\n\r");
        return;
    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK) && !(GET_CLASS(ch) == CLASS_HIGHLANDER && GET_CLASS(victim) == CLASS_HIGHLANDER)) {
        send_to_char(ch, "You cannot bring yourself to violence here.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(victim))
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == victim)) {
        act("$N is your master, you don't want to anger $M.", FALSE, ch, 0,
        victim, 0);
        return;
    }

	act("You extend your bionic blaster and concentrate on priming it.", TRUE, ch, 0, victim, TO_CHAR);
    act("A slim barrel extends from $n's wrist and begins to glow...", TRUE, ch, 0, victim, TO_ROOM);

	int hp_loss = GET_HIT(ch) * .25;
	GET_HIT(ch) -= hp_loss;
	
	int dam = (GET_PCN(ch) + GET_DEX(ch) + GET_CON(ch))/ 3 * hp_loss - GET_AC(victim);
	int ch_hit = (GET_DEX(ch) + GET_PCN(ch) + GET_LEVEL(ch)) * GET_REMORTS(ch);
	int vict_hit = (GET_DEX(victim) + GET_PCN(victim) + GET_LEVEL(victim)) * GET_REMORTS(victim);
	ch_hit += (ch_hit * rand_number(1,10));
    
	if (ch_hit >= vict_hit) 
	{
		char *af_room, *af_vict, *af_char;
		int psi_num = 0;
		int psi_level = 0;
		bool psi_ack = TRUE;

		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_BLAST, 1, GET_IDNUM(ch), exp, ch);	

			af_room = NULL;
			af_char = NULL;
			af_vict = NULL;
		
	
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			dam, 
			TYPE_BLAST, 
			psi_num,
			psi_level,
			psi_ack,
			"$N is thrown backward as you discharge your blaster in a shower of plasma and biomatter!", 
			"A ball of plasma leaps from $n's wrist and blasts you back!", 
			"$N is thrown backward as a ball of plasma from $n's blaster!", 
			af_char, 
			af_vict, 
			af_room,
			NULL, 
			NULL,
			NULL);
		update_combo(ch,victim,SKILL_BLAST);
    }
    else 
	{
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			0, 
			0, 
			0,
			0,
			FALSE,
			"$N steps out of the way at the very last second!", 
			"You barely avoid $n's blast as you step to one side!", 
			"$n's plasma blast misses $N by inches!", 
			NULL, 
			NULL, 
			NULL,
			NULL,
			NULL,
			NULL);
		reset_combo(ch);
    }
}

void make_skull(struct char_data *ch, struct char_data *victim)
{

    struct obj_data *skull;

    if (IS_NPC(ch) || IS_NPC(victim) || GET_CLASS(ch) != CLASS_PREDATOR)
        return;

    /* customize skull for victim */
    if ((skull = read_object(SKULL_VNUM, VIRTUAL))) {

        /* Set ID of victim (for plaque) */
        if (GET_IDNUM(victim) > 0)
            GET_OBJ_VAL(skull, 0) = GET_IDNUM(victim);
        else
            GET_OBJ_VAL(skull, 0) = 0;

        /* Set class of victim */
        GET_OBJ_VAL(skull, 1) = GET_CLASS(victim);

        obj_to_char(skull, ch);
    }
    else {
        send_to_char(ch, "Error creating skull. Inform an Imp please.\n\r");
        perror("Error creating skull trophy");
    }
}

// todo: behead seems to borrow a lot of code form die.  cant we just make this smaller?
ACMD(do_behead)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int essences_lost;
    int pos;
    struct obj_data *head;
    int units = 0;
    int exp;

    one_argument(argument, arg);

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

//    if (!*arg) {
  //      send_to_char(ch, "Behead whom?\n\r");
    //    return;
   // }


  if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      victim = FIGHTING(ch);
    } else {
      send_to_char(ch, "Behead whom?\r\n");
      return;
    }
   }

//    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  //      send_to_char(ch, "You can only behead those in front of you.\n\r");
    //    return;
    //}

    if (!IS_HIGHLANDER(victim)) {
        send_to_char(ch, "Psychotic aren't you? You can only behead highlanders!\n\r");
        return;
    }

    if (!IS_NPC(victim) && GET_LEVEL(victim) >= LVL_IMMORT) {
        send_to_char(ch, "Try beheading someone you can actually damage!\n\r");
        return;
    }

    if (GET_POS(victim) > POS_DEAD) {
        send_to_char(ch, "Your victim is not in a position for that!\n\r");
        return;
    }

    if (!ch->equipment[WEAR_WIELD] || !OBJ_FLAGGED(ch->equipment[WEAR_WIELD], ITEM_SEVERING)) {
        send_to_char(ch, "What are you going to behead with, your wit?\n\r");
        return;
    }

 if ((GET_MOB_VNUM(victim) > 10800) && (GET_MOB_VNUM(victim) < 10900)) {
        load_prize(ch);
}
//    if (!IS_NPC(victim) && !HAS_DOSSIER(ch)) {
//        CREATE(ch->player.dossier, struct char_dossier, 1);
//        GET_PK_DEATHS(ch) = 0;
//        GET_PK_KILLS(ch) = 0;
//        GET_PK_FORCED_RETREATS(ch) = 0;
//        GET_PK_RETREATS(ch) = 0;
//    }

//    if (!IS_NPC(victim) && !HAS_DOSSIER(victim)) {
//        CREATE(victim->player.dossier, struct char_dossier, 1);
//        GET_PK_DEATHS(victim) = 0;
//        GET_PK_KILLS(victim) = 0;
//        GET_PK_FORCED_RETREATS(victim) = 0;
//        GET_PK_RETREATS(victim) = 0;
//    }

    if (victim->player.essences >= 200) essences_lost = 4;
    else if (victim->player.essences >= 100) essences_lost = 2;
    else essences_lost = 1;

    if (essences_lost > victim->player.essences || IS_NPC(victim))
        essences_lost = victim->player.essences;

    victim->player.essences -= essences_lost;

    //give the victor the head of the loser
    /* I moved this up here, because trying to get the name of a mob that's been killed is "bad" -DH */
    if ((head = read_object(SEVERED_HEAD_VNUM, VIRTUAL))) {

        char buf[MAX_STRING_LENGTH];

        //customize head for victim
        sprintf(buf, "%s head", GET_NAME(victim));
        head->name = strdup(buf);

        sprintf(buf, "The severed head of %s", GET_NAME(victim));
        head->short_description = strdup(buf);

        sprintf(buf, "The severed head of %s lies here.", GET_NAME(victim));
        head->description = strdup(buf);

        obj_to_char(head, ch);
    }
    else {
        send_to_char(ch, "Error creating head. Inform an Imp please.\n\r");
        perror("Error creating severed head");
    }

    // distribute exp
    if (IS_NPC(victim) || victim->desc) {

        /* call group gain for exp for group killing */
        if (AFF_FLAGGED(ch, AFF_GROUP) && (ch->followers || ch->master))
            group_gain(ch, victim);
            /* Exp gain section for solo kill */
        else {
            /* Calculate level-difference bonus for experience
            Ideal solo level is mob + 5 = pc level, so a level 25 killing a
            level 20 mob gets no bonus, but a level 25 killing a level 25 mob
            gets a 5% bonus for every level below the ideal.
            */
            exp =((float)GET_EXP(victim) * (1 +(0.05*(float)(GET_LEVEL(victim)-GET_LEVEL(ch) - 5))));

            if (ch->player.num_remorts > 0)
                exp *= 1.0/(float)(ch->player.num_remorts + 1);
            if (IS_SET(ch->player_specials->saved.age_effects, AGE_BATCH_2))
                exp *= .75;

            if (IS_NPC(ch))
                exp = 1;

            if (!IS_NPC(victim))
                exp = 10000;

            exp = MAX(exp, 1);

            if (ch != victim)
                gain_exp(ch, exp, TRUE);
        }
    }

    if (!IS_NPC(victim)) {

        mudlog(BRF, 0, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);

        if (MOB_FLAGGED(ch, MOB_MEMORY)) forget(ch, victim);

        if (HUNTING(ch) == victim)
            HUNTING(ch) = NULL;

        units = GET_UNITS(victim);
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
            do_get(ch, "all xxcorpsexx", 0, 0); }
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSCAVENGER)) {
            do_scavenger(ch, "xxcorpsexx", 0, 0); }
            if (PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
                char buf[MAX_STRING_LENGTH];
                sprintf(buf, "%d", units);
                do_split(ch, buf, 0, 0);
            }
        }


    if (!IS_NPC(victim) && victim->player.essences <= 0) {
        send_to_all("\n\r\n\rTHERE CAN BE ONLY ONE!\n\r\n\r");
        send_to_char(victim, "The darkness envelops you as your head comes off of your neck...permanently!\n\rMaybe you'll have better luck in your next existence.\n\r");

        /* Remove all psionics */
        if (victim->affected) {
            while (victim->affected)
                affect_remove(victim, victim->affected);
        }

        /* Remove all drugs */
        if (victim->drugs_used) {
            while (victim->drugs_used)
                drug_remove(victim, victim->drugs_used);
        }

        /* Remove eq */
        for (pos = 0; pos < NUM_WEARS; pos++) {
            if (victim->equipment[pos])
                obj_to_char(unequip_char(victim, pos, FALSE), victim);
        }

        /* Remove Bionics effects */
        bionics_unaffect(victim);
        //new_bionics_unaffect(victim, TRUE);

        /* Reset points */
        victim->points.max_hit = 0;
        victim->points.max_psi = 0;
        victim->points.max_move = 0;

        victim->points.hit = victim->points.max_hit;
        victim->points.psi = victim->points.max_psi;
        victim->points.move = victim->points.max_move;

        GET_LEVEL(victim) = 1;
        GET_EXP(victim) = 1;
        GET_UNITS(victim) = 1;

        // erm, why do we need to reset their stats?  -Lump
        //  make_highlander(victim);
        victim->player.essences = 100;

        init_char(victim);
        post_init_char(victim);

        save_char(ch);
        save_char(victim);

        extract_char(victim);

    }
    else {
        //just a normal highlander death
        send_to_all("\n\rWild bolts of lightning flash on the horizon.\n\r");
        send_to_char(victim, "You feel your head come away from your body.\n\r");
        send_to_char(victim, "You can feel your essence draining away.\n\r");
        die(victim, ch, FALSE);
    }

    //should be an unnecessary check, but I'll leave it just in case - ews
    //good check, must be left in because now morts can wield severing items -DH
    if (GET_CLASS(ch) == CLASS_HIGHLANDER) {
        ch->player.essences += essences_lost;

        send_to_char(ch, "A blue lightning-like field starts to appear out of nowhere. It criss-crosses\r\n"
        "your body and you drop to your knees as you feel your power leaving you.\r\n");

        if (!IS_NPC(ch))
            GET_POS(ch) = POS_RESTING;

        act("A blue lightning like field starts to appear out of nowhere. Tendrils of \r\n"
        "electricity dance across $n as $e undergoes....the quickening!", FALSE, ch, 0, 0, TO_ROOM);

    }

    save_char(ch);
    save_char(victim);
}
ACMD(do_drain) // Drain skill for callers - Gahan
{
	struct char_data *victim;
	char arg[MAX_INPUT_LENGTH];

	if (GET_SKILL_LEVEL(ch, SKILL_DRAIN) == 0) {
		send_to_char(ch, "You have no idea how to drain.\r\n");
		return;
	}

    one_argument(argument, arg);

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (!*arg) {
        send_to_char(ch, "Drain which animal?\n\r");
        return;
    }

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        send_to_char(ch, "You don't see you're victim here.\n\r");
        return;
    }

	if (!IS_NPC(victim)) {
		send_to_char(ch, "Nah, you probably shouldn't try that.\r\n");
		return;
	}

	else {

		if (!MOB_FLAGGED(victim, MOB_CHARM)) {
			send_to_char(ch, "That victim can not be drained.\r\n");
			return;
		}

		int chance = (GET_LEVEL(ch) + GET_PCN(ch) + GET_CHA(ch) + rand_number(1,10));
		int victsave = (GET_LEVEL(victim) + GET_PCN(victim) + rand_number(1,25));

		if (chance < victsave) {
			send_to_char(ch, "Your victim resists your attempt to drain its energy!\r\n");
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			
			if (!FIGHTING(ch)) {
				send_to_char(ch, "%s jumps to attack you!\r\n", victim->player.short_descr);
				set_fighting(victim, ch);
				set_fighting(ch, victim);
			}
			return;
		}
		
		else {
			int hitgain = GET_HIT(victim) / 10;
			int psigain = GET_PSI(victim) / 10;
			int movegain = GET_MOVE(victim) / 10;

			send_to_char(ch, "Your hands glow green with the force of nature on your side, as your victim collapses to the ground.\r\n");
			act("$n's hands glow with the force nature on their side as $N collapses to the ground!", TRUE, ch, 0, victim, TO_ROOM);
			int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
			increment_skilluse(SKILL_CIRCLE, 1, GET_IDNUM(ch), exp, ch);
			GET_HIT(ch) += hitgain;
			GET_PSI(ch) += psigain;
			GET_MOVE(ch) += movegain;

			if (GET_HIT(ch) > GET_MAX_HIT(ch))
				GET_HIT(ch) = GET_MAX_HIT(ch);
			if (GET_PSI(ch) > GET_MAX_PSI(ch))
				GET_PSI(ch) = GET_MAX_PSI(ch);
			if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
				GET_MOVE(ch) = GET_MAX_MOVE(ch);

			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

			extract_char(victim);
		}
	}
}


ACMD(do_retreat) // Retreat for mercenaries - Gahan
{
    int i;
    int attempt;
    struct follow_type *f;
    sh_int target;
    struct follow_type *next_fol;
    
	if (GET_SKILL_LEVEL(ch, SKILL_RETREAT) == 0) {
		send_to_char(ch, "You have no idea how to retreat from battle.\r\n");
		return;
	}
	if (GET_POS(ch) < POS_FIGHTING) {
        send_to_char(ch, "You need to be in a position to retreat to do that!\r\n");
        return;
    }

	if (!FIGHTING(ch)) {
		send_to_char(ch, "You don't need to do that, you're not in combat.\r\n");
		return;
	}

    for (i = 0; i < 6; i++) {

        attempt = rand_number(0, NUM_OF_DIRS - 1);    /* Select a random direction */
        if (CAN_GO(ch, attempt) && !ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {

            act("@Y$n shouts to his allies to fall back begins to retreat!@n", TRUE, ch, 0, 0, TO_ROOM);

            if (do_simple_move(ch, attempt, TRUE) && GET_MOVE(ch) > 20) {
                send_to_char(ch, "\r\n@YYou successfully retreat from combat!@n\r\n");
				int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 10;
				increment_skilluse(SKILL_RETREAT, 1, GET_IDNUM(ch), exp, ch);
				GET_MOVE(ch) -= 20;

                if (FIGHTING(ch)) {
                    if (ch == FIGHTING(FIGHTING(ch)))
                        stop_fighting(FIGHTING(FIGHTING(ch)));
                        stop_fighting(FIGHTING(ch));
                }
				
				target = IN_ROOM(ch);
				
				for (f = ch->followers; f; f = next_fol) {

			        next_fol = f->next;
        
					if (AFF_FLAGGED(f->follower, AFF_GROUP)) {

						int dieroll = rand_number(1,100);
						if (dieroll > 10) {
							char_from_room(f->follower);
							char_to_room(f->follower, target);
							send_to_char(f->follower, "@YYou join %s with the retreat, and get out combat safely!@n\r\n", GET_NAME(ch));
							send_to_char(ch, "%s joins you with the retreat, and arrives out of combat safely!\r\n", GET_NAME(f->follower));
							int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 10;
							increment_skilluse(SKILL_RETREAT, 1, GET_IDNUM(ch), exp, ch);
							if (FIGHTING(f->follower))
								stop_fighting(FIGHTING(f->follower));
						}
						else {
							send_to_char(f->follower, "@RYou are left behind in %s's retreat!@n\r\n", GET_NAME(ch));
							send_to_char(ch, "@R%s has been left behind!!\r\n", GET_NAME(f->follower));
						}
					}
				}
				return;
            }
        }
        else {
			act("$n fails to retreat from combat!", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char(ch, "Your enemy has blocked your escape route!\r\n");
			return;
		}
    }
}

ACMD(do_circle) // Circle for Stalkers - Gahan
{
    struct char_data *victim;

    if (GET_SKILL_LEVEL(ch, SKILL_CIRCLE) == 0) {
        send_to_char(ch, "You have no idea how to circle your opponents.\n\r");
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

	if (!FIGHTING(ch)) {
		send_to_char(ch, "This ability has to be used in combat.\r\n");
		return;
	}
	if (GET_MOVE(ch) <= 150) {
		send_to_char(ch, "You are too tired to use this ability.\r\n");
		return;
	}

	else {

		if (!(victim = FIGHTING(ch))) {
			send_to_char(ch, "circle who?\r\n");
			return;
		}

		if (victim == ch) {
			send_to_char(ch, "How can you sneak up on yourself?\r\n");
			return;
		}

		if (!GET_EQ(ch, WEAR_WIELD)) {
			send_to_char(ch, "Only piercing, claw, stab, or sting weapons can be used for circling.\r\n");
			return;
		}


		if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT &&
			GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_CLAW - TYPE_HIT &&
			GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STAB - TYPE_HIT &&
			GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_STING - TYPE_HIT 
		)	 
		{
			send_to_char(ch, "Only piercing, claw, stab, or sting weapons can be used for circling.\r\n");
			return;
		}

		int ch_roll = (GET_DEX(ch) + GET_PCN(ch) + GET_HITROLL(ch) + GET_REMORTS(ch)) * GET_LEVEL(ch) / rand_number(1,10);
		int victim_roll = (GET_PCN(victim) + GET_DEX(victim) + GET_REMORTS(victim)) * GET_LEVEL(victim);
		GET_CIRCLETICKS(ch) = 0;
	
		act("You begin to circle behind $N, waiting for your most opportune time to strike.", FALSE, ch, 0, victim, TO_CHAR);
		GET_CIRCLETICKS(ch) = 31;
	    if (ch_roll < victim_roll) {
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			reset_combo(ch);
			exec_delay_hit(
				ch,
				GET_PC_NAME(victim),
				PULSE_VIOLENCE * 1, 
				1, 
				TYPE_PIERCE, 
				0,
				0,
				0,
				"You fail your attempt to circle $N, they see you coming easily!", 
				"$n tries to circle you, but you see them the entire time.", 
				"$n tries to circle $N, but misses their attempt to catch them off guard!", 
				NULL, 
				NULL, 
				NULL,
				NULL,
				NULL, 
				NULL);
		}
	    else {
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			int freakroll = rand_number(1,100);
				if (freakroll < 50) {
					GET_CIRCLETICKS(ch) = 20;
					GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
					GET_MOVE(ch) -= 50;
					int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
					increment_skilluse(SKILL_CIRCLE, 1, GET_IDNUM(ch), exp, ch);
				}
				else if (freakroll > 50 && freakroll <= 80) {
					GET_CIRCLETICKS(ch) = 10;
					GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
					GET_MOVE(ch) -= 100;
					int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 60;
					increment_skilluse(SKILL_CIRCLE, 1, GET_IDNUM(ch), exp, ch);
				}
				else { 
					GET_CIRCLETICKS(ch) = 1;
					GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
					int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 120;
					increment_skilluse(SKILL_CIRCLE, 1, GET_IDNUM(ch), exp, ch);
					GET_MOVE(ch) -= 150;
				}
			update_combo(ch, victim, SKILL_CIRCLE);
			}
		}
}

ACMD(do_electrify) // Electrify skill for borgs - Gahan
{
	char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;

	if (GET_SKILL_LEVEL(ch, SKILL_ELECTRIFY) == 0) {
		send_to_char(ch, "You have no idea how to use your power core.\r\n");
		return;
	}
    
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Electrify who?\n\r");
            return;
        }
    }
    if (victim == ch) {
        send_to_char(ch, "I know porn for borgs isn't too common, but seriously? Go find a socket.\n\r");
        return;
    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}
	if (GET_PSI(ch) < 30) {
		send_to_char(ch, "You don't quite meet the power requirements, perhaps you should regain some Psi first.\r\n");
		return;
	}
	if (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_HOLD)) {
		send_to_char(ch, "You need a free hand to perform this skill.\r\n");
		return;
	}

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK) && !(GET_CLASS(ch) == CLASS_HIGHLANDER && GET_CLASS(victim) == CLASS_HIGHLANDER)) {
        send_to_char(ch, "You cannot bring yourself to violence here.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(victim))
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == victim)) {
        act("$N is your master, you don't want to anger $M.", FALSE, ch, 0,
        victim, 0);
        return;
    }

	act("@WYou initiate your power core, and a @Bblue electrical aura@W begins to form around your h@Ba@Wn@Bd@Ws.@n", TRUE, ch, 0, victim, TO_CHAR);
    act("@WA white light emits from $n's core as $s hands begin to glow a @Bbrilliant blue@W.@n", TRUE, ch, 0, victim, TO_ROOM);
	
	int dam = (GET_STR(ch) + GET_DEX(ch) + GET_CON(ch)) * GET_LEVEL(ch) - (GET_AC(victim) + rand_number(50,250));
	int ch_hit = (GET_DEX(ch) + GET_PCN(ch) + GET_LEVEL(ch)) * GET_REMORTS(ch);
	int vict_hit = (GET_DEX(victim) + GET_PCN(victim) + GET_LEVEL(victim)) * GET_REMORTS(victim);
	ch_hit += (ch_hit * rand_number(1,10));
	GET_PSI(ch) -= 30;
	GET_CORETICKS(ch) += 20;
    
	if (ch_hit >= vict_hit) 
	{
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_ELECTRIFY, 1, GET_IDNUM(ch), exp, ch);
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			dam, 
			0, 
			0,
			0,
			0,
			"@B$N is blasted backwards as an arc of blue electricity flows from your hands.@n", 
			"@BAn arc of blue electricity flows from $n's hands and blasts you backwards!@n", 
			"@B$N is shot backwards as an arc of blue electricity flows from $n's hands!@n", 
			NULL, 
			NULL, 
			NULL,
			NULL, 
			NULL,
			NULL);
		update_combo(ch,victim,SKILL_ELECTRIFY);
    }
    else 
	{
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			0, 
			0, 
			0,
			0,
			FALSE,
			"$N is able to ground out the electricity immediately!", 
			"You are able to ground out $n's arc of electricity and suffer no damage!", 
			"$n's arc of electricity is grounded completely by $N!", 
			NULL, 
			NULL, 
			NULL,
			NULL,
			NULL,
			NULL);
		reset_combo(ch);
    }
}

ACMD(do_opticblast) // Opticblast skill for borgs - Gahan
{
	char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;

	if (GET_SKILL_LEVEL(ch, SKILL_OPTICBLAST) == 0) {
		send_to_char(ch, "You have no idea how to use your optic blast program.\r\n");
		return;
	}
    
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Blast who?\n\r");
            return;
        }
    }
    if (victim == ch) {
        send_to_char(ch, "You can not use this on yourself.\n\r");
        return;
    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}
	if (GET_PSI(ch) < 100) {
		send_to_char(ch, "You don't quite meet the power requirements, perhaps you should regain some Psi first.\r\n");
		return;
	}

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK) && !(GET_CLASS(ch) == CLASS_HIGHLANDER && GET_CLASS(victim) == CLASS_HIGHLANDER)) {
        send_to_char(ch, "You cannot bring yourself to violence here.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(victim))
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == victim)) {
        act("$N is your master, you don't want to anger $M.", FALSE, ch, 0,
        victim, 0);
        return;
    }
	if (GET_CORETICKS(ch) > 60) {
		send_to_char(ch, "You attempt to initiate your optical blast, but begins to overheat your power core.\r\n");
		GET_CORETICKS(ch) += 20;
		return;
	}
	if (GET_CORETICKS(ch) > 90) {
		send_to_char(ch, "You're power core is overheated! Let it cool down, before you do damage to your system.\r\n");
		return;
	}

	int dam = (GET_PCN(ch) + GET_DEX(ch) + GET_CON(ch))/ 3 * rand_number(100,300) + (GET_DAMROLL(ch)) - (GET_AC(victim) * 2);
	int ch_hit = rand_number(1,100);

	GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
	GET_PSI(ch) -= 100;
	GET_CORETICKS(ch) += 40;

    send_to_char(ch, "Your optical sensory begins to heat up, firing a blast of energy towards your target!\n\r");
    act("$n's eyes appear to glow a brilliant white, firing a blast of energy toward $N!", TRUE, ch, 0, victim, TO_ROOM);
    act("$n's eyes appear to glow a brilliant white, firing a blast of energy at you!", TRUE, ch, 0, victim, TO_VICT);

	if (ch_hit > 20) {
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 40;
		increment_skilluse(SKILL_OPTICBLAST, 1, GET_IDNUM(ch), exp, ch);
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE / 2, 
			dam, 
			TYPE_BLAST, 
			0,
			0,
			0,
			"$N is blasted backwards as a beam of white electricity snaps from your eyes!", 
			"A beam of white electricity snaps from $n's eyes and causes you to go into shock!", 
			"$N is snapped back by a beam of white electricity from $n!", 
			NULL, 
			NULL, 
			NULL,
			NULL, 
			NULL,
			NULL);
		update_combo(ch,victim,SKILL_OPTICBLAST);
    }

	else {
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE / 2, 
			0, 
			TYPE_BLAST, 
			0,
			0,
			0,
			"$N is able to ground out your electrical damage!", 
			"You manage to ground out $n's electrical damage!", 
			"$N grounds out $n's electrical damage, and carries on unaffected!", 
			NULL, 
			NULL, 
			NULL,
			NULL, 
			NULL,
			NULL);
		reset_combo(ch);
    }
}

ACMD(do_leapattack) // Leap attack skill for highlanders - Gahan
{
	char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;

	if (GET_SKILL_LEVEL(ch, SKILL_LEAP_ATTACK) == 0) {
		send_to_char(ch, "You have no idea how to leap.\r\n");
		return;
	}
    
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }
	if (ch->equipment[WEAR_WIELD]) {
		if (IS_GUN(ch->equipment[WEAR_WIELD])) {
			send_to_char(ch, "You don't think you could do much damage, leaping with that gun.\r\n");
			return;
		}
    }
    one_argument(argument, arg);

    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
        if (FIGHTING(ch))
            victim = ch->char_specials.fighting;
        else {
            send_to_char(ch, "Leap who?\n\r");
            return;
        }
    }
    if (victim == ch) {
        send_to_char(ch, "Okay, you leap on yourself.  Now what?\n\r");
        return;
    }
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
		send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
	    return;
	}
	if (GET_MOVE(ch) < 30) {
		send_to_char(ch, "You are too tired to perform leap attack.\r\n");
		return;
	}

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_PK_OK) && !(GET_CLASS(ch) == CLASS_HIGHLANDER && GET_CLASS(victim) == CLASS_HIGHLANDER)) {
        send_to_char(ch, "You cannot bring yourself to violence here.\n\r");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(victim))
        return;

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == victim)) {
        act("$N is your master, you don't want to anger $M.", FALSE, ch, 0,
        victim, 0);
        return;
    }

	act("You prepare for your leap attack, and begin to leap through the air!", TRUE, ch, 0, victim, TO_CHAR);
    act("$n crouches and jumps through the air with their weapon readied!", TRUE, ch, 0, victim, TO_ROOM);
	int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 40;
	increment_skilluse(SKILL_CIRCLE, 1, GET_IDNUM(ch), exp, ch);	
	int dam = (GET_STR(ch) + GET_DEX(ch) + GET_CON(ch) + rand_number(25,75) * GET_LEVEL(ch) - (GET_AC(victim) /3));
	int ch_hit = (GET_DEX(ch) + GET_PCN(ch) + GET_LEVEL(ch)) * GET_REMORTS(ch);
	int vict_hit = (GET_DEX(victim) + GET_PCN(victim) + GET_LEVEL(victim)) * GET_REMORTS(victim);
	ch_hit += (ch_hit * rand_number(1,10));
	GET_MOVE(ch) -= 30;
    
	if (ch_hit >= vict_hit) 
	{
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			dam, 
			0, 
			0,
			0,
			0,
			"You land directly on $N, catching $M off guard!", 
			"Flying through the air, $n lands directly on you, knocking you back!", 
			"Flying through the air, $n lands directly on $N, knocking $M back!", 
			NULL, 
			NULL, 
			NULL,
			NULL, 
			NULL,
			NULL);
		update_combo(ch,victim,SKILL_LEAP_ATTACK);
    }
    else 
	{
		exec_delay_hit(
			ch,
			GET_PC_NAME(victim),
			PULSE_VIOLENCE * 2, 
			0, 
			0, 
			0,
			0,
			FALSE,
			"$N drops, rolls and barely avoids your leaping attack!", 
			"You drop and roll, barely avoiding $n's leaping attack!", 
			"$n flying through the air, just barely misses $N with a leaping attack!", 
			NULL, 
			NULL, 
			NULL,
			NULL,
			NULL,
			NULL);
		reset_combo(ch);
    }
}

// The very start of ranged combat. Step #1, acquire target. - Gahan

ACMD(do_aim)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct char_data *victim;
	struct char_data *oldvictim;
	struct obj_data *wielded;

	if (GET_SKILL_LEVEL(ch, SKILL_AIM) == 0) {
		send_to_char(ch, "You have no idea how aim at anyone properly.\r\n");
		return;
	}
    
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char(ch, "Who would you like to aim at, and in what direction?\r\n");
		return;
	}
	
	if (!strcmp(arg1, "none")) {
		if (LOCKED(ch))
			LOCKED(ch)->char_specials.targeted = NULL;
		LOCKED(ch) = NULL;
		send_to_char(ch, "Your targeting system has been reset.\r\n");
		return;
	}
	wielded = GET_EQ(ch, WEAR_WIELD);

	if (RANGED_FIGHTING(ch)) {
		send_to_char(ch, "You are currently in ranged combat with someone!\r\n");
		return;
	}

	if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_GUN) {
		send_to_char(ch, "You need to be wielding a ranged weapon in order to target someone.\r\n");
		return;
	}

	if (!*arg2) {
		if (!(victim = get_lineofsight(ch, ch, arg1, NULL))) {
			send_to_char(ch, "Your specified target is not within your range.\r\n");
			return;
		}
	}
	else {
		if (!(victim = get_lineofsight(ch, ch, arg1, arg2))) {
			send_to_char(ch, "Your specified target is not within your range.\r\n");
			return;
		}
	}
	if (victim) {
		if (!IS_NPC(victim) || victim == NULL) {
			send_to_char(ch, "Your target can not be a player.\r\n");
			return;
		}
		if (MOB_FLAGGED(victim, MOB_NOSNIPE)) {
			send_to_char(ch, "For some reason you can't seem to target %s.\r\n", victim->player.short_descr);
			return;
		}
		if (victim->char_specials.targeted != NULL && victim->char_specials.targeted != ch)
			send_to_char(ch, "%s has already been targeted by someone else.\r\n", GET_NAME(victim));

		else {
			int roll = rand_number(1,100);
			if (roll >= 40) {
				if (ch->char_specials.locked != NULL) {
					send_to_char(ch, "You abandon your previous target and begin to target someone else.\r\n");
					oldvictim = ch->char_specials.locked;
					if (oldvictim)
						oldvictim->char_specials.targeted = NULL;
				}
				if (IN_ROOM(victim) == IN_ROOM(ch))
					send_to_char(ch, "You aim your crosshair and lock your sights on %s.\r\n", victim->player.short_descr);
				else
					send_to_char(ch, "You aim your crosshair and lock your sights on %s to the %s.\r\n", victim->player.short_descr, dirs[ch->char_specials.tmpdir]);
				GET_MOB_HOME(victim) = IN_ROOM(victim);
				GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
				ch->char_specials.locked = victim;
				victim->char_specials.targeted = ch;
				return;
			}
			else {
				send_to_char(ch, "Your targetting system fails to lock onto %s.\r\n", victim->player.short_descr);
				return;
			}	
		}
	}
}

ACMD(do_snipe)
{
	struct char_data *victim = NULL;
	struct obj_data *wielded = NULL;

	if (GET_SKILL_LEVEL(ch, SKILL_SNIPER) == 0) {
		send_to_char(ch, "You have no idea how to shoot from a distance.\r\n");
		return;
	}
    
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

	wielded = GET_EQ(ch, WEAR_WIELD);

	if (RANGED_FIGHTING(ch)) {
		send_to_char(ch, "You are already in ranged combat with someone already.\r\n");
		return;
	}

	if (FIGHTING(ch)) {
		send_to_char(ch, "You can't snipe someone if you're in close ranged combat with someone.\r\n");
		return;
	}
	if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_GUN) {
		send_to_char(ch, "You need to be wielding a ranged weapon in order to engage in ranged combat.\r\n");
		return;
	}
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_GUN) {
		if (!IS_NPC(ch) && !IS_LOADED(ch->equipment[WEAR_WIELD])) {
		    send_to_char(ch, "Click!\r\n");
			return;
		}
	}
	
	if (LOCKED(ch)) {
		victim = LOCKED(ch);

		if (!victim) {
			send_to_char(ch, "Your targeting system malfunctioned, leaving you without a target.\r\n");
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			LOCKED(ch) = NULL;
			return;
		}

		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
			send_to_char(ch, "You cannot shoot at anyone from a safe room.\r\n");
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			return;
		}
		if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL)) {
			send_to_char(ch, "You cannot shoot at anyone who is in a safe room.\r\n");
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			return;
		}
		if (victim == get_lineofsight(ch, LOCKED(ch), NULL, NULL)) {
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
			REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
			REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
			int thaco = rand_number(1,30);
			rangedhit(ch, LOCKED(ch), TYPE_UNDEFINED, thaco);

			return;
		}
		else {
			send_to_char(ch, "Your target is no longer within your line of sight, resetting target info.\r\n");
			LOCKED(ch) = NULL;
			GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

		}
	}
	else
		send_to_char(ch, "You are not currently targeting anyone.\r\n");
}
