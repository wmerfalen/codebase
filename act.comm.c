/**************************************************************************
*  File: act.comm.c                                        Part of tbaMUD *
*  Usage: Player-level communication commands.                            *
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
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "act.h"
#include "modify.h"
#include "constants.h"
#include "craft.h"
#include "psionics.h"
#include "fight.h"
#include "bionics.h"
#include "protocol.h"

#define NEWB_MAXMSG 27
int messagenum = 0;

ACMD(do_say)
{
	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "What would you like to say?\r\n");
	else {
		char buf[MAX_INPUT_LENGTH + 14], *msg;
		struct char_data *vict;
                parse_at(argument);

		snprintf(buf, sizeof(buf), "$n@n says, '%s@n'", argument);
		msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

		for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
			if (vict != ch && GET_POS(vict) > POS_SLEEPING)
				add_history(vict, msg, HIST_SAY);

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else {
			sprintf(buf, "You say, '%s@n'", argument);
			msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
			add_history(ch, msg, HIST_SAY);
		}
	}

  /* Trigger check. */
	speech_mtrigger(ch, argument);
	speech_wtrigger(ch, argument);
}

ACMD(do_gsay)
{
	struct char_data *k;
	struct follow_type *f;
	char *msg, *msg2;

	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "What would you like to tell the group?\r\n");
	else {
		char buf[MAX_STRING_LENGTH];
		char buf2[MAX_STRING_LENGTH];

		if (ch->master)
			k = ch->master;
		else
			k = ch;
                parse_at(argument);

		snprintf(buf, sizeof(buf), "@G%s tells the group, '%s@n'",GET_NAME(ch), argument);

		if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch)) {
			msg = act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
			add_history(k, msg, HIST_GT);
		}
			
		for (f = k->followers; f; f = f->next)
			if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch)) {
				msg = act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
				add_history(f->follower, msg, HIST_GT);
			}
    
		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else {
			snprintf(buf2, sizeof(buf2), "@GYou tell the group, '%s@n'", argument);
			msg2 = act(buf2, FALSE, ch, 0, 0, TO_CHAR);
			add_history(ch, msg2, HIST_GT);
			if (!AFF_FLAGGED(ch, AFF_GROUP))
				send_to_char(ch, "Warning: You do not have any group members.\r\n");
		}
	}
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
	char buf[MAX_STRING_LENGTH], *msg;

	snprintf(buf, sizeof(buf), "%s$n tells you, '%s'%s", CCRED(vict, C_NRM), arg, CCNRM(vict, C_NRM));
	msg = act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	add_history(vict, msg, HIST_TELL);

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(ch, "%s", CONFIG_OK);
	else {
		snprintf(buf, sizeof(buf), "%sYou tell $N, '%s'%s", CCRED(ch, C_NRM), arg, CCNRM(ch, C_NRM));
		msg = act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);     
		add_history(ch, msg, HIST_TELL);
	}

	if (!IS_NPC(vict) && !IS_NPC(ch))
		GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
	if (!ch)
		log("SYSERR: is_tell_ok called with no characters");
	else if (!vict)
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (ch == vict)
		send_to_char(ch, "You try to tell yourself something.\r\n");
	else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
		send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
	else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
		send_to_char(ch, "The walls seem to absorb your words.\r\n");
	else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return (TRUE);

	return (FALSE);
}

/* Yes, do_tell probably could be combined with whisper and ask, but it is
 * called frequently, and should IMHO be kept as tight as possible. */
