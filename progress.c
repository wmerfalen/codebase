//////////////////////////////////////////////////////////////////////////////////
// This progress.c file ha been written from scratch by Gahan.					//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the class progression system for CyberASSAULT as the remort system   //
// currently in place is outdated and needed a facelift               			//
//////////////////////////////////////////////////////////////////////////////////

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "screen.h"
#include "constants.h"
#include "act.h"
#include "class.h"
#include "config.h"
#include "bionics.h"
#include "drugs.h"
#include "skilltree.h"

// Prototype defines go here:
extern const struct progression_info_type progression_info[17];
void progress_mercenary(struct char_data *ch, char *arg);
void progress_crazy(struct char_data *ch, char *arg);
void progress_stalker(struct char_data *ch, char *arg);
void progress_borg(struct char_data *ch, char *arg);
void progress_predator(struct char_data *ch, char *arg);
void progress_highlander(struct char_data *ch, char *arg);
void progress_caller(struct char_data *ch, char *arg);

void post_progress(struct char_data *ch);
// Structure for progressions
const struct progression_info_type progression_info[17] = {
//     QP, FAME, LEVEL
	{  0,   0,   LVL_IMMORT-1 }, // 0
	{  0,   0,   LVL_IMMORT-1 }, // 1
	{  10,  25,  LVL_IMMORT-1 }, // 2
	{  20,  50,  LVL_IMMORT-1 }, // 3
	{  30,  75,  LVL_IMMORT-1 }, // 4
	{  40, 100,  LVL_IMMORT-1 }, // 5
	{  50, 125,  LVL_IMMORT-1 }, // 6
	{  60, 150,  LVL_IMMORT-1 }, // 7
	{  70, 175,  LVL_IMMORT-1 }, // 8
	{  80, 200,  LVL_IMMORT-1 }, // 9
	{  90, 225,  LVL_IMMORT-1 }, // 10
	{ 100, 250,  LVL_IMMORT-1 }, // 11
	{ 110, 275,  LVL_IMMORT-1 }, // 12
	{ 120, 300,  LVL_IMMORT-1 }, // 13
	{ 130, 350,  LVL_IMMORT-1 }, // 14
	{ 140, 400,  LVL_IMMORT-1 }, // 15
	{ 999, 999,  LVL_IMMORT-1 }  // 16
};

