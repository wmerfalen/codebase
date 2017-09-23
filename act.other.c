/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* needed by sysdep.h to allow for definition of <sys/stat.h> */
#define __ACT_OTHER_C__

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
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"  /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "drugs.h"
#include "asciimap.h"
#include "bionics.h"
#include "skilltree.h"
#include "protocol.h"

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
void call_pets(struct char_data *ch);
//static int is_useable(struct obj_data *item);


/* Blinkie stuff - Gahan */
ACMD(do_hp)
{	
	send_to_char(ch, "@WYou have %d@W hit points.@n\r\n", GET_HIT(ch));
}
ACMD(do_unwield)
{	
	send_to_char(ch, "You remove your weapon.\r\n");
             obj_to_char(unequip_char(ch, WEAR_WIELD, TRUE), ch);

}

ACMD(do_cp)
{	
	send_to_char(ch, "@WYou have %d@W psi points.@n\r\n", GET_PSI(ch));
}

ACMD(do_mv)
{	
	send_to_char(ch, "@WYou have %d@W move points.@n\r\n", GET_MOVE(ch));
}

ACMD(do_qp) 
{
	send_to_char(ch, "@YYou have %d questpoints.@n\r\n", GET_QUESTPOINTS(ch));
}
ACMD(do_enc) 
{
                        send_to_char(ch, "Encumbrance at %d%%. \r\n", ENCUMBRANCE(ch));
}
ACMD(do_dup) 
{
        send_to_char(ch, "You are creating %d images.\n\r", ch->char_specials.saved.duplicates);
}
ACMD(do_fame)
{
	send_to_char(ch, "@BYou have acquired %d fame.@n\r\n", GET_FAME(ch));
}

ACMD(do_dash)
{
	if (IN_VEHICLE(ch)) {
		send_to_char(ch, "Your vehicle has %d of %d maximum HP, %d fuel charges remaining, and has an armor class of %d.\r\n", (GET_OBJ_VAL(VEHICLE(ch), 2) - VEHICLE(ch)->damage), GET_OBJ_VAL(VEHICLE(ch), 2), VEHICLE(ch)->fuel, GET_OBJ_VAL(VEHICLE(ch), 4));
		return;
	}
	else
		send_to_char(ch, "You must be inside of a vehicle to read its dashboard values.\r\n");
}
	
ACMD(do_shards)
{
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	struct obj_data *obj;
	struct obj_data *next_obj;
	struct obj_data *loadobj;
	two_arguments(argument, arg, arg2);
	int num;
	int objvnum;

	if (!*arg) {
		send_to_char(ch, "You currently have:\r\n\r\n");
		send_to_char(ch, " @G%3d@n green shards.\r\n", GET_GREEN_SHARDS(ch));
		send_to_char(ch, " @B%3d@n blue shards.\r\n", GET_BLUE_SHARDS(ch));
		send_to_char(ch, " @D%3d@n black shards.\r\n", GET_BLACK_SHARDS(ch));
	}
	else if (!strcmp(arg, "convert")) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_JEWELER)) {
			if (!*arg2) {
				send_to_char(ch, "You can convert up your lower level shards to higher level shards.  You will need\r\n");
				send_to_char(ch, "5 green shards for a blue, and 5 blue shards for a black.\r\n");
				send_to_char(ch, "Type shard convert <color> to convert your shards.\r\n");
				return;
			}
			else if (!strcmp(arg2, "green")) {
				if (GET_GREEN_SHARDS(ch) >= 5) {
					send_to_char(ch, "You convert 5 @Ggreen shards@n into 1 @Bblue shard@n!\r\n");
					GET_GREEN_SHARDS(ch) -= 5;
					GET_BLUE_SHARDS(ch) += 1;
					return;
				}
				else {
					send_to_char(ch, "You need atleast 5 green shards in order to convert them into 1 blue shard.\r\n");
					return;
				}
			}
			else if (!strcmp(arg2, "blue")) {
				if (GET_BLUE_SHARDS(ch) >= 5) {
					send_to_char(ch, "You convert 5 @Bblue shards@n into 1 @Dblack shard@n!\r\n");
					GET_BLUE_SHARDS(ch) -= 5;
					GET_BLACK_SHARDS(ch) += 1;
					return;
				}
				else {
					send_to_char(ch, "You need atleast 5 blue shards in order to convert them into 1 black shard.\r\n");
					return;
				}			
			}
			else if (!strcmp(arg2, "black")) {
				send_to_char(ch, "Currently there is nothing to convert your black shards into.  Use them to buy equipment!\r\n");
				return;
			}
			else {
				send_to_char(ch, "Which color of shards do you wish to convert, green or blue?\r\n");
				return;
			}
		}
		else {
			send_to_char(ch, "You need to find a jeweler first before you can convert your shards.\r\n");
			return;
		}
	}
	else if (!strcmp(arg, "verify")) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_JEWELER)) {
			for (obj = ch->carrying; obj; obj = next_obj) {
				next_obj = obj->next_content;
				if (GET_OBJ_VNUM(obj) == 10890) {
					send_to_char(ch, "You have one of your @Ggreen@n shards verified!\r\n");
					GET_GREEN_SHARDS(ch) += 1;
					obj_from_char(obj);
					extract_obj(obj);
				}
				else if (GET_OBJ_VNUM(obj) == 10891) {
					send_to_char(ch, "You have one of your @Bblue@n shards verified!\r\n");
					GET_BLUE_SHARDS(ch) += 1;
					obj_from_char(obj);
					extract_obj(obj);
				}
				else if (GET_OBJ_VNUM(obj) == 10892) {
					send_to_char(ch, "You have one of your @Dblack@n shards verified!\r\n");
					GET_BLACK_SHARDS(ch) += 1;
					obj_from_char(obj);
					extract_obj(obj);
				}
			}
		}
		else {
			send_to_char(ch, "You need to show your shards to an expert jeweler to convert your shards into points.\r\n");
			return;
		}
	}
	else if (!strcmp(arg, "buy")) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GREENSHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which green shard equipment would you like to buy? Type 'help green' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						if (GET_GREEN_SHARDS(ch) < 15) {
							send_to_char(ch, "You need atleast 15 green shards to purchase that object.\r\n");
							return;
						}
						else {
							objvnum = ((10800 + num) -1);
							GET_GREEN_SHARDS(ch) -= 15;
							loadobj = read_object(real_object(objvnum), REAL);
							obj_to_char(loadobj, ch);
							send_to_char(ch, "You purchase %s!\r\n", loadobj->short_description);
							return;
						}
					}
				}
				else {
					send_to_char(ch, "You must select a number between 1 and 20.\r\n");
					return;
				}
			}
		}
		else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_BLUESHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which blue shard equipment would you like to buy? Type 'help blue' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						if (GET_BLUE_SHARDS(ch) < 15) {
							send_to_char(ch, "You need atleast 15 blue shards to purchase that object.\r\n");
							return;
						}
						else {
							objvnum = ((10820 + num) -1);
							GET_BLUE_SHARDS(ch) -= 15;
							loadobj = read_object(real_object(objvnum), REAL);
							obj_to_char(loadobj, ch);
							send_to_char(ch, "You purchase %s!\r\n", loadobj->short_description);
							return;
						}
					}
				}
				else {
					send_to_char(ch, "You must select a number between 1 and 20.\r\n");
					return;
				}
			}
		}
		else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_BLACKSHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which black shard equipment would you like to buy? Type 'help black' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						if (GET_BLACK_SHARDS(ch) < 15) {
							send_to_char(ch, "You need atleast 15 black shards to purchase that object.\r\n");
							return;
						}
						else {
							objvnum = ((10840 + num) -1);
							GET_BLACK_SHARDS(ch) -= 15;
							loadobj = read_object(real_object(objvnum), REAL);
							obj_to_char(loadobj, ch);
							send_to_char(ch, "You purchase %s!\r\n", loadobj->short_description);
							return;
						}
					}
				}
				else {
					send_to_char(ch, "You must select a number between 1 and 20.\r\n");
					return;
				}
			}
		}
		else {
			send_to_char(ch, "You must be inside a shard shop before buying any shard equipment.\r\n");
			return;
		}
	}
	else if (!strcmp(arg, "scan")) {
		if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_GREENSHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which green shard equipment would you like to scan? Type 'help green shard' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						objvnum = ((10800 + num) -1);
						loadobj = read_object(real_object(objvnum), REAL);
						send_to_char(ch, "You pick up the shopkeepers identifier and scan the item.\r\n");
						call_psionic(ch, NULL, loadobj, PSIONIC_IDENTIFY, 30, 0, TRUE);
						extract_obj(loadobj);
						return;
					}
				}
				else {
					send_to_char(ch, "Please select a number between 1 and 20.  Type 'help green shard' for a list.\r\n");
					return;
				}
			}				
		}
		else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_BLUESHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which blue shard equipment would you like to scan? Type 'help blue shard' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						objvnum = ((10820 + num) -1);
						loadobj = read_object(real_object(objvnum), REAL);
						send_to_char(ch, "You pick up the shopkeepers identifier and scan the item.\r\n");
						call_psionic(ch, NULL, loadobj, PSIONIC_IDENTIFY, 30, 0, TRUE);
						extract_obj(loadobj);
						return;
					}
				}
				else {
					send_to_char(ch, "Please select a number between 1 and 20.  Type 'help blue shard' for a list.\r\n");
					return;
				}
			}
		}
		else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_BLACKSHOP)) {
			if (!*arg2) {
				send_to_char(ch, "Which black shard equipment would you like to scan? Type 'help black shard' to see the list.\r\n");
				return;
			}
			else {
				if (is_number(arg2)) {
					num = atoi(arg2);
					if (num < 1 || num > 20) {
						send_to_char(ch, "You must select a number between 1 and 20.\r\n");
						return;
					}
					else {
						objvnum = ((10840 + num) -1);
						loadobj = read_object(real_object(objvnum), REAL);
						send_to_char(ch, "You pick up the shopkeepers identifier and scan the item.\r\n");
						call_psionic(ch, NULL, loadobj, PSIONIC_IDENTIFY, 30, 0, TRUE);
						extract_obj(loadobj);
						return;
					}
				}
				else {
					send_to_char(ch, "Please select a number between 1 and 20.  Type 'help black shard' for a list.\r\n");
					return;
				}
			}
		}
		else {
			send_to_char(ch, "You must be inside a shard shop before scanning any shard equipment.\r\n");
			return;
		}
	}
	else {
		send_to_char(ch, "Useage:\r\n");
		send_to_char(ch, "shards                 - list all verified shard points\r\n");
		send_to_char(ch, "shards verify          - have an expert jeweler convert your shards into points\r\n");
		send_to_char(ch, "shards convert <color> - will allow you to convert 5 shards for one of a higher level at a jeweler\r\n");
		send_to_char(ch, "shards buy <number>    - purchase gear corresponding with the number given\r\n");
		send_to_char(ch, "shards scan <number>   - shopscan gear corresponding with the number given\r\n");
	}
}

ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (auction_info.high_bidder == ch || auction_info.owner == ch) {
		send_to_char(ch, "You are involved in a market session, please wait for it to end before you quit.\r\n");
		return;
	}
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL, FALSE);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* We used to check here for duping attempts, but we may as well do it right
     * in extract_char(), since there is no check if a player rents out and it
     * can leave them in an equally screwy situation. */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);		/* Char is saved before extracting. */
  }
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
  save_char(ch);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_sneak)
{
  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_SNEAK)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
	send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");
	SET_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
  }
  else {
	  send_to_char(ch, "You stop sneaking around.\r\n");
	  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
  }
}