ACMD(do_tell)
{
	struct char_data *vict = NULL;
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
                parse_at(argument);
	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char(ch, "Who do you wish to tell what??\r\n");
	else if (!strcmp(buf, "m-w")) {
#ifdef CIRCLE_WINDOWS
	/* getpid() is not portable */
		send_to_char(ch, "Sorry, that is not available in the windows port.\r\n");
#else /* all other configurations */
		int i;
		char word[MAX_INPUT_LENGTH], *p, *q;

		if (last_webster_teller != -1L) {
			if (GET_IDNUM(ch) == last_webster_teller) {
			send_to_char(ch, "You are still waiting for a response.\r\n");
			return;
		}
		else {
			send_to_char(ch, "Hold on, m-w is busy. Try again in a couple of seconds.\r\n");
			return;
		}
	}

    /* Only a-z and +/- allowed. */
    for (p = buf2, q = word; *p ; p++)
		if ((LOWER(*p) <= 'z' && LOWER(*p) >= 'a') || (*p == '+') || (*p == '-'))
			*q++ = *p;

    *q = '\0';

    if (!*word) {
		send_to_char(ch, "Sorry, only letters and +/- are allowed characters.\r\n");
		return;
    }
    snprintf(buf, sizeof(buf), "../bin/webster %s %d &", word, (int) getpid());
    i = system(buf);
    last_webster_teller = GET_IDNUM(ch);
    send_to_char(ch, "You look up '%s' in Merriam-Webster.\r\n", word);
#endif /* platform specific part */
	}
	else if (GET_LEVEL(ch) < LVL_IMMORT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (is_tell_ok(ch, vict))
		perform_tell(ch, vict, buf2);
}

ACMD(do_reply)
{
	struct char_data *tch = character_list;

	if (IS_NPC(ch))
		return;

	skip_spaces(&argument);
                parse_at(argument);
	if (GET_LAST_TELL(ch) == NOBODY)
		send_to_char(ch, "You have nobody to reply to!\r\n");
	else if (!*argument)
		send_to_char(ch, "What is your reply?\r\n");
	else {

		while (tch && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
			tch = tch->next;

		if (!tch)
			send_to_char(ch, "They are no longer playing.\r\n");
		else if (is_tell_ok(ch, tch))
			perform_tell(ch, tch, argument);
	}
}
/* CyberASSAULT Ignore - Gahan */
ACMD(do_ignore)
{
    struct char_data *vict;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument, buf);

    if (!*buf) {
        send_to_char(ch, "You are now ignoring nobody.\n\r");
        ch->char_specials.ignoring = 0;
    }
    else {
        if (!(vict = get_player_vis(ch, buf, NULL, 0)))
            send_to_char(ch, "Nobody here by that name...\n\r");
        else if (vict == ch)
            send_to_char(ch, "Ignore yourself? Get a life.\n\r");
        else if (GET_LEVEL(vict) >= LVL_IMMORT)
            send_to_char(ch, "You can't ignore immortals!\r\n");
        else {
            send_to_char(ch, "Okay.\n\r");
            ch->char_specials.ignoring = GET_IDNUM(vict);
        }
    }
}

ACMD(do_spec_comm)
{
	char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
	struct char_data *vict;
	const char *action_sing, *action_plur, *action_others;

	switch (subcmd) {
		case SCMD_WHISPER:
			action_sing = "whisper to";
			action_plur = "whispers to";
			action_others = "$n whispers something to $N.";
			break;

		case SCMD_ASK:
			action_sing = "ask";
			action_plur = "asks";
			action_others = "$n asks $N a question.";
			break;

		default:
			action_sing = "oops";
			action_plur = "oopses";
			action_others = "$n is tongue-tied trying to speak with $N.";
			break;
	}

	half_chop(argument, buf, buf2);

	if (!*buf || !*buf2)
		send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
	else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
		send_to_char(ch, "%s", CONFIG_NOPERSON);
	else if (vict == ch)
		send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
	else {
		char buf1[MAX_STRING_LENGTH];

		snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
		act(buf1, FALSE, ch, 0, vict, TO_VICT);

		if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_NOREPEAT))) 
			send_to_char(ch, "%s", CONFIG_OK);
		else
			send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
    
		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
	}
}

ACMD(do_write)
{
	struct obj_data *paper, *pen = NULL;
	char *papername, *penname;
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

	papername = buf1;
	penname = buf2;

	two_arguments(argument, papername, penname);

	if (!ch->desc)
		return;

	if (!*papername) {
		/* Nothing was delivered. */
		send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
		return;
	}
	if (*penname) {
		/* Nothing was delivered. */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
			send_to_char(ch, "You have no %s.\r\n", papername);
			return;
		}
		if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
			send_to_char(ch, "You have no %s.\r\n", penname);
			return;
		}
	}
	else { /* There was one arg.. let's see what we can find. */
		if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
			send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN) { /* Oops, a pen. */
			pen = paper;
			paper = NULL;
		}
		else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
			send_to_char(ch, "That thing has nothing to do with writing.\r\n");
			return;
		}

		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD)) {
			send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
			send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}

	/* Now let's see what kind of stuff we've found. */
	if (GET_OBJ_TYPE(pen) != ITEM_PEN)
		act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
	else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
	else {
		char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
		backstr = strdup(paper->action_description);
		send_to_char(ch, "There's something written on it already:\r\n");
		send_to_char(ch, "%s", paper->action_description);
    }

    /* We can write. */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}

