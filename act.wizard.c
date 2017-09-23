/**************************************************************************
*  File: act.wizard.c                                      Part of tbaMUD *
*  Usage: Player-level god commands and other goodies.                    *
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
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "shop.h"
#include "act.h"
#include "genzon.h" /* for real_zone_by_thing */
#include "class.h"
#include "genolc.h"
#include "fight.h"
#include "house.h"
#include "modify.h"
#include "quest.h"
#include "ban.h"
#include "bionics.h"
#include "craft.h"
#include "drugs.h"
#include "custom.h"
#include "skilltree.h"

/* local utility functions with file scope */
static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(struct char_data *ch, int level);
static void list_zone_commands_room(struct char_data *ch, room_vnum rvnum);
static void do_stat_room(struct char_data *ch, struct room_data *rm);
static void do_roomwho(struct char_data *ch, struct room_data *rm);
static void do_stat_object(struct char_data *ch, struct obj_data *j);
static void do_stat_character(struct char_data *ch, struct char_data *k);
static void stop_snooping(struct char_data *ch);
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);
static struct char_data *is_in_game(long idnum);
static void mob_checkload(struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(struct char_data *ch, trig_vnum tvnum);
static void mod_llog_entry(struct last_entry *llast,int type);
static int  get_max_recent(void);
static void clear_recent(struct recent_player *this);
static struct recent_player *create_recent(void);

const char *get_spec_func_name(SPECIAL(*func));
bool zedit_get_levels(struct descriptor_data *d, char *buf);

/* Local Globals */
static struct recent_player *recent_list = NULL;  /** Global list of recent players */

int purge_room(room_rnum room)
{
  int j;
  struct char_data *vict;

  if (room == NOWHERE || room > top_of_world) return 0;

  for (vict = world[room].people; vict; vict = vict->next_in_room) {
    if (!IS_NPC(vict))
      continue;

    /* Dump inventory. */
    while (vict->carrying)
      extract_obj(vict->carrying);

    /* Dump equipment. */
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(vict, j))
        extract_obj(GET_EQ(vict, j));

    /* Dump character. */
    extract_char(vict);
  }

  /* Clear the ground. */
  while (world[room].contents)
    extract_obj(world[room].contents);

  return 1;
}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 4];

    if (subcmd == SCMD_EMOTE)
      snprintf(buf, sizeof(buf), "$n %s", argument);
    else {
      strlcpy(buf, argument, sizeof(buf));
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s echoed: %s", GET_NAME(ch), buf);
      }
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}

ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s sent %s: %s", GET_NAME(ch), GET_NAME(vict), buf);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL) {
      if ((location = IN_ROOM(target_mob)) == NOWHERE) {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE) {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* A location has been found -- if you're >= GRGOD, no restrictions. */
  if (GET_LEVEL(ch) >= LVL_GRGOD)
    return (location);

  if (ROOM_FLAGGED(location, ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
    send_to_char(ch, "There's a private conversation going on in that room.\r\n");
  else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(location)))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_rnum location;

  if ((location = find_target_room(ch, argument)) == NOWHERE)
    return;

  if (ZONE_FLAGGED(GET_ROOM_ZONE(location), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char(ch, "Sorry, that zone is off-limits for immortals!");
    return;
  }

  snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, location);

  snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(ch, 0);
  enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
}

ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char(ch, "Go transfer someone your own size.\r\n");
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);

      enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "I think not.\r\n");
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
        enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
      }
    send_to_char(ch, "%s", CONFIG_OK);
  }
}

ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE) {
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
    enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
  }
}

ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  int good_arg = 0;

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob") && (good_arg = 1))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj") && (good_arg =1 ))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "room") && (good_arg = 1))
    if (!vnum_room(buf2, ch))
      send_to_char(ch, "No rooms by that name.\r\n");

  if (is_abbrev(buf, "trig") && (good_arg = 1))
    if (!vnum_trig(buf2, ch))
      send_to_char(ch, "No triggers by that name.\r\n");

  if (!good_arg)
     send_to_char(ch, "Usage: vnum { obj | mob | room | trig } <name>\r\n");
 }

#define ZOCMD zone_table[zrnum].cmd[subcmd]

static void list_zone_commands_room(struct char_data *ch, room_vnum rvnum)
{
  zone_rnum zrnum = real_zone_by_thing(rvnum);
  room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE) {
    send_to_char(ch, "No zone information available.\r\n");
    return;
  }

  get_char_colors(ch);

  send_to_char(ch, "Zone commands in this room:%s\r\n", yel);
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        break;
    }
    if (cmd_room == rrnum) {
      count++;
      /* start listing */
      switch (ZOCMD.command) {

                case 'M':
                    send_to_char(ch, "%sLoad %s [%s%d%s], Max : %d, Freq. : %d\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        mob_proto[ZOCMD.arg1].player.short_descr, cyn,
                        mob_index[ZOCMD.arg1].vnum, yel,
                        ZOCMD.arg2, ZOCMD.arg4
                        );
                    break;

                case 'G':
                    send_to_char(ch, "%sGive it %s [%s%d%s], Max : %d, Freq. : %d\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description,
                        cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        ZOCMD.arg2, ZOCMD.arg4
                        );
                    break;

                case 'O':
                    send_to_char(ch, "%sLoad %s [%s%d%s], Max : %d, Freq. : %d\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description,
                        cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        ZOCMD.arg2, ZOCMD.arg4
                        );
                    break;

                case 'E':
                    send_to_char(ch, "%sEquip with %s [%s%d%s], %s, Max : %d, Freq. : %d\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description,
                        cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        equipment_types[ZOCMD.arg3],
                        ZOCMD.arg2, ZOCMD.arg4
                        );
                    break;

                case 'P':
                    send_to_char(ch, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d, Freq. : %d\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description,
                        cyn, obj_index[ZOCMD.arg1].vnum, yel,
                        obj_proto[ZOCMD.arg3].short_description,
                        cyn, obj_index[ZOCMD.arg3].vnum, yel,
                        ZOCMD.arg2, ZOCMD.arg4
                        );
                    break;

                case 'R':
                    send_to_char(ch, "%sRemove %s [%s%d%s] from room.\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg2].short_description,
                        cyn, obj_index[ZOCMD.arg2].vnum, yel
                        );
                    break;

                case 'L':
                    send_to_char(ch, "%sLoad gun with %s [%s%d%s]\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        obj_proto[ZOCMD.arg1].short_description,
                        cyn, obj_index[ZOCMD.arg1].vnum, yel
                        );
                    break;

                case 'D':
                    send_to_char(ch, "%sSet door %s as %s.\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        dirs[ZOCMD.arg2],
                        ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open"
                        );
                    break;

                case 'T':
                    send_to_char(ch, "%sAttach trigger %s%s%s [%s%d%s] to %s\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        cyn, trig_index[ZOCMD.arg2]->proto->name, yel,
                        cyn, trig_index[ZOCMD.arg2]->vnum, yel,
                        ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
                        ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                        ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????")))
                        );
                    break;

                case 'V':
                    send_to_char(ch, "%sAssign global %s:%d to %s = %s\r\n",
                        ZOCMD.if_flag ? " then " : "",
                        ZOCMD.sarg1, ZOCMD.arg2,
                        ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
                        ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                        ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????"))),
                        ZOCMD.sarg2
                        );
                    break;

                default:
                    send_to_char(ch, "<Unknown Command>\r\n");
                    break;
      }
    }
    subcmd++;
  }
  send_to_char(ch, "%s", nrm);
  if (!count)
    send_to_char(ch, "None!\r\n");
}
#undef ZOCMD

static void do_stat_room(struct char_data *ch, struct room_data *rm)
{
  char buf2[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  send_to_char(ch, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));

  sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
  send_to_char(ch, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), real_room(rm->number), (long) rm->number + ROOM_ID_BASE, buf2);

  sprintbitarray(rm->room_flags, room_bits, RF_ARRAY_MAX, buf2);
  send_to_char(ch, "SpecProc: %s, Flags: %s\r\n", rm->func == NULL ? "None" : get_spec_func_name(rm->func), buf2);

  send_to_char(ch, "Description:\r\n%s", rm->description ? rm->description : "  None.\r\n");

  if (rm->ex_description) {
    send_to_char(ch, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [%s]", desc->keyword);
    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
  }

  send_to_char(ch, "Chars present:%s", CCYEL(ch, C_NRM));
  column = 14;	/* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
		!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));

  if (rm->contents) {
    send_to_char(ch, "Contents:%s", CCGRN(ch, C_NRM));
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;

      column += send_to_char(ch, "%s %s", found++ ? "," : "", j->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  }

  for (i = 0; i < DIR_COUNT; i++) {
    char buf1[128];

    if (!rm->dir_option[i])
      continue;

    if (rm->dir_option[i]->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    else
      snprintf(buf1, sizeof(buf1), "%s%5d%s", CCCYN(ch, C_NRM), GET_ROOM_VNUM(rm->dir_option[i]->to_room), CCNRM(ch, C_NRM));

    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(ch, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywords: %s, Type: %s\r\n%s",
	CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1,
	rm->dir_option[i]->key == NOTHING ? -1 : rm->dir_option[i]->key,
	rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2,
	rm->dir_option[i]->general_description ? rm->dir_option[i]->general_description : "  No exit description.\r\n");
  }
    if (rm->affected) {

        struct affected_type *af;
        char buf2[MAX_STRING_LENGTH];

        send_to_char(ch, "Room Affections:%s\r\n", CCGRN(ch, C_NRM));

        for (af = rm->affected; af; af = af->next) {

            send_to_char(ch, "   PSI: (%3dhr) %s%s%s\r\n", af->duration + 1, CCCYN(ch, C_NRM), psionics_info[af->type].name, CCNRM(ch, C_NRM));

            // this isn't really logical for rooms...what stats does a room have?
            if (af->modifier)
                send_to_char(ch, "      %+d to %s\r\n", af->modifier, apply_types[(int) af->location]);

            if (af->bitvector) {
                sprintbit((long) af->bitvector, room_bits, buf2, sizeof(buf2));
                send_to_char(ch, "      Sets Bits: %s\r\n", buf2);
            }

        }
    }
  /* check the room for a script */
  do_sstat_room(ch, rm);

  list_zone_commands_room(ch, rm->number);
}

ACMD(do_roomwhos)
{
 do_roomwho(ch, NULL);
}

static void do_roomwho(struct char_data *ch, struct room_data *rm)
{
  int found, column;
  struct char_data *k;
  rm = &world[IN_ROOM(ch)];

  send_to_char(ch, "Chars present:%s\r\n", CCYEL(ch, C_NRM));
  column = 14;	/* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
		!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }
  send_to_char(ch, "%s", CCNRM(ch, C_NRM));

}

static void do_stat_object(struct char_data *ch, struct obj_data *j)
{
  int i, found;
  obj_vnum vnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct char_data *tempch;

  send_to_char(ch, "Name: '%s%s%s', Keywords: %s\r\n", CCYEL(ch, C_NRM),
	  j->short_description ? j->short_description : "<None>",
	  CCNRM(ch, C_NRM), j->name);

  vnum = GET_OBJ_VNUM(j);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  send_to_char(ch, "VNum: [%s%5d%s], RNum: [%5d], Idnum: [%5ld], Type: %s, SpecProc: %s\r\n",
    CCGRN(ch, C_NRM), vnum, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), GET_ID(j), buf,
    GET_OBJ_SPEC(j) ? (get_spec_func_name(GET_OBJ_SPEC(j))) : "None");

  send_to_char(ch, "L-Desc: '%s%s%s'\r\n", CCYEL(ch, C_NRM),
	  j->description ? j->description : "<None>",
	  CCNRM(ch, C_NRM));

  send_to_char(ch, "A-Desc: '%s%s%s'\r\n", CCYEL(ch, C_NRM),
	  j->action_description ? j->action_description : "<None>",
	  CCNRM(ch, C_NRM));

  if (j->ex_description) {
    send_to_char(ch, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [%s]", desc->keyword);
    send_to_char(ch, "%s\r\n", CCNRM(ch, C_NRM));
  }

  sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Can be worn on: %s\r\n", buf);

  sprintbitarray(GET_OBJ_AFFECT(j), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "Perm bits : %s\r\n", buf);

  sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
  send_to_char(ch, "Extra flags   : %s\r\n", buf);

    if (OBJ_FLAGGED(j, ITEM_REMORT_ONLY))
        send_to_char(ch, ": x%d\r\n", GET_OBJ_REMORT(j));
    else
        send_to_char(ch, "\r\n");
	if (!IS_COMP_NOTHING(j))
		send_to_char(ch, "Can be salvaged into: [%s] %s\r\n\r\n", GET_ITEM_COMPOSITION(j), GET_ITEM_SUBCOMPOSITION(j));
	if (IS_COMP_NOTHING(j))
		send_to_char(ch, "This item cannot be salvaged.\r\n\r\n"),
    send_to_char(ch, "Weight: %d, Value: %d, Cost/day: %d, Timer: %d, Min level: %d\r\n",
        GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));

    strcpy(buf2, (char *) asctime(localtime(&(GET_OBJ_STAMP(j)))));
    buf2[19] = '\0';
    send_to_char(ch, "Time Stamp: [%s%s %s(%s)%s]\r\n", CCYEL(ch, C_NRM), buf2, CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));

    send_to_char(ch, "In room: %d (%s), ", GET_ROOM_VNUM(IN_ROOM(j)), IN_ROOM(j) == NOWHERE ? "Nowhere" : world[IN_ROOM(j)].name);

    // In order to make it this far, we must already be able to see the character
    // holding the object. Therefore, we do not need CAN_SEE().
    send_to_char(ch, "In object: %s, ", j->in_obj ? j->in_obj->short_description : "None");
    send_to_char(ch, "Carried by: %s, ", j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
    send_to_char(ch, "Worn by: %s\r\n", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

    send_to_char(ch, "Values 0-5: [%d] [%d] [%d] [%d] [%d] [%d]\r\n",
        GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
        GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3),
        GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5));

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) == -1)
      send_to_char(ch, "Hours left: Infinite\r\n");
    else
      send_to_char(ch, "Hours left: [%d]\r\n", GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    send_to_char(ch, "Psionics: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, 0),
	    psionic_name(GET_OBJ_VAL(j, 1)), psionic_name(GET_OBJ_VAL(j, 2)), psionic_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
      send_to_char(ch, "Psionic: %s at level %d, %d (of %d) charges remaining\r\n",
      psionic_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    send_to_char(ch, "Todam: %dd%d, Avg Damage: %.1f. Message type: %s\r\n",
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), ((GET_OBJ_VAL(j, 2) + 1) / 2.0) * GET_OBJ_VAL(j, 1),  attack_hit_text[GET_OBJ_VAL(j, 3)].singular);
    break;
  case ITEM_ARMOR:
    send_to_char(ch, "AC-apply: [%d]\r\n", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf, sizeof(buf));
    send_to_char(ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
	    GET_OBJ_VAL(j, 0), buf, GET_OBJ_VAL(j, 2),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf, sizeof(buf));
    send_to_char(ch, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)), buf);
    break;
  case ITEM_NOTE:
    send_to_char(ch, "Tongue: %d\r\n", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY: /* Nothing */
    break;
  case ITEM_FOOD:
    send_to_char(ch, "Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, 0), YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    send_to_char(ch, "Coins: %d\r\n", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_FURNITURE:
    send_to_char(ch, "Can hold: [%d] Num. of People in: [%d]\r\n", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    send_to_char(ch, "Holding : ");
    for (tempch = OBJ_SAT_IN_BY(j); tempch; tempch = NEXT_SITTING(tempch))
      send_to_char(ch, "%s ", GET_NAME(tempch));
    send_to_char(ch, "\r\n");
    break;
        case ITEM_GUN :
            send_to_char(ch, "Gun Type: %d, ", j->obj_weapon.gun_type);
            send_to_char(ch, "Shots per round: %d, Weapon Range: %d\r\n",
                GET_SHOTS_PER_ROUND(j), GET_RANGE(j));
            send_to_char(ch, "Loaded Damage: %dd%d, Unloaded Damage: %dd%d", j->obj_flags.value[1], j->obj_flags.value[2], GET_GUN_BASH_NUM(j), j->obj_weapon.bash[1]);
            break;

        case ITEM_AMMO :
            send_to_char(ch, "Ammo Type: %d, Ammo Remaining: %d", GET_AMMO_TYPE(j), GET_AMMO_COUNT(j));
            break;

        case ITEM_LOTTO :
            send_to_char(ch, "Lotto numbers: %d - %d - %d - %d\r\nFor Lottery #%d    Purchased by %s",
                GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3),
                GET_OBJ_VAL(j, 4), get_name_by_id(GET_OBJ_VAL(j, 5)));
            break;

        case ITEM_SCRATCH :
            send_to_char(ch, "Scratch Values:  [ %d ]  [ %d ]  [ %d ]   %s\r\n"
                "Scratch Version #%d    Purchased by %s",
                GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2),
                (IS_SCRATCHED(j) ? "Scratched" : "Not Scratched"),
                GET_OBJ_VAL(j, 4), get_name_by_id(GET_OBJ_VAL(j, 5)));
            break;

        case ITEM_RECALL:
            if (GET_CHARGE(j) == -1)
                send_to_char(ch, "Charges left: Infinite");
            else
                send_to_char(ch, "Charges left: [%d]", GET_CHARGE(j));
            break;

        case ITEM_MEDKIT:
            send_to_char(ch, "Restores HP: [%d] PSI: [%d] MOVE: [%d], Charges: [%d]",
                GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
            break;

        case ITEM_DRUG:
            sprinttype(GET_OBJ_VAL(j, 0), drug_names, buf2, sizeof(buf2));
            send_to_char(ch, "Drug: %s", buf2);
            break;

        default:
            break;
  }

  if (j->contains) {
    int column;

    send_to_char(ch, "\r\nContents:%s", CCGRN(ch, C_NRM));
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", j2->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j2->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "%s", CCNRM(ch, C_NRM));
  }

    found = FALSE;
    send_to_char(ch, "\r\nApplies:");

    for (i = 0; i < MAX_OBJ_APPLY; i++)
        if (GET_OBJ_APPLY_MOD(j, i)) {
            sprinttype(GET_OBJ_APPLY_LOC(j, i), apply_types, buf, sizeof(buf));
            send_to_char(ch, "%s %+d to %s", found++ ? "," : "", GET_OBJ_APPLY_MOD(j, i), buf);
        }

    if (!found)
        send_to_char(ch, " None");

    sprintbitarray(GET_OBJ_AFFECT(j), affected_bits, AF_ARRAY_MAX, buf);
    send_to_char(ch, "\n\rPerm Affects: %s\r\n", buf);

    found = 0;
    if ((IS_SET(j->obj_flags.type_flag, ITEM_WEAPON)) || (IS_SET(j->obj_flags.type_flag, ITEM_GUN))) {
        if (j->obj_wpnpsi.which_psionic) {
            sprinttype(j->obj_wpnpsi.which_psionic, wpn_psionics, buf2, sizeof(buf2));
            send_to_char(ch, "\n\rWeapon Effect:");
            send_to_char(ch, "%s Level %d %s", found++ ? "" : "", j->obj_wpnpsi.skill_level, buf2);
        }
    }
	send_to_char(ch, "This item is bound to: ");
	if (j->bound_name != NULL && j->bound_id != 0)
		send_to_char(ch, "%s (%ld)\r\n", j->bound_name, j->bound_id);
	else
		send_to_char(ch, "Nobody\r\n");
  /* check the object for a script */
  do_sstat_object(ch, j);
}