ACMD(do_hide)
{
  int percent_miss, d_roll;

  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_HIDE)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (AFF_FLAGGED(ch, AFF_HIDE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You return from the shadows.\r\n");
    return;
  }
  d_roll = rand_number(1, 5);
  percent_miss = rand_number(1, 101);    /* 101% is a complete failure */
  if (percent_miss == 101) {
	send_to_char(ch, "You fail critically!\r\n");
    return;
  }
  if (d_roll > GET_SKILL_LEVEL(ch, SKILL_HIDE)) {
	send_to_char(ch, "You failed to hide from anyone.\r\n");
    return;
  }
  else {
	send_to_char(ch, "You slip into your hiding spot.\r\n");
	SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
	int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
	increment_skilluse(SKILL_HIDE, 1, GET_IDNUM(ch), exp, ch);
	return;
  }
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (IS_NPC(ch) || !GET_SKILL_LEVEL(ch, SKILL_STEAL)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  /* 101% is a complete failure */
  percent = rand_number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
    percent -= 50;

  /* No stealing if not allowed. If it is no stealing from Imm's or Shopkeepers. */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "units")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos, TRUE), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (percent > GET_SKILL_LEVEL(ch, SKILL_STEAL)) {
	ohoh = TRUE;
	send_to_char(ch, "Oops..\r\n");
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL_LEVEL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal units from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some resource units */
      gold = (GET_UNITS(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
		increase_units(ch, gold);
		decrease_units(vict, gold);
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d units.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe one single unit.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any units...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict)) {
	int thaco = rand_number(1,30);
    hit(ch, vict, TYPE_UNDEFINED, thaco, 0);
  }
}

ACMD(do_practice)
{
    char arg[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
        return;

    one_argument(argument, arg);
	if (!*arg) {
		new_practice_list(ch, -1);
		return;
	}
	else {
		send_to_char(ch, "You might want to find a guild to help you with that, type 'HELP GUILD'\r\n");
		return;
	}
}
ACMD(do_skilltree)
{
	char arg[MAX_INPUT_LENGTH];
	int load_result;
	
	one_argument(argument, arg);

	if (IS_NPC(ch))
		return;
	if (!*arg) {
		send_to_char(ch, "You need to specify a class in order to view their skill trees, please try again.\r\n");
		return;
	}

	else {
		if ((load_result = parse_class(arg, TRUE)) == CLASS_UNDEFINED) {
			send_to_char(ch, "That is not a valid class, please try again.\r\n");
			return;
		}
		else
			new_practice_list(ch, load_result);
	}
}
ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s%s%s.\r\n", GET_NAME(ch), *GET_TITLE(ch) ? " " : "", GET_TITLE(ch));
  }
}

int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP))
    return (0);
  if (ch == vict) {
	SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
	return (1);
  }
  SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}

static void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
	send_to_char(ch, "\r\n Members of your group:\r\n\r\n");
	send_to_char(ch, " %-20s [ %2d %s  %4d/%4d Hp %4d/%4d Psi %4d/%4d Mv ] (Leader)",
	    GET_NAME(ch), GET_LEVEL(ch), CLASS_DUALABBR(ch), GET_HIT(ch), GET_MAX_HIT(ch), GET_PSI(ch), GET_MAX_PSI(ch), GET_MOVE(ch), GET_MAX_MOVE(ch));
  }
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "\r\n Members of your group:\r\n\r\n");

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      sprintf(buf, " %-20s [ %2d %s  %4d/%4d Hp %4d/%4d Psi %4d/%4d Mv ] (Leader)",
	       GET_NAME(k), GET_LEVEL(k), CLASS_DUALABBR(k), GET_HIT(k), GET_MAX_HIT(k), GET_PSI(k), GET_MAX_PSI(k), GET_MOVE(k), GET_MAX_MOVE(k));
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
		continue;

		sprintf(buf, " %-20s [ %2d %s  %4d/%4d Hp %4d/%4d Psi %4d/%4d Mv ]",
	       GET_NAME(f->follower), GET_LEVEL(f->follower), CLASS_DUALABBR(f->follower), GET_HIT(f->follower), GET_MAX_HIT(f->follower), GET_PSI(f->follower), GET_MAX_PSI(f->follower), GET_MOVE(f->follower), GET_MAX_MOVE(f->follower));
		act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
  }
  send_to_char(ch, "\r\n");
}
ACMD(do_hph)
{
  struct char_data *k;
  struct follow_type *f;
  int numplayers = 0;
  int count1 = 0;
  
  send_to_char(ch, "\r\n Hitpoint status of your group:\r\n\r\n");
  k = (ch->master ? ch->master : ch);
  
  count1 = (GET_MAX_HIT(k) - GET_HIT(k));
  if (count1 > 100) {
    send_to_char(ch, " ");
	diag_char_to_char(k, ch);
	send_to_char(ch, " (@R-%d@n)\r\n", count1);
	numplayers++;
  }
  
  for (f = k->followers; f; f = f->next) {
	if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	  continue;
	  
	count1 = (GET_MAX_HIT(f->follower) - GET_HIT(f->follower));
	if (count1 > 100) {
		send_to_char(ch, " ");
		diag_char_to_char(f->follower, ch);
		send_to_char(ch, " (@R-%d@n)\r\n", count1);
		numplayers++;
	}
  }
  if (numplayers == 0)
	send_to_char(ch, " No one in your group currently requires healing.\r\n");
  send_to_char(ch, "\r\n");
}
ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found;

  two_arguments(argument, buf, buf2);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You cannot enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next) {
      found += perform_group(ch, f->follower);
	}
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }
  if (!str_cmp(buf, "join")) {
	if (*buf2) {
	  send_to_char(ch, "Who's group would you like to join?\r\n");
	  return;
	}
	else {
		if (!(vict = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD))) {
		  send_to_char(ch, "You don't see anyone around by that name.\r\n");
		  return;
		}
		else {
		}
	}
  }
  if (!str_cmp(buf, "summon")) {
    if (*buf2) {
	  send_to_char(ch, "Who do you want to summon to your group?\r\n");
	  return;
	}
	else {
	
	}
  }	
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP)) {
	  perform_group(ch, ch);
      perform_group(ch, vict);
	}
    else {
      if (ch != vict) {
		act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
		act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
		act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
		REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
	  }
    }
  }
}

ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
		REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
		if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
			extract_char(f->follower);
			send_to_room(IN_ROOM(f->follower), "%s races out of the room after being freed by its master!\r\n", f->follower->player.short_descr);
		}
		else 
			stop_follower(f->follower);
      }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }
  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (AFF_FLAGGED(tch, AFF_CHARM)) {
	extract_char(tch);
	send_to_room(IN_ROOM(tch), "%s races out of the room after being free'd by its master!\r\n", tch->player.short_descr);
  }
  else
    stop_follower(tch);
}

ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_PSI(ch), GET_MAX_PSI(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, TRUE, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, TRUE, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_UNITS(ch)) {
      send_to_char(ch, "You don't seem to have that many units to split.\r\n");
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your units?\r\n");
      return;
    }

    decrease_units(ch, share * (num - 1));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d units; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d unit%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      increase_units(k, share);
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	  increase_units(f->follower, share);
	  send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d units among %d members, %d resource units each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d unit%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      increase_units(ch, rest);
    }
  } else {
    send_to_char(ch, "How many units do you wish to split with your group?\r\n");
    return;
  }
}

static int is_useable(struct obj_data *item) {
  switch(GET_OBJ_TYPE(item)) {
        case ITEM_STAFF:
        case ITEM_WAND:
        case ITEM_SCROLL:
          return TRUE;
  }
  return FALSE;
}
/*
ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *psi_item;
  int i;
  
  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  for (i = 0; i < NUM_WEARS; i++) {
	if (GET_EQ(ch, i) && is_useable(GET_EQ(ch, i)) && isname(arg, GET_EQ(ch, i)->name)) {
		psi_item = GET_EQ(ch, i);
		break;
	}
  }

  if (!psi_item || !isname(arg, psi_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(psi_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
		send_to_char(ch, "You don't seem to be wearing or holding %s %s.\r\n", AN(arg), arg);
		return;
		
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(psi_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(psi_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
	
  case SCMD_USE:
    if ((GET_OBJ_TYPE(psi_item) != ITEM_WAND) && (GET_OBJ_TYPE(psi_item) !=ITEM_STAFF)) {
        switch (GET_OBJ_TYPE(psi_item)) {
            case ITEM_EXPLOSIVE:
                if (GET_EXPLOSIVE_TYPE(psi_item) == EXPLOSIVE_MINE)
                    do_arm(ch, arg, 0, 0);
                else
                    do_pull(ch, arg, 0, 0);
                return;
            case ITEM_RECALL:
                do_recall(ch, arg, 0, 0);
                return;

            case ITEM_DRUG:
                do_inject(ch, arg, 0, 0);
                return;

            default:
                send_to_char(ch, "You can't seem to figure out how to use that");
                return;
        }
    }
    break;
  }
  psi_object_psionics(ch, psi_item, buf);
}*/

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
    (GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }
  psi_object_psionics(ch, mag_item, buf);
 }

ACMD(do_display)
{
    char arg[MAX_INPUT_LENGTH];    
    int i;
    int x;

    const char *def_prompts[][2] = {
    // This is only for prompts predefined in the code for whatever reason
    // format for new prompt....
    // { "Name of Prompt", "definition of prompt" },
        { "Stock Circle", "@w%hH %pP %vV>@n" },
        { "Colorized Standard Circle", "@R%h@rH @C%p@cP @G%v@gV@w>@n" },
        { "Full Featured", "@cOpponent: @B%o @cTank: @B%t %_@WH@R%h/%H @WP@R%p/%P @WV%v/%V @w>@n" },
        { "Bosstone Special", "@W|@WTo Level: @G%X @W|@WUnits: @G$%u@W|%_@W|@WH:@G%h/%H @WP: @G%p/%P @WV: @G%v/%V@W>@n"},
        { "Immortal's Prompt", "@cRoom(@C%r@c) @rInvis(@R%i@r) @w>@n"},
        { "Virjal's MTScripts Prompt", "%h/%Hh . %p/%Pp . %v/%Vv . %O- . %x xp %X tnl . %u U . %q QP . %A Ammo."},
        { "None", "\n"},
        { "\n", "\n" }
    };

    if (IS_NPC(ch)) {
        send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
        return;
    }

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Usage: display <number>\r\nTo create your own prompt, see @RHelp Prompt@n.\r\n");
        send_to_char(ch, "\r\nThe following pre-set prompts are available...\r\n");

        for (i = 0; *def_prompts[i][0] != '\n'; i++)
            send_to_char(ch, "  %d. %-25s  %s\r\n", i, def_prompts[i][0], def_prompts[i][1]);

        return;
    }

    else if (!isdigit(*arg)) {
        send_to_char(ch, "Usage: display <number>\r\n");
        send_to_char(ch, "Type <display> without arguments for a list of preset prompts.\r\n");
        return;
    }

    i = atoi(arg);

    if (i < 0) {
        send_to_char(ch, "The number cannot be negative.\r\n");
        return;
    }

    for (x = 0; *def_prompts[x][0] != '\n'; x++);

    if (i >= x) {
        send_to_char(ch, "The range for the prompt number is 0-%d.\r\n", x-1);
        return;
     }

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    // for now, a safe way to reset your prompt back to the default
    if (strcmp(def_prompts[i][0], "None"))
        GET_PROMPT(ch) = strdup(def_prompts[i][1]);
    else
        GET_PROMPT(ch) = NULL;

    send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_prompt)
{
    skip_spaces(&argument);

    if (!*argument) {

        if(GET_PROMPT(ch))
           send_to_char(ch, "\r\nYour prompt is currently: %s\r\nType @Rdisplay@n to get a list of pre-made prompts or see @RHelp Prompt@n to design your own.\r\n", GET_PROMPT(ch));
        else
            send_to_char(ch, "Your prompt is currently not defined.");

        return;
    }

    delete_doubledollar(argument);

    if (GET_PROMPT(ch))
        free(GET_PROMPT(ch));

    GET_PROMPT(ch) = strdup(argument);
    send_to_char(ch, "Okay, your prompt is now: %s\r\n", GET_PROMPT(ch));
}