ACMD(do_page)
{
	struct descriptor_data *d;
	struct char_data *vict;
	char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

	half_chop(argument, arg, buf2);

	if (IS_NPC(ch))
		send_to_char(ch, "Monsters can't page.. go away.\r\n");
	else if (!*arg)
		send_to_char(ch, "Whom do you wish to page?\r\n");
	else {
		char buf[MAX_STRING_LENGTH];

		snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
		
		if (!str_cmp(arg, "all")) {
			if (GET_LEVEL(ch) > LVL_GOD) {
				for (d = descriptor_list; d; d = d->next)
					if (STATE(d) == CON_PLAYING && d->character)
						act(buf, FALSE, ch, 0, d->character, TO_VICT);
			}
			else
				send_to_char(ch, "You will never be godly enough to do that!\r\n");
			return;
		}
		if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
			act(buf, FALSE, ch, 0, vict, TO_VICT);
		
			if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(ch, "%s", CONFIG_OK);
			else
				act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
		else
			send_to_char(ch, "There is no such person in the game!\r\n");
	}
}
ACMD(do_event)
{
    char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
    char buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
    char arg1[MAX_INPUT_LENGTH];
	char *msg;
    struct descriptor_data *d;
	
	one_argument(argument, arg1);
	
    if (!*argument) {
        send_to_char(ch, "What would you like to say on the event channel?");
        return;
    }
	skip_spaces(&argument);
    delete_doubledollar(argument);
	
    snprintf(buf1, sizeof(buf1), "@C[ @WEVENT@C ]@w %s: %s@n\r\n", GET_NAME(ch), argument);
    snprintf(buf2, sizeof(buf1), "@C[ @WEVENT@C ]@w Someone: %s@n\r\n", argument);

    for (d = descriptor_list; d; d = d->next) {

        if (IS_PLAYING(d) && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {

			if (IS_MOB(d->character))
				continue;
            if (CAN_SEE(d->character, ch)) {
                msg = strdup(buf1);
                send_to_char(d->character, "%s", buf1);
            }
            else {
                msg = strdup(buf2);
                send_to_char(d->character, "%s", buf2);
            }
            add_history(d->character, msg, HIST_EVENT);
        }
    }
    if (!IS_MOB(ch)) {
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))	
	        send_to_char(ch, "%s", CONFIG_OK);
	}
}
/* Generalized communication function by Fred C. Merkel (Torg). */
ACMD(do_gen_comm)
{
	struct descriptor_data *i;
	char color_on[24];
	char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], *msg;
	bool emoting = FALSE;
	/* Array of flags which must _not_ be set in order for comm to be heard. */
	int channels[] = {
		0,
		PRF_NOSHOUT,
		PRF_NOGOSS,
		PRF_NOAUCT,
		PRF_NOGRATZ,
		PRF_NONEWBIE,
		0,
		0
	};

	int hist_type[] = {
		HIST_HOLLER,
		HIST_SHOUT,
		HIST_GOSSIP,
		HIST_AUCTION,
		HIST_GRATS,
		HIST_NEWBIE,
		HIST_RADIO
	};

	/* com_msgs: [0] Message if you can't perform the action because of noshout
	*           [1] name of the action
	*           [2] message if you're not on the channel
	*           [3] a color string. */
	const char *com_msgs[][4] = {
		{"You cannot holler!!\r\n",
		"holler",
		"",
		KYEL},

		{"You cannot shout!!\r\n",
		"shout",
		"Turn off your noshout flag first!\r\n",
		KYEL},

		{"You cannot gossip!!\r\n",
		"gossip",
		"You aren't even on the channel!\r\n",
		KYEL},

		{"You cannot auction!!\r\n",
		"auction",
		"You aren't even on the channel!\r\n",
		KMAG},

		{"You cannot congratulate!\r\n",
		"congrat",
		"You aren't even on the channel!\r\n",
		KGRN},

        {"You cannot newbie!\r\n",
        "newbie",
        "You aren't even on newbie channel!\r\n",
        KGRN
        },

        {"Your radio isn't turned on!\r\n",
        "broadcast",
        "You cannot hear your radio!\r\n",
        KWHT
        },
        {"You cannot gossip your emotions!\r\n",
        "gossip",
        "You aren't even on the channel!\r\n",
        KYEL
        }
	};

	if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
		send_to_char(ch, "%s", com_msgs[subcmd][0]);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)) {
		send_to_char(ch, "The walls seem to absorb your words.\r\n");
		return;
	}
	if (subcmd == SCMD_GEMOTE) {
		if (!*argument)
			send_to_char(ch, "Gemote? Yes? Gemote what?\r\n");
		else
			do_gmote(ch, argument, 0, 1);
		return;
	}
	if (subcmd == SCMD_RADIO) {
		if (GET_FREQ(ch) == 0) {
			send_to_char(ch, "You radio is turned off.\r\n");
			return;
		}
	}
	/* Level_can_shout defined in config.c. */
	if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT) {
		send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
		return;
	}
	/* Make sure the char is on the channel. */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd])) {
		send_to_char(ch, "%s", com_msgs[subcmd][2]);
		return;
	}
	/* skip leading spaces */
	skip_spaces(&argument);

	/* Make sure that there is something there to say! */
	if (!*argument) {
		send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
		return;
	}
	if (subcmd == SCMD_HOLLER) {
		if (GET_MOVE(ch) < CONFIG_HOLLER_MOVE_COST) {
			send_to_char(ch, "You're too exhausted to holler.\r\n");
			return;
		}
		else
			GET_MOVE(ch) -= CONFIG_HOLLER_MOVE_COST;
	}
	/* Set up the color on code. */
	strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

	/* First, set up strings to be given to the communicator. */
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(ch, "%s", CONFIG_OK);
	else {
                parse_at(argument);
		snprintf(buf1, sizeof(buf1), "%sYou %s, '%s%s'%s", COLOR_LEV(ch) >= C_CMP ? color_on : "",
			com_msgs[subcmd][1], argument, COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));
    
		msg = act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
		add_history(ch, msg, hist_type[subcmd]);
	}
	if (!emoting)
                parse_at(argument);
		snprintf(buf1, sizeof(buf1), "$n %ss, '%s'", com_msgs[subcmd][1], argument);

	/* Now send all the strings out. */
	for (i = descriptor_list; i; i = i->next) {
		if (STATE(i) != CON_PLAYING || i == ch->desc || !i->character ) {
			switch STATE(i) {
			case CON_OEDIT:
			case CON_REDIT:
			case CON_ZEDIT:
			case CON_MEDIT:
			case CON_SEDIT:
			case CON_TEDIT:
			case CON_CEDIT:
			case CON_AEDIT:
			case CON_TRIGEDIT:
			case CON_HEDIT:
			case CON_QEDIT:
				snprintf(buf1, sizeof(buf1), "%s%s %s, '%s%s'%s\r\n", COLOR_LEV(ch) >= C_CMP ? color_on : "",
					PERS(ch, i->character), com_msgs[subcmd][1], argument,
                    COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));
                add_history(i->character, buf1, hist_type[subcmd]);
				log("history: %s", buf1);
				break;
			default:
				break;
			}
			continue;
		}
		if (!IS_NPC(ch) && (PRF_FLAGGED(i->character, channels[subcmd]) || PLR_FLAGGED(i->character, PLR_WRITING)))
		continue;
		if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
		continue;
		if (subcmd == SCMD_SHOUT && ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) || !AWAKE(i->character)))
		continue;
		if (subcmd == SCMD_RADIO && !PRF_FLAGGED(i->character, PRF_RADIOSNOOP) && (GET_FREQ(ch) != GET_FREQ(i->character)))
			continue;
		if (subcmd == SCMD_RADIO && PRF_FLAGGED(i->character, PRF_RADIOSNOOP))
			send_to_char(i->character, "[%d] ", GET_FREQ(ch));

		snprintf(buf2, sizeof(buf2), "%s%s%s", (COLOR_LEV(i->character) >= C_NRM) ? color_on : "", buf1, KNRM); 
		msg = act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		add_history(i->character, msg, hist_type[subcmd]);
	}
}