static void do_stat_character(struct char_data *ch, struct char_data *k)
{
    char buf[MAX_STRING_LENGTH];
    char sb[MAX_STRING_LENGTH];
    size_t len;
    int i;
    int i2;
    int column;
    int found = FALSE;
    struct obj_data *j;
    struct follow_type *fol;
    struct affected_type *aff;
    // OLC Restriction - Lump 7/2/01
#ifdef BUILDPORT
    if (IS_NPC(k) && GET_LEVEL(ch) < LVL_CIMP) {
        if (!can_edit_zone(ch, real_zone(GET_MOB_VNUM(k))) ) {
            send_to_char(ch "You cannot stat mobiles for which you do not have permission to edit.\r\n");
            return;
        }
    }
#endif
    sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
    len = snprintf(sb, sizeof(sb), "%s %s '%s'  IDNum: [%5ld], In room [%5d], Loadroom : [%5d], Home Room : [%5d]\r\n",
        buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
        GET_NAME(k), IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)), IS_NPC(k) ? NOWHERE : GET_LOADROOM(k), GET_MOB_HOME(k));
    if (IS_MOB(k))
        len += snprintf(sb + len, sizeof(sb) - len, "Keyword: %s, VNum: [%5d], RNum: [%5d]\r\n", k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    len += snprintf(sb + len, sizeof(sb) - len, "Title : %s\r\n", k->player.title ? k->player.title : "<None>");
    len += snprintf(sb + len, sizeof(sb) - len, "Prompt: %s\r\n", k->player_specials->prompt_string ? k->player_specials->prompt_string : "<None>");
    len += snprintf(sb + len, sizeof(sb) - len, "L-Desc: %s", k->player.long_descr ? k->player.long_descr : "<None>\r\n");
    len += snprintf(sb + len, sizeof(sb) - len, "D-Desc: %s", k->player.description ? k->player.description : "<None>\r\n");
    len += snprintf(sb + len, sizeof(sb) - len, "%s%s, Lev: [%s%2d%s], XP: [%s%7d%s]\r\n",
        IS_NPC(k) ? "Mobile" : "Class: ", IS_NPC(k) ? "" : pc_class[GET_CLASS(k)].class_name, CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
        CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM));
    if (!IS_NPC(k)) {
        char buf1[64];
        char buf2[64];
        strlcpy(buf1, asctime(localtime(&(k->player.time.birth))), sizeof(buf1));
        strlcpy(buf2, asctime(localtime(&(k->player.time.logon))), sizeof(buf2));
        buf1[10] = buf2[10] = '\0';
        len += snprintf(sb + len, sizeof(sb) - len, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
            buf1, buf2, k->player.time.played / 3600,
            ((k->player.time.played % 3600) / 60), age(k)->year);
        len += snprintf(sb + len, sizeof(sb) - len, "Pracs[%d]/per[%d]/NSTL[%d]",
            GET_PRACTICES(k), int_app[GET_INT(k)].learn, pcn_app[GET_PCN(k)].bonus);
        // Display OLC zone for immorts.
        if (GET_LEVEL(k) >= LVL_GOD) {
            if (GET_OLC_ZONE(k)==AEDIT_PERMISSION)
                len += snprintf(sb + len, sizeof(sb) - len, ", OLC[%sAedit%s]", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            else if (GET_OLC_ZONE(k)==HEDIT_PERMISSION)
                len += snprintf(sb + len, sizeof(sb) - len, ", OLC[%sHedit%s]", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            else if (GET_OLC_ZONE(k) == ALL_PERMISSION)
                len += snprintf(sb + len, sizeof(sb) - len, ", OLC[%sAll%s]", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            else if (GET_OLC_ZONE(k)==NOWHERE)
                len += snprintf(sb + len, sizeof(sb) - len, ", OLC[%sOFF%s]", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
            else
                len += snprintf(sb + len, sizeof(sb) - len, ", OLC[%s%d%s]", CCCYN(ch, C_NRM), GET_OLC_ZONE(k), CCNRM(ch, C_NRM));
        }
        len += snprintf(sb + len, sizeof(sb) - len, "\r\n");
    }
    len += snprintf(sb + len, sizeof(sb) - len, "Str: [%s%d%s]  Int: [%s%d%s]  Pcn: [%s%d%s]  "
        "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
        CCCYN(ch, C_NRM), GET_STR(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_PCN(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
        CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
    len += snprintf(sb + len, sizeof(sb) - len, "Hit p.:[%s%d/%d+%d%s]  Psi p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
        CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
        CCGRN(ch, C_NRM), GET_PSI(k), GET_MAX_PSI(k), psi_gain(k), CCNRM(ch, C_NRM),
        CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
    len += snprintf(sb + len, sizeof(sb) - len, "Units: [%9d], Bank: [%9d] (Total: %d), ",
        GET_UNITS(k), GET_BANK_UNITS(k), GET_UNITS(k) + GET_BANK_UNITS(k));
    if (!IS_NPC(k))
        len += snprintf(sb + len, sizeof(sb) - len, "Screen %s[%s%d%sx%s%d%s]%s\r\n",
            CCCYN(ch, C_NRM), CCYEL(ch, C_NRM), GET_SCREEN_WIDTH(k), CCNRM(ch, C_NRM),
            CCYEL(ch, C_NRM), GET_PAGE_LENGTH(k), CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
    len += snprintf(sb + len, sizeof(sb) - len, "AC: [%d%+d/10], Hitroll: [%2d], Damroll: [%2d], Saving throws: [%d/%d/%d/%d/%d]\r\n",
        GET_AC(k), dex_app[GET_DEX(k)].defensive, k->points.hitroll,
        k->points.damroll, GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2),
        GET_SAVE(k, 3), GET_SAVE(k, 4));
    sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
    len += snprintf(sb + len, sizeof(sb) - len, "Pos: %s, Fighting: %s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");
    if (IS_NPC(k))
        len += snprintf(sb + len, sizeof(sb) - len, ", Attack type: %s", attack_hit_text[(int) k->mob_specials.attack_type].singular);
    if (k->desc) {
        sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
        len += snprintf(sb + len, sizeof(sb) - len, ", Connected: %s", buf);
    }
    if (IS_NPC(k)) {
        sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
        len += snprintf(sb + len, sizeof(sb) - len, ", Default position: %s\r\n", buf);
        sprintbitarray(MOB_FLAGS(k), action_bits, PM_ARRAY_MAX, buf);
        len += snprintf(sb + len, sizeof(sb) - len, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf, CCNRM(ch, C_NRM));
    }
    else {
        len += snprintf(sb + len, sizeof(sb) - len, ", Idle Timer (in tics) [%d]\r\n", k->char_specials.timer);
        sprintbitarray(PLR_FLAGS(k), player_bits, PM_ARRAY_MAX, buf);
        len += snprintf(sb + len, sizeof(sb) - len, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf, CCNRM(ch, C_NRM));
        sprintbitarray(PRF_FLAGS(k), preference_bits, PR_ARRAY_MAX, buf);
        len += snprintf(sb + len, sizeof(sb) - len, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf, CCNRM(ch, C_NRM));
        len += snprintf(sb + len, sizeof(sb) - len, "Quest Points: [%9d] Quests Completed: [%5d]\r\n", GET_QUESTPOINTS(k), GET_NUM_QUESTS(k));
        if (GET_QUEST(k) != NOTHING)
            len += snprintf(sb + len, sizeof(sb) - len, "Current Quest: [%5d] Time Left: [%5d]\r\n", GET_QUEST(k), GET_QUEST_TIME(k));
    }
    if (IS_MOB(k))
        len += snprintf(sb + len, sizeof(sb) - len, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
            (mob_index[GET_MOB_RNUM(k)].func ? get_spec_func_name(mob_index[GET_MOB_RNUM(k)].func) : "None"),
            k->mob_specials.damnodice, k->mob_specials.damsizedice);
    for (i = 0, j = k->carrying; j; j = j->next_content, i++);
        len += snprintf(sb + len, sizeof(sb) - len, "Carried: weight: %d, items: %d; Items in: inventory: %d, ", IS_CARRYING_W(k), IS_CARRYING_N(k), i);
    for (i = 0, i2 = 0; i < NUM_WEARS; i++)
        if (GET_EQ(k, i))
            i2++;
    len += snprintf(sb + len, sizeof(sb) - len, "eq: %d\r\n", i2);
    if (!IS_NPC(k))
        len += snprintf(sb + len, sizeof(sb) - len, "Overdose level: [@R%d@n]  Addiction: [@Y%d@n]\r\n", GET_OVERDOSE(k), GET_ADDICTION(k));
    column = len += snprintf(sb + len, sizeof(sb) - len, "Master is: %s, Followers are:", k->master ? GET_NAME(k->master) : "<none>");
    if (!k->followers)
        len += snprintf(sb + len, sizeof(sb) - len, " <none>\r\n");
    else {
        for (fol = k->followers; fol; fol = fol->next) {
            column += len += snprintf(sb + len, sizeof(sb) - len, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
            if (column >= 62) {
                len += snprintf(sb + len, sizeof(sb) - len, "%s\r\n", fol->next ? "," : "");
                found = FALSE;
                column = 0;
            }
        }
        if (column != 0)
            len += snprintf(sb + len, sizeof(sb) - len, "\r\n");
    }
    len += snprintf(sb + len, sizeof(sb) - len, "Protecting: %s, Protected by: %s\r\n",
        ((k->char_specials.protecting) ? GET_NAME(k->char_specials.protecting) : "<none>"),
        ((k->char_specials.protector) ? GET_NAME(k->char_specials.protector) : "<none>"));
    // Displaying the number of Duplicates
    len += snprintf(sb + len, sizeof(sb) - len, "Duplicates: %d\r\n", k->char_specials.saved.duplicates);
    // Displaying Essences
    if (IS_HIGHLANDER(ch))
        len += snprintf(sb + len, sizeof(sb) - len, "Essences: %d\r\n", k->player.essences);
    // Displaying team
    len += snprintf(sb + len, sizeof(sb) - len, "Team: %s\r\n", teams[(int)GET_TEAM(k)]);
    // Displaying the number of remorts
    len += snprintf(sb + len, sizeof(sb) - len, "Remorts: %d, Attrib Bonus: %d, Psi Mastery: %d\r\n", k->player.num_remorts,
        k->player_specials->saved.attrib_add,k->player_specials->saved.psimastery);
    sprintbitarray(CMB_FLAGS(k), combat_bits, CB_ARRAY_MAX, buf);
	len += snprintf(sb + len, sizeof(sb) - len, "CMB: %s%s%s\r\n", CCYEL(ch, C_NRM), buf, CCNRM(ch, C_NRM));
	// Showing the bitvector
    sprintbitarray(AFF_FLAGS(k), affected_bits, AF_ARRAY_MAX, buf);
    len += snprintf(sb + len, sizeof(sb) - len, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf, CCNRM(ch, C_NRM));
    // Routine to show what psionics a char is affected by
  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      len += snprintf(sb + len, sizeof(sb) - len, "PSI: (%3dhr) %s%-21s%s ", aff->duration + 1, CCCYN(ch, C_NRM), psionics_info[aff->type].name, CCNRM(ch, C_NRM));

      if (aff->modifier)
		len += snprintf(sb + len, sizeof(sb) - len, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector[0] || aff->bitvector[1] || aff->bitvector[2] || aff->bitvector[3] || aff->bitvector[4] || aff->bitvector[5] || aff->bitvector[6] || aff->bitvector[7]) {
        if (aff->modifier)
          len += snprintf(sb + len, sizeof(sb) - len, ", ");
        for (i=0; i<NUM_AFF_FLAGS; i++) {
          if (IS_SET_AR(aff->bitvector, i)) {
            len += snprintf(sb + len, sizeof(sb) - len, "sets %s, ", affected_bits[i]);
          }
        }
      }
      len += snprintf(sb + len, sizeof(sb) - len, "\r\n");
    }
  }
    if (IS_ON_DRUGS(k)) {
        for (aff = k->drugs_used; aff; aff = aff->next) {
            len += snprintf(sb + len, sizeof(sb) - len, "DRUG: (%3dhr) %s%-21s%s ", aff->duration, CCCYN(ch, C_NRM), drug_names[aff->type], CCNRM(ch, C_NRM));
            if (aff->modifier)
                len += snprintf(sb + len, sizeof(sb) - len, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
            if (aff->bitvector) {
                if (aff->modifier)
                    len += snprintf(sb + len, sizeof(sb) - len, ", ");
				for (i=0; i<NUM_AFF_FLAGS; i++) {
					if (IS_SET_AR(aff->bitvector, i))
						len += snprintf(sb + len, sizeof(sb) - len, "sets %s, ", affected_bits[i]);
				}
			}
            len += snprintf(sb + len, sizeof(sb) - len, "\r\n");
        }
    }
    // check mobiles for a script
    if (IS_NPC(k)) {
        do_sstat_character(ch, k);
        if (SCRIPT_MEM(k)) {
            struct script_memory *mem = SCRIPT_MEM(k);
            len += snprintf(sb + len, sizeof(sb) - len, "Script memory:\r\n  Remember             Command\r\n");
            while (mem) {
                struct char_data *mc = find_char(mem->id);
                if (!mc)
                    len += snprintf(sb + len, sizeof(sb) - len, "  ** Corrupted!\r\n");
                else {
                    if (mem->cmd)
                        len += snprintf(sb + len, sizeof(sb) - len, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
                    else
                        len += snprintf(sb + len, sizeof(sb) - len, "  %-20.20s <default>\r\n",GET_NAME(mc));
                }
                mem = mem->next;
            }
        }
    }
    else {
        // this is a PC, display their global variables
        if (k->script && k->script->global_vars) {
            struct trig_var_data *tv;
            char uname[MAX_INPUT_LENGTH];
            len += snprintf(sb + len, sizeof(sb) - len, "Global Variables:\r\n");
            // currently, variable context for players is always 0, so it is not
            // displayed here. in the future, this might change
            for (tv = k->script->global_vars; tv; tv = tv->next) {
                if (*(tv->value) == UID_CHAR) {
                    find_uid_name(tv->value, uname, sizeof(uname));
                    len += snprintf(sb + len, sizeof(sb) - len, "    %10s:  [UID]: %s\r\n", tv->name, uname);
                }
                else
                    len += snprintf(sb + len, sizeof(sb) - len, "    %10s:  %s\r\n", tv->name, tv->value);
            }
        }
    }
    page_string(ch->desc, sb, TRUE);
}

ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;
  struct room_data *room;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Stats on who or what or where?\r\n");
    return;
  } else if (is_abbrev(buf1, "room")) {
    if (!*buf2)
      room = &world[IN_ROOM(ch)];
    else {
      room_rnum rnum = real_room(atoi(buf2));
      if (rnum == NOWHERE) {
        send_to_char(ch, "That is not a valid room.\r\n");
        return;
      }
      room = &world[rnum];
    }
    do_stat_room(ch, room);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      /* Allocate mobile event list */
      victim->events = create_list();
      if (load_char(buf2, victim) >= 0) {
        char_to_room(victim, 0);
        if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char(ch, "Sorry, you can't do that.\r\n");
	else
	  do_stat_character(ch, victim);
	extract_char_final(victim);
      } else {
	send_to_char(ch, "There is no such player.\r\n");
	free_char(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char(ch, "No such object around.\r\n");
    }
  } else if (is_abbrev(buf1, "zone")) {
    if (!*buf2) {
      print_zone(ch, zone_table[world[IN_ROOM(ch)].zone].number);
      return;
    } else {
      print_zone(ch, atoi(buf2));
      return;
    }
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}

ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);
  save_all();
  House_save_all();
  Vehicle_save();
  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting... Estimated downtime: [ 5-10 seconds ]\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance. Estimated downtime: [ 2-5 minutes ]\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting... Estimated downtime: [ 5-10 seconds ]\r\n");
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance. Estimated downtime: [ 2-5 minutes ]\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}

void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen to be snooping
   *  (or snooped) and get advanced/demoted will not be snooping/snooped someone
   *  of a higher/lower level (and thus, not entitled to be snooping. */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
     (GET_LEVEL(ch->desc->snooping->character) >= GET_LEVEL(ch))) {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
     (GET_LEVEL(ch) >= GET_LEVEL(ch->desc->snoop_by->character))) {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

static void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(ch, "You stop snooping.\r\n");

    if (GET_LEVEL(ch) < LVL_IMPL)
      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s stops snooping", GET_NAME(ch));

    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

    if (GET_LEVEL(ch) < LVL_IMPL)
      mudlog(BRF, GET_LEVEL(ch), TRUE, "(GC) %s snoops %s", GET_NAME(ch), GET_NAME(victim));

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}

ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (GET_LEVEL(ch) < LVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
		&& !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s Switched into: %s", GET_NAME(ch), GET_NAME(victim));
    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_return)
{
  if (!IS_NPC(ch) && !ch->desc->original) {
    int level, newlevel;
    level = GET_LEVEL(ch);
    newlevel = GET_LEVEL(ch);
    if (!PLR_FLAGGED(ch, PLR_NOWIZLIST)&& level != newlevel)
      run_autowiz();
  }

  if (ch->desc && ch->desc->original) {
    send_to_char(ch, "You return to your original body.\r\n");

    /* If someone switched into your original body, disconnect them. - JE
     * Zmey: here we put someone switched in our body to disconnect state but
     * we must also NULL his pointer to our character, otherwise close_socket()
     * will damage our character's pointer to our descriptor (which is assigned
     * below in this function). */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_create)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  int i=0, n=1;

  one_argument(two_arguments(argument, buf, buf2), buf3);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: create < obj | mob > <vnum> <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }

  if (atoi(buf3) > 0  && atoi(buf3) <= 100) {
    n = atoi(buf3);
  } else {
    n = 1;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob=NULL;
    mob_rnum r_num;

	if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	  send_to_char(ch, "Sorry, you can't load mobs here.\r\n");
	  return;
	}

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
      mob = read_mobile(r_num, REAL);
      char_to_room(mob, IN_ROOM(ch));

      // todo:  allow each immortal to have their own saying here
      act("$n raises their hand and snaps their fingers.", TRUE, ch, 0, 0, TO_ROOM);
      act("Lightning streaks across the green patched sky", TRUE, ch, 0, 0,TO_ROOM);
      act("$N appears in the middle of the room!", FALSE, ch, 0, mob,TO_ROOM);
      act("You bring forth $N.", FALSE, ch, 0, mob, TO_CHAR);
	  load_mtrigger(mob);
    }
if (n == 1)
{
	  mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has brought forth %s.", GET_NAME(ch), GET_NAME(mob));
}
else
{
	  mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has brought forth %d %s.", GET_NAME(ch), n, GET_NAME(mob));
}
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

	if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	  send_to_char(ch, "Sorry, you can't load objects here.\r\n");
	  return;
	}

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
      obj = read_object(r_num, REAL);
      if (CONFIG_LOAD_INVENTORY)
        obj_to_char(obj, ch);
      else
        obj_to_room(obj, IN_ROOM(ch));
        act("$n activates a matter replication device.", TRUE, ch, 0, 0, TO_ROOM);
        act("$n has replicated $p!", FALSE, ch, obj, 0, TO_ROOM);
        act("You replicate $p.", FALSE, ch, obj, 0, TO_CHAR);
		load_otrigger(obj);
    }
if (n == 1)
{
		mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has replicated %s.", GET_NAME(ch), obj->short_description);
}
else
{
		mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has replicated %d %s.", GET_NAME(ch), n, obj->short_description);
}
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *mob;
  struct obj_data *obj;
  int r_num;

  ACMD(do_tstat);

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: vstat { o | m | r | t | s | z } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  switch (LOWER(*buf)) {
  case 'm':
    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
    break;
  case 'o':
    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
    break;
  case 'r':
    sprintf(buf2, "room %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 'z':
    sprintf(buf2, "zone %d", atoi(buf2));
    do_stat(ch, buf2, 0, 0);
    break;
  case 't':
    sprintf(buf2, "%d", atoi(buf2));
    do_tstat(ch, buf2, 0, 0);
    break;
  case 's':
    sprintf(buf2, "shops %d", atoi(buf2));
    do_show(ch, buf2, 0, 0);
    break;
  default:
    send_to_char(ch, "Syntax: vstat { r | m | o | z | t | s } <number>\r\n");
    break;
  }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  char *t;
  struct char_data *vict;
  struct obj_data *obj;
  struct obj_data *next_o;
  int number, i;

  one_argument(argument, buf);

  if (GET_LEVEL(ch) < LVL_GRGOD && !can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
	send_to_char(ch, "Sorry, you can't purge anything here.\r\n");
	return;
  }

  /* argument supplied. destroy single object or char */
  if (*buf) {
    t = buf;
    number = get_number(&t);
    if ((vict = get_char_vis(ch, buf, &number, FIND_CHAR_ROOM)) != NULL) {      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
        send_to_char(ch, "You can't purge %s!\r\n", HMHR(vict));
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_GOD) {
	mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, &number, world[IN_ROOM(ch)].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else if (GET_LEVEL(ch) >= LVL_GRGOD && !str_cmp("zone", buf)) {
            zone_rnum zone;
            struct char_data *next_v;

            act("$n gestures... The entire area glows blue!", FALSE, ch, 0, 0, TO_ROOM);
            send_to_room(IN_ROOM(ch), "The zone seems empty now...\r\n");
            zone = world[IN_ROOM(ch)].zone;

            for (vict = character_list; vict; vict = next_v) {
                next_v = vict->next;
                if (IS_NPC(vict) && IN_ROOM(vict) != NOWHERE && world[IN_ROOM(vict)].zone == zone) {

                    // extract_char puts this mob into a wait list which may make the objects in
                    // its inventory drop after we purge the zone.  so do this now to force the
                    // zone to be purged
                    while (vict->carrying) {
                        obj = vict->carrying;
                        obj_from_char(obj);
                        obj_to_room(obj, IN_ROOM(vict));
                    }

                    // transfer equipment to room, if any
                    for (i = 0; i < NUM_WEARS; i++)
                        if (GET_EQ(vict, i))
                            obj_to_room(unequip_char(vict, i, FALSE), IN_ROOM(vict));

                    extract_char(vict);
                }
            }

            for (obj = object_list; obj; obj = next_o) {
                next_o = obj->next;
                if (IN_ROOM(obj) != NOWHERE && world[IN_ROOM(obj)].zone == zone)
                    extract_obj(obj);
            }

        }
        else if (GET_LEVEL(ch) >= LVL_GRGOD && !str_cmp("marbles", buf)) {  /* purge all marbles */

            for (obj = object_list; obj; obj = next_o) {
                next_o = obj->next;
                if (GET_OBJ_VNUM(obj) >= 21525 && GET_OBJ_VNUM(obj) <= 21544)
                    extract_obj(obj);
            }
        }
        else if (GET_LEVEL(ch) >= LVL_GRGOD && !str_cmp("qcreate", buf)) {  /* purge all marbles */

            for (obj = object_list; obj; obj = next_o) {
                next_o = obj->next;
                if (GET_OBJ_VNUM(obj) == 65535)
                    extract_obj(obj);
            }
        }	
	else {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", CONFIG_OK);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");
    purge_room(IN_ROOM(ch));
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel, i;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char(ch, "Maybe that's not such a great idea.\r\n");
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel > LVL_IMPL) {
    send_to_char(ch, "%d is the highest possible level.\r\n", LVL_IMPL);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char(ch, "Yeah, right.\r\n");
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    do_start(victim);
    GET_LEVEL(victim) = newlevel;
    send_to_char(victim, "You are momentarily enveloped by darkness!\r\nYou feel somewhat diminished.\r\n");
  } else {
    act("$n makes some strange gestures. A strange feeling comes upon you,\r\n"
      "Like a giant hand, light comes down from above, grabbing your body,\r\n"
      "that begins to pulse with colored lights from inside.\r\n\r\n"
      "Your head seems to be filled with demons from another plane as\r\n"
      "your body dissolves to the elements of time and space itself.\r\n"
      "Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
      "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
}

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  if (oldlevel >= LVL_IMMORT && newlevel < LVL_IMMORT) {
    /* If they are no longer an immortal, remove the immortal only flags. */
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG1);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_NOHASSLE);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    REMOVE_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    if (!PLR_FLAGGED(victim, PLR_NOWIZLIST))
      run_autowiz();
  } else if (oldlevel < LVL_IMMORT && newlevel >= LVL_IMMORT) {
    SET_BIT_AR(PRF_FLAGS(victim), PRF_LOG2);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_HOLYLIGHT);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_SHOWVNUMS);
    SET_BIT_AR(PRF_FLAGS(victim), PRF_AUTOEXIT);
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(victim, i, 100);
   GET_OLC_ZONE(victim) = NOWHERE;
   GET_COND(victim, HUNGER) = -1;
   GET_COND(victim, THIRST) = -1;
   GET_COND(victim, DRUNK)  = -1;
  }

  gain_exp_regardless(victim, (exptolevel(ch, GET_LEVEL(ch))) - GET_EXP(victim));
  save_char(victim);
}
ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct descriptor_data *j;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "@xWhom do you wish to restore?@n\r\n");
   else if (is_abbrev(buf, "all"))
   {
     for (j = descriptor_list; j; j = j->next)
    {
      if (!IS_PLAYING(j) || !(vict = j->character) || GET_LEVEL(vict) >= LVL_IMMORT)
     continue;

      GET_HIT(vict)  = GET_MAX_HIT(vict);
      GET_PSI(vict) = GET_MAX_PSI(vict);
      GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if (affected_by_psionic(vict, PSIONIC_POISON)) {
        affect_from_char(vict, PSIONIC_POISON, TRUE);
        act("A warm feeling runs through your body.", FALSE, vict, 0, 0, TO_CHAR);
        act("$N looks better.", FALSE, ch, 0, vict, TO_ROOM);
    }

    if (IS_ON_DRUGS(vict) || GET_OVERDOSE(vict) > 0 || GET_ADDICTION(vict) > 0) {
        act("A soothing feeling runs through your body.", FALSE, vict, 0, 0, TO_CHAR);
        act("$N looks better.", FALSE, ch, 0, vict, TO_ROOM);
        GET_OVERDOSE(vict) = 0;
        GET_ADDICTION(vict) = 0;
    }

    // cycle through the body parts and restore all parts
    for (i = 1; i < NUM_BODY_PARTS; i++) {

        GET_NO_HITS(vict, i) = 0;

        // clear all previous conditions
        BODYPART_CONDITION(vict, i) = 0;

        // if bionic, set to BIONIC_NORMAL, else set to NORMAL
        if (BIONIC_DEVICE(vict, i))
            SET_BODYPART_CONDITION(vict, i, BIONIC_NORMAL);
        else
            SET_BODYPART_CONDITION(vict, i, NORMAL);

    }

      update_pos(vict);
      send_to_char(ch, "@x%s has been fully healed.@n\r\n", GET_NAME(vict));
      act("@xYou have been fully healed by $N!@n", FALSE, vict, 0, ch, TO_CHAR);
    }
  }
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "@x%s@n", CONFIG_NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_LEVEL(vict) >= GET_LEVEL(ch))
    send_to_char(ch, "@xThey don't need your help.@n\r\n");
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_PSI(vict) = GET_MAX_PSI(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    for (i = 1; i < NUM_BODY_PARTS; i++) {

        GET_NO_HITS(vict, i) = 0;

        // clear all previous conditions
        BODYPART_CONDITION(vict, i) = 0;

        // if bionic, set to BIONIC_NORMAL, else set to NORMAL
        if (BIONIC_DEVICE(vict, i))
            SET_BODYPART_CONDITION(vict, i, BIONIC_NORMAL);
        else
            SET_BODYPART_CONDITION(vict, i, NORMAL);

    }
    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "@x%s@n", CONFIG_OK);
    act("@xYou have been fully healed by @x$N@x!@n", FALSE, vict, 0, ch, TO_CHAR);
  }
}