// do_gen_tog is now superceded by do_toggle
ACMD(do_toggle)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int toggle;
    int toggle_cmd;
    int tp;
    int wimp_lev;
    int result = 0;
    int len = 0;
    const char *types[] = { "off", "brief", "normal", "on", "\n" };

    const struct {
        char *command;
        bitvector_t pref; /* this needs changing once hashmaps are implemented */
        int scmd;
        char min_level;
        char *disable_msg;
        char *enable_msg;
    }
    toggle_info[] = {

        {
            "brief", PRF_BRIEF, SCMD_BRIEF, 0,
            "Brief mode off.\r\n",
            "Brief mode on.\r\n"
        },
        {
            "compact", PRF_COMPACT, SCMD_COMPACT, 0,
            "Compact mode off.\r\n",
            "Compact mode on.\r\n"
        },
        {
            "noshout", PRF_NOSHOUT, SCMD_NOSHOUT, 0,
            "You can now hear shouts.\r\n",
            "You are now deaf to shouts.\r\n"
        },
        {
            "notell", PRF_NOTELL, SCMD_NOTELL, 0,
            "You can now hear tells.\r\n",
            "You are now deaf to tells.\r\n"
        },
        {
            "disphp", PRF_DISPHP, SCMD_DISPHP, 0,
            "Hit points will no longer be displayed.\r\n",
            "Hit points will now be displayed.\r\n"
        },
        {
            "disppsi", PRF_DISPPSI, SCMD_DISPPSI, 0,
            "Psi points will no longer be displayed.\r\n",
            "Psi points will now be displayed.\r\n"
        },
        {
            "dispmove", PRF_DISPMOVE, SCMD_DISPMOVE, 0,
            "Move points will no longer be displayed.\r\n",
            "Move points will now be displayed.\r\n"
        },
        {
            "dispammo", PRF_DISPAMMO, SCMD_DISPAMMO, 0,
            "Remaining ammo will no longer be displayed.\r\n",
            "Remaining ammo will now be displayed.\r\n"
        },
        {
            "nohassle", PRF_NOHASSLE, SCMD_NOHASSLE, LVL_IMMORT,
            "Nohassle disabled.\r\n",
            "Nohassle enabled.\r\n"
        },
        {
            "quest", PRF_QUEST, SCMD_QUEST, 0,
            "You are no longer part of the Quest.\r\n",
            "Okay, you are part of the Quest!\r\n"
        },
        {
            "summonable", PRF_NOSUMMON, SCMD_NOSUMMON, 0,
             "You are now safe from summoning by other players.\r\n",
            "You may now be summoned by other players.\r\n"
        },
        {
            "norepeat", PRF_NOREPEAT, SCMD_NOREPEAT, 0,
            "You will now have your communication repeated.\r\n",
            "You will no longer have your communication repeated.\r\n"
        },
        {
            "holylight", PRF_HOLYLIGHT, SCMD_HOLYLIGHT, LVL_IMMORT,
            "HolyLight mode off.\r\n",
            "HolyLight mode on.\r\n"
        },
        {
            "color", 0, SCMD_COLOR, 0,
            "\n",
            "\n"
        },
        {
            "nowiz", PRF_NOWIZ, SCMD_NOWIZ, LVL_IMMORT,
            "You can now hear the Wiz-channel.\r\n",
            "You are now deaf to the Wiz-channel.\r\n"
        },
        {
            "mortlog", PRF_LOG1, SCMD_MORTLOG, 0,
            "You can no longer see Death and Leveling announcements.\r\n",
            "You are now able to see Death and Leveling announcements.\r\n"
        },
        {
            "syslog", PRF_LOG2, SCMD_SYSLOG, LVL_IMMORT,
            "You can no longer see mud log anouncements.\r\n",
            "You are now able to see mud log announcements.\r\n"
        },
        {
            "noauction", PRF_NOAUCT, SCMD_NOAUCT, 0,
            "You can now hear auctions.\r\n",
            "You are now deaf to auctions.\r\n"
        },
        {
            "nogossip", PRF_NOGOSS, SCMD_NOGOSS, 0,
            "You can now hear gossip.\r\n",
            "You are now deaf to gossip.\r\n"
        },
        {
            "nograts", PRF_NOGRATZ, SCMD_NOGRATZ, 0,
            "You can now hear the congratulation messages.\r\n",
            "You are now deaf to the congratulation messages.\r\n"
        },
        {
            "showvnums", PRF_SHOWVNUMS, SCMD_SHOWVNUMS, LVL_IMMORT,
            "You will no longer see the room flags.\r\n",
            "You will now see the room flags.\r\n"
        },
        {
            "afk", PRF_AFK, SCMD_AFK, 0,
            "AFK flag is now off.\r\n",
            "AFK flag is now on.\r\n"
        },
        {
            "autocon", PRF_AUTOCON, SCMD_AUTOCON, 0,
            "You will no longer see the condition of your opponent automatically.\n\r",
            "You will now see the condition of your opponent automatically.\n\r"
        },
        {
            "autoexits", PRF_AUTOEXIT, SCMD_AUTOEXIT, 0,
            "You will no longer see the exit directions of rooms automatically.\n\r",
            "You will now see the exit directions of rooms automatically.\n\r"
        },
        {
            "autoequip", PRF_AUTOEQUIP, SCMD_AUTOEQUIP, 0,
            "You will no longer wear equipment upon unrenting.\n\r",
            "You will now wear equipment upon unrenting.\n\r"
        },
        {
            "autoloot", PRF_AUTOLOOT, SCMD_AUTOLOOT, 0,
            "You will no longer autoloot corpses.\n\r",
            "You will now autoloot corpses.\n\r"
        },
        {
            "autosplit", PRF_AUTOSPLIT, SCMD_AUTOSPLIT, 0,
            "You will no longer autosplit your loot with group members.\r\n",
            "You will now autosplit your loot from corpses.\r\n"
        },
        {
            "autoassist", PRF_AUTOASSIST, SCMD_AUTOASSIST, 0,
            "You will no longer Auto-Assist.\r\n",
            "You will now Auto-Assist.\r\n"
        },
        {
            "autounits", PRF_AUTOUNITS, SCMD_AUTOUNIT, 0,
            "You will no longer automatically loot corpses for their units.\r\n",
            "You will now automatically loot corpses for their units.\r\n"
        },
        {
            "autodam", PRF_AUTODAM, SCMD_AUTODAM, 0,
            "You will no longer see numerical damage values in combat.\r\n",
            "You will now see numerical damage values in combat.\r\n"
        },
        {
            "autogain", PRF_AUTOGAIN, SCMD_AUTOGAIN, 0,
            "You will no longer auto gain, type gain to advance.\r\n",
            "You will now auto gain when you have the required XP.\r\n"
        },
        {
            "automap", PRF_AUTOMAP, SCMD_AUTOMAP, 1,
            "You will no longer see the mini-map.\r\n",
            "You will now see a mini-map at the side of room descriptions.\r\n"
        },
        {
            "autokey", PRF_AUTOKEY, SCMD_AUTOKEY, 0,
            "You will now have to unlock doors manually before opening.\r\n",
            "You will now automatically unlock doors when opening them (if you have the key).\r\n"
        },
        {
            "autodoor", PRF_AUTODOOR, SCMD_AUTODOOR, 0,
            "You will now need to specify a door direction when opening, closing and unlocking.\r\n",
            "You will now find the next available door when opening, closing or unlocking.\r\n"
        },
        {
            "ldflee", PRF_LDFLEE, SCMD_LDFLEE, 0,
            "You will no longer flee if you are linkless.\n\r",
            "You will now flee if you are hit and you are linkless.\n\r"
        },
        {
            "newbie", PRF_NONEWBIE, SCMD_NONEWBIE, 0,
            "You can now hear newbie.\r\n",
            "You are now deaf to newbie.\r\n"
        },
        {
            "clsolc", PRF_CLS, SCMD_CLS, LVL_BUILDER,
            "Will no longer clear screen in OLC.\r\n",
            "Will now clear screen in OLC.\r\n"
        },
        {
            "buildwalk", PRF_BUILDWALK, SCMD_BUILDWALK, LVL_BUILDER,
            "Buildwalk Off.\r\n",
            "Buildwalk On.\r\n"
        },
        {
            "radiosnoop", PRF_RADIOSNOOP, SCMD_RADIOSNOOP, LVL_GRGOD,
            "Radio snoop de-activated.\r\n",
            "Radio snoop activated.\r\n"
        },
        {
            "spectator", PRF_SPECTATOR, SCMD_SPECTATOR, LVL_IMMORT,
            "Your are no longer a spectator.\r\n",
            "Your are now a spectator.\r\n"
        },
        {
            "tempmortal", PRF_TEMP_MORTAL, SCMD_TEMP_MORTAL, LVL_IMMORT,
            "Your are now an immortal again.\r\n",
            "Your are now a mortal (temporarily).\r\n"
        },
        {
            "oldscore", PRF_OLDSCORE, SCMD_OLDSCORE, 0,
            "Your score will now be displayed the new way.\r\n",
            "Your score will now be displayed the old way.\r\n"
        },
        {
            "nowho", PRF_NOWHO, SCMD_NOWHO, LVL_GOD,
            "You will now show up on the who list.\r\n",
            "You will no longer show up on the who list.\r\n"
        },
        {
            "showdebug", PRF_SHOWDBG, SCMD_SHOWDBG, LVL_GOD,
            "Debug messages de-activated.\r\n",
            "Debug messages activated.\r\n"
        },
        {
            "stealth", PRF_STEALTH, SCMD_SET_STEALTH, 0,
            "Disengaging cloaking device..\r\n",
            "Engaging stealth mode.\r\n"
        },
        {
            "pagelength", 0, SCMD_PAGELENGTH, 0,
            "\n",
            "\n"
        },
        {
            "screenwidth", 0, SCMD_SCREENWIDTH, 0,
            "\n",
            "\n"
        },
        {
            "wimpy", 0, SCMD_WIMPY, 0,
            "\n",
            "\n"
        },
        {
            "slownameserver", 0, SCMD_SLOWNS, LVL_IMPL,
            "Nameserver_is_slow changed to OFF; IP addresses will now be resolved.\r\n",
            "Nameserver_is_slow changed to ON; sitenames will no longer be resolved.\r\n"
        },

        {
            "trackthru", 0, SCMD_TRACK, LVL_IMPL,
            "Players can no longer track through doors.\r\n",
            "Players can now track through doors.\r\n"
        },
        {
            "notypos", PRF_NO_TYPO, SCMD_NO_TYPO, 10,
            "You will now be given suggestions for mistyped commands.\r\n",
            "You will no longer be given suggestions for mistyped commands.\r\n"
        },
        {
            "autoscavenger", PRF_AUTOSCAVENGER, SCMD_AUTOSCAVENGER, 0,
            "You will no longer automatically scavenger corpses.\n\r",
            "You will now automatically scavenger corpses.\n\r"
        },
        {
            "autosacrifice", PRF_AUTOSCAVENGER, SCMD_AUTOSCAVENGER, 0,
            "You will no longer automatically scavenger corpses.\n\r",
            "You will now automatically scavenger corpses.\n\r"
        },
        {
            "fightspam", PRF_NOFIGHTSPAM, SCMD_NOFIGHTSPAM, 0,
            "You will now see full combat spam.\n\r",
            "You will now have combat spam removed.\n\r"
        },
        {
            "oldwho", PRF_OLDWHO, SCMD_OLDWHO, 0,
            "Your who screen will now be displayed the new way.\r\n",
            "Your who screen will now be displayed the old way.\r\n"
        },
        {
            "showeqslots", PRF_OLDEQ, SCMD_OLDEQ, 0,
            "Your equipment screen will not display empty slots.\r\n",
            "Your equipment screen will now display empty slots.\r\n"
        },
		{
			"nomarket", PRF_NOMARKET, SCMD_NOMARKET, 0,
			"You will now be able to hear and use the market system.\r\n",
			"You will no longer hear or use the market system.\r\n"
		},
		{
			"blind", PRF_BLIND, SCMD_BLIND, 0,
			"You have disabled blind spam filter.\r\n",
			"You have enabled blind spam filter.\r\n"
		},
		{
			"spam", PRF_SPAM, SCMD_SPAM, 0,
			"You have disabled spam filter.\r\n",
			"You have enabled spam filter.\r\n"
		},
        {
            "\n", 0, -1, -1, "\n", "\n"  /* must be last */
        }
    };

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);
    any_one_arg(argument, arg2); /* so that we don't skip 'on' */

    if (!*arg) {

        if (GET_LEVEL(ch) == LVL_IMPL) {
            send_to_char(ch,
            " SlowNameserver: %-3s   "
            "                        "
            " Trackthru Doors: %-3s\r\n",

            ONOFF(CONFIG_NS_IS_SLOW),
            ONOFF(CONFIG_TRACK_T_DOORS));
        }

        if (GET_LEVEL(ch) >= LVL_IMMORT) {
            send_to_char(ch, "\r\n");
            send_to_char(ch, "@R@uImmortal Preferences@n\r\n");
            send_to_char(ch, "\r\n");
            send_to_char(ch,
                "      Buildwalk: %-3s    "
                "          NoWiz: %-3s    "
                "         ClsOLC: %-3s\r\n"
                "       NoHassle: %-3s    "
                "      Holylight: %-3s    "
                "      ShowVnums: %-3s\r\n"
                "         Syslog: %-3s    "
                "     TempMortal: %-3s    "
                "     RadioSnoop: %-3s\r\n"
                "          NoWho: %-3s    "
                "      ShowDebug: %-3s\r\n",

                ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
                ONOFF(PRF_FLAGGED(ch, PRF_NOWIZ)),
                ONOFF(PRF_FLAGGED(ch, PRF_CLS)),
                ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
                ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
                ONOFF(PRF_FLAGGED(ch, PRF_SHOWVNUMS)),
                types[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)],
                ONOFF(PRF_FLAGGED(ch, PRF_TEMP_MORTAL)),
                ONOFF(PRF_FLAGGED(ch, PRF_RADIOSNOOP)),
                ONOFF(PRF_FLAGGED(ch, PRF_NOWHO)),
                ONOFF(PRF_FLAGGED(ch, PRF_SHOWDBG))
                );
        }

        send_to_char(ch, "\r\n");
        send_to_char(ch, "You must use the toggle command to change your character preferences.\r\n");
        send_to_char(ch, "Syntax: @Rtoggle <option> <argument>@n @W(@BExample: toggle afk on@n@W)@n\r\n");
        send_to_char(ch, "\r\n");
        send_to_char(ch, "@R@uPlayer Preferences@n\r\n");
        send_to_char(ch, "\r\n");
        send_to_char(ch,
            "Hit Pnt Display: %-3s    "
            "          Brief: %-3s    "
            "     Summonable: %-3s\r\n"

            "   Move Display: %-3s    "
            "        Compact: %-3s    "
            "          Quest: %-3s\r\n"

            "    Psi Display: %-3s    "
            "         NoTell: %-3s    "
            "       NoRepeat: %-3s\r\n"

            "      AutoExits: %-3s    "
            "        NoShout: %-3s\r\n"

            "       NoGossip: %-3s    "
            "      NoAuction: %-3s    "
            "        NoGrats: %-3s\r\n"

            "       Autoloot: %-3s    "
            "      Autounits: %-3s    "
            "      Autosplit: %-3s\r\n"

            "       AutoGain: %-3s    "
            "     AutoAssist: %-3s    "
            "        AutoMap: %-3s\r\n"

            "      Autoequip: %-3s    "
            "        Autocon: %-3s    "
            "        Autodam: %-3s\r\n"

            "        Autokey: %-3s    "
            "       Autodoor: %-3s\r\n"

            "        Mortlog: %-3s    "
            "          Color: %-3s    "
            "       DispAmmo: %-3s\r\n"

            "       NoNewbie: %-3s    "
            "            AFK: %-3s    "
            "   LinkDeadFlee: %-3s\r\n"

            "    PredStealth: %-3s    "
            "       OldScore: %-3s    "
            "(Qst) Spectator: %-3s\r\n"

            "   (Qst) Frozen: %-3s    "
            "     Pagelength: %-3d    "
            "    Screenwidth: %-3d\r\n"

            "      Fightspam: %-3s    "
            "  Autoscavenger: %-3s    "
            "        NoTypos: %-3s\r\n"

            "         OldWho: %-3s    "
            "    ShowEqSlots: %-3s    "
			"       NoMarket: %-3s\r\n"
			"          Blind: %-3s    "
			"           Spam: %-3s    ",


            ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
            ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOSUMMON)),

            ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
            ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
            ONOFF(PRF_FLAGGED(ch, PRF_QUEST)),

            ONOFF(PRF_FLAGGED(ch, PRF_DISPPSI)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOREPEAT)),

            ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOSHOUT)),

            ONOFF(PRF_FLAGGED(ch, PRF_NOGOSS)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOAUCT)),
            ONOFF(PRF_FLAGGED(ch, PRF_NOGRATZ)),

            ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOUNITS)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),

            ONOFF(PRF_FLAGGED(ch, PRF_AUTOGAIN)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAP)),

            ONOFF(PRF_FLAGGED(ch, PRF_AUTOEQUIP)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOCON)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTODAM)),

            ONOFF(PRF_FLAGGED(ch, PRF_AUTOKEY)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTODOOR)),

            ONOFF(PRF_FLAGGED(ch, PRF_LOG1)),
            types[COLOR_LEV(ch)],
            ONOFF(PRF_FLAGGED(ch, PRF_DISPAMMO)),

            ONOFF(PRF_FLAGGED(ch, PRF_NONEWBIE)),
            ONOFF(PRF_FLAGGED(ch, PRF_AFK)),

            ONOFF(PRF_FLAGGED(ch, PRF_LDFLEE)),
            ONOFF(PRF_FLAGGED(ch, PRF_OLDSCORE)),
            ONOFF(PRF_FLAGGED(ch, PRF_STEALTH)),

            ONOFF(PRF_FLAGGED(ch, PRF_SPECTATOR)),
            ONOFF(PRF_FLAGGED(ch, PRF_FROZEN)),

            GET_PAGE_LENGTH(ch),
            GET_SCREEN_WIDTH(ch),

            ONOFF(PRF_FLAGGED(ch, PRF_NOFIGHTSPAM)),
            ONOFF(PRF_FLAGGED(ch, PRF_AUTOSCAVENGER)),
            ONOFF(PRF_FLAGGED(ch, PRF_NO_TYPO)),

            ONOFF(PRF_FLAGGED(ch, PRF_OLDWHO)),
            ONOFF(PRF_FLAGGED(ch, PRF_OLDEQ)),
			ONOFF(PRF_FLAGGED(ch, PRF_NOMARKET)),
			ONOFF(PRF_FLAGGED(ch, PRF_BLIND)),
			ONOFF(PRF_FLAGGED(ch, PRF_SPAM)));

        return;
    }

    // if we get passed a sub-command, use that.  if not, search for the toggle
    if (subcmd == 0) {
        len = strlen(arg);
        for (toggle = 0; *toggle_info[toggle].command != '\n'; toggle++)
            if (!strncmp(arg, toggle_info[toggle].command, len))
                break;

        if (*toggle_info[toggle].command == '\n' || toggle_info[toggle].min_level > GET_LEVEL(ch)) {
            send_to_char(ch, "You can't toggle that!\r\n");
            return;
        }

        toggle_cmd = toggle_info[toggle].scmd;
    }
    else {

        // we still need the preference flag for later toggle setting
        for (toggle = 0; *toggle_info[toggle].command != '\n'; toggle++)
            if (toggle_info[toggle].scmd == subcmd)
                break;

        if (*toggle_info[toggle].command == '\n' || toggle_info[toggle].min_level > GET_LEVEL(ch)) {
            send_to_char(ch, "You can't toggle that!\r\n");
            return;
        }

        toggle_cmd = subcmd;
    }

    switch (toggle_cmd) {
        case SCMD_COLOR:
            if (!*arg2) {
                send_to_char(ch, "Your current color level is %s.\r\n", types[COLOR_LEV(ch)]);
                return;
            }

            if (((tp = search_block(arg2, types, FALSE)) == -1)) {
                send_to_char(ch, "Usage: toggle color { Off | Brief | Normal | On }\r\n");
                return;
            }

            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
            if (tp & 1) SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
            if (tp & 2) SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);

            send_to_char(ch, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR), CCNRM(ch, C_OFF), types[tp]);
            return;

        case SCMD_SYSLOG:
            if (!*arg2) {
                send_to_char(ch, "Your syslog is currently %s.\r\n", types[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
                return;
            }

            if (((tp = search_block(arg2, types, FALSE)) == -1)) {
                send_to_char(ch, "Usage: toggle syslog { Off | Brief | Normal | On }\r\n");
                return;
            }

            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
            REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
            if (tp & 1) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
            if (tp & 2) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);

            send_to_char(ch, "Your syslog is now %s.\r\n", types[tp]);
            return;

        case SCMD_SLOWNS:
            result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
            break;

        case SCMD_TRACK:
            result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
            break;

        case SCMD_BUILDWALK:
            if (GET_LEVEL(ch) < LVL_BUILDER) {
                send_to_char(ch, "Builders only, sorry.\r\n");
                return;
            }

            result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
            if (PRF_FLAGGED(ch, PRF_BUILDWALK))
                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s turned buildwalk on.  Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
            else
                mudlog(CMP, GET_LEVEL(ch), TRUE, "OLC: %s turned buildwalk off.  Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
            break;

        case SCMD_AFK:
            if ((result = PRF_TOG_CHK(ch, PRF_AFK)))
                act("$n is now away from $s keyboard.", TRUE, ch, 0, 0, TO_ROOM);
            else {
                act("$n has returned to $s keyboard.", TRUE, ch, 0, 0, TO_ROOM);

                if (has_mail(GET_IDNUM(ch)))
                    send_to_char(ch, "You have mail waiting.\r\n");
            }
            break;

        case SCMD_WIMPY:
            if (!*arg2) {
                if (GET_WIMP_LEV(ch)) {
                    send_to_char(ch, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
                    return;
                }
                else {
                    send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
                    return;
                }
            }

            if (isdigit(*arg2)) {
                if ((wimp_lev = atoi(arg2)) != 0) {
                    if (wimp_lev < 0)
                        send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
                    else if (wimp_lev > GET_MAX_HIT(ch))
                        send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
                    else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
                        send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
                    else {
                        send_to_char(ch, "Okay, you'll wimp out if you drop below %d hit points.", wimp_lev);
                        GET_WIMP_LEV(ch) = wimp_lev;
                    }
                }
                else {
                    send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.");
                    GET_WIMP_LEV(ch) = 0;
                }
            }
            else
                send_to_char(ch, "Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n");
            break;

        case SCMD_PAGELENGTH:
            if (!*arg2)
                send_to_char(ch, "You current page length is set to %d lines.", GET_PAGE_LENGTH(ch));
            else if (is_number(arg2)) {
                GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg2), 5), 50000000);
                send_to_char(ch, "Okay, your page length is now set to %d lines.", GET_PAGE_LENGTH(ch));
            }
            else
                send_to_char(ch, "Please specify a number of lines (5 - 255).");
            break;

        case SCMD_SCREENWIDTH:
            if (!*arg2)
                send_to_char(ch, "Your current screen width is set to %d characters.", GET_SCREEN_WIDTH(ch));
            else if (is_number(arg2)) {
                GET_SCREEN_WIDTH(ch) = MIN(MAX(atoi(arg2), 40), 200);
                send_to_char(ch, "Okay, your screen width is now set to %d characters.", GET_SCREEN_WIDTH(ch));
            }
            else
                send_to_char(ch, "Please specify a number of characters (40 - 200).");
            break;

        case SCMD_AUTOMAP:
            if (can_see_map(ch)) {
                if (!*arg2) {
                    TOGGLE_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                    result = (PRF_FLAGGED(ch, toggle_info[toggle].pref));
                }
                else if (!strcmp(arg2, "on")) {
                    SET_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                    result = 1;
                }
                else if (!strcmp(arg2, "off"))
                    REMOVE_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                else {
                    send_to_char(ch, "Value for %s must either be 'on' or 'off'.\r\n", toggle_info[toggle].command);
                    return;
                }
            }
            else {
                send_to_char(ch, "Sorry, automap is currently disabled.\r\n");
                return;
            }
            break;

        default:
            if (!*arg2) {
                TOGGLE_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                result = (PRF_FLAGGED(ch, toggle_info[toggle].pref));
            }
            else if (!strcmp(arg2, "on")) {
                SET_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                result = 1;
            }
            else if (!strcmp(arg2, "off")) {
                REMOVE_BIT_AR(PRF_FLAGS(ch), toggle_info[toggle].pref);
                result = 0;
            }
            else {
                send_to_char(ch, "Value for %s must either be 'on' or 'off'.\r\n", toggle_info[toggle].command);
                return;
            }
            break;
    }

    if (result)
        send_to_char(ch, "%s", toggle_info[toggle].enable_msg);
    else
        send_to_char(ch, "%s", toggle_info[toggle].disable_msg);
}