// Class command, shows the classes
ACMD(do_class)
{
	char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "Which class tree would you like to see?\r\n");
		send_to_char(ch, "Your options are, Mercenary, Crazy, Stalker, Borg, Caller, Predator, Highlander.\r\n");
		return;
	}

	if (!strcmp(arg, "mercenary"))
		send_to_char(ch, "%s", mercenary_classes);
	else if (!strcmp(arg, "crazy"))
		send_to_char(ch, "%s", crazy_classes);
	else if (!strcmp(arg, "stalker"))
		send_to_char(ch, "%s", stalker_classes);
	else if (!strcmp(arg, "borg"))
		send_to_char(ch, "%s", borg_classes);
	else if (!strcmp(arg, "caller"))
		send_to_char(ch, "%s", caller_classes);
	else if (!strcmp(arg, "predator"))
		send_to_char(ch, "%s", predator_classes);
	else if (!strcmp(arg, "highlander"))
		send_to_char(ch, "%s", highlander_classes);
	else {
		send_to_char(ch, "That is not a valid core class.\r\n");
		send_to_char(ch, "Your options are, Mercenary, Crazy, Stalker, Borg, Caller, Predator, Highlander.\r\n");
		return;
	}
}
// Command to increase your stats with progression points
ACMD(do_increase)
{
	char arg[MAX_INPUT_LENGTH];
	bool MOD = FALSE;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !ch->desc)
		return;
	if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
		send_to_char(ch, "You...can't...move!\r\n");
		return;
	}
	if (!*arg) {
		if (ch->player_specials->saved.attrib_add > 0) {
			send_to_char(ch, "@WWhat attribute would you like to increase?\r\n");
			send_to_char(ch, "@WThe choices are:\r\n");
			send_to_char(ch, "@CStr @W- Add 1 Strength\r\n");
			send_to_char(ch, "@CDex @W- Add 1 Dexterity\r\n");
			send_to_char(ch, "@CCon @W- Add 1 Constitution\r\n");
			send_to_char(ch, "@CPcn @W- Add 1 Perception\r\n");
			send_to_char(ch, "@CInt @W- Add 1 Intelligence\r\n");
			send_to_char(ch, "@CCha @W- Add 1 Charisma\r\n");
			send_to_char(ch, "@CHp  @W- Add 30 Max Hit points\r\n");
			send_to_char(ch, "@CPsi @W- Add 30 Max Psionic points\r\n");
			send_to_char(ch, "@CMove@W- Add 30 Movement points\r\n@n");
			return;
		}
		else {
			send_to_char(ch, "@WYou do not have any progression points left to use, sorry.@n\r\n");
			return;
		}
	}
	else {
		if (ch->player_specials->saved.attrib_add > 0) {
			if (!strcmp(arg, "str") && GET_STR(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.str++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "dex") && GET_DEX(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.dex++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "con") && GET_CON(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.con++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "int") && GET_INT(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.intel++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "pcn") && GET_PCN(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.pcn++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "cha") && GET_CHA(ch) < 25) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->real_abils.cha++;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "hp")) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->points.max_hit += 30;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "psi")) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->points.max_psi += 30;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else if (!strcmp(arg, "move")) {
				ch->player_specials->saved.attrib_add -= 1;
				ch->points.max_move += 30;
				send_to_char(ch, "You spend one of your progression points.\r\n");
				MOD = TRUE;
			}
			else {
				send_to_char(ch, "@WThat is not something you can practice, sorry.@n\r\n");
				return;
			}
		}
		else {
			send_to_char(ch, "@WYou do not have any progression points left to use, sorry.@n\r\n");
			return;
		}
	}
	if (MOD == TRUE)
		save_char(ch);
		affect_total(ch);
}
// Progress function, obviously the heart of this code
ACMD(do_progress)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	int pos;
	int i;
	bool found = FALSE;

	one_argument(argument, arg);

	if (IS_NPC(ch) || !ch->desc)
        return;

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }
	
	if (!*arg) {
		send_to_char(ch, "@W-----------[ @GCharacter Progression@W ]-----------@n\r\n\r\n");
		send_to_char(ch, "@W You are currently on progression @G%d@W.@n\r\n", GET_PROGRESS(ch));
		send_to_char(ch, "@W You are currently ranked at @GTier: %d@W.@n\r\n", GET_TIER(ch));
		if (GET_CLASS2(ch) > 0)
			send_to_char(ch, "@W Your multiclass is @G%d@W, on Tier @G%d@W.@n\r\n", GET_CLASS2(ch), GET_TIER2(ch));
		send_to_char(ch, "@W You have @G%d@W progression points remaining to increase your stats.@n\r\n", ch->player_specials->saved.attrib_add);
		send_to_char(ch, "@W To continue to progression @G%d@W, you will need:@n\r\n\r\n", GET_PROGRESS(ch)+1);
		send_to_char(ch, "@W Fame: @G%d@W   Questpoints:@G %d@W   Level:@G %d@n\r\n\r\n",
			progression_info[GET_PROGRESS(ch) +1].fame, progression_info[GET_PROGRESS(ch) +1].questpoints, progression_info[GET_PROGRESS(ch) +1].level);
		send_to_char(ch, "@W And you currently have:@n\r\n\r\n");
		if (GET_FAME(ch) >= progression_info[GET_PROGRESS(ch) +1].fame)
			send_to_char(ch, "@W Fame:           [ @G%5d@W ]@n\r\n", GET_FAME(ch));
		else
			send_to_char(ch, "@W Fame:           [ @R%5d@W ] @RYou need %d more fame.@n\r\n", GET_FAME(ch), progression_info[GET_PROGRESS(ch) +1].fame - GET_FAME(ch));
	
		if (GET_QUESTPOINTS(ch) >= progression_info[GET_PROGRESS(ch) +1].questpoints)
			send_to_char(ch, "@W Questpoints:    [ @G%5d@W ]@n\r\n", GET_QUESTPOINTS(ch));
		else
			send_to_char(ch, "@W Questpoints:    [ @R%5d@W ] @RYou need %d more questpoints.@n\r\n", GET_QUESTPOINTS(ch), progression_info[GET_PROGRESS(ch) +1].questpoints - GET_QUESTPOINTS(ch));
	
		if (GET_LEVEL(ch) >= progression_info[GET_PROGRESS(ch) +1].level)
			send_to_char(ch, "@W Level:          [ @G%5d@W ]@n\r\n", GET_LEVEL(ch));
		else
			send_to_char(ch, "@W Level:          [ @R%5d@W ] @RYou need %d more levels.@n\r\n", GET_LEVEL(ch), progression_info[GET_PROGRESS(ch) +1].level - GET_LEVEL(ch));
		if (GET_LEVEL(ch) == (LVL_IMMORT -1)) {
			if (GET_EXP(ch) >= (10000000 * GET_PROGRESS(ch)))
				send_to_char(ch, "@W Experience:     [ @G%10d@w ]@n\r\n", GET_EXP(ch));
			else
				send_to_char(ch, "@W Experience:     [ @R%10d@w ] @RYou need %d more experience.@n\r\n", GET_EXP(ch), (10000000 * GET_PROGRESS(ch)) - GET_EXP(ch));
		}
		
		send_to_char(ch, "\r\n@W-----------------------------------------------@n\r\n\r\n");
		send_to_char(ch, "@W Type '@Yprogress list@W' to see the information about your class.@n\r\n");
		return;
	}
	else {
		if (!strcmp(arg, "list")) {
			if (IS_BORG(ch))
				send_to_char(ch, "%s", borg_classes);
			if (IS_MERCENARY(ch))
				send_to_char(ch, "%s", mercenary_classes);
			if (IS_CRAZY(ch))
				send_to_char(ch, "%s", crazy_classes);
			if (IS_CALLER(ch))
				send_to_char(ch, "%s", caller_classes);
			if (IS_STALKER(ch))
				send_to_char(ch, "%s", stalker_classes);
			if (IS_PREDATOR(ch))
				send_to_char(ch, "%s", predator_classes);
			if (IS_HIGHLANDER(ch))
				send_to_char(ch, "%s", highlander_classes);
			return;
		}
		// check for fame requirements
		if (GET_FAME(ch) < progression_info[GET_PROGRESS(ch)+1].fame) {
			send_to_char(ch, "You have not met the @Gfame@n requirements to progress your character.\r\n");
			return;
		}
		// check exp requirements
		if (GET_EXP(ch) < (10000000 * GET_PROGRESS(ch))) {
			send_to_char(ch, "You have not met the @Gexperience@n requirements to progress your character.\r\n");
			return;
		}
		// check for questpoint requirements
		else if (GET_QUESTPOINTS(ch) < progression_info[GET_PROGRESS(ch)+1].questpoints) {
			send_to_char(ch, "You have not met the @Gquest point@n requirements to progress your character.\r\n");
			return;
		}
		// check for level requirements
		else if (GET_LEVEL(ch) < progression_info[GET_PROGRESS(ch)+1].level) {
			send_to_char(ch, "You have not met the @Glevel@n requirements to progress your character.\r\n");
			return;
		}
		else if (auction_info.high_bidder == ch || auction_info.owner == ch) {
			send_to_char(ch, "You are involved in a market session, please wait for it to end before you do this.\r\n");
			return;
		}
		// Passed all the requirements, now...
		else {
			// You must find the appropriate guildmaster in order to progress
			if (IS_MERCENARY(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_MERCCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_MERCCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_MERCFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_CRAZY(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CRAZYCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CRAZYCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CRAZYFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_BORG(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_BORGCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_BORGCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_BORGFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_STALKER(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_STALKERCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_STALKERCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_STALKERFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_CALLER(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CALLERCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CALLERCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_CALLERFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_PREDATOR(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_PREDATORCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_PREDATORCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_PREDATORFOCUS))
							found = TRUE;
					}
				}
			}
			else if (IS_HIGHLANDER(ch)) {
				if (GET_PROGRESS(ch) <= 1) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_HIGHLANDERCORE))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) <= 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_HIGHLANDERCALLING))
							found = TRUE;
					}
				}
				else if (GET_PROGRESS(ch) > 6) {
					for (vict = world[IN_ROOM(ch)].people; vict && !found; vict = vict->next_in_room) {
						if (MOB_FLAGGED(vict, MOB_HIGHLANDERFOCUS))
							found = TRUE;
					}
				}
			}

			if (found == FALSE) {
				send_to_char(ch, "You need to find a guild that can progress you.\r\n");
				return;
			}
			// Remove all psionics
			if (ch->affected) {
				while (ch->affected)
					affect_remove(ch, ch->affected);
			}
		//dossier is taken care of during save procedures
			if (PLR_FLAGGED(ch, PLR_PK))
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_PK);

		// Remove all drugs
			if (IS_ON_DRUGS(ch)) {
				while (ch->drugs_used)
					drug_remove(ch, ch->drugs_used);
			}

		// Remove eq
			for (pos = 0; pos < NUM_WEARS; pos++) {
				if (ch->equipment[pos])
					obj_to_char(unequip_char(ch, pos, FALSE), ch);
			}

			// Remove Bionic affects
			bionics_unaffect(ch);
			//new_bionics_unaffect(ch, TRUE);

			// remove bionics
			for (i = 0; i < MAX_BIONIC; i++)
				if (BIONIC_LEVEL(ch, i) != BIONIC_REMORT)
					ch->char_specials.saved.bionics[i] =  BIONIC_NONE;

			// All pre-routine stuff has been handled, lets get them going on their journey
			
			if (IS_BORG(ch))
				progress_borg(ch, arg);
			if (IS_MERCENARY(ch))
				progress_mercenary(ch, arg);
			if (IS_CRAZY(ch))
				progress_crazy(ch, arg);
			if (IS_CALLER(ch))
				progress_caller(ch, arg);
			if (IS_STALKER(ch))
				progress_stalker(ch, arg);
			if (IS_PREDATOR(ch))
				progress_predator(ch, arg);
			if (IS_HIGHLANDER(ch))
				progress_highlander(ch, arg);
		}
	}
}