ACMD(do_qcomm)
{
    if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_NOSHOUT)) {
        send_to_char(ch, "You can't quest-say.\n\r");
        return;
    }
	if (!PRF_FLAGGED(ch, PRF_QUEST)) {
		send_to_char(ch, "You aren't even part of the quest!\r\n");
		return;
	}
	skip_spaces(&argument);

	if (!*argument)
		send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
	else {
		char buf[MAX_STRING_LENGTH];
		struct descriptor_data *i;

		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(ch, "%s", CONFIG_OK);
		else if (subcmd == SCMD_QSAY) {
			snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
			act(buf, FALSE, ch, 0, argument, TO_CHAR);
		}
		else
			act(argument, FALSE, ch, 0, argument, TO_CHAR);

		if (subcmd == SCMD_QSAY)
			snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
		else {
			strlcpy(buf, argument, sizeof(buf));
			mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s qechoed: %s", GET_NAME(ch), argument);
		}
		for (i = descriptor_list; i; i = i->next)
			if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
				act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
	}
}

char *newb_msgs[NEWB_MAXMSG] = {
    "@YNewbie radio@R will broadcast tips to you every tick!!! @Yradio off@n @Rwill turn it off!@n",
    "Watch out for instant Death Traps! Which are always marked with (DT) in the @yexits@n information.",
    "Before you get started on CyberASSAULT, check out the practice range for some good experience and gear (east, up, west, west, south from recall/Armory).",
    "Don't forget to read the rules of CyberASSAULT with the command @cpolicy@n",
    "Change the frequency of your radio by typing: @cradio <freq>@n where freq is 'off' or a number from 1 to 2000000000.",
    "Want to see who else is listening?  Try @cradio who@n similar to @cwho@n to see everyone visible in the MUD.",
    "Grouping is a MUST if you want to kill those harder mobs! Or get some better equipment off an auction or donation.",
    "Not sure about something?  Try the help command first, then ask your fellow players. Board commands are in @yhelp board@n",
    "To gain a level type @ygain@n when you have enough exp.  You can save up exp and gain multiple levels in a row.",
    "Quests are like jobs, the more you work, the richer you get- Find your first questmaster just up from recall!",
    "If a mob kills you, go the morgue (east, up, west, up, up, west, down from recall) and recover your items using @crecover@n then. If you go south twice after you wake up in the regen pod, it should put you right in the morgue too.",
    "Read the newbie guide which you pick up at the end of the bootcamp, it will give basic tips. Also read @Ghelp tips@n.",
    "To find the stats of an object, practice the gadgetry skill and buy a pocket identifier from the Electronics Store, then type @cscan <item name>@n.",
    "You practice skills and psionics in your class guild, each guild is listed on the map. You can find guild info on @Yhelp guild@n.",
    "Make sure you consider an enemy before attacking it using the @Mconsider <mob name here>@n command. Also read @Yhelp supermob@n.",
    "Make sure to check those exits when exploring new zones!  The @cexits@n and @clook@n commands are handy.",
    "The 'locate object' psionic can be used to find @yquest marbles@n which can be exchanged for QuestPoints.",
    "There are @yrainbow tokens@n scattered randomly on mobs every hour, which can be exchanged or assembled into a quest item!",
    "You get bonus experience points for using skills and psionics!",
    "To load your gun, simply wield the gun, have the correct ammo in inventory and type @Yload@n.",
    "To learn where to go to find fortune and adventure, see @Ghelp areas@n or refer to your newbie guide.",
    "You don't gain levels automatically (unless you have autogain toggled on), you need to type @cgain@n when YOU want to level up, wearing +stat equipment gives bonus HPV!",
    "Your HPV stats will increase (regenerate) every few seconds.  You can increase the rate by sleeping, meditating, or going to a regen bonus room.",
    "Use your quest points to buy equipment from the Master Fabricator or use in trades for high-level items from other players, they can be worth a lot! Qps can also be used to exchange for units, look at the hologram 2 easts of recall for more info.",
    "This is a very interactive mud, so try to talk to other characters and use the search command to find hidden items and exits. The search skill will make it easier to find hidden things.",
    "There are different methods to attack, you can choose from a wide range of melee weapons, guns & explosives, or psionic powers!"
    "For blind players, please see @Yhelp filter@n and @yhelp vip@n.",
    "@RWARNING! Newbie radio will be re-broadcasting this spam again! @Yradio off@R will turn it off@n"
};