void show_happyhour(struct char_data *ch)
{
  char happyexp[80], happygold[80], happyqp[80];
  int secs_left;

  if ((IS_HAPPYHOUR) || (GET_LEVEL(ch) >= LVL_GRGOD))
  {
      if (HAPPY_TIME)
        secs_left = ((HAPPY_TIME - 1) * SECS_PER_MUD_HOUR) + next_tick;
      else
        secs_left = 0;

      sprintf(happyqp,   "%s+%d%%%s to Questpoints per quest\r\n", CCYEL(ch, C_NRM), HAPPY_QP,   CCNRM(ch, C_NRM));
      sprintf(happygold, "%s+%d%%%s to Units gained per kill\r\n",  CCYEL(ch, C_NRM), HAPPY_UNITS, CCNRM(ch, C_NRM));
      sprintf(happyexp,  "%s+%d%%%s to Experience per kill\r\n",   CCYEL(ch, C_NRM), HAPPY_EXP,  CCNRM(ch, C_NRM));

      send_to_char(ch, "CyberASSAULT Happy Hour!\r\n"
                       "------------------\r\n"
                       "%s%s%sTime Remaining: %s%d%s hours %s%d%s mins %s%d%s secs\r\n",
                       (IS_HAPPYEXP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyexp : "",
                       (IS_HAPPYUNITS || (GET_LEVEL(ch) >= LVL_GOD)) ? happygold : "",
                       (IS_HAPPYQP || (GET_LEVEL(ch) >= LVL_GOD)) ? happyqp : "",
                       CCYEL(ch, C_NRM), (secs_left / 3600), CCNRM(ch, C_NRM),
                       CCYEL(ch, C_NRM), (secs_left % 3600) / 60, CCNRM(ch, C_NRM),
                       CCYEL(ch, C_NRM), (secs_left % 60), CCNRM(ch, C_NRM) );
  }
  else
  {
      send_to_char(ch, "Sorry, there is currently no happy hour!\r\n");
  }
}

ACMD(do_happyhour)
{
  char arg[MAX_INPUT_LENGTH], val[MAX_INPUT_LENGTH];
  int num;

  if (GET_LEVEL(ch) < LVL_GOD)
  {
    show_happyhour(ch);
    return;
  }

  /* Only Imms get here, so check args */
  two_arguments(argument, arg, val);

  if (is_abbrev(arg, "experience"))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_EXP = num;
    send_to_char(ch, "Happy Hour Exp rate set to +%d%%\r\n", HAPPY_EXP);
  }
  else if ((is_abbrev(arg, "units")) || (is_abbrev(arg, "units")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_UNITS = num;
    send_to_char(ch, "Happy Hour Units rate set to +%d%%\r\n", HAPPY_UNITS);
  }
  else if ((is_abbrev(arg, "time")) || (is_abbrev(arg, "ticks")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    if (HAPPY_TIME && !num)
      game_info("Happyhour has been stopped!");
    else if (!HAPPY_TIME && num)
      game_info("A Happyhour has started!");

    HAPPY_TIME = num;
    send_to_char(ch, "Happy Hour Time set to %d ticks (%d hours %d mins and %d secs)\r\n",
                                HAPPY_TIME,
                                 (HAPPY_TIME*SECS_PER_MUD_HOUR)/3600,
                                ((HAPPY_TIME*SECS_PER_MUD_HOUR)%3600) / 60,
                                 (HAPPY_TIME*SECS_PER_MUD_HOUR)%60 );
  }
  else if ((is_abbrev(arg, "qp")) || (is_abbrev(arg, "questpoints")))
  {
    num = MIN(MAX((atoi(val)), 0), 1000);
    HAPPY_QP = num;
    send_to_char(ch, "Happy Hour Questpoints rate set to +%d%%\r\n", HAPPY_QP);
  }
  else if (is_abbrev(arg, "show"))
  {
    show_happyhour(ch);
  }
  else if (is_abbrev(arg, "default"))
  {
    HAPPY_EXP = 100;
    HAPPY_UNITS = 50;
    HAPPY_QP  = 50;
    HAPPY_TIME = 48;
    game_info("A Happyhour has started!");
  }
  else
  {
    send_to_char(ch, "Usage: %shappyhour              %s- show usage (this info)\r\n"
                     "       %shappyhour show         %s- display current settings (what mortals see)\r\n"
                     "       %shappyhour time <ticks> %s- set happyhour time and start timer\r\n"
                     "       %shappyhour qp <num>     %s- set qp percentage gain\r\n"
                     "       %shappyhour exp <num>    %s- set exp percentage gain\r\n"
                     "       %shappyhour units <num>   %s- set units percentage gain\r\n"
                     "       @yhappyhour default      @w- sets a default setting for happyhour\r\n\r\n"
                     "Configure the happyhour settings and start a happyhour.\r\n"
                     "Currently 1 hour IRL = %d ticks\r\n"
                     "If no number is specified, 0 (off) is assumed.\r\nThe command @yhappyhour time@n will therefore stop the happyhour timer.\r\n",
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCYEL(ch, C_NRM), CCNRM(ch, C_NRM),
                     (3600 / SECS_PER_MUD_HOUR) );
  }
}

ACMD(do_scavenger)
{
    char arg[MAX_INPUT_LENGTH];
    struct obj_data *j;
    struct obj_data *jj;
    struct obj_data *next_thing2;
    struct obj_data *item;
    int die_size = 400;
    int die_roll = rand_number(0, die_size);
    int i = 0;

    // last entry must always have index die_size
    const struct scavenge_info {
        int die_roll;
        char *message;
        int item_number;
        int units;
    }
    scavenge_list[] = {
        {0, "You find 1000 resource units!\r\n", -1, 1000},
        {11, "Nothing of interest is found.\r\n", -1, 0},
        {12, "You find a huge box of clip ammo!\r\n", 1513, 0},
        {13, "You find some trash!\r\n", 8015, 0},
        {14, "You find @Ya Golden Stone!@n\r\n", 5, 0},
        {15, "You find a machine pistol!\r\n", 3023, 0},
        {16, "You find a buff drink!\r\n", 8044, 0},
        {17, "You find some nachos with cheese!\r\n", 3014, 0},
        {18, "You find a restring ticket!\r\n", 22771, 0},
        {19, "You find a Ridged Hilt with Handguard!\r\n", 1290, 0},
        {20, "You find a human head! Eww!\r\n", 22745, 0},
        {21, "You find a piece of bio armor!\r\n", 21100, 0},
        {22, "You find 1000 resource units!\r\n", -1, 1000},
        {26, "You find a huge box of clip ammo!\r\n", 1513, 0},
        {27, "You find a huge box of shell ammo!\r\n", 1514, 0},
        {28, "You find a large backpack of energy!\r\n", 1518, 0},
        {29, "You find a Vial of GOD!\r\n", 1006, 0},
        {30, "You find a Vial of Menace!\r\n", 1009, 0},
        {31, "You find a Vial of Voodoo!\r\n", 1020, 0},
        {32, "You find a Vial of KO!\r\n", 1008, 0},
        {33, "You find a large backpack of energy!\r\n", 1518, 0},
//        {34, "You find a colorful gift!!\r\n", 2761, 0},
//        {34, "You find a practice egg!\r\n", 3057, 0},
// Halloween 
//        {35, "You find a buff shroom!\r\n", 1192, 0},
//        {36, "You find some candy!!\r\n", 21606, 0},
        {die_size, "You find one measly resource unit.\r\n", -1, 1}
    };

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Scavenge what?\n\r");
        return;
    }

    if (!(j = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(j = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) {
        send_to_char(ch, "It doesn't seem to be here.\n\r");
        return;
    }
	if (GET_OBJ_TYPE(j) == ITEM_BODY_PART) {
		salvagebionic(ch, j);
		return;
	}
    if (!CAN_WEAR(j, ITEM_WEAR_TAKE) || (!IS_CORPSE(j))) {
        send_to_char(ch, "You can't scavenge that!\n\r");
        return;
    }

    act("$n searches through $p like a scavenger.", FALSE, ch, j, 0, TO_ROOM);
        send_to_char(ch, "You scavenge what you can from %s before ruthlessly destroying it.\r\n", GET_OBJ_SHORT(j));
	
	if (die_roll == 18 && rand_number(0,100) > 40)
		die_roll = 16;
    // search through the scavenge list to find where the dieroll gets you
    for (i = 0; scavenge_list[i].die_roll < die_size && die_roll > scavenge_list[i].die_roll; i++);

    send_to_char(ch, "%s", scavenge_list[i].message);
    if (scavenge_list[i].item_number != -1) {
        item = read_object(real_object(scavenge_list[i].item_number), REAL);
        obj_to_char(item, ch);
    }
    else if (scavenge_list[i].units != -1) {
        GET_UNITS(ch) += scavenge_list[i].units;
    }

    // remove the scavenged item
    for (jj = j->contains; jj; jj = next_thing2) {
        next_thing2 = jj->next_content;       /* Next in inventory */
        obj_from_obj(jj);

        if (j->carried_by)
            obj_to_room(jj, IN_ROOM(j));
        else if (IN_ROOM(j) != NOWHERE)
            obj_to_room(jj, IN_ROOM(j));
        else
            assert(FALSE);
    }

    extract_obj(j);
}

ACMD(do_medkit)
{
    struct char_data *patient;
    struct obj_data *kit;
    int hit = 0;
    int psi = 0;
    int move = 0;
    int overdose = 0;
    bool found = FALSE;
    char arg[MAX_INPUT_LENGTH];

    if (GET_SKILL_LEVEL(ch, SKILL_FIRST_AID) == 0) {
        send_to_char(ch, "You have no idea how to perform first aid on anything.\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, arg);

    kit = GET_EQ(ch, WEAR_HOLD);

    if (!kit || GET_OBJ_TYPE(kit) != ITEM_MEDKIT) {
        kit = ch->carrying;
        while (kit && !found) {
            if (GET_OBJ_TYPE(kit) == ITEM_MEDKIT) {
                found = TRUE;
                break;
            }
            kit = kit->next_content;
        }
        if (!kit || GET_OBJ_TYPE(kit) != ITEM_MEDKIT) {
            send_to_char(ch, "You need to be holding a medkit to perform first aid.\r\n");
            return;
        }
    }

    patient = get_char_room_vis(ch, arg, NULL);

    if (!patient) {
        send_to_char(ch, "You don't see anyone around by that name.  Maybe you're too late!\r\n");
        return;
    }

    if (patient == ch) {
        hit = GET_OBJ_VAL(kit, 0);
        GET_HIT(patient) = MIN(GET_MAX_HIT(patient), GET_HIT(patient) + hit);

        psi = GET_OBJ_VAL(kit, 1);
        GET_PSI(patient) = MIN(GET_MAX_PSI(patient), GET_PSI(patient) + psi);

        move = GET_OBJ_VAL(kit, 2);
        GET_MOVE(patient) = MIN(GET_MAX_MOVE(patient), GET_MOVE(patient) + move);

        send_to_char(ch, "You bandage your wounds.\r\n");
        act("$n bandages $s wounds.", TRUE, ch, 0, 0, TO_ROOM);
        update_pos(ch);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 15;
		increment_skilluse(SKILL_FIRST_AID, 1, GET_IDNUM(ch), exp, ch);

        if (GET_OBJ_VAL(kit, 3)-- <= 0) {
            send_to_char(ch, "The medkit is now empty.\r\n");
            extract_obj(kit);
        }
    }
    else {
        hit = GET_OBJ_VAL(kit, 0);
        GET_HIT(patient) = MIN(GET_MAX_HIT(patient), GET_HIT(patient) + hit);

        psi = GET_OBJ_VAL(kit, 1);
        GET_PSI(patient) = MIN(GET_MAX_PSI(patient), GET_PSI(patient) + psi);

        move = GET_OBJ_VAL(kit, 2);
        GET_MOVE(patient) = MIN(GET_MAX_MOVE(patient), GET_MOVE(patient) + move);

        send_to_char(ch, "You bandage the wounds.\r\n");
        act("$n bandages $N's wounds.", TRUE, ch, 0, patient, TO_NOTVICT);
        act("$n bandages your wounds.", FALSE, ch, 0, patient, TO_VICT);
        update_pos(patient);
		int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 30;
		increment_skilluse(SKILL_FIRST_AID, 1, GET_IDNUM(ch), exp, ch);

        if (GET_OBJ_VAL(kit, 3)-- <= 0) {
            send_to_char(ch, "The medkit is now empty.\r\n");
            extract_obj(kit);
        }
    }
	
	if (!IS_NPC(patient)) {
		overdose = (hit + psi + move) / 50;
		GET_OVERDOSE(patient) += overdose;
	}

    if (GET_OVERDOSE(patient) > 200) {
        send_to_char(patient, "You start to feel funny inside, all those injections are inter-mixing!\r\n");
        act("$N starts to sway and stumble.", TRUE, ch, 0, patient, TO_NOTVICT);
        do_say(patient, "I don't feel so good...", 0, 0);
        act("$N suddenly faints and falls to the ground, convulsing!", TRUE, ch, 0, patient, TO_NOTVICT);
        update_pos(patient);
    }
}

ACMD(do_scan)
{
    bool found = FALSE;
    struct obj_data *i;
    struct obj_data *obj;
    char obj_temp[MAX_INPUT_LENGTH];
    int j = 0;

    if (GET_SKILL_LEVEL(ch, SKILL_GADGETRY) == 0) {
        send_to_char(ch, "You need an identifier and the gadgetry skill to scan items.\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't....move!\r\n");
        return;
    }

    argument = one_argument(argument, obj_temp);

    i = ch->carrying;

    while (i != NULL && found != TRUE) {
        if (GET_OBJ_TYPE(i) == ITEM_IDENTIFY)
            found = TRUE;
        else
            i = i->next_content;
    }

    for (j = 0; j < NUM_WEARS; j++)
        if (GET_EQ(ch, j) && GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_IDENTIFY) {
            found = TRUE;
            break;
        }

    if (!found) {
        send_to_char(ch, "Too bad you don't have an identifier.\n\r");
        return;
    }

    if (!*obj_temp) {
        send_to_char(ch, "Yes, but WHAT do you want to scan?\r\n");
        return;
    }

    if (!(obj = get_obj_in_list_vis(ch, obj_temp, NULL, ch->carrying)))
        send_to_char(ch, "You don't seem to have %s %s.\n\r", AN(obj_temp), obj_temp);
    else
        call_psionic(ch, NULL, obj, PSIONIC_IDENTIFY, 30, 0, TRUE);

}

#define NO_RECALL(room) (ROOM_FLAGGED((room), ROOM_NO_RECALL) || \
                        ZONE_FLAGGED(world[(room)].zone, ZONE_NO_RECALL) || \
                        ROOM_FLAGGED((room), ROOM_RADIATION))

ACMD(do_recall)
{
	char arg1[MAX_INPUT_LENGTH];
    int i;
    bool enhanced = FALSE;
	struct obj_data *obj, *next_obj;
    struct obj_data *beacon = NULL;
    struct follow_type *f;
    struct follow_type *next_fol;
	struct char_data *target;
	struct char_data *newtarget;
	int num = 0;
	int recallroom = 0;

	one_argument(argument, arg1);

	if (!*arg1) {
	    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
		    send_to_char(ch, "You...can't...move!\r\n");
			return;
		}
		if (GET_LEVEL(ch) < LVL_IMMORT && ZONE_FLAGGED(world[IN_ROOM(ch)].zone, ZONE_QUEST)) {
			send_to_char(ch, "Interference prevents sensors from locking onto your signal!\n\r");
			return;
		}
	    // We're gonna cost them 150 moves when they recall, having to do with re-arranging atoms or something,
		// should make you really tired, so now you gotta go regen :)  -Lump
		if (!IS_NPC(ch) && GET_LEVEL(ch) >= 15 && GET_LEVEL(ch) < LVL_IMMORT && GET_MOVE(ch) < 100) {
			send_to_char(ch, "Your body is too tired to handle a transportation right now.\r\n");
			return;
		}
		if (IN_VEHICLE(ch)) {
			send_to_char(ch, "Interference from your vehicle's GPS prevent sensors from locking onto your signal!\r\n");
			return;
		}
	    // signal enhancement:  If they have an ITEM_RECALL object worn, then they'll turn it on before they recall
		// they must have the gadgetry skill, and the beacon must have a charge (value slot 0)
		// this allows infinite recalls, if the value is -1 (similar to lights)
		if (!IS_NPC(ch) && GET_SKILL_LEVEL(ch, SKILL_GADGETRY) && GET_POS(ch) != POS_FIGHTING) {
			for (i = 0; i < NUM_WEARS; i++) {
				if ((beacon = GET_EQ(ch, i)) && CAN_SEE_OBJ(ch, beacon) && GET_OBJ_TYPE(beacon) == ITEM_RECALL) {
					if (GET_CHARGE(beacon) != 0) {

						act("You activate $p.", FALSE, ch, beacon, 0, TO_CHAR);
						enhanced = TRUE;

						if (GET_CHARGE(beacon) > 0)
							GET_CHARGE(beacon)--; /* decrease the charge */

						break;
					}

					// if the beacon doesn't have a charge left, then ignore it
				}
			}
		}

		if (NO_RECALL(IN_ROOM(ch)) && GET_LEVEL(ch) < LVL_IMMORT && !enhanced) {
			send_to_char(ch, "Interference prevents sensors from locking onto your signal!\n\r");
			return;
		}

		act("$n slowly is engulfed with a beam of light and dematerializes.\r\n", TRUE, ch, 0, 0, TO_ROOM);
		char_from_room(ch);
	
		if (GET_LEVEL(ch) <= 7 && GET_PROGRESS(ch) <= 3) {
			recallroom = 4111;
        act("You are engulfed in a beam of light and materialize in High Point.\r\n", true, ch, 0, 0, TO_CHAR);
}
		else if (GET_LEVEL(ch) < LVL_IMMORT) {
			recallroom = 3001;
        act("You are engulfed in a beam of light and materialize at The Old Motown Armory.\r\n", true, ch, 0, 0, TO_CHAR);
}

		else {
			recallroom = 1204;
        act("You are teleported to the Immortal Board Room.\r\n", true, ch, 0, 0, TO_CHAR);
		}	
		char_to_room(ch, real_room(recallroom));

		act("$n slowly materializes into existence.", TRUE, ch, 0, 0, TO_ROOM);
			look_at_room(ch, 0);

		if (!IS_NPC(ch) && GET_LEVEL(ch) >= 15 && GET_LEVEL(ch) < LVL_IMMORT)
			GET_MOVE(ch) -= 100; /* recalling tires you out */

		entry_memory_mtrigger(ch);
		greet_mtrigger(ch, -1);
		greet_memory_mtrigger(ch);

		// heh, battery died
		if (enhanced && beacon && GET_CHARGE(beacon) == 0)
			act("The power light on $p slowly dims, then dies.", FALSE, ch, beacon, 0, TO_CHAR);

		// pet auto-recall! -DH
		for (f = ch->followers; f; f = next_fol) {

			next_fol = f->next;

	        if (AFF_FLAGGED(f->follower, AFF_GROUP) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
				char_from_room(f->follower);
				char_to_room(f->follower, real_room(recallroom));
			}

		}

		if (FIGHTING(ch))
			stop_fighting(ch);
		if (RANGED_FIGHTING(ch)) {
			target = RANGED_FIGHTING(ch);
			if (target != NULL)
				stop_ranged_fighting(target);
			stop_ranged_fighting(ch);
		}//testsnipebug
		if (LOCKED(ch)) {
			newtarget = LOCKED(ch);
			if (RANGED_FIGHTING(newtarget))
				stop_ranged_fighting(newtarget);
			LOCKED(ch) = NULL;
		}
	}
	else {
		if (!strcmp(arg1, "vehicle")) {
			if (IN_VEHICLE(ch)) {
				send_to_char(ch, "You can't recall a vehicle when you're inside it.\r\n");
				return;
			}
			for (obj = object_list; obj; obj = next_obj) {
				next_obj = obj->next;
				if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
					if (IS_BOUND(obj)) {
						if (obj->bound_id == GET_IDNUM(ch)) {
							num++;
							obj_from_room(obj);
							obj_to_room(obj, real_room(LOT_VNUM));
						}
				
					}
				}
			}
			if (num > 0)
				send_to_char(ch, "Your vehicles have been returned to their parking lot.\r\n");
			else
				send_to_char(ch, "You don't have any vehicles to return to the parking lot.\r\n");
		}
	}
}

ACMD(do_transport)
{
    char where[MAX_INPUT_LENGTH];
    int loc;
    bool in_transporter = FALSE;
    bool found = FALSE;
    room_rnum to_room;

    const struct {
        room_vnum transporter;
        room_vnum destination;
        char *name;
        bitvector_t class_restriction;
        bool remort_restriction;
        char *desc;
    }
    trans_locations[] = {
        { 1605,  10800, "arena",    0,              0, "arena - The CyberASSAULT arena"},
        { 16563, 3301,  "dragon",   0,              0, "dragon - Red Dragon Space Station"},
        { 3093,  1532,  "academy",  0,              0, "academy - Far into the desert\r\n"},
        { 3093,  2900,  "dixie",    0,              0, "dixie - Watch out for rednecks!\r\n"},
        { 3093,  13100, "doom",     0,              0, "doom - Nuff said\r\n"},
        { 3093,  16577, "library",  0,              0, "library - Motown Library\r\n"},
//        { 3093,  15354, "ca",       0,              1, "ca - CyberASSAULT Staging Grounds (remort only!)\r\n"},
        { 3093,  13600, "pred",     CLASS_PREDATOR, 0, "pred - The predator planet (predator only!)\r\n"},
        { 13657, 13666, "kendra",   CLASS_PREDATOR, 0, "Kendra\r\n"},
        { 13657, 13689, "benecia",  CLASS_PREDATOR, 0, "Benecia\r\n"},
        { 13657, 13702, "xanthras", CLASS_PREDATOR, 0, "Xanthras\r\n"},
        { 13657, 13724, "deriben",  CLASS_PREDATOR, 0, "Deriben\r\n"},
        { 13657, 13750, "melona",   CLASS_PREDATOR, 0, "Melona\r\n"},
        { 22100, 1532,  "academy",  0,              0, "academy - Far into the desert\r\n"},
        { 22100, 2900,  "dixie",    0,              0, "dixie - Watch out for rednecks!\r\n"},
        { 22100, 13100, "doom",     0,              0, "doom - Nuff said\r\n"},
        { 22100, 16577, "library",  0,              0, "library - Neo-chicago library\r\n"},
        { 22100, 13600, "pred",     CLASS_PREDATOR, 0, "pred - The predator planet (predator only!)\r\n"},
        { 9908,  -1,    "shrine",   0,              1, "Shrine - Immortal Museum\r\n"},
        {    -1,    -1, NULL,       0,              0, NULL}
    };

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    one_argument(argument, where);
    *where = LOWER(*where);

    // cycle through the transport locations looking for a match by room
    for (loc = 0; trans_locations[loc].name != NULL; loc++) {

        // are we in a transport room?
        if (IN_ROOM(ch) == real_room(trans_locations[loc].transporter)) {

            in_transporter = TRUE;

            // do we have a desired location that matches?
            if (trans_locations[loc].name != NULL && !strcmp(where, trans_locations[loc].name)) {

                // check for remort restrictions
                if (trans_locations[loc].remort_restriction && !IS_REMORT(ch)) {
                        if (!trans_locations[loc].name)
                            send_to_char(ch, "Only Remorts may use this transporter.\n\r");
                        else
                            send_to_char(ch, "Only Remorts may use this transporter to %s.\n\r", where);
                        return;
                }

                // check for class restrictions
				if (trans_locations[loc].class_restriction) {
					if (trans_locations[loc].class_restriction == CLASS_PREDATOR && !IS_PREDATOR(ch)) {
					    send_to_char(ch, "Only players in the Predator tree may transport here.\n\r");
                        return;
					}
                }

                to_room = real_room(trans_locations[loc].destination);
                // special code to handle the random shrine -- w00t!
                if (IN_ROOM(ch) == real_room(9908)) {
                    switch (rand_number(1,9)) {
                        case 1: to_room = real_room(9911);  break;
                        case 2: to_room = real_room(9913);  break;
                        case 3: to_room = real_room(9915);  break;
                        case 4: to_room = real_room(9917);  break;
                        case 5: to_room = real_room(9919);  break;
                        case 6: to_room = real_room(9921);  break;
                        case 7: to_room = real_room(9923);  break;
                        case 8: to_room = real_room(9925);  break;
                        case 9: to_room = real_room(9927);  break;
                    }
                }

                send_to_char(ch, "Energizing...\n\r");
                act("$n disappears into the portal", FALSE, ch, 0, 0, TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, to_room);
            act("$n slowly materializes into existence.", TRUE, ch, 0, 0, TO_ROOM);
            do_look(ch, "", 15, 0);
            entry_memory_mtrigger(ch);
            greet_mtrigger(ch, -1);
            greet_memory_mtrigger(ch);
                return;
            }
        }
    }

    // did not find a suitable destination
    if (in_transporter) {
        if (!*where) {
            // just typed 'transport' in room, tell them where they can go
            found = FALSE;
            for (loc = 0; trans_locations[loc].name != NULL; loc++) {
                if (IN_ROOM(ch) == real_room(trans_locations[loc].transporter))
                    if (trans_locations[loc].desc) {
                        if (!found) {
                            send_to_char(ch, "Try the following locations:\r\n");
                            found = TRUE;
                        }
                        send_to_char(ch, "%s", trans_locations[loc].desc);
                    }
            }
        }
        else
            send_to_char(ch, "That location is not accessable from this transporter.  Type 'transport' to get a list of locations.\n\r");
    }
    else
        send_to_char(ch, "You cannot transport from your current location.\n\r");

}

ACMD(do_suicid)
{
	send_to_char(ch, "If you want to commit suicide, please type it out in full.\r\n");
	return;
}
	
ACMD(do_suicide)
{

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_SUICIDE)) {
        send_to_char(ch, "You try but something stops you.\r\n");
        return;
    }

    switch(rand_number(1, 6)) {

        case 1:
            act("$n just can't take it anymore and raises a pistol to $s temple!", FALSE, ch, 0, 0, TO_ROOM);
            act("$n squeezes the trigger and brains and blood spray!", FALSE, ch, 0,0, TO_ROOM);
            break;

        case 2:
            act("$n can't handle the pressures of mudding on CyberASSAULT anymore.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n fashions a noose and hangs $mself!", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 3:
            act("$n puts some poison in a glass of water and drinks it!", FALSE, ch, 0, 0, TO_ROOM);
            act("$n has committed suicide!\n\r", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 4:
            act("$n can't handle the pressures of mudding on CyberASSAULT anymore.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n listens to Vanilla Ice songs over and over until $e dies!", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 5:
            act("$n can't handle the pressure of mudding on CyberASSAULT anymore.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n hooks $mself up to Dr. Jack Kevorkian's 'Do It Yourself Compassion Drip' and pushes the button!",
            FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 6:
            act("$n can't handle this world anymore.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n finds the old hamper and pulls out a pair of Shadowrunner's dirty Underwear.", FALSE, ch, 0, 0, TO_ROOM);
            act("$n takes a BIG sniff of the fly surrounded garments.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        default:
            // This should never ever happen!
            break;
    }

    send_to_char(ch, "You have committed suicide!\n\r");
    mudlog(BRF, 0, TRUE, "%s committed suicide.", GET_NAME(ch));

    raw_kill(ch, ch, FALSE);

}

ACMD(do_ventriloquate)
{
    struct char_data *vict;
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];

    half_chop(argument, buf, buf2);

    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
    }

    if (!*buf || !*buf2)
        send_to_char(ch, "Ventriloquate what for who?!?!?\n\r");
    else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
        send_to_char(ch, "No-one by that name here...\n\r");
    else if (ch == vict)
        send_to_char(ch, "Why do that? Just say it!\n\r");
    else if (GET_LEVEL(vict) > LVL_IMMORT)
        send_to_char(ch, "That might not be a very good idea...\r\n");
    else if (!HAS_BIONIC(ch, BIO_VOICE) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You ventriloquate a message in an impersonated voice.\n\r");
        act("$n does a bad impression of John Wayne, or is it Bugs Bunny?", FALSE, ch, 0, 0, TO_ROOM);
    }
    else {
        sprintf(buf, "%s says '%s'\r\n", GET_NAME(vict), buf2);
        act(buf, FALSE,  ch,  0, 0, TO_ROOM);
        send_to_char(ch, "You ventriloquate a message in an impersonated voice.\n\r");
    }
}

ACMD(do_recover)
{
    FILE *fl;
    char filename[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
    obj_save_data *loaded;
    obj_save_data *current;
	int num_objs;
	int cost;
	int i;
	struct obj_data *cont_row[MAX_BAG_ROWS];

	one_argument(argument, arg);

    snprintf(filename, sizeof(filename), LIB_DEATHOBJ"%s.corpse", GET_NAME(ch));

	if (!(fl = fopen(filename, "r"))) {
		send_to_char(ch, "You do not have any objects to recover.\r\n");
		return;
	}

	if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOSPITAL)) {
		send_to_char(ch, "You can only recover your corpse from within the morgue.\r\n");
		send_to_char(ch, "To see a list of rooms that are morgues, please type 'help morgue'\r\n");
		fclose(fl);
		return;
	}

	if (GET_LEVEL(ch) <= 10) {
		cost = 0;
		send_to_char(ch, "The cost to recover objects is free until level 10.\r\n");
	}
	else if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20)
		cost = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 50;
	else if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30)
		cost = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 100;
	else if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40)
		cost = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 200;
	else if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50)
		cost = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 400;
	else
		cost = 0;
	if (!*arg) {
		if (cost > GET_UNITS(ch)) {
			send_to_char(ch, "You do not have enough units to afford a recovery.\r\n");
			send_to_char(ch, "You can type 'recover exp' to recover at the expense of experience points.\r\n");
			fclose(fl);
			return;
		}

		else {
			GET_UNITS(ch) -= cost;
			send_to_char(ch, "You spend %d units.\r\n", cost);
		}
	}

	else if (!strcmp(arg, "exp")) {
		cost = cost * 2;
		GET_EXP(ch) -= cost;
		send_to_char(ch, "You spend %d experience.\r\n", cost);
	}

	else {
		send_to_char(ch, "Improper argument for 'recover' command.  Type 'help recover' for more details.\r\n");
		fclose(fl);
		return;
	}

    for (i = 0; i < MAX_BAG_ROWS; i++)
        cont_row[i] = NULL;

    loaded = objsave_parse_objects(fl);

    for (current = loaded; current != NULL; current=current->next)
        num_objs += handle_obj(current->obj, ch, current->locate, cont_row);

    // now it's safe to free the obj_save_data list - all members of it
    // have been put in the correct lists by obj_to_room()
    while (loaded != NULL) {
        current = loaded;
        loaded = loaded->next;
        free(current);
    }

    fclose(fl);

	if (remove(filename)) {
		return;
	}
	save_char(ch);
	send_to_char(ch, "\r\nAn emergency robot appears in the room with a giant duffelbag.\r\n");
	send_to_char(ch, "After scanning your retina, the robot dumps all of your equipment onto you.\r\n");
}