void progress_mercenary(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class = 0;
	int chosen_class = 0;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", mercenary_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_MERCENARY &&
		(load_result = parse_class(arg, TRUE)) != CLASS_MARKSMAN &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BRAWLER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BOUNTYHUNTER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ENFORCER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_COVERT &&
		(load_result = parse_class(arg, TRUE)) != CLASS_PRIEST ) {
		send_to_char(ch, "%s", mercenary_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_PRIEST || GET_CLASS(ch) == CLASS_COVERT || GET_CLASS(ch) == CLASS_MARKSMAN)
			compare_class = CLASS_MARKSMAN;
		if (GET_CLASS(ch) == CLASS_BOUNTYHUNTER || GET_CLASS(ch) == CLASS_ENFORCER || GET_CLASS(ch) == CLASS_BRAWLER)
			compare_class = CLASS_BRAWLER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_BOUNTYHUNTER || (load_result = parse_class(arg, TRUE)) == CLASS_ENFORCER)
			chosen_class = CLASS_BRAWLER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_COVERT || (load_result = parse_class(arg, TRUE)) == CLASS_PRIEST)
			chosen_class = CLASS_MARKSMAN;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_PRIEST || (load_result = parse_class(arg, TRUE)) == CLASS_COVERT || (load_result = parse_class(arg, TRUE)) == CLASS_ENFORCER || (load_result = parse_class(arg, TRUE)) == CLASS_BOUNTYHUNTER) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_MERCENARY) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_MERCENARY) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void progress_crazy(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", crazy_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_CRAZY &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ANARCHIST &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SUMMONER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_PSIONICIST &&
		(load_result = parse_class(arg, TRUE)) != CLASS_PSYCHOTIC &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ELEMENTALIST &&
		(load_result = parse_class(arg, TRUE)) != CLASS_TECHNOMANCER ) {
		send_to_char(ch, "%s", crazy_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_PSIONICIST || GET_CLASS(ch) == CLASS_PSYCHOTIC || GET_CLASS(ch) == CLASS_ANARCHIST)
			compare_class = CLASS_ANARCHIST;
		if (GET_CLASS(ch) == CLASS_ELEMENTALIST || GET_CLASS(ch) == CLASS_TECHNOMANCER || GET_CLASS(ch) == CLASS_SUMMONER)
			compare_class = CLASS_SUMMONER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_PSIONICIST || (load_result = parse_class(arg, TRUE)) == CLASS_PSYCHOTIC)
			chosen_class = CLASS_ANARCHIST;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_ELEMENTALIST || (load_result = parse_class(arg, TRUE)) == CLASS_TECHNOMANCER)
			chosen_class = CLASS_SUMMONER;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_TECHNOMANCER || (load_result = parse_class(arg, TRUE)) == CLASS_ELEMENTALIST || (load_result = parse_class(arg, TRUE)) == CLASS_PSYCHOTIC || (load_result = parse_class(arg, TRUE)) == CLASS_PSIONICIST) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_CRAZY) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_CRAZY) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void progress_stalker(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", stalker_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_STALKER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SNIPER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SABOTEUR &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ASSASSIN &&
		(load_result = parse_class(arg, TRUE)) != CLASS_STRIKER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_TERRORIST &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SPY ) {
		send_to_char(ch, "%s", stalker_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_ASSASSIN || GET_CLASS(ch) == CLASS_STRIKER || GET_CLASS(ch) == CLASS_SNIPER)
			compare_class = CLASS_SABOTEUR;
		if (GET_CLASS(ch) == CLASS_TERRORIST || GET_CLASS(ch) == CLASS_SPY || GET_CLASS(ch) == CLASS_SABOTEUR)
			compare_class = CLASS_SNIPER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_ASSASSIN || (load_result = parse_class(arg, TRUE)) == CLASS_STRIKER)
			chosen_class = CLASS_SABOTEUR;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_TERRORIST || (load_result = parse_class(arg, TRUE)) == CLASS_SPY)
			chosen_class = CLASS_SNIPER;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_SPY || (load_result = parse_class(arg, TRUE)) == CLASS_TERRORIST || (load_result = parse_class(arg, TRUE)) == CLASS_STRIKER || (load_result = parse_class(arg, TRUE)) == CLASS_ASSASSIN) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_STALKER) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_STALKER) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void progress_borg(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", borg_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_BORG &&
		(load_result = parse_class(arg, TRUE)) != CLASS_DRONE &&
		(load_result = parse_class(arg, TRUE)) != CLASS_DESTROYER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ASSIMILATOR &&
		(load_result = parse_class(arg, TRUE)) != CLASS_GUARDIAN &&
		(load_result = parse_class(arg, TRUE)) != CLASS_JUGGERNAUT &&
		(load_result = parse_class(arg, TRUE)) != CLASS_PANZER ) {
		send_to_char(ch, "%s", borg_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_ASSIMILATOR || GET_CLASS(ch) == CLASS_GUARDIAN || GET_CLASS(ch) == CLASS_DRONE)
			compare_class = CLASS_DRONE;
		if (GET_CLASS(ch) == CLASS_JUGGERNAUT || GET_CLASS(ch) == CLASS_PANZER || GET_CLASS(ch) == CLASS_DESTROYER)
			compare_class = CLASS_DESTROYER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_ASSIMILATOR || (load_result = parse_class(arg, TRUE)) == CLASS_GUARDIAN)
			chosen_class = CLASS_DRONE;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_JUGGERNAUT || (load_result = parse_class(arg, TRUE)) == CLASS_PANZER)
			chosen_class = CLASS_DESTROYER;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_PANZER || (load_result = parse_class(arg, TRUE)) == CLASS_JUGGERNAUT || (load_result = parse_class(arg, TRUE)) == CLASS_GUARDIAN || (load_result = parse_class(arg, TRUE)) == CLASS_ASSIMILATOR) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_BORG) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_BORG) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void progress_predator(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", predator_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_PREDATOR &&
		(load_result = parse_class(arg, TRUE)) != CLASS_YOUNGBLOOD &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BADBLOOD &&
		(load_result = parse_class(arg, TRUE)) != CLASS_WEAPONMASTER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ELDER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_PREDALIEN &&
		(load_result = parse_class(arg, TRUE)) != CLASS_MECH ) {
		send_to_char(ch, "%s", predator_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_WEAPONMASTER || GET_CLASS(ch) == CLASS_ELDER || GET_CLASS(ch) == CLASS_YOUNGBLOOD)
			compare_class = CLASS_YOUNGBLOOD;
		if (GET_CLASS(ch) == CLASS_PREDALIEN || GET_CLASS(ch) == CLASS_MECH || GET_CLASS(ch) == CLASS_BADBLOOD)
			compare_class = CLASS_BADBLOOD;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_WEAPONMASTER || (load_result = parse_class(arg, TRUE)) == CLASS_ELDER)
			chosen_class = CLASS_YOUNGBLOOD;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_PREDALIEN || (load_result = parse_class(arg, TRUE)) == CLASS_MECH)
			chosen_class = CLASS_BADBLOOD;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_MECH || (load_result = parse_class(arg, TRUE)) == CLASS_PREDALIEN || (load_result = parse_class(arg, TRUE)) == CLASS_ELDER || (load_result = parse_class(arg, TRUE)) == CLASS_WEAPONMASTER) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_PREDATOR) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_PREDATOR) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void progress_highlander(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", highlander_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_HIGHLANDER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_KNIGHT &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BLADESINGER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BLADEMASTER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_REAVER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_ARCANE &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BARD ) {
		send_to_char(ch, "%s", highlander_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_BLADEMASTER || GET_CLASS(ch) == CLASS_REAVER || GET_CLASS(ch) == CLASS_KNIGHT)
			compare_class = CLASS_KNIGHT;
		if (GET_CLASS(ch) == CLASS_ARCANE || GET_CLASS(ch) == CLASS_BARD || GET_CLASS(ch) == CLASS_BLADESINGER)
			compare_class = CLASS_BLADESINGER;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_BLADEMASTER || (load_result = parse_class(arg, TRUE)) == CLASS_REAVER)
			chosen_class = CLASS_KNIGHT;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_ARCANE || (load_result = parse_class(arg, TRUE)) == CLASS_BARD)
			chosen_class = CLASS_BLADESINGER;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_BARD || (load_result = parse_class(arg, TRUE)) == CLASS_ARCANE || (load_result = parse_class(arg, TRUE)) == CLASS_REAVER || (load_result = parse_class(arg, TRUE)) == CLASS_BLADEMASTER) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					GET_NUM_ESSENCES(ch) = 100;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						GET_NUM_ESSENCES(ch) = 100;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						GET_NUM_ESSENCES(ch) = 100;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						GET_NUM_ESSENCES(ch) = 100;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_HIGHLANDER) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_HIGHLANDER) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				GET_NUM_ESSENCES(ch) = 100;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			GET_NUM_ESSENCES(ch) = 100;
			post_progress(ch);
			return;
		}
	}
}