/* Newbie Information Radio!  -Lump
   spits out an information message to frequency 10 every tick
   todo: make an int array which will hold the sequence of messages */
void bc_newbie(void)
{
    struct descriptor_data *i;

    if (messagenum >= NEWB_MAXMSG)
        return;

    for (i = descriptor_list; i; i = i->next) {
        if (!i->connected && i->character && GET_FREQ(i->character) == NEWB_RADIO && !PLR_FLAGGED(i->character, PLR_WRITING) &&
            !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {

            if (COLOR_LEV(i->character) >= C_NRM)
                send_to_char(i->character, KWHT);

            send_to_char(i->character, "NewbRadio broadcasts, '%s'\r\n", newb_msgs[messagenum]);

            if (COLOR_LEV(i->character) >= C_NRM)
                send_to_char(i->character, KNRM);
        }
    }

    /* todo: change index to the next in a sequence of numbers */
    if (++messagenum == NEWB_MAXMSG)
        messagenum = 0;
}

/* function to check if ch is allowed to listen to freq */
int check_frequency(struct char_data *ch, int freq)
{
    switch (freq) {

        case 69:
            if (GET_LEVEL(ch) < LVL_IMMORT && !PLR_FLAGGED(ch, PLR_REP)) {
                send_to_char(ch, "This frequency is scrambled!\r\n");
                return (FALSE);
            }
            break;

        default:
            return (TRUE);
    }
    return (TRUE);
}

ACMD(do_radio)
{
    struct descriptor_data *i;
    int freq = 0;
    char arg[MAX_INPUT_LENGTH];

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Usage: radio <who|off|#>\r\n\r\n");
        send_to_char(ch, "You are currently on frequency %d\r\n", GET_FREQ(ch));
        return;
    }

    if (!str_cmp(arg, "who")) {
        if (GET_LEVEL(ch) < LVL_GOD) {
            if (GET_FREQ(ch) == 0)
                send_to_char(ch, "But your radio is turned off!\r\n");
            else {
                send_to_char(ch, "Listeners\r\n");
                send_to_char(ch, "---------\r\n");
                for (i = descriptor_list; i; i = i->next)
                    if (!i->connected) {
                        if (GET_FREQ(i->character) == GET_FREQ(ch) && CAN_SEE(ch, i->character))
                            send_to_char(ch, "%s\r\n", GET_NAME(i->character));
                    }
            }

        }
        else {
            /* immortal */

            send_to_char(ch, "Frequency     Player\r\n");
            send_to_char(ch, "------------  --------------------\r\n");
            for (i = descriptor_list; i; i = i->next) {
                if (!i->connected && GET_FREQ(i->character) != 0) {
                    send_to_char(ch, "[%10d]  %s\r\n",
                    GET_FREQ(i->character), GET_NAME(i->character));
                }
            }
        }
    }
    else {
        if (is_number(arg))
            freq = atoi(arg);
        else
            freq = radio_find_alias(arg);

        if (freq < 0 || freq > 2000000000) {
            send_to_char(ch, "Invalid frequency.  Try 1-2000000000.\r\n");
            return;
        }

        if (check_frequency(ch, freq)) {
			GET_FREQ(ch) = freq;
            send_to_char(ch, "*Click-Click*  Done.\r\n");
			if (freq == 0)
                send_to_char(ch, "You now hear nothing but your own fears.\r\n");
            else if (freq == 1)
                send_to_char(ch, "Welcome to the main radio signal.  We're #1!!! Woot!\r\n");
            else if (freq == 10)
                send_to_char(ch, "Help! When I need somebody... Help! Not just anybody... Help! When I need someone....HHHHEEEEELP!\r\n");
            else if (freq == 11)
                send_to_char(ch, "Welcome to Deep Thoughts, by Jack Handey\r\n");
            else if (freq == 69)
                send_to_char(ch, "Oooooohhhh baaaby, let's get it oonnnnn!\r\n");
        }
        else
            send_to_char(ch, "All you hear is static, this frequency must be private.\r\n");
    }
}