ACMD(do_gain)
{
    int i;
    bool level_gain = FALSE;


    if ((GET_LEVEL(ch) < (LVL_IMMORT - 1)) && GET_LEVEL(ch) > 0) {
        if (!IS_NPC(ch)) {
            for (i = 0; (exptolevel(ch, i)) <= GET_EXP(ch); i++) {
                while ((level_gain != TRUE) && i > GET_LEVEL(ch)) {

					GET_EXP(ch) -= (exptolevel(ch, GET_LEVEL(ch) + 1));
					if (!PRF_FLAGGED(ch, PRF_BLIND)) {
						send_to_char(ch, "@Y       ___         ___                     ___        \r\n");
						send_to_char(ch, "|     |     |  /  |     |           |   | |   |   |   \r\n");
						send_to_char(ch, "|     |-+-  | +   |-+-  |           |   | |-+-    +   \r\n");
						send_to_char(ch, "|     |     |/    |     |           |   | |           \r\n");
						send_to_char(ch, " ---   ---         ---   ---         ---          -   @n\r\n\r\n");
					}
                    send_to_char(ch, "You rise a level!\r\n");
                    GET_LEVEL(ch) = i;
                    advance_level(ch);
                    level_gain = TRUE;
                }
            }
        }
    }

    if (level_gain == FALSE)
        send_to_char(ch, "But you don't have enough experience!\r\n");
}