void progress_caller(struct char_data *ch, char *arg)
{
	int load_result;
	int compare_class;
	int chosen_class;

	if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
		send_to_char(ch, "%s", caller_classes);
		send_to_char(ch, "\r\n@RThat's not a valid class.  Please type out the full class name.@n\r\n");
		return;
	}
			
	if ((load_result = parse_class(arg, TRUE)) != CLASS_CALLER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_FERAL &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SURVIVALIST &&
		(load_result = parse_class(arg, TRUE)) != CLASS_HUNTER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_BEASTMASTER &&
		(load_result = parse_class(arg, TRUE)) != CLASS_VANGUARD &&
		(load_result = parse_class(arg, TRUE)) != CLASS_SHAMAN ) {
		send_to_char(ch, "%s", caller_classes);
		send_to_char(ch, "@RThat's not a valid selection, select a class within your core class.@n\r\n");
		return;
	}

	else {

		if (GET_CLASS(ch) == CLASS_HUNTER || GET_CLASS(ch) == CLASS_BEASTMASTER || GET_CLASS(ch) == CLASS_FERAL)
			compare_class = CLASS_FERAL;
		if (GET_CLASS(ch) == CLASS_VANGUARD || GET_CLASS(ch) == CLASS_SHAMAN || GET_CLASS(ch) == CLASS_SURVIVALIST)
			compare_class = CLASS_SURVIVALIST;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_HUNTER || (load_result = parse_class(arg, TRUE)) == CLASS_BEASTMASTER)
			chosen_class = CLASS_FERAL;
		if ((load_result = parse_class(arg, TRUE)) == CLASS_VANGUARD || (load_result = parse_class(arg, TRUE)) == CLASS_SHAMAN)
			chosen_class = CLASS_SURVIVALIST;

		if ((load_result = parse_class(arg, TRUE)) == CLASS_SHAMAN || (load_result = parse_class(arg, TRUE)) == CLASS_VANGUARD || (load_result = parse_class(arg, TRUE)) == CLASS_BEASTMASTER || (load_result = parse_class(arg, TRUE)) == CLASS_HUNTER) {
					
			if (ch->player_specials->saved.progress < 6) {
				send_to_char(ch, "@RThat class is not available to your level of progression, yet.@n\r\n");
				return;
			}
			else if (chosen_class != compare_class && compare_class != 0) {
				send_to_char(ch, "@RYou need to choose a focus class within the same calling.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch) && GET_TIER(ch) < 5) {
				send_to_char(ch, "@RYou need to achieve Tier 5 in your currently class to multi-class.@n\r\n");
				return;
			}
			else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) >= 5) {
				send_to_char(ch, "@RYou have already completed your progression through your focus class, please choose your multi-class.@n\r\n");
				return;
			}
			else {
				if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch)) {
					GET_TIER(ch) += 1;
					GET_CLASS(ch) = load_result;
					GET_PROGRESS(ch) += 1;
					post_progress(ch);
					return;
				}
				else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
					if (compare_class == 0) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) == 6) {
						GET_TIER(ch) = 1;
						GET_CLASS(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else if (compare_class != 0 && GET_TIER(ch) == 5 && GET_PROGRESS(ch) >= 11) {
						GET_TIER2(ch) += 1;
						GET_CLASS2(ch) = load_result;
						GET_PROGRESS(ch) += 1;
						post_progress(ch);
						return;
					}
					else {
						send_to_char(ch, "[ERROR: 01] - Please report this error code to Gahan.\r\n"); 
						return;
					}
				}
				else {
					send_to_char(ch, "[ERROR: 02] - Please report this error code to Gahan.\r\n"); 
					return;
				}
			}
		}
		else if ((load_result = parse_class(arg, TRUE)) == CLASS_CALLER) {
			send_to_char(ch, "@RYou have completed your progression through your core class, please choose a calling class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) == GET_CLASS(ch) && GET_TIER(ch) == 5) {
			send_to_char(ch, "@RYou have completed your progression through your calling class, please choose a focus class.@n\r\n");
			return;
		}
		else if ((load_result = parse_class(arg, TRUE)) != GET_CLASS(ch)) {
			
			if (GET_CLASS(ch) == CLASS_CALLER) {
				GET_TIER(ch) = 1;
				GET_CLASS(ch) = load_result;
				GET_PROGRESS(ch) += 1;
				post_progress(ch);
				return;
			}
			else {
				send_to_char(ch, "@RYou can only level one calling class at a time.@n\r\n");
				return;
			}
		}
		else {
			GET_TIER(ch) += 1;
			GET_CLASS(ch) = load_result;
			GET_PROGRESS(ch) += 1;
			post_progress(ch);
			return;
		}
	}
}