long radio_find_alias(char *arg)
{
    long freq;

    if (!str_cmp(arg, "off"))
        freq = 0;
    else if (!str_cmp(arg, "main"))
        freq = 1;
    else if (!str_cmp(arg, "newbie") || !str_cmp(arg, "help"))
        freq = 10;
    else if (!str_cmp(arg, "reps") || !str_cmp(arg, "imm"))
        freq = 69;
    else if (!str_cmp(arg, "deepthoughts") || !str_cmp(arg, "deep") || !str_cmp(arg, "thoughts"))
        freq = 11;
    else
        freq = -1;

    return (freq);
}

/* Market code added by Gahan 09/11
 Basically an auto-auction code, but modernized. */

ACMD(do_market)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *obj;
	char buf[MAX_INPUT_LENGTH];

	two_arguments(argument, arg1, arg2);

	if (ch == NULL)
		return;

	if (!IS_NPC(ch)) {
		if (PRF_FLAGGED(ch, PRF_NOMARKET)) {
			send_to_char(ch, "Please turn off 'NoMarket' prior to using the market system.");
			return;
		}
	}

	if (IS_NPC(ch)) {
		if (GET_MOB_VNUM(ch) == 16462)
			send_to_char(ch, "Okay.");
		else
			return;
	}

	if (!*arg1) {
		send_to_char(ch, "What would you like to put on the market?\r\n");
		return;
	}

	if (!strcmp(arg1, "cancel")) {

		if (auction_info.owner == ch) {
			sprintf(buf, "@DThe market session has been cancelled, all resource units have been refunded.\r\n");
			auction_channel(buf);

			if (auction_info.high_bidder != NULL)
				auction_info.high_bidder->points.units += auction_info.current_bid;
			obj_from_room(auction_info.item);
			obj_to_char(auction_info.item, auction_info.owner);

			auction_info.item = NULL;
			auction_info.owner = NULL;
			auction_info.current_bid = 0;
			auction_info.status = 0;
			return;
		}
		else {
			send_to_char(ch, "You can't end someone elses market session.\r\n");
			return;
		}
	}

	if (!strcmp(arg1, "info")) {
		obj = auction_info.item;

		if (!obj) {
			send_to_char(ch, "There is nothing on the market right now.\r\n");
			return;
		}

		if (auction_info.owner == ch) {
			sprintf(buf, "You currently have %s up on the market.\r\n\r\n", obj->short_description);
			send_to_char(ch, buf, ch);
			show_obj_stats(ch, obj);
			return;
		}

		else {
			show_obj_stats(ch, obj);
			return;
		}
	}

	if (!strcmp(arg1, "bid")) {

		int bid;
		obj = auction_info.item;

		if (!obj) {
			send_to_char(ch, "There is nothing on the market right now.\r\n");
			return;
		}

		if (auction_info.owner == ch) {
			send_to_char(ch, "You can't bid on your own item.\r\n");
			return;
		}

		if (!*arg2) {
			send_to_char(ch, "Please specify the amount you wish to bid on this item.\r\n");
			return;
		}

		bid = atoi(arg2);

		if (bid > 1000000000) {
			send_to_char(ch, "The maximum amount you can bid for an item is 1 billion resource units.");
			return;
		}

		if (bid < auction_info.minimum_bid) {
			sprintf(buf, "You must enter a bid 10 precent higher than the current current bid. (%d RU)\r\n", auction_info.minimum_bid);
			send_to_char(ch, "%s", buf);
			return;
		}

		if (GET_UNITS(ch) < bid) {
			send_to_char(ch, "You don't have enough resource units to place that bid.\r\n");
			return;
		}

		else {
			if (auction_info.high_bidder != NULL && auction_info.current_bid > 0)
				GET_UNITS(auction_info.high_bidder) += auction_info.current_bid;

			GET_UNITS(ch) -= bid;
			auction_info.high_bidder = ch;
			float newbid = (bid * 1.10);
			auction_info.minimum_bid = (int) newbid;
			auction_info.current_bid = bid;



			sprintf(buf, "@D%s has offered %d resource units for %s@D.\r\n", auction_info.high_bidder->player.name, bid, auction_info.item->short_description);
			auction_channel(buf);

			if (auction_info.status == AUCTION_LENGTH -1 || auction_info.status == AUCTION_LENGTH -2) {	
				auction_info.status -= 3;
				
				if (auction_info.status < 0)
					auction_info.status = 0;
			}

			if (auction_info.status == AUCTION_LENGTH -3 || auction_info.status == AUCTION_LENGTH -4) {
				auction_info.status--;
				if (auction_info.status < 0)
					auction_info.status = 0;
			}
			
			return;
		}
	}

	obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying);

	if (auction_info.item != NULL) {
		send_to_char(ch, "There's already a market session in progess.  Please wait until this one finishes.\r\n");
		return;
	}

	if (!obj) {
		send_to_char(ch, "You don't seem to be carrying that item.\r\n");
		return;
	}
	
	if (OBJ_FLAGGED(obj, ITEM_NODROP) || IS_BOUND(obj)) {
		send_to_char(ch, "You can't put that item up on the market.\r\n");
		return;
	}

	/* Certain types of objects should never be put on the market. */

	if (GET_OBJ_TYPE(obj) == ITEM_KEY || GET_OBJ_TYPE(obj) == ITEM_EXPLOSIVE
		|| GET_OBJ_TYPE(obj) == ITEM_AUTOQUEST || GET_OBJ_TYPE(obj) == ITEM_QUEST
		|| GET_OBJ_TYPE(obj) == ITEM_BODY_PART || GET_OBJ_TYPE(obj) == ITEM_MEDICAL) {
			send_to_char(ch, "You can't put that item up on the market.\r\n");
			return;
	}
	
	/* Probably a bad idea to let them auction containers with stuff in them.
	   Example: Cursed items, nodrops, etc? */

	if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {
		send_to_char(ch, "You must empty your container before selling it on the market.\r\n");
		return;
	}
	
	/* Make it so they dont auction just ANYTHING for no reason. */

	if (GET_UNITS(ch) < 500) {
		send_to_char(ch, "You do not have the 500 resourse units needed for using the markets services.\r\n");
		return;
	}

	if (!*arg2) {
		auction_info.minimum_bid = 1;
	}
	else {
		int minbid;
		minbid = atoi(arg2);
		auction_info.minimum_bid = minbid;

		if (auction_info.minimum_bid > 1000000000 || auction_info.minimum_bid <= 0) {
			send_to_char(ch, "The range of minimum bids are between 1 and 1,000,000,000 resource units.\r\n");
			return;
		}		

	}

	GET_UNITS(ch) -= 500;
	auction_info.owner = ch;
	auction_info.item = obj;
	auction_info.current_bid = 0;
	auction_info.status = 0;

	sprintf(buf, "@D%s has put %s @Don the market for %d resource units.\r\n", auction_info.owner->player.name, auction_info.item->short_description, auction_info.minimum_bid);
	auction_channel(buf);
	obj_from_char(obj);
	obj_to_room(obj, real_room(12));

	return;
}