void perform_immort_vis(struct char_data *ch)
{
  if ((GET_INVIS_LEV(ch) == 0) && (!AFF_FLAGGED(ch, AFF_HIDE) && !AFF_FLAGGED(ch, AFF_INVISIBLE))) {
    send_to_char(ch, "You are already fully visible.\r\n");
    return;
  }

  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char(ch, "You are now fully visible.\r\n");
}

static void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch || IS_NPC(tch))
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}

ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
	send_to_char(pt->character, "%s\r\n", argument);

    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s gechoed: %s", GET_NAME(ch), argument);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}

ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close_socket(), but various
   * people pointed out this could cause a crash if you're closing the person
   * below you on the descriptor list.  Just setting to CON_CLOSE leaves things
   * in a massively inconsistent state so I had to add this new flag to the
   * descriptor. -je It is a much more logical extension for a CON_DISCONNECT
   * to be used for in-game socket closes and CON_CLOSE for out of game
   * closings. This will retain the stability of the close_me hack while being
   * neater in appearance. -gg For those unlucky souls who actually manage to
   * get disconnected by two different immortals in the same 1/10th of a
   * second, we have the below 'if' check. -gg */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char(ch, "They're already being disconnected.\r\n");
  else {
    /* Remember that we can disconnect people not in the game and that rather
     * confuses the code when it expected there to be a character context. */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}

ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
}

ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}

/* altered from stock to the following:
   last [name] [#]
   last without arguments displays the last 10 entries.
   last with a name only displays the 'stock' last entry.
   last with a number displays that many entries (combines with name) */
const char *last_array[11] = {
  "Connect",
  "Enter Game",
  "Reconnect",
  "Takeover",
  "Quit",
  "Idleout",
  "Disconnect",
  "Shutdown",
  "Reboot",
  "Crash",
  "Playing"
};

struct last_entry *find_llog_entry(int punique, long idnum) {
  FILE *fp;
  struct last_entry mlast;
  struct last_entry *llast;
  int size, recs, tmp, i;

  if(!(fp=fopen(LAST_FILE,"r"))) {
    log("Error opening last_file for reading, will create.");
    return NULL;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */
  recs = size/sizeof(struct last_entry);
  /* we'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for(tmp=recs-1; tmp > 0; tmp--) {
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);
    i = fread(&mlast,sizeof(struct last_entry),1,fp);
        /*another one to keep that stepback */
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.idnum == idnum && mlast.punique == punique) {
      /* then we've found a match */
      CREATE(llast,struct last_entry,1);
      memcpy(llast,&mlast,sizeof(struct last_entry));
      fclose(fp);
      return llast;
    }
    /*not the one we seek. next */
  }
  /*not found, no problem, quit */
  fclose(fp);
  return NULL;
}

/* mod_llog_entry assumes that llast is accurate */
static void mod_llog_entry(struct last_entry *llast,int type) {
  FILE *fp;
  struct last_entry mlast;
  int size, recs, tmp, i, j;

  if(!(fp=fopen(LAST_FILE,"r+"))) {
    log("Error opening last_file for reading and writing.");
    return;
  }
  fseek(fp,0L,SEEK_END);
  size=ftell(fp);

  /* recs = number of records in the last file */
  recs = size/sizeof(struct last_entry);

  /* We'll search last to first, since it's faster than any thing else we can
   * do (like searching for the last shutdown/etc..) */
  for(tmp=recs; tmp > 0; tmp--) {
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);
    i = fread(&mlast,sizeof(struct last_entry),1,fp);
    /* Another one to keep that stepback. */
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);

    if(mlast.idnum == llast->idnum && mlast.punique == llast->punique) {
      /* Then we've found a match, lets assume quit is inviolate, mainly
       * because disconnect is called after each of these */
      if(mlast.close_type != LAST_QUIT &&
        mlast.close_type != LAST_IDLEOUT &&
        mlast.close_type != LAST_REBOOT &&
        mlast.close_type != LAST_SHUTDOWN) {
        mlast.close_type=type;
      }
      mlast.close_time=time(0);
      /*write it, and we're done!*/
      j = fwrite(&mlast,sizeof(struct last_entry),1,fp);
      fclose(fp);
      return;
    }
    /* Not the one we seek, next. */
  }
  fclose(fp);

  /* Not found, no problem, quit. */
  return;
}

void add_llog_entry(struct char_data *ch, int type) {
  FILE *fp;
  struct last_entry *llast;
  int i;

  /* so if a char enteres a name, but bad password, otherwise loses link before
   * he gets a pref assinged, we won't record it */
  if(GET_PREF(ch) <= 0) {
    return;
  }

  /* See if we have a login stored */
  llast = find_llog_entry(GET_PREF(ch), GET_IDNUM(ch));

  /* we didn't - make a new one */
  if(llast == NULL) {  /* no entry found, add ..error if close! */
    CREATE(llast,struct last_entry,1);
    strncpy(llast->username,GET_NAME(ch),16);
    strncpy(llast->hostname,GET_HOST(ch),128);
    llast->idnum=GET_IDNUM(ch);
    llast->punique=GET_PREF(ch);
    llast->time=time(0);
    llast->close_time=0;
    llast->close_type=type;

    if(!(fp=fopen(LAST_FILE,"a"))) {
      log("error opening last_file for appending");
      free(llast);
      return;
    }
    i = fwrite(llast,sizeof(struct last_entry),1,fp);
    fclose(fp);
  } else {
    /* We've found a login - update it */
    mod_llog_entry(llast,type);
  }
  free(llast);
}

void clean_llog_entries(void) {
  FILE *ofp, *nfp;
  struct last_entry mlast;
  int recs, i, j;

  if(!(ofp=fopen(LAST_FILE,"r")))
    return; /* no file, no gripe */

  fseek(ofp,0L,SEEK_END);
  recs=ftell(ofp)/sizeof(struct last_entry);
  rewind(ofp);

  if (recs < MAX_LAST_ENTRIES) {
    fclose(ofp);
    return;
  }

  if (!(nfp=fopen("etc/nlast", "w"))) {
    log("Error trying to open new last file.");
    fclose(ofp);
    return;
  }

  /* skip first entries */
  fseek(ofp,(recs-MAX_LAST_ENTRIES)* (sizeof(struct last_entry)),SEEK_CUR);

  /* copy the rest */
  while (!feof(ofp)) {
    i = fread(&mlast,sizeof(struct last_entry),1,ofp);
    j = fwrite(&mlast,sizeof(struct last_entry),1,nfp);
  }
  fclose(ofp);
  fclose(nfp);

  remove(LAST_FILE);
  rename("etc/nlast", LAST_FILE);
}

/* debugging stuff, if you wanna see the whole file */
void list_llog_entries(struct char_data *ch)
{
  FILE *fp;
  struct last_entry llast;
  int i;

  if(!(fp=fopen(LAST_FILE,"r"))) {
    log("bad things.");
    send_to_char(ch, "Error! - no last log");
  }
  send_to_char(ch, "Last log\r\n");
  i = fread(&llast, sizeof(struct last_entry), 1, fp);

  while(!feof(fp)) {
    send_to_char(ch, "%10s\t%d\t%s\t%s", llast.username, llast.punique,
        last_array[llast.close_type], ctime(&llast.time));
    i = fread(&llast, sizeof(struct last_entry), 1, fp);
  }
}

static struct char_data *is_in_game(long idnum) {
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (i->character && GET_IDNUM(i->character) == idnum) {
      return i->character;
    }
  }
  return NULL;
}

ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;
  struct char_data *temp;
  int recs, i, num = 0;
  FILE *fp;
  time_t delta;
  struct last_entry mlast;

  *name = '\0';

  if (*argument) { /* parse it */
    half_chop(argument, arg, argument);
    while (*arg) {
      if ((*arg == '*') && (GET_LEVEL(ch) == LVL_IMPL)) {
        list_llog_entries(ch);
        return;
      }
      if (isdigit(*arg)) {
        num = atoi(arg);
        if (num < 0)
          num = 0;
      } else
        strncpy(name, arg, sizeof(name)-1);
      half_chop(argument, arg, argument);
    }
  }

  if (*name && !num) {
    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    /* Allocate mobile event list */
    vict->events = create_list();
    if (load_char(name, vict) <  0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }

    if ((GET_LEVEL(vict) > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
      send_to_char(ch, "You are not sufficiently godly for that!\r\n");
      return;
    }

    send_to_char(ch, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
    GET_IDNUM(vict), (int) GET_LEVEL(vict),
    CLASS_ABBR(vict), GET_NAME(vict),
    GET_HOST(vict) && *GET_HOST(vict) ? GET_HOST(vict) : "(NOHOST)",
    ctime(&vict->player.time.logon));
    free_char(vict);
    return;
    }

  if(num <= 0 || num >= 100) {
    num=10;
  }

  if(!(fp=fopen(LAST_FILE,"r"))) {
    send_to_char(ch, "No entries found.\r\n");
    return;
  }
  fseek(fp,0L,SEEK_END);
  recs=ftell(fp)/sizeof(struct last_entry);

  send_to_char(ch, "Last log\r\n");
  while(num > 0 && recs > 0) {
    fseek(fp,-1* (sizeof(struct last_entry)),SEEK_CUR);
    i = fread(&mlast,sizeof(struct last_entry),1,fp);
    fseek(fp,-1*(sizeof(struct last_entry)),SEEK_CUR);
    if(!*name ||(*name && !str_cmp(name, mlast.username))) {
      send_to_char(ch,"%10.10s %20.20s %16.16s - ",
        mlast.username, mlast.hostname, ctime(&mlast.time));
      if((temp=is_in_game(mlast.idnum)) && mlast.punique == GET_PREF(temp)) {
        send_to_char(ch, "Still Playing  ");
      } else {
        send_to_char(ch, "%5.5s ",ctime(&mlast.close_time)+11);
        delta=mlast.close_time - mlast.time;
        send_to_char(ch, "(%5.5s) ",asctime(gmtime(&delta))+11);
        send_to_char(ch, "%s", last_array[mlast.close_type]);
      }

      send_to_char(ch, "\r\n");
      num--;
    }
    recs--;
  }
  fclose(fp);
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

  half_chop(argument, arg, to_force);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && GET_LEVEL(ch) < LVL_GOD)
      send_to_char(ch, "You cannot force players.\r\n");
    else if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char(ch, "No, no, no!\r\n");
    else {
      send_to_char(ch, "%s", CONFIG_OK);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_LEVEL(vict) >= GET_LEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}
ACMD(do_glist)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *vict;
    struct gain_data *glist = global_gain;
    struct skill_data *curr;

    one_argument(argument, arg);

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
        send_to_char(ch, "There is no such character.\r\n");
        return;
    } else {
        while(glist) {
            if (glist->playerid == GET_IDNUM(vict)) {
				send_to_char(ch, " @WGlobal Skill/Psionic Memory for:\r\n\r\n");
				send_to_char(ch, " @WPlayer [@G %s@W ]@n\r\n", GET_NAME(vict));
                send_to_char(ch, " @WIdnum  [@G %4ld@W ]@n\r\n", GET_IDNUM(vict));
				send_to_char(ch, " @WLevel  [@G %2d@W ]@n\r\n\r\n", glist->playerlevel);

                if (glist->use == NULL) {
                    send_to_char(ch, "@WPlayer has no skill/psionic table.@n\r\n");
                } else {
                    curr = glist->use;
                    while(curr) {
							int pcnt = (int) 100 * ((float)curr->totalgained / (((float)titles[GET_LEVEL(vict)+1].exp) - (float)titles[GET_LEVEL(vict)].exp));

						if (curr->type == 0)
							send_to_char(ch, " @CPsi    @W[ @G%3d@W ]     @WTotal exp: [ @G%10d@W ]   %% of Level [ @G%3d%%@W ]   Times used: [ @G%4d @W]@n\r\n",curr->skillid, curr->totalgained, pcnt, curr->timesused);
						if (curr->type == 1)
							send_to_char(ch, " @cSkill  @W[ @G%3d@W ]     @WTotal exp: [ @G%10d @W]   %% of Level [ @G%3d%%@W ]   Times used: [ @G%4d @W]@n\r\n",curr->skillid, curr->totalgained, pcnt, curr->timesused);
						
						curr = curr->next;
                    }
                }
                return;
            }
            glist = glist->next;
        }
    }
}

// should this include any other info? email? icq? add show staff command?
typedef struct
{
    char* name;
    int   level;
} staff_members_struct;

// Is this the buildport imms, or the main port imms??? -Lump
#ifdef BUILDPORT
staff_members_struct staff_members[] = {
    { "Lump",         45 },
    { "Legion",       44 },
    { "Bosstone",     45 },
    { "Warblade",     45 },
	{ "Gahan",		  45 },
    { NULL,           -1 } /* Very important!!! Don't remove the end of array */
};
#else

staff_members_struct staff_members[] = {
    { "Lump",         45 },
    { "Sappho",       41 },
    { "Jack",         42 },
    { "Shadowrunner", 41 },
    { "Cayce",        41 },
    { "Legion",       44 },
    { "Bosstone",     45 },
    { "Rayth",        41 },
    { "Godfire",      41 },
    { "Gaus",         41 },
    { "FallenNgel",   44 },
    { "Tails",        44 },
    { "Destroyer",    41 },
    { "Warblade",     45 },
	{ "Gahan",		  45 },
	{ "Grifter",	  44 },
    { NULL,           -1 } /* Very important!!! Don't remove the end of array */
};

#endif
ACMD(do_wiznet)
{
    char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
    char buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
    char *msg;
//    char pbuf[MAX_INPUT_LENGTH];
    struct descriptor_data *d;
    char emote = FALSE;
    char any = FALSE;
    int level = LVL_IMMORT;

    skip_spaces(&argument);
    delete_doubledollar(argument);

    if (!*argument) {
        send_to_char(ch, "Usage: wiznet [ #<level> ] [<text> | *<emotetext> | @@ ]\r\n");
        return;
    }

    switch (*argument) {

        case '*':
            emote = TRUE;

        case '#':

            one_argument(argument + 1, buf1);

            if (is_number(buf1)) {

                half_chop(argument+1, buf1, argument);
                level = MAX(atoi(buf1), LVL_IMMORT);

                if (level > GET_LEVEL(ch)) {
                    send_to_char(ch, "You can't wizline above your own level.\r\n");
                    return;
                }
            }
            else if (emote)
                argument++;
            break;

        case '@':
            send_to_char(ch, "God channel status:\r\n");
            for (any = 0, d = descriptor_list; d; d = d->next) {

                if (STATE(d) != CON_PLAYING || GET_LEVEL(d->character) < LVL_IMMORT)
                    continue;

                if (!CAN_SEE(ch, d->character))
                    continue;

                send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
                    PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
                    PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
                    PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
            }
            return;

        case '\\':
            ++argument;
            break;

        default:
            break;
    }

    if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
        send_to_char(ch, "You are offline!\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument) {
        send_to_char(ch, "Don't bother the gods like that!\r\n");
        return;
    }

    if (level > LVL_IMMORT) {
        snprintf(buf1, sizeof(buf1), "@B[@YWiznet@B] @Y%s:@B <@R%d@B>@n@Y %s%s@n\r\n", GET_NAME(ch), level, emote ? "@Y<---@Y " : "", argument);
        snprintf(buf2, sizeof(buf1), "@B[@YWiznet@B] @YSomeone:@B <@R%d@Y>@Y %s%s@n\r\n", level, emote ? "@Y<---@Y " : "", argument);
    }
    else {
        snprintf(buf1, sizeof(buf1), "@B[@YWiznet@B] @Y%s: %s%s@n\r\n", GET_NAME(ch), emote ? "@Y<---@B " : "", argument);
        snprintf(buf2, sizeof(buf1), "@B[@YWiznet@B] @YSomeone: @Y%s%s@n\r\n", emote ? "@Y<---@n " : "", argument);
    }

    for (d = descriptor_list; d; d = d->next) {

        if (IS_PLAYING(d) && (GET_LEVEL(d->character) >= level) &&  (!PRF_FLAGGED(d->character, PRF_NOWIZ))
            && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {

            if (CAN_SEE(d->character, ch)) {
                msg = strdup(buf1);
                send_to_char(d->character, "%s", buf1);
            }
            else {
                msg = strdup(buf2);
                send_to_char(d->character, "%s", buf2);
            }

            add_history(d->character, msg, HIST_WIZNET);
        }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
        send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;
  bool full = FALSE;
  one_argument(argument, arg);

  if (*arg == '*') {
    if (GET_LEVEL(ch) < LVL_GOD){
      send_to_char(ch, "You do not have permission to reset the entire world.\r\n");
      return;
   } else {
      for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i, full);
    send_to_char(ch, "Reset world.\r\n");
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset entire world.", GET_NAME(ch));
    return; }
  } else if (*arg == '.' || !*arg)
    i = world[IN_ROOM(ch)].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i <= top_of_zone_table && (can_edit_zone(ch, i) || GET_LEVEL(ch) > LVL_IMMORT)) {
    reset_zone(i, full);
    send_to_char(ch, "Reset zone #%d: %s.\r\n", zone_table[i].number, zone_table[i].name);
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset zone %d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
  } else
    send_to_char(ch, "You do not have permission to reset this zone. Try %d.\r\n", GET_OLC_ZONE(ch));
}