void post_progress(struct char_data *ch)
{
	int i;
	int j;
	struct obj_data *device;
    struct obj_data *loaded_object;

    for (j = 1; j < NUM_BODY_PARTS; j++)
        if ((device = BIONIC_DEVICE(ch, j)))
			if ((device = remove_bionic(ch, j)))
				obj_to_char(device, ch);
				
	// Set base stats
	set_basestats(ch);
	// Set Actual stats
	ch->aff_abils = ch->real_abils;
	// Add some progression points
	ch->player_specials->saved.attrib_add += ((GET_PROGRESS(ch) + 1) * 2) + 8;
    GET_LEVEL(ch) = 1;
    GET_EXP(ch) = 1;
    // set their hpv back to 0
    ch->points.max_hit = 0;
    ch->points.max_psi = 0;
    ch->points.max_move = 0;
    ch->points.hitroll = 0;
    ch->points.damroll = 0;
    ch->points.armor = 0;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, DRUNK) = -1;
	GET_QUESTPOINTS(ch) -= progression_info[GET_PROGRESS(ch)].questpoints;
	GET_EXP(ch) = 0;
    // remove all skills - OK to use MAX_SKILLS here
    // todo: check if this should be <= like other code
    for (i = 0; i < MAX_SKILLS; i++)
        SET_SKILL(ch, i, 0)

    // remove all psionics - OK to use MAX_PSIONICS here
    // todo: check if this should be <= like other code
    for (i = 0; i < MAX_PSIONICS; i++)
        SET_PSIONIC(ch, i, 0)

    GET_AGE_MOD(ch) = 0;
    ch->player_specials->saved.practices = 25 + (GET_PROGRESS(ch) * 5);
    post_init_char(ch);
    GET_REMORTS(ch) += 1;
    save_char(ch);
	bionics_save(ch);

	mudlog(BRF, 0, TRUE, "@WStorms begin to gather strength as %s becomes one of the reborn.@n@g", GET_NAME(ch));

	
	send_to_char(ch, "@YYou have been awarded a progression reward item. Check your inventory!\r\n");
	
	if (GET_PROGRESS(ch) == 2) {
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X2), REAL);
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 3)	{
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X3), REAL); 
		obj_to_char(loaded_object, ch);
    }
	if (GET_PROGRESS(ch) == 4) {
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X4), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if ((GET_PROGRESS(ch) == 4) && IS_HIGHLANDER(ch)) {
		loaded_object = read_object(real_object(PROGRESSION_GEAR_HIGHLANDER), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if ((GET_PROGRESS(ch) == 4) && IS_PREDATOR(ch)) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_PREDATOR), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 5) {
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X5), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 6) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X6), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 7) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X7), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 8) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X8), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 9) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X9), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 10) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X10), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 11) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X11), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 12) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X12), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 13) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X13), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 14) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X14), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 15) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X15), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 16) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X16), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 17) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X17), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 18) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X18), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 19) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X19), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 20) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X20), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 21) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X21), REAL); 
		obj_to_char(loaded_object, ch);
	}
	if (GET_PROGRESS(ch) == 22) {	
		loaded_object = read_object(real_object(PROGRESSION_GEAR_X22), REAL); 
		obj_to_char(loaded_object, ch);
	}
	return;
}