/* Ticks down the auction timer and checks to see if a bidder or market owner went missing. - Gahan 09/11 */

void auction_update(void)
{
	char buf[MAX_STRING_LENGTH];
	struct char_data *ch;
	struct char_data *next_ch;
	struct char_data *own;
	struct char_data *next_own;
	bool foundown = FALSE;
	bool found = FALSE;

	if (auction_info.item == NULL)
		return;
	
	if (auction_info.owner != NULL) {

		for (own = character_list; own; own = next_own) {
			next_own = own->next;

			if (own == auction_info.owner) {
				foundown = TRUE;
			}
		}

		if (!foundown) {
			sprintf(buf, "The owner of this item has gone missing, proceeds are being donated to the Immortals.");
			auction_channel(buf);
			auction_info.owner = NULL;
		}
	}
			

	if (auction_info.high_bidder != NULL) {

	    for (ch = character_list; ch; ch = next_ch) {

		    next_ch = ch->next;

			if (ch == auction_info.high_bidder) {
				found = TRUE;
			}
		}

		if (!found) {
			sprintf(buf, "The current high bidder has gone missing, resetting market session.");
			auction_channel(buf);
			auction_info.current_bid = auction_info.minimum_bid;
			auction_info.current_bid = 0;
			auction_info.high_bidder = NULL;
			auction_info.status = 0;
		}
	}

	auction_info.status++;

	if (auction_info.status == AUCTION_LENGTH) {

		sprintf(buf, "@D%s has been sold to %s for %d resource units.\r\n", auction_info.item->short_description,
			auction_info.high_bidder->player.name, auction_info.current_bid);
		auction_channel(buf);
		
		if (foundown == TRUE) {
			auction_info.owner->points.units += auction_info.current_bid;
			sprintf(buf, "You receive %d resource units.\r\n", auction_info.current_bid);
			send_to_char(auction_info.owner, "%s", buf);
		}
		obj_from_room(auction_info.item);
		obj_to_char(auction_info.item, auction_info.high_bidder);

		sprintf(buf, "A strange white rift opens up as %s falls into your posession.\r\n", auction_info.item->short_description);
		send_to_char(auction_info.high_bidder, "%s", buf);

		auction_info.item = NULL;
		auction_info.owner = NULL;
		auction_info.high_bidder = NULL;
		auction_info.current_bid = 0;
		auction_info.status = 0;
		auction_info.units_held = 0;
		return;
	}

	if (auction_info.status == AUCTION_LENGTH -1) {

		sprintf(buf, "@D%s going twice for %d resource units.\r\n", auction_info.item->short_description, auction_info.current_bid);
		auction_channel(buf);
		return;
	}

	if (auction_info.status == AUCTION_LENGTH -2) {

		if (auction_info.current_bid == 0) {

			sprintf(buf, "@DNo bids on %s, item has been returned to seller.\r\n", auction_info.item->short_description);
			auction_channel(buf);
			if (foundown == TRUE) {
				obj_from_room(auction_info.item);
				obj_to_char(auction_info.item, auction_info.owner);
				sprintf(buf, "@D%s has been returned to your inventory.\r\n", auction_info.item->short_description);
				send_to_char(auction_info.owner, "%s", buf);
			}
			/* TODO: MAKE SURE THIS GOES TO DONATIONS */
			else
				extract_obj(auction_info.item);

			auction_info.item = NULL;
			auction_info.owner = NULL;
			auction_info.high_bidder = NULL;
			auction_info.current_bid = 0;
			auction_info.status = 0;
			auction_info.units_held = 0;
			return;
		}

		sprintf(buf, "@D%s is going once for %d resource units.\r\n", auction_info.item->short_description,
			auction_info.current_bid);
		auction_channel(buf);
		return;
	}

	if (auction_info.status == AUCTION_LENGTH -3) {
		if (auction_info.current_bid == 0) {
			sprintf(buf, "@DStill accepting bids on %s for %d resource units.\r\n", auction_info.item->short_description, auction_info.minimum_bid);
			auction_channel(buf);
		}
		else {
			sprintf(buf, "@DThe minimum bid on %s is %d resource units.\r\n", auction_info.item->short_description, auction_info.current_bid);
			auction_channel(buf);
		}
	}
	
	if (auction_info.status == AUCTION_LENGTH -4) {
		if (auction_info.current_bid == 0) {
			sprintf(buf, "@DStill accepting bids on %s for %d resource units.\r\n", auction_info.item->short_description, auction_info.minimum_bid);
			auction_channel(buf);
		}
		else {
			sprintf(buf, "@DThe minimum bid on %s is %d resource units.\r\n", auction_info.item->short_description, auction_info.current_bid);
			auction_channel(buf);
		}
	}

	if (auction_info.status == AUCTION_LENGTH -5) {
		if (auction_info.current_bid == 0) {
			sprintf(buf, "@DStill accepting bids on %s for %d resource units.\r\n", auction_info.item->short_description, auction_info.minimum_bid);
			auction_channel(buf);
		}
		else {
			sprintf(buf, "@DThe minimum bid on %s is %d resource units.\r\n", auction_info.item->short_description, auction_info.current_bid);
			auction_channel(buf);
		}
	}

	return;
}