/*  General fn for wizcommands of the sort: cmd <player> */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_LEVEL(vict) >= GET_LEVEL(ch) && vict != ch)
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER)) {
	send_to_char(ch, "Your victim is not flagged.\r\n");
	return;
      }
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THIEF);
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_KILLER);
      send_to_char(ch, "Pardoned.\r\n");
      send_to_char(vict, "You have been pardoned by the Gods!\r\n");
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_MUTE:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Mute %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Mute %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Your victim is already pretty cold.\r\n");
	return;
      }
      SET_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
		GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	return;
      }
      mudlog(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected || AFF_FLAGS(vict)) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
      AFF_FLAGS(vict)[taeller] = 0;
    send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
	send_to_char(ch, "All psionics removed.\r\n");
      } else {
	send_to_char(ch, "Your victim does not have any affections!\r\n");
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      /*  SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       *  but this function handles 'reroll', 'pardon', 'freeze', etc. */
      break;
    }
    save_char(vict);
  }
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 FIXME: overflow possible */
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;

  if (listall) {
    int i, j, k, l, m, n, o;
    char buf[MAX_STRING_LENGTH];

    sprintbitarray(zone_table[zone].zone_flags, zone_bits, ZN_ARRAY_MAX, buf);

    tmp = snprintf(bufptr, left,
	"%3d %-30.30s%s By: %-10.10s%s Age: %3d; Reset: %3d (%s); Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name, KNRM, zone_table[zone].builders, KNRM,
	zone_table[zone].age, zone_table[zone].lifespan,
        zone_table[zone].reset_mode ? ((zone_table[zone].reset_mode == 1) ? "Reset when no players are in zone" : "Normal reset") : "Never reset",
	zone_table[zone].bot, zone_table[zone].top);
        i = j = k = l = m = n = o = 0;

        for (i = 0; i < top_of_world; i++)
          if (world[i].number >= zone_table[zone].bot && world[i].number <= zone_table[zone].top)
            j++;

        for (i = 0; i < top_of_objt; i++)
          if (obj_index[i].vnum >= zone_table[zone].bot && obj_index[i].vnum <= zone_table[zone].top)
            k++;

        for (i = 0; i < top_of_mobt; i++)
          if (mob_index[i].vnum >= zone_table[zone].bot && mob_index[i].vnum <= zone_table[zone].top)
            l++;

        for (i = 0; i<= top_shop; i++)
          if (SHOP_NUM(i) >= zone_table[zone].bot && SHOP_NUM(i) <= zone_table[zone].top)
            m++;

        for (i = 0; i < top_of_trigt; i++)
          if (trig_index[i]->vnum >= zone_table[zone].bot && trig_index[i]->vnum <= zone_table[zone].top)
            n++;

        o = count_quests(zone_table[zone].bot, zone_table[zone].top);

	tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Flags:    %s\r\n"
                        "         Min Lev:  %2d\r\n"
                        "         Max Lev:  %2d\r\n"
                        "         Rooms:    %2d\r\n"
                        "         Objects:  %2d\r\n"
                        "         Mobiles:  %2d\r\n"
                        "         Shops:    %2d\r\n"
                        "         Triggers: %2d\r\n"
                        "         Quests:   %2d\r\n",
			buf, zone_table[zone].min_level, zone_table[zone].max_level,
                        j, k, l, m, n, o);

    return tmp;
  }

    return snprintf(bufptr, left,
        "%3d %-*s%s By: %-10.10s%s Range: %5d-%5d\r\n", zone_table[zone].number,
	count_color_chars(zone_table[zone].name)+30, zone_table[zone].name, KNRM,
	zone_table[zone].builders, KNRM, zone_table[zone].bot, zone_table[zone].top);
}