const char *mercenary_classes =
"\r\n@WMercenary Class Progression:\r\n\r\n"
"   @GCore          @YCalling           @BFocus@n\r\n\r\n"
"@W                           +---@BPriest         @n\r\n"
"@W            +---@YMarksman@W---|                @n\r\n"
"@W            |              +---@BCovert         @n\r\n"
"@GMercenary@W---|                                 @n\r\n"
"@W            |              +---@BBountyHunter    @n\r\n"
"@W            +---@YBrawler@W----|                @n\r\n"
"@W                           +---@BEnforcer       @n\r\n"
"@W\r\nCore class: @GMercenary@n\r\n"
"@WCalling classes for Mercenary: @YMarksman, Brawler@n\r\n"
"@WFocus Classes for Marksman: @BPriest, Covert@n\r\n"
"@WFocus Classes for Brawler: @BBountyHunter, Enforcer@n\r\n";

const char *crazy_classes =
"\r\n@WCrazy Class Progression:\r\n\r\n"
"   @GCore          @YCalling           @BFocus@n\r\n\r\n"
"@W                           +---@BPsionicist    @n\r\n"
"@W            +---@YAnarchist@W--|                @n\r\n"
"@W            |              +---@BPsychotic     @n\r\n"
"@GCrazy@W-------|                                @n\r\n"
"@W            |              +---@BElementalist   @n\r\n"
"@W            +---@YSummoner@W---|                @n\r\n"
"@W                           +---@BTechnomancer         @n\r\n"
"@W\r\nCore class: @GCrazy@n\r\n"
"@WCalling classes for Crazy: @YAnarchist, Summoner@n\r\n"
"@WFocus Classes for Anarchist: @BPsionicist, Psychotic@n\r\n"
"@WFocus Classes for Summoner: @BElementalist, Technomancer@n\r\n";