ACMD(do_qcreate)
{
    struct obj_data *qobj;
    char buf2[MAX_INPUT_LENGTH];

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
        send_to_char(ch, "Create what?\r\n");
        return;
    }

    qobj = create_obj();
    qobj->item_number = NOTHING;
    IN_ROOM(qobj) = NOWHERE;
    sprintf(buf2, "quest %s", argument);
    qobj->name = strdup(buf2);
    sprintf(buf2, "%s has been left here.", argument);
    qobj->description = strdup(buf2);
    sprintf(buf2, "%s", argument);
    qobj->short_description = strdup(buf2);
    GET_OBJ_TYPE(qobj) = ITEM_QUEST;
    SET_BIT_AR(GET_OBJ_WEAR(qobj), ITEM_WEAR_TAKE);
    SET_BIT_AR(GET_OBJ_WEAR(qobj), ITEM_WEAR_HOLD);
    SET_BIT_AR(GET_OBJ_EXTRA(qobj), ITEM_NORENT);
    SET_BIT_AR(GET_OBJ_EXTRA(qobj), ITEM_NODONATE);

    GET_OBJ_WEIGHT(qobj) = 1;
    obj_to_char(qobj, ch);

    act("$n skillfully creates something!\r\n", FALSE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "You skillfully create something! \r\n");
}