/* The medium for displaying the bufs for market system. - Gahan 09/11 */

void auction_channel(char *msg)
{
	char buf[MAX_INPUT_LENGTH];
	struct descriptor_data *d;

	sprintf(buf, "\r\n@G[ @WMARKET@G ]@D %s@D", msg);

	for (d = descriptor_list; d != NULL; d = d->next)
	{
		struct char_data *victim;
		
		victim = d->original ? d->original : d->character;

		if (d->connected == CON_PLAYING && !PRF_FLAGGED(d->character, PRF_NOMARKET))
			send_to_char(victim, "%s", buf);
	}
	return;
}

/* The market info display for market system - Gahan 09/11 */

void show_obj_stats(struct char_data *ch, struct obj_data *obj)
{
    int i;
    int found;
    size_t len;

    if (obj) {

        char bitbuf[MAX_STRING_LENGTH];

        sprinttype(GET_OBJ_TYPE(obj), item_types, bitbuf, sizeof(bitbuf));
        send_to_char(ch, "\r\nObject '%s', Item type: %s\r\n", obj->short_description, bitbuf);

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

        /* todo: this might need modification because the remort flag might not always be last*/
        if (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY))
            send_to_char(ch, ": x%d\r\n", GET_OBJ_REMORT(obj));
        else
            send_to_char(ch, "\r\n");

        /* todo: remove the rent info if we go with free rent*/
		if (!IS_COMP_NOTHING(obj))
			send_to_char(ch, "Can be salvaged into: [%s] %s\r\n\r\n", GET_ITEM_COMPOSITION(obj), GET_ITEM_SUBCOMPOSITION(obj));
		if (IS_COMP_NOTHING(obj))
			send_to_char(ch, "This item cannot be salvaged.\r\n\r\n");
        send_to_char(ch, "Weight: %d, Value: %d, Min Level: %d\r\n",
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
//        send_to_char(ch, "\r\n");

        if ((IS_SET(obj->obj_flags.type_flag, ITEM_WEAPON)) || (IS_SET(obj->obj_flags.type_flag, ITEM_GUN))) {
            if (obj->obj_wpnpsi.which_psionic) {
                send_to_char(ch, "\n\rHas the following extra effect:");
                sprinttype(obj->obj_wpnpsi.which_psionic, wpn_psionics, bitbuf, sizeof(bitbuf));
                send_to_char(ch, "%s Level %d %s", found++ ? "" : "", obj->obj_wpnpsi.skill_level, bitbuf);
            }
        }

		if (obj->ex_description != NULL) {
			send_to_char(ch, "Long description:\r\n\r\n");
			send_to_char(ch, "%s", obj->ex_description->description);
		}

		if (auction_info.current_bid > 0)
			send_to_char(ch, "\r\nThe current bid on this item is: [ %d Resource Unit(s) ]\r\n", auction_info.current_bid);
		else if (auction_info.minimum_bid > 0)
			send_to_char(ch, "\r\nThe minimum bid on this item is: [ %d Resource Unit(s) ]\r\n", auction_info.minimum_bid);
		else
			send_to_char(ch, "\r\n");

		if (ch == auction_info.high_bidder) 
			send_to_char(ch, "The highest bidder is: You\r\n");
		else if (auction_info.high_bidder == NULL)
			send_to_char(ch, "There has been no bidders on this item yet.\r\n");
		else
			send_to_char(ch, "The highest bidder is: %s\r\n", (*auction_info.high_bidder).player.name);
    }

}