const char *stalker_classes =
"\r\n@WStalker Class Progression:\r\n\r\n"
"@W                           +---@BTerrorist    @n\r\n"
"@W            +---@YSniper@W-----|                @n\r\n"
"@W            |              +---@BSpy     @n\r\n"
"@GStalker@W-----|                                @n\r\n"
"@W            |              +---@BStriker   @n\r\n"
"@W            +---@YSaboteur@W---|                @n\r\n"
"@W                           +---@BAssassin         @n\r\n"
"@W\r\nCore class: @GStalker@n\r\n"
"@WCalling classes for Stalker: @YSniper, Saboteur@n\r\n"
"@WFocus Classes for Sniper: @BTerrorist, Spy@n\r\n"
"@WFocus Classes for Saboteur: @BStriker, Assassin@n\r\n";

const char *borg_classes =
"\r\n@WBorg Class Progression:\r\n\r\n"
"@W                           +---@BAssimilator @n\r\n"
"@W            +---@YDrone@W------|                @n\r\n"
"@W            |              +---@BGuardian    @n\r\n"
"@GBorg@W--------|                                @n\r\n"
"@W            |              +---@BJuggernaut  @n\r\n"
"@W            +---@YDestroyer@W--|                @n\r\n"
"@W                           +---@BPanzer      @n\r\n"
"@W\r\nCore class: @GBorg@n\r\n"
"@WCalling classes for Borg: @YDrone, Destroyer@n\r\n"
"@WFocus Classes for Drone: @BAssimilator, Guardian@n\r\n"
"@WFocus Classes for Destroyer: @BJuggernaut, Panzer@n\r\n";