ACMD(do_recon)
{
  struct char_data *i;
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
    "[Eagle Eye] in the distance ", //8 rooms
    "[Eagle Eye] in the distance ",
    "[Eagle Eye] in the distance ",
  };

    if (GET_SKILL_LEVEL(ch, SKILL_RECON) == 0) {
        send_to_char(ch, "You have no idea how to recon.\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_BLIND) && GET_LEVEL(ch) < LVL_IMMORT) {
    act("You can't see anything, you're blind!", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((GET_MOVE(ch) < 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    act("You are too exhausted.", TRUE, ch, 0, 0, TO_CHAR);
    return;
  }
  maxdis = (1 + ((GET_SKILL_LEVEL(ch, SKILL_RECON) * 5) / 100));
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    maxdis = 7;
//    if (HAS_BIONIC(ch, BIO_EYE)) old changed to below 5/26/2016 bosstone
if (GET_BODYPART_CONDITION(ch, BODY_PART_LEFT_EYE, BIONIC_NORMAL))
    maxdis = 6;
if (GET_BODYPART_CONDITION(ch, BODY_PART_RIGHT_EYE, BIONIC_NORMAL))
    maxdis = 6;
if (GET_SKILL_LEVEL(ch, SKILL_EAGLE_EYE) != 0)
    maxdis = 10;


  act("You perform a quick recon of your surroundings and see:", TRUE, ch, 0, 0, TO_CHAR);
  act("$n recons the area like a true soldier.", FALSE, ch, 0, 0, TO_ROOM);
  if (GET_LEVEL(ch) < LVL_IMMORT)
    GET_MOVE(ch) -= 20;

  is_in = ch->in_room;
  for (dir = 0; dir < NUM_OF_DIRS; dir++) {
    ch->in_room = is_in;
    for (dis = 0; dis <= maxdis; dis++) {
      if (((dis == 0) && (dir == 0)) || (dis > 0)) {
        for (i = world[ch->in_room].people; i; i = i->next_in_room) {
          if ((!((ch == i) && (dis == 0))) && CAN_SEE(ch, i)) {
		  	if (LOCKED(ch)) {
				if (i == LOCKED(ch))
					send_to_char(ch, "@R(TARGET)@n");
			}
            sprintf(buf, "%33s: %s%s%s%s", GET_NAME(i), distance[dis],
                    ((dis > 0) && (dir < (NUM_OF_DIRS - 2))) ? "to the " : "",
                    (dis > 0) ? dirs[dir] : "",
                    ((dis > 0) && (dir > (NUM_OF_DIRS - 3))) ? "wards" : "");
            act(buf, TRUE, ch, 0, 0, TO_CHAR);
        send_to_char(i, "@D...You feel someone's eyes upon your back.@n\n\r");
            found++;
          }
        }
      }
      if (!CAN_GO(ch, dir) || (world[ch->in_room].dir_option[dir]->to_room== is_in))
        break;
      else
        ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
    }
  }
  if (found == 0)
    act("Nobody anywhere near you.", TRUE, ch, 0, 0, TO_CHAR);
  ch->in_room = is_in;
}

ACMD(do_callpet)
{
call_pets(ch);
}

void call_pets(struct char_data *ch)
{
    struct follow_type *f;
    sh_int target;
    struct follow_type *next_fol;

    if (GET_PSIONIC_LEVEL(ch, PSIONIC_CALLPET) == 0) {
        send_to_char(ch, "You must first train this to use it.\r\n");
        return;
    }
    
	if (GET_PSI(ch) <= 50) {
        send_to_char(ch, "You dont have the psi points for that!\r\n");
        return;
	}

    if (GET_PSI(ch) >= 50) {
		(GET_PSI(ch) -= 50);
	}

    if (!AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch, "You dont seem to be grouped with any charmies!\r\n");
        return;
    }

    // pet auto-recall! -DH
    
    target = IN_ROOM(ch);

	send_to_char(ch, "@xYou send a mental signal to your followers, who quickly appear in the room!@n\r\n");

	for (f = ch->followers; f; f = next_fol) {

        next_fol = f->next;
        
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && AFF_FLAGGED(f->follower, AFF_CHARM)) {

			char_from_room(f->follower);
			char_to_room(f->follower, target);
			send_to_char(ch, "%s races into the room looking for its master.\r\n", f->follower->player.short_descr);
	    }
	}
}

ACMD(do_whistle)
{
	struct char_data *pet;
	struct follow_type *f;
	struct follow_type *next_fol;
	struct affected_type af;
	char arg[MAX_INPUT_LENGTH];
	int total = 0;
	bool group = FALSE;
	bool found = FALSE;

	one_argument(argument, arg);

    if AFF_FLAGGED(ch, AFF_CHARM)
        return;

	if (!IS_NPC(ch) && GET_SKILL_LEVEL(ch, SKILL_WHISTLE) == 0) {
		send_to_char(ch, "You have no idea how to whistle for a pet!\r\n");
		return;
	}
	if (GET_PSI(ch) <= 100 && !IS_NPC(ch)) {
		send_to_char(ch, "You don't have enough psionc energy to whistle!\r\n");
		return;
	}
    if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY)) {
        send_to_char(ch, "You...can't...move!\r\n");
        return;
	}
	if (!*arg) {
		send_to_char(ch, "You need to specifiy which pet you want to focus on calling.\r\n");
		return;
	}

	for (f = ch->followers; f; f = next_fol) {
		next_fol = f->next;
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && AFF_FLAGGED(f->follower, AFF_CHARM))
			total++;

	}		

	if (total >= 2 && !IS_NPC(ch)) {
		send_to_char(ch, "You can't control that many pets!\r\n");
		group = FALSE;
		return;
	}
	else {
	
		if (!str_cmp(arg, "wolf")) {
			pet = read_mobile(real_mobile(6099), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "shishi")) {
			pet = read_mobile(real_mobile(6098), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "crocodile")) {
			pet = read_mobile(real_mobile(6097), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "spidermonkey")) {
			pet = read_mobile(real_mobile(6096), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "bombrat")) {
			pet = read_mobile(real_mobile(6095), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "killerbees")) {
			pet = read_mobile(real_mobile(6094), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "shocksnake")) {
			pet = read_mobile(real_mobile(6093), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "turtle")) {
			pet = read_mobile(real_mobile(6092), REAL);
			found = TRUE;
		}
		else if (!str_cmp(arg, "owl")) {
			pet = read_mobile(real_mobile(6091), REAL);
			found = TRUE;
		} 
		else if (!str_cmp(arg, "cougar")) {
			pet = read_mobile(real_mobile(6090), REAL);
			found = TRUE;
		}
			
		else if (!str_cmp(arg, "list")) {
			send_to_char(ch, "Available pets are: wolf, shishi, crocodile, spidermonkey, bombrat,\r\n");
			send_to_char(ch, "killerbees, shocksnake, turtle, owl, cougar.\r\n");
			send_to_char(ch, "Please type whistle <name> to call an animal.\r\n");
			found = FALSE;
		}
		else
			send_to_char(ch, "Unrecognized argument. Try again.\r\n");
			
		if (found == TRUE) {
			if (!IS_NPC(ch))
				GET_PSI(ch) -= 100;
			char_to_room(pet, IN_ROOM(ch));
			new_affect(&af);
			af.type      = PSIONIC_CHARM;
			af.duration  = -1;
			SET_BIT_AR(af.bitvector, AFF_CHARM);
			affect_to_char(pet, &af);

			GET_LEVEL(pet) = GET_LEVEL(ch);
			GET_MAX_HIT(pet) += GET_LEVEL(ch) * 50;
			GET_AC(pet) += GET_LEVEL(ch) * 8;
			GET_HITROLL(pet) += GET_LEVEL(ch) * 15;
			GET_DAMROLL(pet) += GET_LEVEL(ch) * 5;

			int exp = (GET_LEVEL(ch) * (GET_LEVEL(ch) / 2)) * 20;
			increment_skilluse(SKILL_WHISTLE, 1, GET_IDNUM(ch), exp, ch);			
			add_follower(pet, ch);
			perform_group(ch, ch);
			perform_group(ch, pet);
			return;
		}
	}
}