ACMD(do_show)
{
  int i, j, k, l, con, builder =0;		/* i, j, k to specifics? */
  size_t len, nlen;
  zone_rnum zrn;
  zone_vnum zvn;
  byte self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj;
  struct descriptor_data *d;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH],
	arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  int r, g, b;
  char colour[16];

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_IMMORT },			/* 1 */
    { "player",		LVL_IMMORT },
    { "rent",		LVL_IMMORT },
    { "stats",		LVL_IMMORT },
    { "errors",		LVL_IMMORT },			/* 5 */
    { "death",		LVL_IMMORT },
    { "godrooms",	LVL_IMMORT },
    { "shops",		LVL_IMMORT },
    { "houses",		LVL_IMMORT },
    { "snoop",		LVL_IMMORT },			/* 10 */
    { "thaco",      LVL_IMMORT },
    { "exp",        LVL_IMMORT },
    { "colour",     1 },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));	/* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l) {
  /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone, 1);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, sizeof(buf), zrn, 1);
      else {
	send_to_char(ch, "That is not a valid zone.\r\n");
	return;
      }
    } else {
      char *buf2;
      if (*value)
        builder = 1;
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++) {
        if (*value) {
          buf2 = strtok(strdup(zone_table[zrn].builders), " ");
          while (buf2) {
            if (!str_cmp(buf2, value)) {
              if (builder == 1)
                builder++;
              break;
          }
            buf2 = strtok(NULL, " ");
          }
          if (!buf2)
	    continue;
	}
	nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn, 0);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    }
    if (builder == 1)
      send_to_char(ch, "%s has not built any zones here.\r\n", CAP(value));
    else if (builder == 2)
      send_to_char(ch, "The following zones have been built by: %s\r\n", CAP(value));
    page_string(ch->desc, buf, TRUE);
    break;

  /* show player */
  case 2:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    /* Allocate mobile event list */
    vict->events = create_list();
    if (load_char(value, vict) < 0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }
    send_to_char(ch, "Player: %-12s (%s) [%2d %s]\r\n", GET_NAME(vict),
      genders[(int) GET_SEX(vict)], GET_LEVEL(vict), CLASS_ABBR(vict));
    send_to_char(ch, "Ru: %-8d  Bal: %-8d  Exp: %-8d  Lessons: %-3d\r\n",
    GET_UNITS(vict), GET_BANK_UNITS(vict), GET_EXP(vict), GET_PRACTICES(vict));

    /* ctime() uses static buffer: do not combine. */
    send_to_char(ch, "Started: %-20.16s  ", ctime(&vict->player.time.birth));
    send_to_char(ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
      ctime(&vict->player.time.logon),
      (int) (vict->player.time.played / 3600),
      (int) (vict->player.time.played / 60 % 60));
    free_char(vict);
    break;

  /* show rent */
  case 3:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(ch, value);
    break;

  /* show stats */
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    send_to_char(ch,
	"Current stats:\r\n"
	"  %5d players in game  %5d connected\r\n"
	"  %5d registered\r\n"
	"  %5d mobiles          %5d prototypes\r\n"
	"  %5d objects          %5d prototypes\r\n"
	"  %5d rooms            %5d zones\r\n"
  "  %5d triggers         %5d shops\r\n"
  "  %5d large bufs       %5d autoquests\r\n"
	"  %5d buf switches     %5d overflows\r\n"
	"  %5d lists\r\n",
	i, con,
	top_of_p_table + 1,
	j, top_of_mobt + 1,
	k, top_of_objt + 1,
	top_of_world + 1, top_of_zone_table + 1,
	top_of_trigt + 1, top_shop + 1,
	buf_largecount, total_quests,
	buf_switches, buf_overflows, global_lists->iSize
	);
    break;

  /* show errors */
  case 5:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < DIR_COUNT; j++) {
      	if (!W_EXIT(i,j))
      	  continue;
        if (W_EXIT(i,j)->to_room == 0) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+40, world[i].name, QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf))
              break;
            len += nlen;
        }
        if (W_EXIT(i,j)->to_room == NOWHERE && !W_EXIT(i,j)->general_description) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+ 40, world[i].name, QNRM, dirs[j]);
            if (len + nlen >= sizeof(buf))
              break;
            len += nlen;
        }
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show death */
  case 6:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show godrooms */
  case 7:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s%s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name, QNRM);
        if (len + nlen >= sizeof(buf))
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show shops */
  case 8:
    show_shops(ch, value);
    break;

  /* show houses */
  case 9:
    hcontrol_list_houses(ch, value);
    break;

  /* show snoop */
  case 10:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_LEVEL(ch) < GET_LEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      i++;
      send_to_char(ch, "%-10s%s - snooped by %s%s.\r\n", GET_NAME(d->snooping->character), QNRM, GET_NAME(d->character), QNRM);
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;

  /* show thaco */
  case 11:
     send_to_char(ch, "PK Rooms\r\n-----------\r\n");
	 for (i = 0, j = 0; i <= top_of_world; i++)
       if (ROOM_FLAGGED(i, ROOM_PK_OK))
         send_to_char(ch, "%2d: [%5d] %s\r\n", ++j, world[i].number, world[i].name);
	 break;

  /* show experience tables */
  case 12:
    send_to_char(ch, "Hunter Mobs\r\n-----------\r\n");
	for (i = 0, j = 0; i <= top_of_mobt; i++)
      if (MOB_FLAGGED(&mob_proto[i], MOB_HUNTER))
        send_to_char(ch, "%2d. [%5d] %s\r\n", ++j, mob_index[i].vnum, mob_proto[i].player.short_descr);
	break;

  case 13:
    len = strlcpy(buf, "Colours\r\n--------------------------\r\n", sizeof(buf));
		k = 0;	
    for (r = 0; r < 6; r++)
			for (g = 0; g < 6; g++)
			  for (b = 0; b < 6; b++) {
					  sprintf(colour, "F%d%d%d", r, g, b);
					nlen = snprintf(buf + len, sizeof(buf) - len,  "%s%s%s", ColourRGB(ch->desc, colour), colour, ++k % 6 == 0 ? "\tn\r\n" : "    ");
				if (len + nlen >= sizeof(buf))
					break;
				len += nlen;
				}	
    page_string(ch->desc, buf, TRUE);	
    break;

  /* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}

/* The do_set function */

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
        if (on) SET_BIT_AR(flagset, flags); \
        else if (off) REMOVE_BIT_AR(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

/* The set options available */
  struct set_struct {
    const char *cmd;
    const int cmd_no;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
    { "ac",          SET_AC,        LVL_BUILDER,   BOTH,   NUMBER  },
    { "afk",         SET_AFK,       LVL_BUILDER,   PC,     BINARY  },
    { "age",         SET_AGE,       LVL_GOD,       BOTH,   NUMBER  },
    { "attrib",      SET_ATTR,      LVL_GRGOD,     PC,     NUMBER  },
    { "bank",        SET_BANK,      LVL_BUILDER,   PC,     NUMBER  },
    { "bionics",     SET_BIONIC,    LVL_GOD,       PC,     MISC    },
    { "brief",       SET_BRIEF,     LVL_GOD,       PC,     BINARY  },
    { "cha",         SET_CHA,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "class",       SET_CLASS,     LVL_BUILDER,   BOTH,   MISC    },
    { "color",       SET_COLOR,     LVL_GOD,       PC,     BINARY  },
    { "con",         SET_CON,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "damroll",     SET_DAMROLL,   LVL_BUILDER,   BOTH,   NUMBER  },
    { "deleted",     SET_DELETE,    LVL_IMPL,      PC,     BINARY  },
    { "dex",         SET_DEX,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "drunk",       SET_DRUNK,     LVL_BUILDER,   BOTH,   MISC    },
    { "dts",         SET_DTS,       LVL_GRGOD,     PC,     NUMBER  },
    { "essences",    SET_ESSENCES,  LVL_GOD,       PC,     NUMBER  },
    { "exp",         SET_EXP,       LVL_GOD,       BOTH,   NUMBER  },
    { "fame",        SET_FAME,      LVL_IMPL,       PC,     NUMBER  },
    { "frequency",   SET_FREQ,      LVL_GOD,       PC,     NUMBER  },
    { "frozen",      SET_FROZEN,    LVL_GRGOD,     PC,     BINARY  },
    { "units",       SET_UNITS,     LVL_BUILDER,   BOTH,   NUMBER  },
    { "height",      SET_HEIGHT,    LVL_BUILDER,   BOTH,   NUMBER  },
    { "hitpoints",   SET_HIT,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "hitroll",     SET_HITROLL,   LVL_BUILDER,   BOTH,   NUMBER  },
    { "hunger",      SET_HUNGER,    LVL_BUILDER,   BOTH,   MISC    },
    { "idnum",       SET_IDNUM,     LVL_IMPL,      PC,     NUMBER  },
    { "int",         SET_INT,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "invis",       SET_INVIS,     LVL_GOD,       PC,     NUMBER  },
    { "invstart",    SET_ISTART,    LVL_BUILDER,   PC,     BINARY  },
    { "killer",      SET_KILLER,    LVL_GOD,       PC,     BINARY  },
    { "level",       SET_LEVEL,     LVL_GRGOD,     BOTH,   NUMBER  },
    { "loadroom",    SET_LDROOM,    LVL_BUILDER,   PC,     MISC    },
    { "psi",         SET_PSI,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "maxhit",      SET_MAXHIT,    LVL_BUILDER,   BOTH,   NUMBER  },
    { "maxpsi",      SET_MAXPSI,    LVL_BUILDER,   BOTH,   NUMBER  },
    { "maxmove",     SET_MAXMOVE,   LVL_BUILDER,   BOTH,   NUMBER  },
    { "move",        SET_MOVE,      LVL_BUILDER,   BOTH,   NUMBER  },
    { "multiplay",   SET_MULTI,     LVL_CIMP,      PC,     BINARY  },
    { "name",        SET_NAME,      LVL_IMMORT,    PC,     MISC    },
    { "nodelete",    SET_NODELETE,  LVL_GOD,       PC,     BINARY  },
    { "nohassle",    SET_NOHASS,    LVL_GOD,       PC,     BINARY  },
    { "nosummon",    SET_NOSUMM,    LVL_BUILDER,   PC,     BINARY  },
    { "nowho",       SET_NOWHO,     LVL_GOD,       PC,     BINARY  },
    { "nowizlist",   SET_NOWIZ,     LVL_GRGOD,     PC,     BINARY  },
    { "olc",         SET_OLC,       LVL_GRGOD,     PC,     MISC    },
    { "password",    SET_PASS,      LVL_IMPL,      PC,     MISC    },
    { "pcn",         SET_PCN,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "pk",          SET_PK,        LVL_GRGOD,     PC,     BINARY  },
    { "poofin",      SET_POOFIN,    LVL_IMMORT,    PC,     MISC    },
    { "poofout",     SET_POOFOUT,   LVL_IMMORT,    PC,     MISC    },
    { "practices",   SET_PRAC,      LVL_GOD,       PC,     NUMBER  },
    { "psimastery",  SET_PSIMAST,   LVL_CIMP,      PC,     NUMBER  },
    { "quest",       SET_QUEST,     LVL_GOD,       PC,     NUMBER  },
    { "questpoints", SET_QPS,       LVL_GOD,       PC,     NUMBER  },
    { "questhistory",SET_QHIST,     LVL_GOD,       PC,     NUMBER  },
    { "remortprac",  SET_REMRTPRAC, LVL_GRGOD,     PC,     NUMBER  },
    { "rep",         SET_REP,       LVL_CIMP,      PC,     BINARY  },
    { "remorts",      SET_REMORTS,    LVL_CIMP,      PC,     NUMBER  },
    { "room",        SET_ROOM,      LVL_BUILDER,   BOTH,   NUMBER  },
    { "screenwidth", SET_SCREEN,    LVL_GOD,       PC,     NUMBER  },
    { "sex",         SET_SEX,       LVL_GOD,       BOTH,   MISC    },
    { "showvnums",   SET_SHOWVNUMS, LVL_BUILDER,   PC,     BINARY  },
    { "siteok",      SET_SITEOK,    LVL_GOD,       PC,     BINARY  },
    { "spectator",   SET_SPECT,     LVL_GRGOD,     PC,     BINARY  },
    { "str",         SET_STR,       LVL_BUILDER,   BOTH,   NUMBER  },
    { "stradd",      SET_STRADD,    LVL_BUILDER,   BOTH,   NUMBER  },
    { "team",        SET_TEAM,      LVL_GOD,       BOTH,   MISC    },
    { "tempmortal",  SET_TEMPMORT,  LVL_GRGOD,     PC,     BINARY  },
    { "thief",       SET_THIEF,     LVL_GOD,       PC,     BINARY  },
    { "thirst",      SET_THIRST,    LVL_BUILDER,   BOTH,   MISC    },
    { "title",       SET_TITLE,     LVL_GOD,       PC,     MISC    },
    { "variable",    SET_VAR,       LVL_GRGOD,     PC,     MISC    },
    { "weight",      SET_WEIGHT,    LVL_BUILDER,   BOTH,   NUMBER  },
    { "\n",          -1,            0,             BOTH,   MISC    }
  };

static int perform_set(struct char_data *ch, struct char_data *vict, int cmd, char *val_arg)
{
    int i;
    int on = 0;
    int off = 0;
    int value = 0;
    int qvnum;
    room_rnum rnum;
    room_vnum rvnum;

    // Check to make sure all the levels are correct
    if (GET_LEVEL(ch) != LVL_IMPL) {
        if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
            send_to_char(ch, "Maybe that's not such a great idea...\r\n");
            return (0);
        }
    }

    if (GET_LEVEL(ch) < set_fields[cmd].level) {
        send_to_char(ch, "You are not godly enough for that!\r\n");
        return (0);
    }

    // Make sure the PC/NPC is correct
    if (IS_NPC(vict) && !(set_fields[cmd].pcnpc & NPC)) {
        send_to_char(ch, "You can't do that to a beast!\r\n");
        return (0);
    }
    else if (!IS_NPC(vict) && !(set_fields[cmd].pcnpc & PC)) {
        send_to_char(ch, "That can only be done to a beast!\r\n");
        return (0);
    }

    // Find the value of the argument
    if (set_fields[cmd].type == BINARY) {

        if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
            on = 1;
        else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
            off = 1;
        else {
            send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
            return (0);
        }

    }
    else if (set_fields[cmd].type == NUMBER)
        value = atoi(val_arg);

    switch (set_fields[cmd].cmd_no) {
        case SET_AC:
            vict->points.armor = RANGE(0, 10000);
            affect_total(vict);
            log("(GC) %s changed the AC for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_AFK:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_AFK);
            log("(GC) %s sets %s AFK.", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_AGE:
            if (value < 2 || value > 200) {    /* Arbitrary limits. */
                send_to_char(ch, "Ages 2 to 200 accepted.\r\n");
                return (0);
            }
            // NOTE: May not display the exact age specified due to the integer
            // division used elsewhere in the code.  Seems to only happen for
            // some values below the starting age (17) anyway. -gg 5/27/98 */
            vict->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
            log("(GC) %s sets %s age to %d.", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_BANK:
            GET_BANK_UNITS(vict) = RANGE(0, 1000000000);
            log("(GC) %s sets the amount of money %s has in the bank to %d.", GET_NAME(ch), GET_NAME(vict), GET_BANK_UNITS(vict));
            break;

        case SET_BRIEF:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
            log("(GC) %s set %s to PRF_BRIEF", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_CHA:
            RANGE(3, 30);
            vict->real_abils.cha = value;
            affect_total(vict);
            log("(GC) %s set CHA for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_CLASS:
            if ((i = parse_class(val_arg, TRUE)) == CLASS_UNDEFINED) {
                send_to_char(ch, "That is not a class.\r\n");
                return (0);
            }
            GET_CLASS(vict) = i;
            log("(GC) %s set the class of %s to %s", GET_NAME(ch), GET_NAME(vict), pc_class[GET_CLASS(vict)].class_name);
            break;

        case SET_COLOR:
            SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1));
            SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_2));
            log("(GC) %s set the color of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_CON:
            RANGE(3, 30);
            vict->real_abils.con = value;
            affect_total(vict);
            log("(GC) %s set CON for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_DAMROLL:
            vict->points.damroll = RANGE(-20000, 20000);
            log("(GC) %s set %s's damroll to %d", GET_NAME(ch), GET_NAME(vict), GET_DAMROLL(vict));
            affect_total(vict);
            break;

        case SET_DELETE:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
            log("(GC) %s changed the DELETED status of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_DEX:
            RANGE(3, 30);
            vict->real_abils.dex = value;
            affect_total(vict);
            log("(GC) %s set DEX for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_DRUNK:
            if (!str_cmp(val_arg, "off")) {
                GET_COND(vict, DRUNK) = -1;
                send_to_char(ch, "%s's drunkenness is now off.\r\n", GET_NAME(vict));
                log("(GC) %s has set the drunkeness of %s to off", GET_NAME(ch), GET_NAME(vict));
            }
            else if (is_number(val_arg)) {
                value = atoi(val_arg);
                RANGE(0, 24);
                GET_COND(vict, DRUNK) = value;
                send_to_char(ch, "%s's drunkenness set to %d.\r\n", GET_NAME(vict), value);
                log("(GC) %s has set the drunkeness of %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            }
            else {
                send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
                return (0);
            }
            break;

        case SET_EXP:
            vict->points.exp = RANGE(0, 2050000000);
            log("(GC) %s adjusted %s's experience", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_FROZEN:
            if (ch == vict && on) {
                send_to_char(ch, "Better not -- could be a long winter!\r\n");
                return (0);
            }
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
            log("(GC) %s changed the FROZEN setting on %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_UNITS:
            GET_UNITS(vict) = RANGE(0, 1000000000);
            log("(GC) %s set the amount of money %s has to %d", GET_NAME(ch), GET_NAME(vict), GET_UNITS(vict));
            break;

        case SET_HEIGHT:
            GET_HEIGHT(vict) = value;
            affect_total(vict);
            log("(GC) %s has set the height of %s to %d", GET_NAME(ch), GET_NAME(vict), GET_HEIGHT(vict));
            break;

        case SET_HIT:
            vict->points.hit = RANGE(-9, vict->points.max_hit);
            affect_total(vict);
            log("(GC) %s set HIT for %s to %d", GET_NAME(ch), GET_NAME(vict), vict->points.hit);
            break;

        case SET_HITROLL:
            vict->points.hitroll = RANGE(-20, 20000);
            affect_total(vict);
            log("(GC) %s set HITROLL for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_HITROLL(vict));
            break;

        case SET_HUNGER:
            if (!str_cmp(val_arg, "off")) {
                GET_COND(vict, HUNGER) = -1;
                send_to_char(ch, "%s's hunger is now off.\r\n", GET_NAME(vict));
                log("(GC) %s has set the hunger of %s to off", GET_NAME(ch), GET_NAME(vict));
            }
            else if (is_number(val_arg)) {
                value = atoi(val_arg);
                RANGE(0, 24);
                GET_COND(vict, HUNGER) = value;
                send_to_char(ch, "%s's hunger set to %d.\r\n", GET_NAME(vict), value);
                log("(GC) %s has set the hunger of %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            }
            else {
                send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
                return (0);
            }
            break;

        case SET_INT:
            RANGE(3, 30);
            vict->real_abils.intel = value;
            affect_total(vict);
            log("(GC) %s set INT for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_INVIS:
            if (GET_LEVEL(ch) < LVL_IMPL && GET_LEVEL(vict) >= GET_LEVEL(ch) && ch != vict) {
                send_to_char(ch, "You aren't godly enough for that!\r\n");
                return (0);
            }
            GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
            log("%s set %s invis level to %d", GET_NAME(ch), GET_NAME(vict), GET_INVIS_LEV(vict));
            break;

        case SET_ISTART:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
            log("(GC) %s set invis start for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_KILLER:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
            log("(GC) %s changed the KILLER status of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_PRAC:
            GET_PRACTICES(vict) = RANGE(0, 10000);
            log("(GC) %s set the # of practices for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_PRACTICES(vict));
            break;

        case SET_LEVEL:
            if ((!IS_NPC(vict) && value > GET_LEVEL(ch)) || value > LVL_IMPL) {
                send_to_char(ch, "You can't do that.\r\n");
                return (0);
            }
            RANGE(0, LVL_IMPL);
            vict->player.level = value;
            log("(GC) %s set %s 's level to %d", GET_NAME(ch), GET_NAME(vict), GET_LEVEL(vict));
            break;

        case SET_LDROOM:
            if (!str_cmp(val_arg, "off") || !str_cmp(val_arg, "none"))
                REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
            else if (is_number(val_arg)) {

                rvnum = atoi(val_arg);

                if (real_room(rvnum) != NOWHERE) {
                    SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
                    GET_LOADROOM(vict) = rvnum;
                    send_to_char(ch, "%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
                    log("(GC) %s set the loadroom of %s to %d", GET_NAME(ch), GET_NAME(vict), GET_LOADROOM(vict));
                }
                else {
                    send_to_char(ch, "That room does not exist!\r\n");
                    return (0);
                }
            }
            else {
                send_to_char(ch, "Must be 'off', 'none', or a room's virtual number.\r\n");
                return (0);
            }
            break;

        case SET_PSI:
            vict->points.psi = RANGE(0, vict->points.max_psi);
            affect_total(vict);
            log("(GC) %s set psi points for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_PSI(vict));
            break;

        case SET_REMORTS:
            vict->player.num_remorts = RANGE(0, 100);;
            log("(GC) %s set %s 's remort level to %d", GET_NAME(ch), GET_NAME(vict), GET_REMORTS(vict));
            break;

        case SET_MAXHIT:
            vict->points.max_hit = RANGE(1, 50000);
            affect_total(vict);
            log("(GC) %s set max hit for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_MAX_HIT(vict));
            break;

        case SET_MAXPSI:
            vict->points.max_psi = RANGE(1, 50000);
            affect_total(vict);
            log("(GC) %s set max psi for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_MAX_PSI(vict));
            break;

        case SET_MAXMOVE:
            vict->points.max_move = RANGE(1, 50000);
            affect_total(vict);
            log("(GC) %s set max move for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_MAX_MOVE(vict));
            break;

        case SET_MOVE:
            vict->points.move = RANGE(0, vict->points.max_move);
            affect_total(vict);
            log("(GC) %s set move points for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_MOVE(vict));
            break;

        case SET_NAME:

            if (ch != vict && GET_LEVEL(ch) < LVL_IMPL) {
                send_to_char(ch, "Only Imps can change the name of other players.\r\n");
                return (0);
            }

            if (!change_player_name(ch, vict, val_arg)) {
                send_to_char(ch, "Name has not been changed!\r\n");
                return (0);
            }
            break;

        case SET_NODELETE:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
            log("(GC) %s changed the NODELETE setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_NOHASS:
            if (GET_LEVEL(ch) < LVL_GOD && ch != vict) {
            send_to_char(ch, "You aren't godly enough for that!\r\n");
            return (0);
            }
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
            log("(GC) %s changed the NOHASSLE setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_NOSUMM:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOSUMMON);
            send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
            log("(GC) %s changed the NOSUMMON setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_NOWIZ:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
            log("(GC) %s changed the NOWIZLIST setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_OLC:
            if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions") || is_abbrev(val_arg, "aedit")) {
                GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
                log("(GC) %s granted AEDIT olc permission to %s", GET_NAME(ch), GET_NAME(vict));
            }
            else if (is_abbrev(val_arg, "hedit") || is_abbrev(val_arg, "help")) {
                GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
                log("(GC) %s granted HEDIT olc permission to %s", GET_NAME(ch), GET_NAME(vict));
            }
            else if (*val_arg == '*' || is_abbrev(val_arg, "all")) {
                GET_OLC_ZONE(vict) = ALL_PERMISSION;
                log("(GC) %s granted ALL olc permission to %s", GET_NAME(ch), GET_NAME(vict));
            }
            else if (is_abbrev(val_arg, "off")) {
                GET_OLC_ZONE(vict) = NOWHERE;
                log("(GC) %s removed olc permission from %s", GET_NAME(ch), GET_NAME(vict));
            }
            else if (!is_number(val_arg))  {
                send_to_char(ch, "Value must be a zone number, 'aedit', 'hedit', 'off' or 'all'.\r\n");
                return (0);
            }
            else {
                GET_OLC_ZONE(vict) = atoi(val_arg);
                log("(GC) %s granted ALL olc permission to %s", GET_NAME(ch), GET_NAME(vict));
            }
            break;

        case SET_PASS:
            if (GET_LEVEL(vict) >= LVL_GRGOD) {
                send_to_char(ch, "You cannot change that.\r\n");
                return (0);
            }
            strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);    /* strncpy: OK (G_P:MAX_PWD_LENGTH) */
            *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
            send_to_char(ch, "Password changed to '%s'.\r\n", val_arg);
            log("(GC) %s changed the password of %s!!", GET_NAME(ch), GET_NAME(vict));
            break;
			
        case SET_POOFIN:
            if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL)) {

                skip_spaces(&val_arg);

                if (POOFIN(vict))
                    free(POOFIN(vict));

                if (!*val_arg) {
                    POOFIN(vict) = NULL;
                    log("(GC) %s has cleared the poof-in for %s", GET_NAME(ch), GET_NAME(vict));
                }
                else {
                    POOFIN(vict) = strdup(val_arg);
                    log("(GC) %s has set the poof-in for %s", GET_NAME(ch), GET_NAME(vict));
                }
            }
            break;

        case SET_POOFOUT:
            if ((vict == ch) || (GET_LEVEL(ch) == LVL_IMPL)) {
                skip_spaces(&val_arg);

                if (POOFOUT(vict))
                    free(POOFOUT(vict));

                if (!*val_arg) {
                    POOFOUT(vict) = NULL;
                    log("(GC) %s has cleared the poof-out for %s", GET_NAME(ch), GET_NAME(vict));
                }
                else {
                    POOFOUT(vict) = strdup(val_arg);
                    log("(GC) %s has set the poof-out for %s", GET_NAME(ch), GET_NAME(vict));
                }
            }
            break;

        case SET_ROOM:
            if ((rnum = real_room(value)) == NOWHERE) {
                send_to_char(ch, "No room exists with that number.\r\n");
                return (0);
            }
            if (IN_ROOM(vict) != NOWHERE)
                char_from_room(vict);
            char_to_room(vict, rnum);
            log("(GC) %s sent %s to room %d", GET_NAME(ch), GET_NAME(vict), rnum);
            break;

        case SET_SCREEN:
            GET_SCREEN_WIDTH(vict) = RANGE(40, 200);
            log("(GC) %s changed the screen width of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_SEX:
            if ((i = search_block(val_arg, genders, FALSE)) < 0) {
                send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
                return (0);
            }
            GET_SEX(vict) = i;
            log("(GC) %s changed %s sex to %s", GET_NAME(ch), GET_NAME(vict), genders[GET_SEX(vict)]);
            break;

        case SET_SHOWVNUMS:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SHOWVNUMS);
            log("(GC) %s changed the SHOWVNUMS setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_SITEOK:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
            log("(GC) %s changed the SITEOK setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_STR:
            RANGE(3, 30);
            vict->real_abils.str = value;
            vict->real_abils.str_add = 0;
            affect_total(vict);
            log("(GC) %s set STR for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_QUEST:
            RANGE(1, 59999);
            GET_QUEST(vict) = value;
            log("(GC) %s set quest for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_FAME:
            RANGE(1, 1000);
            GET_FAME(vict) = value;
            log("(GC) %s set fame for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        //case SET_STRADD:
        //    vict->real_abils.str_add = RANGE(0, 100);
        //    if (value > 0)
        //    vict->real_abils.str = 25;
        //    affect_total(vict);
        //    log("(GC) %s set STR ADD for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_STR_ADD(vict));
        //    break;

        case SET_THIEF:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
            log("(GC) %s changed the THIEF status of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_THIRST:
            if (!str_cmp(val_arg, "off")) {
                GET_COND(vict, THIRST) = -1;
                send_to_char(ch, "%s's thirst is now off.\r\n", GET_NAME(vict));
                log("(GC) %s has set the thirst of %s to off", GET_NAME(ch), GET_NAME(vict));
            }
            else if (is_number(val_arg)) {
                value = atoi(val_arg);
                RANGE(0, 24);
                GET_COND(vict, THIRST) = value;
                send_to_char(ch, "%s's thirst set to %d.\r\n", GET_NAME(vict), value);
                log("(GC) %s has set the thirst of %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            }
            else {
                send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
                return (0);
            }
            break;

        case SET_TITLE:
            set_title(vict, val_arg);
            send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
            log("(GC) %s set %s title to %s", GET_NAME(ch), GET_NAME(vict), GET_TITLE(vict));
            break;

        case SET_VAR:
            return (perform_set_dg_var(ch, vict, val_arg));
            break;

        case SET_WEIGHT:
            GET_WEIGHT(vict) = value;
            affect_total(vict);
            log("(GC) %s set the weight of %s to %d", GET_NAME(ch), GET_NAME(vict), GET_WEIGHT(vict));
            break;

        case SET_PCN:
            RANGE(3, 30);
            vict->real_abils.pcn = value;
            affect_total(vict);
            log("(GC) %s set PCN for %s to %d", GET_NAME(ch), GET_NAME(vict), value);
            break;

        case SET_QPS:
            GET_QUESTPOINTS(vict) = RANGE(0, 100000000);
            log("(GC) %s set the questpoints of %s to %d", GET_NAME(ch), GET_NAME(vict), GET_QUESTPOINTS(vict));
            break;

        case SET_QHIST:
            qvnum = atoi(val_arg);
            if (real_quest(qvnum) == NOTHING) {
                send_to_char(ch, "That quest doesn't exist.\r\n");
                return (FALSE);
            }

            if (is_complete(vict, qvnum)) {
                remove_completed_quest(vict, qvnum);
                send_to_char(ch, "Quest %d removed from history for player %s.\r\n", qvnum, GET_NAME(vict));
                log("(GC) %s has reset the quest history for quest %d of %s", GET_NAME(ch), qvnum, GET_NAME(vict));
            }
            else {
                add_completed_quest(vict, qvnum);
                send_to_char(ch, "Quest %d added to history for player %s.\r\n", qvnum, GET_NAME(vict));
                log("(GC) %s has set the quest history as completed for quest %d of %s", GET_NAME(ch), qvnum, GET_NAME(vict));
            }
            break;

        case SET_ESSENCES:
            vict->player.essences = RANGE(0, 2500);
            log("(GC) %s has set the number of essences of %s to %d", GET_NAME(ch), GET_NAME(vict), vict->player.essences);
            break;

        //case SET_REMRTPRAC:
        //    vict->player_specials->saved.remort_practices = RANGE(0, 10000);
        //    log("(GC) %s has set %s REMORT PRACTICES to %d", GET_NAME(ch), GET_NAME(vict), GET_REMORT_PRACTICES(vict));
        //    break;

        case SET_ATTR:
            vict->player_specials->saved.attrib_add = value;
            log("(GC) %s has set %s ATTRIB add to %d", GET_NAME(ch), GET_NAME(vict), vict->player_specials->saved.attrib_add);
            break;

        case SET_PK:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_PK);
            log("(GC) %s changed the PK status of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_REP:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_REP);
            log("(GC) %s changed the REP status of %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_NOWHO:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOWHO);
            log("(GC) %s changed the NOWHO setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_MULTI:
            SET_OR_REMOVE(PLR_FLAGS(vict), PLR_MULTI);
            log("(GC) %s changed the MULTI setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_FREQ:
            GET_FREQ(vict) = RANGE(0, 32000);
            log("(GC) %s has changed the frequency setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_SPECT:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SPECTATOR);
            log("(GC) %s changed the SPECTATOR setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_TEMPMORT:
            SET_OR_REMOVE(PRF_FLAGS(vict), PRF_TEMP_MORTAL);
            log("(GC) %s changed the TEMP_MORTAL setting for %s", GET_NAME(ch), GET_NAME(vict));
            break;

        case SET_DTS:
            GET_NUM_DTS(vict) = RANGE(0, 32000);
            log("(GC) %s changed the DT counter for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_NUM_DTS(vict));
            break;

        case SET_PSIMAST:
            GET_PSIMASTERY(vict) = RANGE(0,250);
            log("(GC) %s changed the PSIMASTERY for %s to %d", GET_NAME(ch), GET_NAME(vict), GET_PSIMASTERY(vict));
            break;

        default:
            send_to_char(ch, "Can't set that!\r\n");
            return (0);

    }

    // Show the new value of the variable
    if (set_fields[cmd].type == BINARY)
        send_to_char(ch, "%s %s for %s.\r\n", set_fields[cmd].cmd, ONOFF(on), GET_NAME(vict));
    else if (set_fields[cmd].type == NUMBER)
        send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[cmd].cmd, value);
    else
        send_to_char(ch, "%s", CONFIG_OK);

    return (1);
}

void show_set_help(struct char_data *ch)
{
  const char *set_levels[] = {"Imm", "God", "GrGod", "CO-IMP", "IMP"};
  const char *set_targets[] = {"PC", "NPC", "BOTH"};
  const char *set_types[] = {"MISC", "BINARY", "NUMBER"};
  char buf[MAX_STRING_LENGTH];
  int i, len=0, add_len=0;

  len = snprintf(buf, sizeof(buf), "%sCommand             Lvl    Who?  Type%s\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
  for (i = 0; *(set_fields[i].cmd) != '\n'; i++) {
	if (set_fields[i].level <= GET_LEVEL(ch)) {
      add_len = snprintf(buf+len, sizeof(buf)-len, "%-20s%-5s  %-4s  %-6s\r\n", set_fields[i].cmd,
                                        set_levels[((int)(set_fields[i].level) - LVL_IMMORT)],
                                        set_targets[(int)(set_fields[i].pcnpc)-1],
                                        set_types[(int)(set_fields[i].type)]);
      len += add_len;
    }
  }
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "help")) {
    show_set_help(ch);
    return;
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    send_to_char(ch, "       %sset help%s will display valid fields\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such player.\r\n");
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such creature.\r\n");
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    /* Allocate mobile event list */
    cbuf->events = create_list();
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char(ch, "Sorry, you can't do that.\r\n");
	return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  if (*(set_fields[mode].cmd) == '\n') {
    retval = 0; /* skips saving below */
    send_to_char(ch, "Can't set that!\r\n");
  } else
  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict);
    if (is_file) {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_saveall)
{
 if (GET_LEVEL(ch) < LVL_BUILDER)
    send_to_char (ch, "You are not holy enough to use this privelege.\n\r");
 else {
    save_all();
    House_save_all();
    send_to_char(ch, "World and house files saved.\n\r");
 }
}

ACMD(do_links)
{
  zone_rnum zrnum;
  zone_vnum zvnum;
  room_rnum nr, to_room;
  int first, last, j;
  char arg[MAX_INPUT_LENGTH];


  skip_spaces(&argument);
  one_argument(argument, arg);

  if (!is_number(arg)) {
    zrnum = world[IN_ROOM(ch)].zone;
    zvnum = zone_table[zrnum].number;
  } else {
    zvnum = atoi(arg);
    zrnum = real_zone(zvnum);
  }

  if (zrnum == NOWHERE || zvnum == NOWHERE) {
    send_to_char(ch, "No zone was found with that number.\n\r");
    return;
  }

  last  = zone_table[zrnum].top;
  first = zone_table[zrnum].bot;

  send_to_char(ch, "Zone %d is linked to the following zones:\r\n", zvnum);
  for (nr = 0; nr <= top_of_world && (GET_ROOM_VNUM(nr) <= last); nr++) {
    if (GET_ROOM_VNUM(nr) >= first) {
      for (j = 0; j < DIR_COUNT; j++) {
        if (world[nr].dir_option[j]) {
          to_room = world[nr].dir_option[j]->to_room;
          if (to_room != NOWHERE && (zrnum != world[to_room].zone))
          send_to_char(ch, "%3d %-30s at %5d (%-5s) ---> %5d\r\n",
                       zone_table[world[to_room].zone].number,
                       zone_table[world[to_room].zone].name,
                       GET_ROOM_VNUM(nr), dirs[j], world[to_room].number);
        }
      }
    }
  }
}

/* Zone Checker Code below */
/*mob limits*/
#define MAX_DAMROLL_ALLOWED      MAX(GET_LEVEL(mob), 1)
#define MAX_HITROLL_ALLOWED      MAX(GET_LEVEL(mob), 1)
#define MAX_MOB_GOLD_ALLOWED     GET_LEVEL(mob)*3000000
#define MAX_EXP_ALLOWED          GET_LEVEL(mob)*GET_LEVEL(mob)*GET_LEVEL(mob) * 25
#define MAX_LEVEL_ALLOWED        LVL_IMPL
#define GET_OBJ_AVG_DAM(obj)     (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1))
/* arbitrary limit for per round dam */
#define MAX_MOB_DAM_ALLOWED      500

#define ZCMD2 zone_table[zone].cmd[cmd_no]  /*fom DB.C*/

/*item limits*/
#define MAX_DAM_ALLOWED            2000    /* for weapons  - avg. dam*/
#define MAX_AFFECTS_ALLOWED        6
#define MAX_OBJ_UNITS_ALLOWED       10000000
/* Armor class limits*/
#define TOTAL_WEAR_CHECKS  (NUM_ITEM_WEARS-2)  /*minus Wield and Take*/
struct zcheck_armor {
  bitvector_t bitvector;          /* from Structs.h                       */
  int ac_allowed;                 /* Max. AC allowed for this body part  */
  char *message;                  /* phrase for error message            */
} zarmor[] = {
    {ITEM_WEAR_FINGER,          10, "Ring"},
    {ITEM_WEAR_NECK,            10, "Necklace"},
    {ITEM_WEAR_BODY,            10, "Body armor"},
    {ITEM_WEAR_HEAD,            10, "Head gear"},
    {ITEM_WEAR_LEGS,            10, "Legwear"},
    {ITEM_WEAR_FEET,            10, "Footwear"},
    {ITEM_WEAR_HANDS,           10, "Glove"},
    {ITEM_WEAR_ARMS,            10, "Armwear"},
    {ITEM_WEAR_SHIELD,          10, "Shield"},
    {ITEM_WEAR_ABOUT,           10, "Cloak"},
    {ITEM_WEAR_WAIST,           10, "Belt"},
    {ITEM_WEAR_WRIST,           10, "Wristwear"},
    {ITEM_WEAR_HOLD,            10, "Held item"},
    {ITEM_WEAR_IMPLANT,         10, "Implant"},
    {ITEM_WEAR_FACE,            10, "Face"},
    {ITEM_WEAR_EARS,            10, "Ears"},
    {ITEM_FLOAT_NEARBY,         10, "Floating"},
    {ITEM_WEAR_NIPPLE,          10, "Nipple Ring"},
    {ITEM_WEAR_FINGER,          10, "Extra Finger Ring"},
    {ITEM_WEAR_EARRING,         10, "Earring"},
    {ITEM_WEAR_NOSER,           10, "Nose Ring"},
    {ITEM_WEAR_EXPANSION,       10, "Expansion"},
	{ITEM_WEAR_MEDICAL,			10, "Medical"},
};

/*These are strictly boolean*/
#define CAN_WEAR_WEAPONS         0     /* toggle - can a weapon also be a piece of armor? */
#define MAX_APPLIES_LIMIT        1     /* toggle - is there a limit at all?               */
#define CHECK_ITEM_RENT          0     /* do we check for rent cost == 0 ?                */
#define CHECK_ITEM_COST          0     /* do we check for item cost == 0 ?                */
/* Applies limits !! Very Important:  Keep these in the same order as in Structs.h.
 * To ignore an apply, set max_aff to -99. These will be ignored if MAX_APPLIES_LIMIT = 0 */
struct zcheck_affs {
  int aff_type;    /*from Structs.h*/
  int min_aff;     /*min. allowed value*/
  int max_aff;     /*max. allowed value*/
  char *message;   /*phrase for error message*/
} zaffs[] = {
    {APPLY_NONE,              0,  -99, "unused0"                  },
    {APPLY_STR,              -5,    3, "strength"                 },
    {APPLY_DEX,              -5,    3, "dexterity"                },
    {APPLY_INT,              -5,    3, "intelligence"             },
    {APPLY_PCN,              -5,    3, "perception"               },
    {APPLY_CON,              -5,    3, "constitution"             },
    {APPLY_CHA,              -5,    3, "charisma"                 },
    {APPLY_SKILL,             0,    0, "skill"                    },
    {APPLY_PSI_MASTERY,       0,    0, "psi mastery"              },
    {APPLY_AGE,             -10,   10, "age"                      },
    {APPLY_CHAR_WEIGHT,     -50,   50, "character weight"         },
    {APPLY_CHAR_HEIGHT,     -50,   50, "character height"         },
    {APPLY_PSI,             -50,   50, "psi"                      },
    {APPLY_HIT,             -50,   50, "hit points"               },
    {APPLY_MOVE,            -50,   50, "movement"                 },
    {APPLY_AC,              -10,   10, "psionic AC"               },
    {APPLY_HITROLL,           0,  -99, "hitroll"                  },       /* Handled seperately below */
    {APPLY_DAMROLL,           0,  -99, "damroll"                  },       /* Handled seperately below */
    {APPLY_SAVING_PSIONIC,   -2,    2, "saving throw (psionic)"   },
    {APPLY_PSI_REGEN,      -100,  100, "psi regen"                },
    {APPLY_HIT_REGEN,      -100,  100, "hit regen"                },
    {APPLY_MOVE_REGEN,     -100,  100, "move regen"               },
    {APPLY_HITNDAM,        -100,  100, "hit and dam"              },
    {APPLY_REGEN_ALL,      -100,  100, "regen all"                },
    {APPLY_HPV,            -100,  100, "hpv"                      },
    {APPLY_PSI2HIT,        -100,  100, "psi to hit"               },
    {APPLY_ALL_STATS,      -100,  100, "all stats"                }
};

/* These are ABS() values. */
#define MAX_APPLY_HITROLL_TOTAL   5
#define MAX_APPLY_DAMROLL_TOTAL   5

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
const int offlimit_zones[] = {0,12,13,14,-1};  /*what zones can no room connect to (virtual num) */
#define MIN_ROOM_DESC_LENGTH   80       /* at least one line - set to 0 to not care. */
#define MAX_COLOUMN_WIDTH      80       /* at most 80 chars per line */

ACMD (do_zcheck)
{
  zone_rnum zrnum;
  room_vnum exroom=0;
  int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  char buf[MAX_STRING_LENGTH];
  size_t len = 0;
  struct extra_descr_data *ext, *ext2;
  one_argument(argument, buf);

  if (!is_number(buf) || !strcmp(buf, "."))
    zrnum = world[IN_ROOM(ch)].zone;
  else
    zrnum = real_zone(atoi(buf));

  if (zrnum == NOWHERE) {
    send_to_char(ch, "Check what zone ?\r\n");
    return;
  } else
    send_to_char(ch, "Checking zone %d!\r\n", zone_table[zrnum].number);


  /* Check rooms */
  send_to_char(ch, "\r\nChecking Rooms for limits...\r\n");
  for (i=0; i<top_of_world;i++) {
    if (world[i].zone==zrnum) {
      for (j = 0; j < DIR_COUNT; j++) {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!world[i].dir_option[j])
          continue;
        exroom=world[i].dir_option[j]->to_room;
        if (exroom==NOWHERE)
          continue;
        if (world[exroom].zone == zrnum)
          continue;
        if (world[exroom].zone == world[i].zone)
          continue;

        for (k=0;offlimit_zones[k] != -1;k++) {
          if (world[exroom].zone == real_zone(offlimit_zones[k]) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Exit %s cannot connect to %d (zone off limits).\r\n",
                            dirs[j], world[exroom].number);
        } /* for (k.. */
      } /* cycle directions */

     if (ROOM_FLAGGED(i, ROOM_ATRIUM) || ROOM_FLAGGED(i, ROOM_HOUSE) || ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) || ROOM_FLAGGED(i, ROOM_OLC) || ROOM_FLAGGED(i, ROOM_BFS_MARK))
         len += snprintf(buf + len, sizeof(buf) - len,
         "- Has illegal affection bits set (%s %s %s %s %s)\r\n",
                            ROOM_FLAGGED(i, ROOM_ATRIUM) ? "ATRIUM" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE) ? "HOUSE" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                            ROOM_FLAGGED(i, ROOM_OLC) ? "OLC" : "",
                            ROOM_FLAGGED(i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(world[i].description)<MIN_ROOM_DESC_LENGTH && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
          "- Room description is too short. (%4.4d of min. %d characters).\r\n",
          (int)strlen(world[i].description), MIN_ROOM_DESC_LENGTH);

      if (strncmp(world[i].description, "   ", 3) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not formatted with indent (/fi in the editor).\r\n");

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(world[i].description, "\r\n")>MAX_COLOUMN_WIDTH) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                             MAX_COLOUMN_WIDTH);

     for (ext2 = NULL, ext = world[i].ex_description; ext; ext = ext->next)
       if (strncmp(ext->description, "   ", 3))
         ext2 = ext;

      if (found) {
        send_to_char(ch, "[%5d] %-30s: \r\n",
                       world[i].number, world[i].name ? world[i].name : "An unnamed room");
        send_to_char(ch, "%s", buf);
        strcpy(buf, "");
        len = 0;
        found = 0;
      }
    } /*is room in this zone?*/
  } /*checking rooms*/

  send_to_char(ch, "\r\nChecking exit consistency and logic..\r\n");
  for (i=0; i<top_of_world;i++) 
  {
    if (world[i].zone==zrnum) 
    {
      m++;
      for (j = 0, k = 0; j < DIR_COUNT; j++)
      {
        if (!world[i].dir_option[j])
          k++;
        
        else
        {
            if (world[i].dir_option[j]->to_room != NOWHERE)
            {
                if (!world[world[i].dir_option[j]->to_room].dir_option[rev_dir[j]])
                    send_to_char(ch, "-[Room %d]- Exit %s has no reverse direction.\r\n",world[i].number, dirs[j]);
                else if ( world[world[world[i].dir_option[j]->to_room].dir_option[rev_dir[j]]->to_room].number != world[i].number )
                    send_to_char(ch, "-[Room %d]- Exit %s does not return to this room.\r\n",world[i].number, dirs[j]);
            }
        }
      }

      if (k == DIR_COUNT)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(ch, "More than 1/3 of the rooms are not linked.\r\n");

}
static void mob_checkload(struct char_data *ch, mob_vnum mvnum)
 {
   int cmd_no;
   zone_rnum zone;
   mob_rnum mrnum = real_mobile(mvnum);
 
  if (mrnum == NOBODY) {
       send_to_char(ch, "That mob does not exist.\r\n");
       return;
   }
 
  send_to_char(ch, "Checking load info for the mob %s...\r\n",
                     mob_proto[mrnum].player.short_descr);
 
  for (zone=0; zone <= top_of_zone_table; zone++) {
     for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
       if (ZCMD2.command != 'M')
         continue;
 
      /* read a mobile */
       if (ZCMD2.arg1 == mrnum) {
         send_to_char(ch, "  [%5d] %s (%d MAX)\r\n",
                          world[ZCMD2.arg3].number,
                          world[ZCMD2.arg3].name,
                          ZCMD2.arg2);
       }
     }
   }
 }
 
static void obj_checkload(struct char_data *ch, obj_vnum ovnum)
 {
   int cmd_no;
   zone_rnum zone;
   obj_rnum ornum = real_object(ovnum);
   room_vnum lastroom_v = 0;
   room_rnum lastroom_r = 0;
   mob_rnum lastmob_r = 0;
 
  if (ornum ==NOTHING) {
     send_to_char(ch, "That object does not exist.\r\n");
     return;
   }
 
  send_to_char(ch, "Checking load info for the obj %s...\r\n",
                    obj_proto[ornum].short_description);
 
  for (zone=0; zone <= top_of_zone_table; zone++) {
     for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
       switch (ZCMD2.command) {
         case 'M':
           lastroom_v = world[ZCMD2.arg3].number;
           lastroom_r = ZCMD2.arg3;
           lastmob_r = ZCMD2.arg1;
           break;
         case 'O':                   /* read an object */
           lastroom_v = world[ZCMD2.arg3].number;
           lastroom_r = ZCMD2.arg3;
           if (ZCMD2.arg1 == ornum) {
		     if (ZCMD2.arg4)
				send_to_char(ch, "  [%5d] %s (%d Max) (%d %%load)\r\n",
                              lastroom_v,
                              world[lastroom_r].name,
                              ZCMD2.arg2,
							  ZCMD2.arg4);
			 else
			    send_to_char(ch, "  [%5d] %s (%d Max)\r\n", lastroom_v, world[lastroom_r].name, ZCMD2.arg2);
           }
		   break;
         case 'P':                   /* object to object */
           if (ZCMD2.arg1 == ornum) {
             if (ZCMD2.arg4)
				send_to_char(ch, "  [%5d] %s (Put in another object [%d Max]) (%d %%load)\r\n",
                              lastroom_v,
                              world[lastroom_r].name,
                              ZCMD2.arg2,
							  ZCMD2.arg4);
			 else
		        send_to_char(ch, "  [%5d] %s (Put in another object [%d Max])\r\n",
                              lastroom_v,
                              world[lastroom_r].name,
                              ZCMD2.arg2);
           }
		   break;
         case 'G':                   /* obj_to_char */
           if (ZCMD2.arg1 == ornum) {
		     if (ZCMD2.arg4)
				send_to_char(ch, "  [%5d] %s (Given to %s [%d][%d Max]) (%d %%load)\r\n",
                              lastroom_v,
							  world[lastroom_r].name,
                              mob_proto[lastmob_r].player.short_descr,
                              mob_index[lastmob_r].vnum,
                              ZCMD2.arg2,
							  ZCMD2.arg4);
			 else
			    send_to_char(ch, "  [%5d] %s (Given to %s [%d][%d Max])\r\n",
				    		lastroom_v,
							world[lastroom_r].name,
							mob_proto[lastmob_r].player.short_descr,
							mob_index[lastmob_r].vnum,
							ZCMD2.arg2);
           }
		   break;
         case 'E':                   /* object to equipment list */
           if (ZCMD2.arg1 == ornum) {
		     if (ZCMD2.arg4)
				send_to_char(ch, "  [%5d] %s (Equipped to %s [%d][%d Max]) (%d %%load)\r\n",
                              lastroom_v,
                              world[lastroom_r].name,
                              mob_proto[lastmob_r].player.short_descr,
                              mob_index[lastmob_r].vnum,
                              ZCMD2.arg2,
							  ZCMD2.arg4);
			 else 
				send_to_char(ch, " [%5d] %s (Equipped to %s [%d][%d Max])\r\n",
							  lastroom_v,
							  world[lastroom_r].name,
							  mob_proto[lastmob_r].player.short_descr,
							  mob_index[lastmob_r].vnum,
							  ZCMD2.arg2);
             }
			 break;
           case 'R': /* rem obj from room */
             lastroom_v = world[ZCMD2.arg1].number;
             lastroom_r = ZCMD2.arg1;
             if (ZCMD2.arg2 == ornum)
               send_to_char(ch, "  [%5d] %s (Removed from room)\r\n",
                                lastroom_v,
                                world[lastroom_r].name);
             break;
       }/* switch */
     } /*for cmd_no......*/
   }  /*for zone...*/
 }
 
static void trg_checkload(struct char_data *ch, trig_vnum tvnum)
 {
   int cmd_no, found = 0;
   zone_rnum zone;
   trig_rnum trnum = real_trigger(tvnum);
   room_vnum lastroom_v = 0;
   room_rnum lastroom_r = 0, k;
   mob_rnum lastmob_r = 0, i;
   obj_rnum lastobj_r = 0, j;
   struct trig_proto_list *tpl;
 
  if (trnum == NOTHING) {
     send_to_char(ch, "That trigger does not exist.\r\n");
     return;
   }
 
  send_to_char(ch, "Checking load info for the %s trigger '%s':\r\n",
                     trig_index[trnum]->proto->attach_type == MOB_TRIGGER ? "mobile" :
                     (trig_index[trnum]->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),
                     trig_index[trnum]->proto->name);
 
  for (zone=0; zone <= top_of_zone_table; zone++) {
     for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
       switch (ZCMD2.command) {
         case 'M':
           lastroom_v = world[ZCMD2.arg3].number;
           lastroom_r = ZCMD2.arg3;
           lastmob_r = ZCMD2.arg1;
           break;
         case 'O':                   /* read an object */
           lastroom_v = world[ZCMD2.arg3].number;
           lastroom_r = ZCMD2.arg3;
           lastobj_r = ZCMD2.arg1;
           break;
         case 'P':                   /* object to object */
           lastobj_r = ZCMD2.arg1;
           break;
         case 'G':                   /* obj_to_char */
           lastobj_r = ZCMD2.arg1;
           break;
         case 'E':                   /* object to equipment list */
           lastobj_r = ZCMD2.arg1;
           break;
         case 'R':                   /* rem obj from room */
           lastroom_v = 0;
           lastroom_r = 0;
           lastobj_r = 0;
           lastmob_r = 0;
         case 'T':                   /* trigger to something */
           if (ZCMD2.arg2 != trnum)
             break;
           if (ZCMD2.arg1 == MOB_TRIGGER) {
             send_to_char(ch, "mob [%5d] %-60s (zedit room %5d)\r\n",
                                mob_index[lastmob_r].vnum,
                                mob_proto[lastmob_r].player.short_descr,
                                lastroom_v);
             found = 1;
           } else if (ZCMD2.arg1 == OBJ_TRIGGER) {
             send_to_char(ch, "obj [%5d] %-60s  (zedit room %d)\r\n",
                                obj_index[lastobj_r].vnum,
                                obj_proto[lastobj_r].short_description,
                                lastroom_v);
             found = 1;
           } else if (ZCMD2.arg1==WLD_TRIGGER) {
             send_to_char(ch, "room [%5d] %-60s (zedit)\r\n",
                                lastroom_v,
                                world[lastroom_r].name);
             found = 1;
           }
         break;
       } /* switch */
     } /*for cmd_no......*/
   }  /*for zone...*/
 
  for (i = 0; i < top_of_mobt; i++) {
     if (!mob_proto[i].proto_script)
       continue;
 
    for (tpl = mob_proto[i].proto_script;tpl;tpl = tpl->next)
       if (tpl->vnum == tvnum) {
         send_to_char(ch, "mob [%5d] %s\r\n",
                          mob_index[i].vnum,
                          mob_proto[i].player.short_descr);
         found = 1;
       }
   }
 
  for (j = 0; j < top_of_objt; j++) {
     if (!obj_proto[j].proto_script)
       continue;
 
    for (tpl = obj_proto[j].proto_script;tpl;tpl = tpl->next)
       if (tpl->vnum == tvnum) {
         send_to_char(ch, "obj [%5d] %s\r\n",
                          obj_index[j].vnum,
                          obj_proto[j].short_description);
         found = 1;
       }
   }
 
  for (k = 0;k < top_of_world; k++) {
     if (!world[k].proto_script)
       continue;
 
    for (tpl = world[k].proto_script;tpl;tpl = tpl->next)
       if (tpl->vnum == tvnum) {
         send_to_char(ch, "room[%5d] %s\r\n",
                          world[k].number,
                          world[k].name);
         found = 1;
       }
   }
 
  if (!found)
     send_to_char(ch, "This trigger is not attached to anything.\r\n");
 }
 
ACMD(do_checkloadstatus)
 {
   char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
 
  two_arguments(argument, buf1, buf2);
 
  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2))) {
     send_to_char(ch, "Checkload <M | O | T> <vnum>\r\n");
     return;
   }
 
  if (LOWER(*buf1) == 'm') {
     mob_checkload(ch, atoi(buf2));
     return;
   }
 
  if (LOWER(*buf1) == 'o') {
     obj_checkload(ch, atoi(buf2));
     return;
   }
 
  if (LOWER(*buf1) == 't') {
     trg_checkload(ch, atoi(buf2));
     return;
   }
 }
 /* Zone Checker code above. */
 
/* Zone Checker code above. */
// do_findkey, finds where the key to a door loads
ACMD(do_findkey)
{
    int dir;
    int key;
    int rkey;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Format: findkey <dir>\r\n");
        return;
    }

    switch (*arg) {
        case 'n':
            dir = NORTH;
            break;

        case 'e':
            dir = EAST;
            break;

        case 's':
            dir = SOUTH;
            break;

        case 'w':
            dir = WEST;
            break;

        case 'u':
            dir = UP;
            break;

        case 'd':
            dir = DOWN;
            break;

        default:
            send_to_char(ch, "What direction is that?!?\r\n");
            return;
    }

    if (!EXIT(ch, dir)) {
        send_to_char(ch, "There's no exit in that direction!\r\n");
        return;
    }

    if ((key = EXIT(ch, dir)->key) <= 0) {
        send_to_char(ch, "There's no key for that exit.\r\n");
        return;
    }

    send_to_char(ch, "Vnum: %d (%s).\r\n", key, (rkey = real_object(key)) > -1 ? obj_proto[rkey].short_description : "doesn't exist");

    // todo: test this.  do we want a virtual number or a real number?
    obj_checkload(ch, key);
}

// do_finddoor, finds the door(s) that a key goes to
ACMD(do_finddoor)
{
    int d;
    int vnum = 0;
    int num = 0;
    room_rnum i;
    char arg[MAX_INPUT_LENGTH];
    struct char_data *tmp_char;
    struct obj_data *obj;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "Format: finddoor <obj/vnum>\r\n");
        return;
    }

    if (is_number(arg))    // must be a vnum - easy
        vnum = atoi(arg);
    else {

        generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP,
                 ch, &tmp_char, &obj);

        if (!obj) {
            send_to_char(ch, "What key do you want to find a door for?\r\n");
            return;
        }

        if (GET_OBJ_TYPE(obj) != ITEM_KEY) {
            act("$p isn't a key, it seems.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        }

        vnum = GET_OBJ_VNUM(obj);
    }

    if (real_object(vnum) == NOTHING) {
        send_to_char(ch, "There is no object with that vnum found.\r\n");
        return;
    }

    for (i = 0; i <= top_of_world; i++)
        for (d = 0; d < NUM_OF_DIRS; d++)
            if (world[i].dir_option[d] && world[i].dir_option[d]->key && world[i].dir_option[d]->key == vnum)
                send_to_char(ch, "[%3d] Room %d, %s (%s)\r\n", ++num, world[i].number,
                    dirs[d], world[i].dir_option[d]->keyword);
}
/* (c) 1996-97 Erwin S. Andreasen. */
ACMD(do_copyover)
{
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf [100], buf2[100];
  int i;

  fp = fopen (COPYOVER_FILE, "w");
    if (!fp) {
      send_to_char (ch, "Copyover file not writeable, aborted.\n\r");
      return;
    }
   save_all();
   House_save_all();
   Vehicle_save();
   sprintf (buf, "\n\r *** COPYOVER by %s - please remain seated!\n\r", GET_NAME(ch));

   /* write boot_time as first line in file */
   fprintf(fp, "%ld\n", (long)boot_time);

   /* For each playing descriptor, save its state */
   for (d = descriptor_list; d ; d = d_next) {
     struct char_data * och = d->character;
   /* We delete from the list , so need to save this */
     d_next = d->next;

  /* drop those logging on */
   if (!d->character || d->connected > CON_PLAYING) {
     write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r");
     close_socket (d); /* throw'em out */
   } else {
      fprintf (fp, "%d %ld %s %s %s\n", d->descriptor, GET_PREF(och), GET_NAME(och), d->host, CopyoverGet(d));
      /* save och */
      GET_LOADROOM(och) = GET_ROOM_VNUM(IN_ROOM(och));
      Crash_rentsave(och,0);
      save_char(och);
      write_to_descriptor (d->descriptor, buf);
    }
  }

  fprintf (fp, "-1\n");
  fclose (fp);

  /* exec - descriptors are inherited */
  sprintf (buf, "%d", port);
  sprintf (buf2, "-C%d", mother_desc);

  /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
  i = chdir ("..");

  /* Close reserve and other always-open files and release other resources */
   execl (EXE_FILE, "circle", buf2, buf, (char *) NULL);

   /* Failed - successful exec will not return */
   perror ("do_copyover: execl");
   send_to_char (ch, "Copyover FAILED!\n\r");

 exit (1); /* too much trouble to try to recover! */
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;

  act ("As $n makes a strange arcane gesture, a golden light descends\r\n"
       "from the heavens stopping all the fighting.\r\n",FALSE, ch, 0, 0, TO_ROOM);
  send_to_room(IN_ROOM(ch), "Everything is quite peaceful now.\r\n");
    for (vict = world[IN_ROOM(ch)].people; vict; vict=next_v) {
        next_v = vict->next_in_room;
		if (RANGED_FIGHTING(vict)) {
			next_v = RANGED_FIGHTING(vict);
			if (next_v != NULL)
				stop_ranged_fighting(next_v);
			stop_ranged_fighting(vict);
		}
        if (FIGHTING(vict))
            stop_fighting(vict);
        if (IS_NPC(vict))
            clearMemory(vict);
    }
}

ACMD(do_zpurge)
{
  int vroom, room, vzone = 0, zone = 0;
  char arg[MAX_INPUT_LENGTH];
  int purge_all = FALSE;
  one_argument(argument, arg);
  if (*arg == '.' || !*arg) {
    zone = world[IN_ROOM(ch)].zone;
    vzone = zone_table[zone].number;
  }
  else if (is_number(arg)) {
    vzone = atoi(arg);
    zone = real_zone(vzone);
    if (zone == NOWHERE || zone > top_of_zone_table) {
      send_to_char(ch, "That zone doesn't exist!\r\n");
      return;
    }
  }
  else if (*arg == '*') {
    purge_all = TRUE;
  }
  else {
    send_to_char(ch, "That isn't a valid zone number!\r\n");
    return;
  }
  if (GET_LEVEL(ch) < LVL_GOD && !can_edit_zone(ch, zone)) {
    send_to_char(ch, "You can only purge your own zone!\r\n");
    return;
  }
  if (!purge_all) {
    for (vroom = zone_table[zone].bot; vroom <= zone_table[zone].top; vroom++) {
      purge_room(real_room(vroom));
    }
    send_to_char(ch, "Purged zone #%d: %s.\r\n", zone_table[zone].number, zone_table[zone].name);
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged zone %d (%s)", GET_NAME(ch), zone_table[zone].number, zone_table[zone].name);
  }
  else {
    for (room = 0; room <= top_of_world; room++) {
      purge_room(room);
    }
    send_to_char(ch, "Purged world.\r\n");
    mudlog(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s purged entire world.", GET_NAME(ch));
  }
}

/** Used to read and gather a bit of information about external log files while
 * in game.
 * Makes use of the '@' color codes in the file status information.
 * Some of the methods used are a bit wasteful (reading through the file
 * multiple times to gather diagnostic information), but it is
 * assumed that the files read with this function will never be very large.
 * Files to be read are assumed to exist and be readable and if they aren't,
 * log the name of the missing file.
 */
ACMD(do_file)
{
  /* Local variables */
  int def_lines_to_read = 15;  /* Set the default num lines to be read. */
  int max_lines_to_read = 300; /* Maximum number of lines to read. */
  FILE *req_file;              /* Pointer to file to be read. */
  size_t req_file_size = 0;    /* Size of file to be read. */
  int req_file_lines = 0;      /* Number of total lines in file to be read. */
  int lines_read = 0; /* Counts total number of lines read from the file. */
  int req_lines = 0;  /* Number of lines requested to be displayed. */
  int i, j;           /* Generic loop counters. */
  int l;              /* Marks choice of file in fields array. */
  char field[MAX_INPUT_LENGTH];  /* Holds users choice of file to be read. */
  char value[MAX_INPUT_LENGTH];  /* Holds # lines to be read, if requested. */
  char buf[MAX_STRING_LENGTH];   /* Display buffer for req_file. */

  /* Defines which files are available to read. */
  struct file_struct {
    char *cmd;          /* The 'name' of the file to view */
    char level;         /* Minimum level needed to view. */
    char *file;         /* The file location, relative to the working dir. */
    int read_backwards; /* Should the file be read backwards by default? */
  } fields[] = {
        //{ "bugs",           LVL_GOD,    BUG_FILE,               TRUE},
        //{ "typos",          LVL_GOD,    TYPO_FILE,              TRUE},
        //{ "ideas",          LVL_GOD,    IDEA_FILE,              TRUE},
        { "xnames",         LVL_GOD,    XNAME_FILE,             TRUE},
        { "levels",         LVL_GOD,    LEVELS_LOGFILE,         TRUE},
        { "rip",            LVL_GOD,    RIP_LOGFILE,            TRUE},
        { "new",            LVL_GOD,    NEWPLAYERS_LOGFILE,     TRUE},
        { "rentgone",       LVL_GOD,    RENTGONE_LOGFILE,       TRUE},
        { "errors",         LVL_GOD,    ERRORS_LOGFILE,         TRUE},
        { "godcmds",        LVL_GOD,    GODCMDS_LOGFILE,        TRUE},
        { "syslog",         LVL_GOD,    SYSLOG_LOGFILE,         TRUE},
        { "syslog1",        LVL_GOD,    LOG_SYSLOG1,            TRUE},
        { "syslog2",        LVL_GOD,    LOG_SYSLOG2,            TRUE},
        { "syslog3",        LVL_GOD,    LOG_SYSLOG3,            TRUE},
        { "syslog4",        LVL_GOD,    LOG_SYSLOG4,            TRUE},
        { "syslog5",        LVL_GOD,    LOG_SYSLOG5,            TRUE},
        { "syslog6",        LVL_GOD,    LOG_SYSLOG6,            TRUE},
        { "syslog7",        LVL_GOD,    LOG_SYSLOG7,            TRUE},
        { "syslog8",        LVL_GOD,    LOG_SYSLOG8,            TRUE},
        { "syslog9",        LVL_GOD,    LOG_SYSLOG9,            TRUE},
        { "syslog10",       LVL_GOD,    LOG_SYSLOG10,           TRUE},
        { "crash",          LVL_GOD,    CRASH_LOGFILE,          TRUE},
        { "help",           LVL_GOD,    HELP_LOGFILE,           TRUE},
        { "changelog",      LVL_GOD,    CHANGE_LOG_FILE,        TRUE},
        { "deletes",        LVL_GOD,    DELETES_LOGFILE,        TRUE},
        { "restarts",       LVL_GOD,    RESTARTS_LOGFILE,       TRUE},
        { "usage",          LVL_GOD,    USAGE_LOGFILE,          TRUE},
        { "badpws",         LVL_GOD,    BADPWS_LOGFILE,         TRUE},
        { "olc",            LVL_GOD,    OLC_LOGFILE,            TRUE},
        { "trigger",        LVL_GOD,    TRIGGER_LOGFILE,        TRUE},
        { "dts",            LVL_GOD,    LOG_DTS,                TRUE},
        { "saber",          LVL_GOD,    LOG_SABER,              TRUE},
        { "qp",             LVL_GOD,    LOG_QP,                 TRUE},
        { "warning",        LVL_GOD,    LOG_WARNINGS,           TRUE},
        { "last",           LVL_GOD,    LOG_LAST,               TRUE},
        { "\n",             0,          "\n",                   FALSE } /* This must be the last entry */
  };

   /* Initialize buffer */
   buf[0] = '\0';

   /**/
   /* End function variable set-up and initialization. */

   skip_spaces(&argument);

   /* Display usage if no argument. */
   if (!*argument) {
     send_to_char(ch, "USAGE: file <filename> <num lines>\r\n\r\nFile options:\r\n");
     for (j = 0, i = 0; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         send_to_char(ch, "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     return;
   }

   /* Begin validity checks. Is the file choice valid and accessible? */
   /**/
   /* There are some arguments, deal with them. */
   two_arguments(argument, field, value);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
   {
     if (!strncmp(field, fields[l].cmd, strlen(field)))
       break;
   }

   if(*(fields[l].cmd) == '\n') {
     send_to_char(ch, "'%s' is not a valid file.\r\n", field);
     return;
   }

   if (GET_LEVEL(ch) < fields[l].level) {
     send_to_char(ch, "You have not achieved a high enough level to view '%s'.\r\n",
         fields[l].cmd);
     return;
   }

   /* Number of lines to view. Default is 15. */
   if(!*value)
     req_lines = def_lines_to_read;
   else if (!isdigit(*value))
   {
     /* This check forces the requisite positive digit and prevents negative
      * numbers of lines from being read. */
     send_to_char(ch, "'%s' is not a valid number of lines to view.\r\n", value);
     return;
   }
   else
   {
     req_lines = atoi(value);
     /* Limit the maximum number of lines */
     req_lines = MIN( req_lines, max_lines_to_read );
   }

   /* Must be able to access the file on disk. */
   if (!(req_file=fopen(fields[l].file,"r"))) {
     send_to_char(ch, "The file %s can not be opened.\r\n", fields[l].file);
     mudlog(BRF, LVL_IMPL, TRUE,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }
   /**/
   /* End validity checks. From here on, the file should be viewable. */

   /* Diagnostic information about the file */
   req_file_size = file_sizeof(req_file);
   req_file_lines = file_numlines(req_file);

   snprintf( buf, sizeof(buf),
       "@gFile:@n %s@g; Min. Level to read:@n %d@g; File Location:@n %s@g\r\n"
       "File size (bytes):@n %ld@g; Total num lines:@n %d\r\n",
       fields[l].cmd, fields[l].level, fields[l].file, (long) req_file_size,
       req_file_lines);

   /* Should the file be 'headed' or 'tailed'? */
   if ( (fields[l].read_backwards == TRUE) && (req_lines < req_file_lines) )
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
               "@gReading from the tail of the file.@n\r\n\r\n" );
     lines_read = file_tail( req_file, buf, sizeof(buf), req_lines );
   }
   else
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
              "@gReading from the head of the file.@n\r\n\r\n" );
     lines_read = file_head( req_file, buf, sizeof(buf), req_lines );
   }

   /** Since file_head and file_tail will add the overflow message, we
    * don't check for status here. */
   if ( lines_read == req_file_lines )
   {
     /* We're reading the entire file */
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n@gEntire file returned (@n%d @glines).@n\r\n",
         lines_read );
   }
   else if ( lines_read == max_lines_to_read )
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n@gMaximum number of @n%d @glines returned.@n\r\n",
         lines_read );
   }
   else
   {
     snprintf( buf + strlen(buf), sizeof(buf) - strlen(buf),
         "\r\n%d @glines returned.@n\r\n",
         lines_read );
   }

   /* Clean up before return */
   fclose(req_file);

   page_string(ch->desc, buf, 1);
}

ACMD(do_changelog)
{
  time_t rawtime;
  char tmstr[MAX_INPUT_LENGTH], line[READ_SIZE], last_buf[READ_SIZE],
      buf[READ_SIZE];
  FILE *fl;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: changelog <change>\r\n");
    return;
  }

	fl = fopen(CHANGE_LOG_FILE, "a"); 


  rawtime = time(0);
  strftime(tmstr, sizeof(tmstr), "%b %d %Y", localtime(&rawtime));

  sprintf(buf, "[%s] - %s", tmstr, GET_NAME(ch));

  fprintf(fl, "%s\n", buf);
  fprintf(fl, "  %s\n", argument);

  fclose(fl);
  send_to_char(ch, "Change added.\r\n");
}

#define PLIST_FORMAT \
  "Usage: plist [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-i] [-m]"

ACMD(do_plist)
{
  int i, len = 0, count = 0;
  char mode, buf[MAX_STRING_LENGTH * 20], name_search[MAX_NAME_LENGTH], time_str[MAX_STRING_LENGTH];
  struct time_info_data time_away;
  int low = 0, high = LVL_IMPL, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24;

  skip_spaces(&argument);
  strcpy(buf, argument);        /* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      if (sscanf(arg, "%d-%d", &low, &high) == 1)
        high = low;
      strcpy(buf, buf1);        /* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);        /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'l':
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        half_chop(buf1, name_search, buf);
        break;
      case 'i':
        strcpy(buf, buf1);
        low = LVL_IMMORT;
        break;
      case 'm':
        strcpy(buf, buf1);
        high = LVL_IMMORT - 1;
        break;
      case 'd':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1)
          high_day = low_day;
        break;
      case 'h':
        half_chop(buf1, arg, buf);
        if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1)
          high_hr = low_hr;
        break;
      default:
        send_to_char(ch, "%s\r\n", PLIST_FORMAT);
        return;
      }
    } else {
      send_to_char(ch, "%s\r\n", PLIST_FORMAT);
      return;
    }
  }

  len = 0;
  len += snprintf(buf + len, sizeof(buf) - len, "@W[ Id] (Lv) Name         Last@n\r\n"
                  "%s-------------------------------------%s\r\n", CCCYN(ch, C_NRM),
                  CCNRM(ch, C_NRM));

  for (i = 0; i <= top_of_p_table; i++) {
    if (player_table[i].level < low || player_table[i].level > high)
      continue;

    time_away = *real_time_passed(time(0), player_table[i].last);

    if (*name_search && str_cmp(name_search, player_table[i].name))
      continue;

    if (time_away.day > high_day || time_away.day < low_day)
      continue;
    if (time_away.hours > high_hr || time_away.hours < low_hr)
      continue;

    strcpy(time_str, asctime(localtime(&player_table[i].last)));
    time_str[strlen(time_str) - 1] = '\0';

    len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] (%2d) %c%-15s %s\r\n",
                    player_table[i].id, player_table[i].level,
                    UPPER(*player_table[i].name), player_table[i].name + 1, time_str);
    count++;
  }
  snprintf(buf + len, sizeof(buf) - len, "%s-------------------------------------%s\r\n"
           "%d players listed.\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), count);
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_wizupdate)
{
  run_autowiz();
  send_to_char(ch, "Wizlists updated.\n\r");
}

/* NOTE: This is called from perform_set */
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name)
{
  struct char_data *temp_ch=NULL;
  int plr_i = 0, i, j, k;
  char old_name[MAX_NAME_LENGTH], old_pfile[50], new_pfile[50], buf[MAX_STRING_LENGTH];

  if (!ch)
  {
    log("SYSERR: No char passed to change_player_name.");
    return FALSE;
  }

  if (!vict)
  {
    log("SYSERR: No victim passed to change_player_name.");
    send_to_char(ch, "Invalid victim.\r\n");
    return FALSE;
  }

  if (!new_name || !(*new_name) || strlen(new_name) < 2 ||
      strlen(new_name) > MAX_NAME_LENGTH || !valid_name(new_name) ||
      fill_word(new_name) || reserved_word(new_name) ) {
    send_to_char(ch, "Invalid new name.\r\n");
    return FALSE;
  }

  // Check that someone with new_name isn't already logged in
  if ((temp_ch = get_player_vis(ch, new_name, NULL, FIND_CHAR_WORLD)) != NULL) {
    send_to_char(ch, "Sorry, the new name already exists.\r\n");
    return FALSE;
  } else  {
    /* try to load the player off disk */
    CREATE(temp_ch, struct char_data, 1);
    clear_char(temp_ch);
    CREATE(temp_ch->player_specials, struct player_special_data, 1);
    /* Allocate mobile event list */
    temp_ch->events = create_list();
    if ((plr_i = load_char(new_name, temp_ch)) > -1) {
      free_char(temp_ch);
      send_to_char(ch, "Sorry, the new name already exists.\r\n");
      return FALSE;
    }
  }

  /* New playername is OK - find the entry in the index */
  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == GET_IDNUM(vict))
      break;

  if (player_table[i].id != GET_IDNUM(vict))
  {
    send_to_char(ch, "Your target was not found in the player index.\r\n");
    log("SYSERR: Player %s, with ID %ld, could not be found in the player index.", GET_NAME(vict), GET_IDNUM(vict));
    return FALSE;
  }

  /* Set up a few variables that will be needed */
  sprintf(old_name, "%s", GET_NAME(vict));
  if (!get_filename(old_pfile, sizeof(old_pfile), PLR_FILE, old_name))
  {
    send_to_char(ch, "Unable to ascertain player's old pfile name.\r\n");
    return FALSE;
  }
  if (!get_filename(new_pfile, sizeof(new_pfile), PLR_FILE, new_name))
  {
    send_to_char(ch, "Unable to ascertain player's new pfile name.\r\n");
    return FALSE;
  }

  /* Now start changing the name over - all checks and setup have passed */
  free(player_table[i].name);              // Free the old name in the index
  player_table[i].name = strdup(new_name); // Insert the new name into the index
  for (k=0; (*(player_table[i].name+k) = LOWER(*(player_table[i].name+k))); k++);

  free(GET_PC_NAME(vict));
  GET_PC_NAME(vict) = strdup(CAP(new_name));    // Change the name in the victims char struct

  /* Rename the player's pfile */
  sprintf(buf, "mv %s %s", old_pfile, new_pfile);
  j = system(buf);

  /* Save the changed player index - the pfile is saved by perform_set */
  save_player_index();

  mudlog(BRF, LVL_IMMORT, TRUE, "(GC) %s changed the name of %s to %s", GET_NAME(ch), old_name, new_name);

  if (vict->desc)  /* Descriptor is set if the victim is logged in */
    send_to_char(vict, "Your login name has changed from %s%s%s to %s%s%s.\r\n", CCYEL(vict, C_NRM), old_name, CCNRM(vict, C_NRM),
                                                                                 CCYEL(vict, C_NRM), new_name, CCNRM(vict, C_NRM));

  return TRUE;
}

ACMD(do_zlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char      arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int       counter = 0;
  bool      fail = FALSE;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Locks a zone so that building or editing is not possible.\r\n");
    send_to_char(ch, "The 'list' shows all currently locked zones.\r\n");
    send_to_char(ch, "'zlock all' will lock every zone with the GRID flag set.\r\n");
    send_to_char(ch, "'zlock all all' will lock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all")) {
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "You do not have sufficient access to lock all zones.\r\n");
      return;
    }
    if (!*arg2) {
      for (zn = 0; zn <= top_of_zone_table; zn++) {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD) && ZONE_FLAGGED(zn, ZONE_GRID)) {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn)) {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          } else {
            fail = TRUE;
          }
        }
      }
    } else if (is_abbrev(arg2, "all")) {
      for (zn = 0; zn <= top_of_zone_table; zn++) {
        if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
          counter++;
          SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
          if (save_zone(zn)) {
            log("(GC) %s has locked zone %d", GET_NAME(ch), zone_table[zn].number);
          } else {
            fail = TRUE;
          }
        }
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are no unlocked zones to lock!\r\n");
      return;
    }
    if (fail) {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been locked.\r\n", counter);
    mudlog(BRF, LVL_GOD, TRUE, "(GC) %s has locked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list")) {
    /* Show all locked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        if (!counter) send_to_char(ch, "Locked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
          QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name)+30, zone_table[zn].name,
          QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are currently no locked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0) {
    send_to_char(ch, "Usage: %szlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE) {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRGOD && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum) {
    send_to_char(ch, "You do not have sufficient access to lock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zlock <num>' */
  if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
    send_to_char(ch, "Zone %d is already locked!\r\n", znvnum);
    return;
  }
  SET_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn)) {
    mudlog(NRM, LVL_GRGOD, TRUE, "(GC) %s has locked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

ACMD(do_zunlock)
{
  zone_vnum znvnum;
  zone_rnum zn;
  char      arg[MAX_INPUT_LENGTH];
  int       counter = 0;
  bool      fail = FALSE;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "%s       zunlock list%s\r\n\r\n", QYEL, QNRM);
    send_to_char(ch, "Unlocks a 'locked' zone to allow building or editing.\r\n");
    send_to_char(ch, "The 'list' shows all currently unlocked zones.\r\n");
    send_to_char(ch, "'zunlock all' will unlock every zone in the MUD.\r\n");
    return;
  }
  if (is_abbrev(arg, "all")) {
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char(ch, "You do not have sufficient access to lock zones.\r\n");
      return;
    }
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        counter++;
        REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
        if (save_zone(zn)) {
          log("(GC) %s has unlocked zone %d", GET_NAME(ch), zone_table[zn].number);
        } else {
          fail = TRUE;
        }
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are no locked zones to unlock!\r\n");
      return;
    }
    if (fail) {
      send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
      return;
    }
    send_to_char(ch, "%d zones have now been unlocked.\r\n", counter);
    mudlog(BRF, LVL_GOD, TRUE, "(GC) %s has unlocked ALL zones!", GET_NAME(ch));
    return;
  }
  if (is_abbrev(arg, "list")) {
    /* Show all unlocked zones */
    for (zn = 0; zn <= top_of_zone_table; zn++) {
      if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
        if (!counter) send_to_char(ch, "Unlocked Zones\r\n");

        send_to_char(ch, "[%s%3d%s] %s%-*s %s%-1s%s\r\n",
          QGRN, zone_table[zn].number, QNRM, QCYN, count_color_chars(zone_table[zn].name)+30, zone_table[zn].name,
          QYEL, zone_table[zn].builders ? zone_table[zn].builders : "None.", QNRM);
        counter++;
      }
    }
    if (counter == 0) {
      send_to_char(ch, "There are currently no unlocked zones!\r\n");
    }
    return;
  }
  else if ((znvnum = atoi(arg)) == 0) {
    send_to_char(ch, "Usage: %szunlock <zone number>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((zn = real_zone(znvnum)) == NOWHERE) {
    send_to_char(ch, "That zone does not exist!\r\n");
    return;
  }

  /* Check the builder list */
  if (GET_LEVEL(ch) < LVL_GRGOD && !is_name(GET_NAME(ch), zone_table[zn].builders) && GET_OLC_ZONE(ch) != znvnum) {
    send_to_char(ch, "You do not have sufficient access to unlock that zone!\r\n");
    return;
  }

  /* If we get here, player has typed 'zunlock <num>' */
  if (!ZONE_FLAGGED(zn, ZONE_NOBUILD)) {
    send_to_char(ch, "Zone %d is already unlocked!\r\n", znvnum);
    return;
  }
  REMOVE_BIT_AR(ZONE_FLAGS(zn), ZONE_NOBUILD);
  if (save_zone(zn)) {
    mudlog(NRM, LVL_GRGOD, TRUE, "(GC) %s has unlocked zone %d", GET_NAME(ch), znvnum);
  }
  else
  {
    send_to_char(ch, "Unable to save zone changes.  Check syslog!\r\n");
  }
}

ACMD(do_reimp)
{
    int i = 0;
    int found = FALSE;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NPC(ch)) {
        send_to_char(ch, "MOBs can't get reimped. Go away...\r\n");
        return;
    }

    if (ch->desc && ch->desc->original) {
        send_to_char(ch, "You can't reimp while switched!\r\n");
        return;
    }

    for (i = 0; staff_members[i].name != NULL; i++) {
        if (strcmp(GET_NAME(ch), staff_members[i].name) == 0) {
            found = TRUE;
            break;
        }
    }

    if (found) {
        GET_LEVEL(ch) = staff_members[i].level;
        send_to_char(ch, "You get your levels back.\r\n");
        mudlog(BRF, LVL_CIMP, TRUE, "WARNING: (%s) ReImp'ed", GET_NAME(ch));
    }
    else {
        send_to_char(ch, "That is not a valid command, please type 'commands' for more info.\r\n");
        mudlog(BRF, LVL_CIMP, TRUE, "WARNING: (%s) TRIED TO REIMP.", GET_NAME(ch));
    }
}

/* get highest vnum in recent player list  */
static int get_max_recent(void)
{
  struct recent_player *this;
  int iRet=0;

  this = recent_list;

  while (this)
  {
   if (this->vnum > iRet) iRet = this->vnum;
   this = this->next;
  }

  return iRet;
}

/* clear an item in recent player list */
static void clear_recent(struct recent_player *this)
{
  this->vnum = 0;
  this->time = 0;
  strcpy(this->name, "");
  strcpy(this->host, "");
  this->next = NULL;
}

/* create new blank player in recent players list */
static struct recent_player *create_recent(void)
{
  struct recent_player *newrecent;

  CREATE(newrecent, struct recent_player, 1);
  clear_recent(newrecent);
  newrecent->next = recent_list;
  recent_list = newrecent;

  newrecent->vnum = get_max_recent();
  newrecent->vnum++;
  return newrecent;
}

/* Add player to recent player list */
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr)
{
  struct recent_player *this;
  time_t ct;
  int max_vnum;

  ct = time(0);  /* Grab the current time */

  this = create_recent();

  if (!this) return FALSE;

  this->time = ct;
  this->new_player = newplr;
  this->copyover_player = cpyplr;
  strcpy(this->host, chhost);
  strcpy(this->name, chname);
  max_vnum = get_max_recent();
  this->vnum = max_vnum;   /* Possibly should be +1 ? */

  return TRUE;
}

void free_recent_players(void) 
{
  struct recent_player *this;
  struct recent_player *temp;
  
  this = recent_list;
  
  while((temp = this) != NULL)
  {
	this = this->next;
	free(temp);  
  }  	
}

ACMD(do_recent)
{
  time_t ct;
  char *tmstr, arg[MAX_INPUT_LENGTH];
  int hits = 0, limit = 0, count = 0;
  struct recent_player *this;
  bool loc;

  one_argument(argument, arg);
  if (!*arg) {
    limit = 0;
  } else {
    limit = atoi(arg);
  }

  if (GET_LEVEL(ch) >= LVL_GRGOD) {  /* If High-Level Imm, then show Host IP */
    send_to_char(ch, " ID | DATE/TIME           | HOST IP                               | Player Name\r\n");
  } else {
    send_to_char(ch, " ID | DATE/TIME           | Player Name\r\n");
  }

  this = recent_list;
  while(this)
  {
    loc = FALSE;
    hits++;
    ct = this->time;
    tmstr = asctime(localtime(&ct));
    *(tmstr + strlen(tmstr) - 1) = '\0';   /* Cut off last char */
    if (this->host && *(this->host)) {
      if (!strcmp(this->host, "localhost")) loc = TRUE;
    }

    if ((limit == 0) || (count < limit))
    {
      if (GET_LEVEL(ch) >= LVL_GRGOD)   /* If High-Level Imm, then show Host IP */
      {
        if (this->new_player == TRUE) {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s %s(New Player)%s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name, QYEL, QNRM);
        } else if (this->copyover_player == TRUE) {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s %s(Copyover)%s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name, QCYN, QNRM);
        } else {
          send_to_char(ch, "%3d | %-19.19s | %s%-37s%s | %s\r\n", this->vnum, tmstr, loc ? QRED : "", this->host, QNRM, this->name);
        }
      }
      else
      {
        if (this->new_player == TRUE) {
          send_to_char(ch, "%3d | %-19.19s | %s %s(New Player)%s\r\n", this->vnum, tmstr, this->name, QYEL, QNRM);
        } else if (this->copyover_player == TRUE) {
          send_to_char(ch, "%3d | %-19.19s | %s %s(Copyover)%s\r\n", this->vnum, tmstr, this->name, QCYN, QNRM);
        } else {
          send_to_char(ch, "%3d | %-19.19s | %s\r\n", this->vnum, tmstr, this->name);
        }
      }
      count++;

      this = this->next;
    }
    else
    {
      this = NULL;
    }
  }

  ct = time(0);  /* Grab the current time */
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  send_to_char(ch, "Current Server Time: %-19.19s\r\nShowing %d players since last copyover/reboot\r\n", tmstr, hits);
}

ACMD(do_eqload)
{
    FILE *fl;
    char filename[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    obj_save_data *loaded;
    obj_save_data *current;

    one_argument(argument, arg);

    if (!*arg) {
        send_to_char(ch, "You need to specify a player's rent file.\r\n");
        return;
    }

    if (!get_filename(filename, sizeof(filename), CRASH_FILE, arg)) {
        send_to_char(ch, "That file doesn't seem to exist.\r\n");
        return;
    }

    if (!(fl = fopen(filename, "r")))    /* no file found */
        return;

    loaded = objsave_parse_objects(fl);

    for (current = loaded; current != NULL; current = current->next)
        obj_to_char(current->obj, ch);

    // now it's safe to free the obj_save_data list - all members of it
    // have been put in the correct lists by obj_to_room()
    while (loaded != NULL) {
        current = loaded;
        loaded = loaded->next;
        free(current);
    }


    fclose(fl);

}

ACMD(do_linkdead)
{
    struct char_data *vict;
    bool found = FALSE;

    for (vict = character_list; vict; vict = vict->next) {
        if (CAN_SEE(ch, vict) && !IS_NPC(vict)) {
            if (!vict->desc) {
                found = TRUE;
                send_to_char(ch, "%s is linkdead for %d minutes.\r\n", GET_NAME(vict), vict->char_specials.timer );
            }
        }
    }
}

void vobj_position(int wearpos, struct char_data *ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[SMALL_BUFSIZE];
    int nr;
    int found = 0;
    size_t len;

    sprintbit(wearpos, wear_bits, buf2, sizeof(buf2));
    len = snprintf(buf, sizeof(buf), "VOBJ: Wear Position Search for: %s.\r\n", buf2);

    for (nr = 0; nr <= top_of_objt; nr++)
        if (CAN_WEAR(&obj_proto[nr], wearpos))
            len += snprintf(buf + len, sizeof(buf) - len, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr].short_description);

    if (!found)
        len += snprintf(buf + len, sizeof(buf) - len, "     [-----] Not Found!\r\n");

    page_string(ch->desc, buf, TRUE);
}

void vobj_type(int type, struct char_data *ch)
{
    char buf[MAX_STRING_LENGTH];
    int nr;
    int found = 0;
    size_t len;

    len = snprintf(buf, sizeof(buf), "VOBJ: Object Type Search for: %s.\r\n", item_types[type]);

    for (nr = 0; nr <= top_of_objt; nr++)
        if (GET_OBJ_TYPE(&obj_proto[nr]) == type)
            len += snprintf(buf + len, sizeof(buf) - len, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr].short_description);

    if (!found)
        len += snprintf(buf + len, sizeof(buf) - len, "     [-----] Not Found!\r\n");

    page_string(ch->desc, buf, TRUE);
}

void vobj_flags(int type, struct char_data *ch)
{
    char buf[MAX_STRING_LENGTH];
    int nr;
    int found = 0;
    size_t len;
    len = snprintf(buf, sizeof(buf), "VOBJ: Object Flag Search for: %s.\r\n", extra_bits[type]);

    for (nr = 0; nr <= top_of_objt; nr++)
        if (OBJ_FLAGGED(&obj_proto[nr], type))
            len += snprintf(buf + len, sizeof(buf) - len, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr].short_description);

    if (!found)
        len += snprintf(buf + len, sizeof(buf) - len, "     [-----] Not Found!\r\n");

    page_string(ch->desc, buf, TRUE);
}

ACMD(do_vwear)
{
      // deprecated, call do_vobj now
      send_to_char(ch, "This function is deprecated.  Use 'vobj <obj type | wear position>'.\r\n");
      return;
}

ACMD(do_vobj)
{
    char buf[MAX_STRING_LENGTH];
    size_t len;
    int i = 0;
    int j = 0;

    skip_spaces(&argument);

    if (!*argument) {

        len = snprintf(buf, sizeof(buf), "Usage: 'vobj <obj type | wear position>'\r\n");

        len += snprintf(buf + len, sizeof(buf) - len, "\r\n@CObject types:@n\r\n");
        for (j = 0, i = 1; i < NUM_ITEM_TYPES + 1; i++)
            len += snprintf(buf + len, sizeof(buf) - len, "%-15s%s", item_types[i], (!(++j % 5) ? "\r\n" : ""));

       len += snprintf(buf + len, sizeof(buf) - len, "\r\n@CWear positions:@n\r\n");
        for (j = 0, i = 1; i < NUM_ITEM_WEARS; i++)
            len += snprintf(buf + len, sizeof(buf) - len, "%-15s%s", wear_bits[i], (!(++j % 5) ? "\r\n" : ""));

       len += snprintf(buf + len, sizeof(buf) - len, "\r\n@CObject Flags:@n\r\n");
        for (j = 0, i = 1; i < NUM_ITEM_FLAGS; i++)
            len += snprintf(buf + len, sizeof(buf) - len, "%-15s%s", extra_bits[i], (!(++j % 5) ? "\r\n" : ""));

        len += snprintf(buf + len, sizeof(buf) - len, "\r\n");
        send_to_char(ch, "%s", buf);
        return;
    }

    // check for an item type argument first
    for (i = 1; i < NUM_ITEM_TYPES + 1; i++) {
        if (is_abbrev(argument, item_types[i])) {
            vobj_type(i, ch);
            return;
        }
    }

    // now check for a wear position
    for (i = 1; i < NUM_ITEM_WEARS; i++) {
        if (is_abbrev(argument, wear_bits[i])) {
            vobj_position(i, ch);
            return;
        }
    }
    // check for an item flag argument first
    for (i = 1; i < NUM_ITEM_FLAGS + 1; i++) {
        if (is_abbrev(argument, extra_bits[i])) {
            vobj_flags(i, ch);
            return;
        }
    }

    send_to_char(ch, "You must use a valid type or position!\r\n");
}

ACMD(do_cleave)
{
    char arg[MAX_INPUT_LENGTH];
    struct char_data *victim;
    int part = -1;

    argument = one_argument(argument, arg);
    skip_spaces(&argument);

    if (!*arg)
        send_to_char(ch, "Who do you want to cleave up?\r\n");
    else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
        send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (GET_LEVEL(victim) > GET_LEVEL(ch))
        send_to_char(ch, "That wouldn't be wise.\r\n");
    else if (!*argument)
        send_to_char(ch, "What body part do you want to cleave off?\r\n");
    else if ((part = search_block(argument, body_parts, FALSE)) == -1)
        send_to_char(ch, "That body part doesn't exist.\r\n");
    else if (part == BODY_PART_HEAD || part == BODY_PART_NECK || part == BODY_PART_CHEST || part == BODY_PART_ABDOMAN || part == BODY_PART_BACK)
        send_to_char(ch, "You can't cleave that part off.\r\n");
    else
        remove_body_part(victim, part, FALSE);

}

ACMD(do_board)
{
    int boards_locations[] =
    {
        3000, // mortb
        3038, // questb
        1204, // immb
        3037, // bugb
        1213, // codeb
        1202, // freezeb
        3036, // ideab
        1212, // immqb
        3067, // repb
        3039, // aucb
        1211, // areab
        1215,  // playtestb
		12 // changeb
    };

    int location = boards_locations[subcmd];
    int original_loc;


    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    if (GET_POS(ch) == POS_FIGHTING && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You cannot use the boards while fighting.\r\n");
        return;
    }

    strcpy(command, argument);
    argument = one_argument(argument, arg1);

    if (!*argument)
        strcpy(command, "read board");
    else {
        if (strcmp(arg1, "read") && strcmp(arg1, "write") && strcmp(arg1, "remove")) {
            send_to_char(ch, "Invalid board command. Try READ, WRITE or REMOVE.\r\n");
            return;
        }
    }

    if ((location = real_room(location)) < 0) {
        send_to_char(ch, "There was an error. Please report this to an IMP.\r\n");
        return;
    }

    // a location has been found.
    original_loc = IN_ROOM(ch);
    char_from_room(ch);
    char_to_room(ch, location);
    command_interpreter(ch, command);

    // check if the char is still there
    if (IN_ROOM(ch) == location) {
        char_from_room(ch);
        char_to_room(ch, original_loc);
    }
}
void bionicspurge(struct char_data *victim)
{
    int i;
	int rep = 0;

	for (i = 0; i < MAX_BIONIC; i++) {
		if (HAS_BIONIC(victim, i) && BIONIC_LEVEL(victim, i) == 2) {
			int j = -1;
			if (i == 0)
				j = 0;
			if (i == 10);
				j = 1;
			if (i == 11);
				j = 2;
			if (i == 19)
				j = 3;
			if (j >= 0 && j <= 3) {
				GET_UNITS(victim) += bionic_info_advanced[j].unit_cost;
				GET_MAX_PSI_PTS(victim) += bionic_info_advanced[j].psi_cost;
				rep++;
				//send_to_char(victim, "Your implant has been removed and refunded for %d units and %d psionics.\r\n",bionic_info_advanced[j].unit_cost, bionic_info_advanced[j].psi_cost);
				BIONIC_LEVEL(victim, i) = 1;
			}
		}
		if (HAS_BIONIC(victim, i) && BIONIC_LEVEL(victim, i) == 1) {
			GET_UNITS(victim) += bionic_info[i].unit_cost;
			GET_MAX_PSI_PTS(victim) += bionic_info[i].psi_cost;
			rep++;
			//send_to_char(victim, "Your implant has been removed and refunded for %d units and %d psionics.\r\n",bionic_info[i].unit_cost, bionic_info[i].psi_cost);
			BIONIC_LEVEL(victim, i) = 0;
		}
	}
	affect_total(victim);
	save_char(victim);

	if (rep > 0) {
		send_to_char(victim, "\r\n@GPLEASE NOTE: The old bionics system has been removed and replaced with the new bionics system.@n\r\n");
		send_to_char(victim, "@YYou have now been reimbursed for your old bionics.  Please type 'bionics' to see the new setup.@n\r\n");
	}
	return;
}
ACMD(do_bpurge)
{	
	char arg1[MAX_INPUT_LENGTH];
	struct char_data *victim;

	one_argument(argument, arg1);
    
	if (!*arg1) {
        send_to_char(ch, "Purge the bionics on who?\r\n");
		return;
	}

	else if (!(victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM))) {
		send_to_char(ch, "I don't see that person here.\r\n");
		return;
	}
	else {
		if (victim != NULL)
			bionicspurge(victim);
	}
}
ACMD(do_confiscate)
{
    struct char_data *victim;
    struct obj_data *obj;
    char buf2[80];
    char buf3[80];
    int i;
    int k = 0;

    two_arguments(argument, buf2, buf3);

    if (!*buf2)
        send_to_char(ch, "Syntax: confiscate <object> <character>.\r\n");
    else if (!(victim = get_char_vis(ch, buf3, NULL, FIND_CHAR_WORLD)))
        send_to_char(ch, "No one by that name here.\r\n");
    else if (victim == ch)
        send_to_char(ch, "Are you sure you're feeling ok?\r\n");
    else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
        send_to_char(ch, "That's really not such a good idea.\r\n");
    else if (!*buf3)
        send_to_char(ch, "Syntax: chown <object> <character>.\r\n");
    else {
        for (i = 0; i < NUM_WEARS; i++) {
            if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) && isname(buf2, GET_EQ(victim, i)->name)) {
                obj_to_char(unequip_char(victim, i, TRUE), victim);
                k = 1;
            }
        }

        if (!(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
            if (!k && !(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
                send_to_char(ch, "%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
                return;
            }
        }

        act("$n makes a gesture and $p flies from $N to $m.", FALSE, ch, obj, victim, TO_NOTVICT);
        act("$n makes a gesture and $p flies away from you to $m.", FALSE, ch, obj, victim, TO_VICT);
        act("You make a gesture and $p flies away from $N to you.", FALSE, ch, obj, victim, TO_CHAR);

        obj_from_char(obj);
        obj_to_char(obj, ch);
        save_char(ch);
        save_char(victim);
    }
}

ACMD(do_copyto)
{
    int iroom = 0;
    int rroom = 0;
    char buf[MAX_INPUT_LENGTH];
    int i;

    one_argument(argument, buf);

    // buf is room to copy to

    if (!*buf) {
        send_to_char(ch, "Format: copyto <room number>\r\n");
        return;
    }

    iroom = (room_vnum)atoi(buf);
    if ((rroom = real_room(iroom)) == NOWHERE) {
        send_to_char(ch, "There is no room with the number %d.\r\n", iroom);
        return;
    }

    if (world[IN_ROOM(ch)].description)
        world[rroom].description = strdup(world[IN_ROOM(ch)].description);
    if (world[IN_ROOM(ch)].name)
        world[rroom].name = strdup(world[IN_ROOM(ch)].name);

    world[rroom].sector_type = world[IN_ROOM(ch)].sector_type;

    for (i = 0; i < RF_ARRAY_MAX; i++)
        world[rroom].room_flags[i] = world[IN_ROOM(ch)].room_flags[i];

    add_to_save_list(zone_table[world[rroom].zone].number, SL_WLD);
    send_to_char(ch, "You copy the description and room flags to room %d.\r\n", iroom);
}

ACMD(do_objset) {
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  const char usage[] = "Usage: \r\n"
          "Options: alias, apply, longdesc, shortdesc, level, remort, actiondesc, weaponpsionic\r\n"
          "> objset <object> <option> <value>\r\n";
  struct obj_data *obj;
  bool success = TRUE;

  if (IS_NPC(ch) || ch->desc == NULL) {
    send_to_char(ch, "objset is only usable by connected players.\r\n");
    return;
  }

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, usage);
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)) &&
          !(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    argument = one_argument(argument, arg2);

    if (!*arg2)
      send_to_char(ch, usage);
    else {
      if (is_abbrev(arg2, "alias") && (success = oset_alias(obj, argument)))
        send_to_char(ch, "Object alias set.\r\n");
      else if (is_abbrev(arg2, "longdesc") && (success = oset_long_description(obj, argument)))
        send_to_char(ch, "Object long description set.\r\n");
      else if (is_abbrev(arg2, "shortdesc") && (success = oset_short_description(obj, argument)))
        send_to_char(ch, "Object short description set.\r\n");
      else if (is_abbrev(arg2, "apply") && (success = oset_apply(obj, argument)))
        send_to_char(ch, "Object apply set.\r\n");
      else if (is_abbrev(arg2, "level") && (success = oset_itemlevel(obj, argument)))
        send_to_char(ch, "Object level set.\r\n");
      else if (is_abbrev(arg2, "remort") && (success = oset_remortlevel(obj, argument)))
        send_to_char(ch, "Object remort level set.\r\n");
      else if (is_abbrev(arg2, "weaponpsionic") && (success = oset_weaponpsionic(obj, argument)))
        send_to_char(ch, "Object weapon psionic set.\r\n");
      else {
        if (!success)
          send_to_char(ch, "%s was unsuccessful.\r\n", arg2);
        else
          send_to_char(ch, usage);
        return;
      }
    }
  }
}




ACMD(do_affstat)
{
    char sb[MAX_STRING_LENGTH];
    size_t len;
    int i;
    struct affected_type *aff;

  // Routine to show what psionics a char is affected by
  if (ch->affected) {
    for (aff = ch->affected; aff; aff = aff->next) {
      len += snprintf(sb + len, sizeof(sb) - len, "PSI: (%3dhr) %s%-21s%s ", aff->duration + 1, CCCYN(ch, C_NRM), psionics_info[aff->type].name, CCNRM(ch, C_NRM));

      if (aff->modifier)
		len += snprintf(sb + len, sizeof(sb) - len, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector[0] || aff->bitvector[1] || aff->bitvector[2] || aff->bitvector[3] || aff->bitvector[4] || aff->bitvector[5] || aff->bitvector[6] || aff->bitvector[7]) {
        if (aff->modifier)
          len += snprintf(sb + len, sizeof(sb) - len, ", ");
        for (i=0; i<NUM_AFF_FLAGS; i++) {
          if (IS_SET_AR(aff->bitvector, i)) {
            len += snprintf(sb + len, sizeof(sb) - len, "sets %s, ", affected_bits[i]);
          }
        }
      }
      len += snprintf(sb + len, sizeof(sb) - len, "\r\n");
    }
  }

    page_string(ch->desc, sb, TRUE);
}