const char *caller_classes =
"\r\n@WCaller Class Progression:\r\n\r\n"
"@W                            +---@BHunter      @n\r\n"
"@W            +---@YFeral@W-------|                @n\r\n"
"@W            |               +---@BBeastmaster @n\r\n"
"@GCaller@W------|                                 @n\r\n"
"@W            |               +---@BVanguard    @n\r\n"
"@W            +---@YSurvivalist@W-|                @n\r\n"
"@W                            +---@BShaman      @n\r\n"
"@W\r\nCore class: @GCaller@n\r\n"
"@WCalling classes for Caller: @YFeral, Survivalist@n\r\n"
"@WFocus Classes for Feral: @BHunter, Beastmaster@n\r\n"
"@WFocus Classes for Survivalist: @BVanguard, Shaman@n\r\n";

const char *highlander_classes =
"\r\nHighlander Class Progression:\r\n\r\n"
"   @GCore          @YCalling           @BFocus@n\r\n\r\n"
"@W                            +---@BBlademaster @n\r\n"
"@W            +---@YKnight@W------|                @n\r\n"
"@W            |               +---@BReaver      @n\r\n"
"@GHighlander@W--|                                 @n\r\n"
"@W            |               +---@BArcane      @n\r\n"
"@W            +---@YBladesinger@W-|                @n\r\n"
"@W                            +---@BBard        @n\r\n"
"@W\r\nCore class: @GHighlander@n\r\n"
"@WCalling classes for Highlander: @YKnight, Bladesinger @n\r\n"
"@WFocus Classes for Knight: @BBlademaster, Reaver@n\r\n"
"@WFocus Classes for Bladesinger: @BArcane, Bard@n\r\n";

const char *predator_classes =
"\r\n@WPredator Class Progression:\r\n\r\n"
"@W                           +---@BWeaponmaster @n\r\n"
"@W            +---@YYoungBlood@W-|                 @n\r\n"
"@W            |              +---@BElder        @n\r\n"
"@GPredator@W----|                                 @n\r\n"
"@W            |              +---@BPredalien    @n\r\n"
"@W            +---@YBadBlood@W---|                 @n\r\n"
"@W                           +---@BMech         @n\r\n"
"@W\r\nCore class: @GPredator@n\r\n"
"@WCalling classes for Predator: @YYoungBlood, BadBlood@n\r\n"
"@WFocus Classes for YoungBlood: @BWeaponmaster, Elder@n\r\n"
"@WFocus Classes for BadBlood: @BPredalien, Mech@n\r\n";
