/**************************************************************************
*  File: interpreter.c                                     Part of tbaMUD *
*  Usage: Parse user commands, search for specials, call ACMD functions.  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "psionics.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "act.h" /* ACMDs located within the act*.c files */
#include "ban.h"
#include "class.h"
#include "graph.h"
#include "hedit.h"
#include "house.h"
#include "config.h"
#include "modify.h" /* for do_skillset... */
#include "quest.h"
#include "asciimap.h"
#include "prefedit.h"
#include "ibt.h"
#include "mud_event.h"
#include "bionics.h"
#include "craft.h"
#include "custom.h"
#include "events.h"
#include "skilltree.h"
/* local (file scope) functions */
static int perform_dupe_check(struct descriptor_data *d);
static struct alias_data *find_alias(struct alias_data *alias_list, char *str);
static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
static int _parse_name(char *arg, char *name);
//static bool perform_new_char_dupe_check(struct descriptor_data *d);
/* sort_commands utility */
static int sort_commands_helper(const void *a, const void *b);

/* globals defined here, used here and elsewhere */
int *cmd_sort_info = NULL;

ACMD(do_alias);
//ACMD(do_mutilate);
ACMD(do_affstat);
ACMD(do_rank);
ACMD(do_query);
ACMD(do_deathlink);
ACMD(do_callpet);
ACMD(do_ammo);
ACMD(do_drag);
ACMD(do_push);
ACMD(do_maniac);
ACMD(do_whistle);
ACMD(do_progress);
ACMD(do_class);
ACMD(do_increase);
ACMD(do_mail);
ACMD(do_bpurge);
ACMD(do_gps);
ACMD(do_repair);
ACMD(do_questrun);
ACMD(do_tag);
ACMD(do_freezetag);
ACMD(do_testvar);
ACMD(do_objset);
ACMD(do_unwield);
ACMD(do_enc);
ACMD(do_dup);
ACMD(do_roomwhos);
ACMD(do_testvar)
{
	send_to_char(ch, "%s\r\n", GET_EXITS(ch));
}

struct command_info *complete_cmd_info;

/* This is the Master Command List. You can put new commands in, take commands
 * out, change the order they appear in, etc.  You can adjust the "priority"
 * of commands simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask", just put
 * "assist" above "ask" in the Master Command List. In general, utility
 * commands such as "at" should have high priority; infrequently used and
 * dangerously destructive commands should have low priority. */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, 0 }, /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , "e"       , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , "s"       , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , "w"       , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , "u"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , "d"       , POS_STANDING, do_move     , 0, SCMD_DOWN },
  { "north"    , "n"       , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , "e"       , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , "s"       , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , "w"       , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , "u"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , "d"       , POS_STANDING, do_move     , 0, SCMD_DOWN },
  { "northwest", "northw"  , POS_STANDING, do_move     , 0, SCMD_NW },
  { "nw"       , "nw"      , POS_STANDING, do_move     , 0, SCMD_NW },
  { "northeast", "northe"  , POS_STANDING, do_move     , 0, SCMD_NE },
  { "ne"       , "ne"      , POS_STANDING, do_move     , 0,  SCMD_NE },
  { "southeast", "southe"  , POS_STANDING, do_move     , 0, SCMD_SE },
  { "se"       , "se"      , POS_STANDING, do_move     , 0, SCMD_SE },
  { "southwest", "southw"  , POS_STANDING, do_move     , 0, SCMD_SW },
  { "sw"       , "sw"      , POS_STANDING, do_move     , 0, SCMD_SW },
  
  /* now, the main list */
    { "testvar"     , "testvar"     , POS_DEAD    , do_testvar      , LVL_IMPL,    0              },
    { "at"          , "at"          , POS_DEAD    , do_at           , LVL_IMMORT , 0              },
	{ "attack"		, "att"			, POS_FIGHTING, do_hit			, 0			 , 0		      },
    { "advance"     , "adv"         , POS_DEAD    , do_advance      , LVL_CIMP   , 0              },
    { "aedit"       , "aed"         , POS_DEAD    , do_oasis_aedit  , LVL_IMMORT , 0              },
    { "aid"         , "aid"         , POS_STANDING, do_medkit       , 0          , 0              },
	{ "aim"			, "aim"			, POS_STANDING, do_aim			, 0			 , 0			  },
    { "affection"   , "aff"         , POS_DEAD    , do_affections   , 0          , 0              },
//    { "affstat"   , "affs"         , POS_DEAD    , do_affstat   , 0          , 0              },
    { "alias"       , "ali"         , POS_DEAD    , do_alias        , 0          , 0              },
    { "afk"         , "afk"         , POS_DEAD    , do_toggle       , 0          , SCMD_AFK       },
    { "anatomy"     , "anat"        , POS_DEAD    , do_anatomy      , 0          , 0              },
    { "ambush"      , "ambush"      , POS_STANDING, do_ambush       , 0          , 0              },
    { "ammo"      , "amm"      , POS_DEAD, do_ammo       , 0          , 0              },
    { "appraise"    , "appraise"    , POS_STANDING, do_not_here     , 0          , 0              },
    { "areab"       , "areab"       , POS_DEAD    , do_board        , LVL_BUILDER, SCMD_AREAB     },
    { "areas"       , "are"         , POS_DEAD    , do_areas        , 0          , 0              },
    { "arm"         , "arm"         , POS_STANDING, do_arm          , 0          , 0              },
    { "armor"         , "armo"         , POS_DEAD, do_armor          , 0          , 0       },
    { "assist"      , "as"          , POS_FIGHTING, do_assist       , 1          , 0              },
    { "assemble"    , "assemble"    , POS_SITTING , do_assemble     , 0          , 0              },
    { "assassinate" , "assassin"    , POS_STANDING, do_assassinate  , 0          , 0              },
    { "ask"         , "ask"         , POS_RESTING , do_spec_comm    , 0          , SCMD_ASK       },
    { "astat"       , "ast"         , POS_DEAD    , do_astat        , 0          , 0              },
    { "attach"      , "atta"        , POS_STANDING, do_attach       , 0          , 0              },
    { "aucb"        , "aucb"        , POS_SLEEPING, do_board        , 0          , SCMD_AUCB      },
    { "autoassist"  , "autoass"     , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOASSIST},
    { "autoexits"   , "autoex"      , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOEXIT  },
    { "autocon"     , "autocon"     , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOCON   },
    { "autodamage"  , "autodam"     , POS_DEAD    , do_toggle       , 0          , SCMD_AUTODAM   },
    { "autodiag"    , "autodiag"    , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOCON   },
    { "autogain"    , "autogain"    , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOGAIN  },
    { "autoloot"    , "autoloot"    , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOLOOT  },
    { "autoscavenger", "autosc"     , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOSCAVENGER  },
    { "autosacrifice", "autosac"    , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOSCAVENGER  },
    { "autosplit"   , "autosplit"   , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOSPLIT },
    { "autoequip"   , "autoeq"      , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOEQUIP },
    { "autowear"    , "autowear"    , POS_DEAD    , do_toggle       , 0          , SCMD_AUTOEQUIP },

    { "backstab"    , "ba"          , POS_STANDING, do_backstab     , 1          , 0              },
    { "ban"         , "ban"         , POS_DEAD    , do_ban          , LVL_GRGOD  , 0              },
    { "balance"     , "bal"         , POS_STANDING, do_not_here     , 1          , 0              },
    { "blast"   , "bl"   , POS_FIGHTING, do_blast    , 0          , SCMD_BLAST              },
    { "brief"       , "br"          , POS_DEAD    , do_toggle       , 0          , SCMD_BRIEF     },
    { "buildwalk"   , "buildwalk"   , POS_STANDING, do_toggle       , LVL_BUILDER, SCMD_BUILDWALK },
    { "buy"         , "bu"          , POS_STANDING, do_not_here     , 0          , 0              },
    { "bug"         , "bug"         , POS_DEAD    , do_ibt    , 0          , SCMD_BUG       },
    { "bc"          , "bc"          , POS_RESTING , do_gen_comm     , 0          , SCMD_RADIO     },
    { "behead"      , "behead"      , POS_FIGHTING, do_behead       , 0          , 0              },
    { "berserk"     , "berserk"     , POS_FIGHTING, do_berserk      , 0          , 0              },
    { "bionics"     , "bionics"     , POS_FOCUSING, do_anatomy      , 0          , 0              },  
	{ "bpurge"		, "bpurge"		, POS_DEAD,		do_bpurge		, LVL_IMPL   , 0			  },
    { "bodyblock"   , "bodyblock"   , POS_FIGHTING, do_bodyblock    , 0          , 0              },
    { "broadcast"   , "broadcast"   , POS_RESTING , do_gen_comm     , 0          , SCMD_RADIO     },
    { "bugb"        , "bugb"        , POS_SLEEPING, do_ibt        , 0          , SCMD_BUGB      },

    { "callpet"      , "cal"        , POS_STANDING    , do_callpet       , 0  , 0              },
    { "cedit"       , "cedit"       , POS_DEAD    , do_oasis_cedit  , LVL_IMPL   , 0              },
    { "changelog"   , "change"      , POS_DEAD    , do_changelog    , LVL_GOD    , 0              },
	{ "changes"		, "cha"			, POS_DEAD	  , do_board		, 0			 , SCMD_CHANGEB	  },
	{ "changeb"		, "changeb"		, POS_DEAD	  , do_board		, 0			 , SCMD_CHANGEB	  },
	{ "check"       , "ch"          , POS_STANDING, do_not_here     , 1          , 0              },
    { "checkload"   , "checkl"      , POS_DEAD    , do_checkloadstatus, LVL_GOD  , 0              },
	{ "circle"		, "circle"		, POS_DEAD	  , do_circle		, 0			 , 0			  },
	{ "class"		, "class"		, POS_DEAD	  , do_class		, 0			 , 0			  },
    { "close"       , "cl"          , POS_SITTING , do_gen_door     , 0          , SCMD_CLOSE     },
    { "clear"       , "cle"         , POS_DEAD    , do_gen_ps       , 0          , SCMD_CLEAR     },
    { "cleave"      , "clea"        , POS_DEAD    , do_cleave       , LVL_GRGOD  , 0              },
    { "cls"         , "cls"         , POS_DEAD    , do_gen_ps       , 0          , SCMD_CLEAR     },
    { "consider"    , "con"         , POS_RESTING , do_consider     , 0          , 0              },
    { "commands"    , "com"         , POS_DEAD    , do_commands     , 0          , SCMD_COMMANDS  },
    { "compact"     , "comp"        , POS_DEAD    , do_toggle       , 0          , SCMD_COMPACT   },
    { "condition"    , "cond"       , POS_RESTING , do_diagnose     , 0          , 0              },
    { "copyover"    , "copyover"    , POS_DEAD    , do_copyover     , LVL_CIMP   , 0              },
	{ "cp"			, "cp"			, POS_DEAD	  , do_cp			, 0			 , 0			  },
	{ "crafting"	, "craft"		, POS_DEAD	  , do_craft		, LVL_IMPL	 , 0              },
    { "credits"     , "cred"        , POS_DEAD    , do_gen_ps       , 0          , SCMD_CREDITS   },
    { "charge"      , "charge"      , POS_FIGHTING, do_charge       , 0          , 0              },
    { "codeb"       , "codeb"       , POS_STANDING, do_board        , LVL_IMMORT , SCMD_CODEB     },
	{ "combo"		, "combo"		, POS_DEAD	  , do_combo		, 0			 , 0			  },
    { "confiscate"  , "confiscate"  , POS_DEAD    , do_confiscate   , LVL_GOD    , 0              },
    { "color"       , "color"       , POS_DEAD    , do_toggle       , 0          , SCMD_COLOR     },
    { "copyto "     , "copyto"      , POS_DEAD    , do_copyto       , LVL_BUILDER, 0              },
    { "coverfire"   , "coverfire"   , POS_FIGHTING, do_coverfire    , 0          , 0              },
    { "create"      , "create"      , POS_DEAD    , do_create       , LVL_GOD    , 0              },
	{ "custom"		, "custom"		, POS_STANDING, do_custom		, 1			 , 0			  },

    { "damroll"     , "damr"        , POS_DEAD    , do_hitroll      , 0          , 0              },
	{ "dash"		, "dash"		, POS_DEAD	  , do_dash         , 0          , 0              },
    { "date"        , "da"          , POS_DEAD    , do_date         , LVL_IMMORT , SCMD_DATE      },
    { "dc"          , "dc"          , POS_DEAD    , do_dc           , LVL_GOD    , 0              },
    { "deposit"     , "depo"        , POS_STANDING, do_not_here     , 1          , 0              },
    //{ "deathlink"      , "deat"        , POS_STANDING, do_deathlink , LVL_GOD          , 0              },
    { "detach"      , "deta"        , POS_STANDING, do_detach       , 0          , 0              },
    { "diagnose"    , "diag"        , POS_RESTING , do_diagnose     , 0          , 0              },
    { "dig"         , "dig"         , POS_DEAD    , do_dig          , LVL_BUILDER, 0              },
    { "display"     , "disp"        , POS_DEAD    , do_display      , 0          , 0              },
    { "disarm"      , "disarm"      , POS_FIGHTING, do_disarm       , 0          , 0              },
	{ "dissect"		, "dissect"		, POS_STANDING, do_dissect		, LVL_IMPL	 , 0			  },
    { "donate"      , "don"         , POS_RESTING , do_drop         , 0          , SCMD_DONATE    },
    { "drag"       , "dra"          , POS_STANDING , do_drag        , 0        , 0     },
	{ "drain"		, "drain"		, POS_FIGHTING , do_drain		, 0			 , 0			  },
    { "drink"       , "dri"         , POS_RESTING , do_drink        , 0          , SCMD_DRINK     },
	{ "drive"		, "drive"		, POS_RESTING , do_drive		, 0			 , 0			  }, 
    { "drop"        , "dro"         , POS_RESTING , do_drop         , 0          , SCMD_DROP      },
   // { "demote"      , "demote"      , POS_DEAD    , do_demote       , LVL_IMPL   , 0              },
    //{ "depress"     , "depress"     , POS_FOCUSING, do_depress      , 0          , 0              },
    { "dodge"       , "dodge"       , POS_STANDING, do_dodge        , 0          , 0              },
    //{ "dossier"     , "dossier"     , POS_DEAD    , do_dossier      , 0          , 0              },
	{ "duplicates"			, "dup"			, POS_DEAD	  , do_dup			, 0			 , 0			  },

    { "eat"         , "ea"          , POS_RESTING , do_eat          , 0          , SCMD_EAT       },
    { "echo"        , "ec"          , POS_SLEEPING, do_echo         , LVL_IMMORT , SCMD_ECHO      },
    { "emote"       , "em"          , POS_RESTING , do_echo         , 0          , SCMD_EMOTE     },
    { ":"           , ":"           , POS_RESTING , do_echo         , 1          , SCMD_EMOTE     },
	{ "electrify"	, "elect"		, POS_FIGHTING, do_electrify	, 0			 , 0			  },
	{ "encumb"			, "enc"			, POS_DEAD	  , do_enc			, 0			 , 0			  },

    { "enter"       , "ent"         , POS_STANDING, do_enter        , 0          , 0              },
    { "equipment"   , "eq"          , POS_SLEEPING, do_equipment    , 0          , 0              },
    { "exits"       , "ex"          , POS_RESTING , do_exits        , 0          , 0              },
    { "examine"     , "exa"         , POS_SITTING , do_examine      , 0          , 0              },
    { "exchange"    , "exchange"    , POS_STANDING, do_not_here     , 0          , 0              },
    { "exp"         , "exp"         , POS_DEAD    , do_exp          , 0          , 0              },
    { "eqload"      , "eqload"      , POS_DEAD    , do_eqload       , LVL_GRGOD  , 0              },
	{ "event"		, "event"		, POS_DEAD	  , do_event		, 0			 , 0			  },
	{ "eventinfo"	, "eventi"		, POS_DEAD	  , do_eventinfo	, 0			 , 0			  },
	{ "eventleaders", "eventl"		, POS_DEAD	  , do_eventleaders , 0			 , 0			  },

	{ "fame"		, "fame"		, POS_DEAD	  , do_fame			, 0			 , 0			  },
    { "force"       , "force"       , POS_SLEEPING, do_force        , LVL_GOD    , 0              },
    { "fill"        , "fil"         , POS_STANDING, do_pour         , 0          , SCMD_FILL      },
    { "file"        , "file"        , POS_SLEEPING, do_file         , LVL_GOD    , 0              },
    { "flee"        , "fl"          , POS_FIGHTING, do_flee         , 1          , 0              },
    { "follow"      , "fol"         , POS_RESTING , do_follow       , 0          , 0              },
    { "freeze"      , "freeze"      , POS_DEAD    , do_wizutil      , LVL_FREEZE , SCMD_FREEZE    },
    { "finddoor"    , "finddoor"    , POS_DEAD    , do_finddoor     , LVL_GOD    , 0              },
    { "findkey"     , "findkey"     , POS_DEAD    , do_findkey      , LVL_GOD    , 0              },
    { "firstaid"    , "fir"    , POS_STANDING, do_medkit       , 0          , 0              },
    { "flank"       , "flank"       , POS_FIGHTING, do_flank        , 0          , 0              },
    { "focus"       , "focus"       , POS_SLEEPING, do_focus        , 0          , 0              },
    //{ "forcerent"   , "forcerent"   , POS_DEAD    , do_forcerent    , LVL_GRGOD  , 0              },
    { "freezeb"     , "freezeb"     , POS_STANDING, do_board        , LVL_FREEZE , SCMD_FREEZEB   },
    { "freezetag"   , "freezetag"   , POS_STANDING, do_freezetag    , 0          , 0              },
    { "get"         , "g"           , POS_RESTING , do_get          , 0          , 0              },
    { "gecho"       , "gecho"       , POS_DEAD    , do_gecho        , LVL_GOD    , 0              },
    { "gemote"      , "gem"         , POS_SLEEPING, do_gen_comm     , 0          , SCMD_GEMOTE    },
    { "give"        , "giv"         , POS_RESTING , do_give         , 0          , 0              },
	{ "gps"			, "gps"			, POS_DEAD	  , do_gps			, 0			 , 0			  },
	{ "glist"		, "glist"		, POS_RESTING,	do_glist		, 40		 , 0			  },
    { "goto"        , "go"          , POS_SLEEPING, do_goto         , LVL_IMMORT , 0              },
    { "gossip"      , "gos"         , POS_SLEEPING, do_gen_comm     , 0          , SCMD_GOSSIP    },
    { "group"       , "gr"          , POS_RESTING , do_group        , 1          , 0              },
    { "grab"        , "grab"        , POS_RESTING , do_grab         , 0          , 0              },
    { "grats"       , "grat"        , POS_SLEEPING, do_gen_comm     , 0          , SCMD_GRATZ     },
    { "gsay"        , "gsay"        , POS_SLEEPING, do_gsay         , 0          , 0              },
    { "gtell"       , "gt"          , POS_SLEEPING, do_gsay         , 0          , 0              },
    { "gain"        , "gain"        , POS_FOCUSING, do_gain         , 0          , 0              },

    { "help"        , "h"           , POS_DEAD    , do_help         , 0          , 0              },
    { "happyhour"   , "ha"          , POS_DEAD    , do_happyhour    , 0              ,     },
    { "hedit"       , "hedit"       , POS_DEAD    , do_oasis_hedit  , LVL_GOD    , 0              },
    { "hindex"      , "hind"        , POS_DEAD    , do_hindex       , 0          , 0              },
    { "helpcheck"   , "helpch"      , POS_DEAD    , do_helpcheck    , LVL_GOD    , 0              },
    { "hide"        , "hi"          , POS_RESTING , do_hide         , 1          , 0              },
    { "handbook"    , "handb"       , POS_DEAD    , do_gen_ps       , LVL_IMMORT , SCMD_HANDBOOK  },
    { "hcontrol"    , "hcontrol"    , POS_DEAD    , do_hcontrol     , LVL_GOD    , 0              },
    { "history"     , "history"     , POS_DEAD    , do_history      , 0          , 0              },
    { "hit"         , "hit"         , POS_FIGHTING, do_hit          , 0          , SCMD_HIT       },
    { "hitroll"     , "hitr"        , POS_DEAD    , do_hitroll      , 0          , 0              },
    { "hold"        , "hold"        , POS_RESTING , do_grab         , 1          , 0              },
    { "holler"      , "holler"      , POS_RESTING , do_gen_comm     , 1          , SCMD_HOLLER    },
    { "holylight"   , "holy"        , POS_DEAD    , do_toggle       , LVL_IMMORT , SCMD_HOLYLIGHT },
    { "home"        , "home"        , POS_RESTING , do_home         , 0          , 0              },
    { "house"       , "house"       , POS_RESTING , do_house        , 0          , 0              },
    { "heal"        , "heal"        , POS_STANDING, do_medkit       , 0          , 0              },
	{ "hp"			, "hp"			, POS_DEAD	  , do_hp			, 0			 , 0			  },
	{ "hph"			, "hph"			, POS_DEAD	  , do_hph			, 0			 , 0			  },

    { "inventory"   , "i"           , POS_DEAD    , do_inventory    , 0          , 0              },
    { "idea"        , "id"          , POS_DEAD    , do_ibt    , 0          , SCMD_IDEA      },
    { "imotd"       , "imo"         , POS_DEAD    , do_gen_ps       , LVL_IMMORT , SCMD_IMOTD     },
    { "immlist"     , "imm"         , POS_DEAD    , do_gen_ps       , 0          , SCMD_IMMLIST   },
    { "info"        , "info"        , POS_SLEEPING, do_gen_ps       , 0          , SCMD_INFO      },
    { "invis"       , "invi"        , POS_DEAD    , do_invis        , LVL_IMMORT , 0              },
    { "ideab"       , "ideab"       , POS_DEAD    , do_board        , 0          , SCMD_IDEAB     },
    { "ignore"      , "ignore"      , POS_DEAD    , do_ignore       , 0          , 0              },
    { "immb"        , "immb"        , POS_SLEEPING, do_board        , LVL_IMMORT , SCMD_IMMB      },
    { "immqb"       , "immqp"       , POS_SLEEPING, do_board        , LVL_IMMORT , SCMD_IMMQB     },
    { "imbue"       , "imbue"       , POS_STANDING, do_not_here     , 0          , 0              },
    { "increase"    , "increase"    , POS_DEAD    , do_increase     , 0          , 0              },
    { "inject"      , "inject"      , POS_RESTING , do_inject       , 0          , 0              },
	{ "install"		, "install"		, POS_STANDING, do_not_here		, 0			 , 0			  },
    { "junk"        , "j"           , POS_RESTING , do_drop         , 0          , SCMD_JUNK      },

    { "kill"        , "k"           , POS_FIGHTING, do_kill         , 0          , 0              },
    { "look"        , "l"           , POS_FOCUSING, do_look         , 0          , SCMD_LOOK      },
    { "last"        , "last"        , POS_DEAD    , do_last         , LVL_IMMORT , 0              },
	{ "leap"		, "leap"		, POS_FIGHTING, do_leapattack	, 0			 , 0			  },
    { "leave"       , "lea"         , POS_STANDING, do_leave        , 0          , 0              },
    { "levels"      , "lev"         , POS_DEAD    , do_levels       , 0          , 0              },
    { "light"       , "ligh"        , POS_STANDING, do_light        , 0          , 0              },
    { "list"        , "lis"         , POS_STANDING, do_not_here     , 0          , 0              },
    { "links"       , "lin"         , POS_STANDING, do_links        , LVL_GOD    , 0              },
    { "lock"        , "loc"         , POS_SITTING , do_gen_door     , 0          , SCMD_LOCK      },
    { "ldflee"      , "ldflee"      , POS_STUNNED , do_toggle       , 0          , SCMD_LDFLEE    },
    { "linkdead"    , "linkdead"    , POS_DEAD    , do_linkdead     , LVL_IMMORT , 0              },
    { "load"        , "load"        , POS_FOCUSING, do_load         , 0          , 0              },
    //{ "lotto"       , "lotto"       , POS_RESTING , do_lotto        , 0          , 0              },

    { "motd"        , "motd"        , POS_DEAD    , do_gen_ps       , 0          , SCMD_MOTD      },
    { "mail"        , "mail"        , POS_STANDING, do_mail         , 1          , 0              },
    { "maniac"      , "mani"      , POS_STANDING, do_maniac       , 0          , 0              },
    { "map"         , "map"         , POS_STANDING, do_map          , 1          , 0              },
	{ "market"		, "market"		, POS_FIGHTING, do_market		, 0			 , 0			  },
    { "medit"       , "med"         , POS_DEAD    , do_oasis_medit  , LVL_BUILDER, 0              },
    { "mlist"       , "mlist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_MLIST},
    { "mcopy"       , "mcopy"       , POS_DEAD    , do_oasis_copy   , LVL_GOD    , CON_MEDIT      },
	{ "mudbackup"	, "mudbackup"	, POS_DEAD	  , do_mudbackup	, LVL_IMPL	 ,				  },
    { "mute"        , "mute"        , POS_DEAD    , do_wizutil      , LVL_GOD    , SCMD_MUTE      },
//  { "mutilate", "muti", POS_FIGHTING, do_mutilate, 0, 0},
    { "meditate"    , "meditate"    , POS_SLEEPING, do_meditate     , 0          , 0              },
    { "mortb"       , "mortb"       , POS_SLEEPING, do_board        , 0          , SCMD_MORTB     },
    { "mortlog"     , "mortlog"     , POS_DEAD    , do_toggle       , 1          , SCMD_MORTLOG   },
	{ "mv"			, "mv"			, POS_DEAD	  , do_mv			, 0			 , 0			  },

	{ "eventbuy"	, "eventbuy"	, POS_DEAD	  , do_newevent		, 0			 , 0			  },
    { "news"        , "news"        , POS_SLEEPING, do_board        , 0          , SCMD_CHANGEB   },
    { "nmotd"       , "nmotd"       , POS_DEAD    , do_gen_ps       , 0          , SCMD_NMOTD     },
    { "noauction"   , "noauction"   , POS_DEAD    , do_toggle       , 0          , SCMD_NOAUCT    },
    { "nogossip"    , "nogossip"    , POS_DEAD    , do_toggle       , 0          , SCMD_NOGOSS    },
    { "nograts"     , "nograts"     , POS_DEAD    , do_toggle       , 0          , SCMD_NOGRATZ   },
    { "nohassle"    , "nohassle"    , POS_DEAD    , do_toggle       , LVL_IMMORT , SCMD_NOHASSLE  },
	{ "nomarket"	, "nomarket"	, POS_DEAD	  , do_toggle		, 0			 , SCMD_NOMARKET  },
    { "norepeat"    , "norepeat"    , POS_DEAD    , do_toggle       , 0          , SCMD_NOREPEAT  },
    { "noshout"     , "noshout"     , POS_SLEEPING, do_toggle       , 1          , SCMD_NOSHOUT   },
    { "nosummon"    , "nosummon"    , POS_DEAD    , do_toggle       , 1          , SCMD_NOSUMMON  },
    { "notell"      , "notell"      , POS_DEAD    , do_toggle       , 1          , SCMD_NOTELL    },
    { "notitle"     , "notitle"     , POS_DEAD    , do_wizutil      , LVL_GRGOD  , SCMD_NOTITLE   },
    { "nowiz"       , "nowiz"       , POS_DEAD    , do_toggle       , LVL_IMMORT , SCMD_NOWIZ     },
    { "newbie"      , "newbie"      , POS_SLEEPING, do_gen_comm     , 0          , SCMD_NEWBIE    },
    { "ns"          , "ns"          , POS_DEAD    , do_newscore     , 0          , 0              },
    { "newscore"    , "newscore"    , POS_DEAD    , do_newscore     , 0          , 0              },
    { "newwho"    , "neww"    , POS_DEAD    , do_newwho     , 0          , 0              },
    { "nonewbie"    , "nonewbie"    , POS_DEAD    , do_toggle       , 0          , SCMD_NONEWBIE  },
    { "nowho"       , "no who"      , POS_DEAD    , do_toggle       , LVL_GOD    , SCMD_NOWHO     },
	{ "noleave"		, "noleave"		, POS_DEAD	  , do_noleave		, 0			 , 0			  },

    { "objset"      , "objset"      , POS_DEAD    , do_objset       , LVL_GOD    , 0              },
    { "open"        , "o"           , POS_SITTING , do_gen_door     , 0          , SCMD_OPEN      },
    { "order"       , "ord"         , POS_RESTING , do_order        , 1          , 0              },
    { "offer"       , "off"         , POS_STANDING, do_not_here     , 1          , 0              },
    { "olc"         , "olc"         , POS_DEAD    , do_show_save_list, LVL_BUILDER, 0             },
    { "olist"       , "olist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_OLIST },
    { "oedit"       , "oedit"       , POS_DEAD    , do_oasis_oedit  , LVL_BUILDER, 0              },
    { "ocopy"       , "ocopy"       , POS_DEAD    , do_oasis_copy   , LVL_GOD    , CON_OEDIT      },
    { "oldscore"    , "oldscore"    , POS_DEAD    , do_oldscore     , 0          , 0              },
    { "oldwho"    , "oldw"    , POS_DEAD    , do_oldwho     , 0          , 0              },
	{ "opticblast"	, "optic"		, POS_FIGHTING, do_opticblast	, 0			 , 0			  },
	{ "oset"		, "oset"		, POS_DEAD	  , do_oset			, LVL_GOD	 , 0			  },

    { "put"         , "p"           , POS_RESTING , do_put          , 0          , 0              },
    { "peace"       , "pe"          , POS_DEAD    , do_peace        , LVL_BUILDER, 0              },
    { "pick"        , "pi"          , POS_STANDING, do_gen_door     , 1          , SCMD_PICK      },
    { "practice"    , "pr"          , POS_RESTING , do_practice     , 1          , 0              },
    { "page"        , "pag"         , POS_DEAD    , do_page         , 0    , 0              },
    { "pardon"      , "pardon"      , POS_DEAD    , do_wizutil      , LVL_GOD    , SCMD_PARDON    },
    { "plist"       , "plist"       , POS_DEAD    , do_plist        , LVL_GOD    , 0              },
    { "policy"      , "pol"         , POS_DEAD    , do_gen_ps       , 0          , SCMD_POLICIES  },
    { "pour"        , "pour"        , POS_STANDING, do_pour         , 0          , SCMD_POUR      },
    { "prompt"      , "pro"         , POS_DEAD    , do_prompt       , 0          , 0              },
    { "prefedit"    , "pre"         , POS_DEAD    , do_oasis_prefedit, 0         , 0              },
    { "purge"       , "purge"       , POS_DEAD    , do_purge        , LVL_BUILDER, 0              },
	{ "progress"	, "progress"	, POS_DEAD	  , do_progress		, 0			 , 0			  },
    { "playtestb"   , "playtestb"   , POS_DEAD    , do_board        , 25         , SCMD_PLAYTESTB },
    { "ptb"         , "ptb"         , POS_DEAD    , do_board        , 25         , SCMD_PLAYTESTB },
	{ "protect"     , "protect"     , POS_STANDING, do_protect      , 0          , 0              },
    { "psi"         , "psi"         , POS_SITTING , do_psi          , 1          , 0              },
    { "psionics"    , "psionics"    , POS_DEAD    , do_psionics     , 0          , 0              },
    { "psionicset"  , "psionicset"  , POS_SLEEPING, do_psionicset   , LVL_GRGOD  , 0              },
    { "pull"        , "pull"        , POS_FIGHTING, do_pull         , 0          , 0              },
	{ "purify"      , "purify"      , POS_STANDING, do_not_here     , 0          , 0              },

    { "qedit"       , "qedit"       , POS_DEAD    , do_oasis_qedit  , LVL_BUILDER, 0              },
    { "qlist"       , "qlist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_QLIST },
    { "quaff"       , "qua"         , POS_RESTING , do_use          , 0          , SCMD_QUAFF     },
    { "qecho"       , "qec"         , POS_DEAD    , do_qcomm        , LVL_IMMORT , SCMD_QECHO     },
	{ "qp"			, "qp"			, POS_DEAD	  , do_qp           , 0          , 0              },
    { "query"       , "quer"         , POS_DEAD    , do_query        , 0          , 0              },
    { "quest"       , "que"         , POS_DEAD    , do_quest        , 0          , 0              },
    { "questrun"    , "questrun"   ,  POS_DEAD    , do_questrun     , LVL_GOD, 0 },
    { "qui"         , "qui"         , POS_DEAD    , do_quit         , 0          , 0              },
    { "quit"        , "quit"        , POS_DEAD    , do_quit         , 0          , SCMD_QUIT      },
    { "qsay"        , "qsay"        , POS_RESTING , do_qcomm        , 0          , SCMD_QSAY      },
    { "questb"      , "questb"      , POS_SLEEPING, do_board        , 0          , SCMD_QUESTB    },
    { "qcreate"     , "qcreate"     , POS_DEAD    , do_qcreate      , LVL_GOD    , 0              },

    { "reply"       , "r"           , POS_SLEEPING, do_reply        , 0          , 0              },
    { "rank"       , "ra"           , POS_SLEEPING, do_rank        , 0          , 0              },
    { "rest"        , "res"         , POS_RESTING , do_rest         , 0          , 0              },
    { "read"        , "rea"         , POS_RESTING , do_look         , 0          , SCMD_READ      },
    { "reload"      , "reload"      , POS_DEAD    , do_reboot       , LVL_CIMP   , 0              },
    { "recite"      , "reci"        , POS_RESTING , do_use          , 0          , SCMD_RECITE    },
    { "receive"     , "rece"        , POS_STANDING, do_not_here     , 1          , 0              },
    { "recon"       , "reco"   , POS_STANDING, do_recon    , 0          , 0              },
	{ "recover"		, "recover"		, POS_RESTING , do_recover		, 0			 , 0			  },
	{ "refuel"		, "refuel"		, POS_STANDING, do_refuel		, 0			 , 0			  },
    { "remove"      , "rem"         , POS_RESTING , do_remove       , 0          , 0              },
    //{ "rent"        , "rent"        , POS_STANDING, do_god_rent     , 1          , 0              },
	{ "replay"		, "repl"		, POS_DEAD	  , do_history		, 0			 , 0			  },
    { "report"      , "repo"        , POS_RESTING , do_report       , 0          , 0              },
    { "reroll"      , "rero"        , POS_DEAD    , do_wizutil      , LVL_GRGOD  , SCMD_REROLL    },
    { "rescue"      , "resc"        , POS_FIGHTING, do_rescue       , 1          , 0              },
    { "restore"     , "resto"       , POS_DEAD    , do_restore      , LVL_IMMORT    , 0              },
	{ "retreat"		, "retreat"		, POS_DEAD	  , do_retreat		, 0			 , 0			  },
    { "return"      , "retu"        , POS_DEAD    , do_return       , 0          , 0              },
    { "redit"       , "redit"       , POS_DEAD    , do_oasis_redit  , LVL_BUILDER, 0              },
    { "rlist"       , "rlist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_RLIST },
    { "rcopy"       , "rcopy"       , POS_DEAD    , do_oasis_copy   , LVL_GOD    , CON_REDIT      },
    { "roomflags"   , "roomflags"   , POS_DEAD    , do_toggle       , LVL_IMMORT , SCMD_SHOWVNUMS },
	{ "roomwho"			, "roomw"			, POS_DEAD	  , do_roomwhos			, 0			 , 0			  },

    { "/"           , "/"           , POS_STANDING , do_recall       , 0          , 0              },
    { "radio"       , "radio"       , POS_RESTING , do_radio        , 0          , 0              },
    { "radiosnoop"  , "radiosnoop"  , POS_DEAD    , do_toggle       , LVL_GRGOD  , SCMD_RADIOSNOOP},
    { "rsay"        , "rsay"        , POS_RESTING , do_gen_comm     , 0          , SCMD_RADIO     },
    { "rtalk"       , "rtalk"       , POS_RESTING , do_gen_comm     , 0          , SCMD_RADIO     },
    { "recall"      , "recall"      , POS_STANDING , do_recall       , 0          , 0              },
    { "recent"      , "recent"      , POS_DEAD    , do_recent       , LVL_GRGOD  , 0              },
    { "redeem"      , "redeem"      , POS_STANDING, do_not_here     , 0          , 0              },
    { "reimp"       , "reimp"       , POS_DEAD    , do_reimp        , 0          , 0              },
	{ "repair"		, "repair"		, POS_STANDING, do_repair		, 0			 , 0			  },
    { "repb"        , "repb"        , POS_SLEEPING, do_board        , 0          , SCMD_REPB      },
    { "restring"    , "restring"    , POS_DEAD    , do_restring     , LVL_IMMORT , 0              },
	{ "rupture"		, "rupture"		, POS_FIGHTING, do_rupture		, 0			 , 0			  },

    { "say"         , "s"           , POS_RESTING , do_say          , 0          , 0              },
    { "sacrifice"   , "sac"         , POS_STANDING, do_scavenger    , 0          , 0              },
    { "score"       , "sc"          , POS_DEAD    , do_score        , 0          , 0              },
    { "scavenger"   , "sca"         , POS_STANDING, do_scavenger    , 0          , 0              },
    { "scopy"       , "scopy"       , POS_DEAD    , do_oasis_copy   , LVL_GOD    , CON_SEDIT      },
    { "sit"         , "si"          , POS_FOCUSING, do_sit          , 0          , 0              },
    { "'"           , "'"           , POS_RESTING , do_say          , 0          , 0              },
    { "save"        , "sav"         , POS_SLEEPING, do_save         , 0          , 0              },
    { "saveall"     , "saveall"     , POS_DEAD    , do_saveall      , LVL_BUILDER, 0              },
    { "salvage"     , "salvage"     , POS_STANDING, do_salvageitem  , LVL_GOD    , 0              },
	{ "search"		, "search"		, POS_STANDING,	do_search		, 0			 , 0			  },
    { "sell"        , "sell"        , POS_STANDING, do_not_here     , 0          , 0              },
    { "sedit"       , "sedit"       , POS_DEAD    , do_oasis_sedit  , LVL_GOD    , 0              },
    { "send"        , "send"        , POS_SLEEPING, do_send         , LVL_GOD    , 0              },
    { "set"         , "set"         , POS_DEAD    , do_set          , LVL_GOD    , 0              },
	{ "shards"		, "shards"		, POS_DEAD	  , do_shards		, 0			 , 0			  },
    { "shout"       , "sho"         , POS_RESTING , do_gen_comm     , 0          , SCMD_SHOUT     },
    { "show"        , "show"        , POS_DEAD    , do_show         , 1 , 0              },
    { "shutdow"     , "shutdow"     , POS_DEAD    , do_shutdown     , LVL_CIMP   , 0              },
    { "shutdown"    , "shutdown"    , POS_DEAD    , do_shutdown     , LVL_CIMP   , SCMD_SHUTDOWN  },
    { "sip"         , "sip"         , POS_RESTING , do_drink        , 0          , SCMD_SIP       },
    { "skills"      , "skills"      , POS_DEAD    , do_skills       , 0          , 0              },
    { "skillset"    , "skillset"    , POS_SLEEPING, do_skillset     , LVL_GRGOD  , 0              },
	{ "skilltree"	, "skilltree"	, POS_DEAD	  , do_skilltree	, 0			 , 0			  },
    { "slash"       , "slash"       , POS_FIGHTING, do_slash         , 0          , 0              },
    { "sleep"       , "sl"          , POS_SLEEPING, do_sleep        , 0          , 0              },
    { "slist"       , "slist"       , POS_SLEEPING, do_oasis_list   , LVL_BUILDER, SCMD_OASIS_SLIST },
    { "sneak"       , "sneak"       , POS_STANDING, do_sneak        , 1          , 0              },
	{ "snipe"		, "snipe"		, POS_STANDING, do_snipe		, 1			 , 0			  },
    { "snoop"       , "snoop"       , POS_DEAD    , do_snoop        , LVL_GRGOD  , 0              },
    { "socials"     , "socials"     , POS_DEAD    , do_commands     , 0          , SCMD_SOCIALS   },
    { "split"       , "split"       , POS_SITTING , do_split        , 1          , 0              },
    { "stand"       , "st"          , POS_SLEEPING, do_stand        , 0          , 0              },
    { "stat"        , "stat"        , POS_DEAD    , do_stat         , LVL_IMMORT , 0              },
	{ "storage"		, "storage"		, POS_DEAD	  , do_storage		, 0			 , 0			  },
    { "switch"      , "switch"      , POS_DEAD    , do_switch       , LVL_GRGOD  , 0              },
    { "scan"        , "scan"        , POS_SITTING , do_scan         , 0          , 0              },
    { "shopscan"    , "shopscan"    , POS_STANDING, do_not_here     , 0          , 0              },
    { "showdbg"     , "showdbg"     , POS_DEAD    , do_toggle       , LVL_GOD    , SCMD_SHOWDBG   },
    { "slay"        , "slay"        , POS_DEAD    , do_slay         , LVL_GOD    , 0              },
    { "slowns"      , "slowns"      , POS_DEAD    , do_toggle       , LVL_IMPL   , SCMD_SLOWNS    },
	{ "stop"		, "stop"		, POS_DEAD	  , do_stop			, 0			 , 0			  },
    { "stun"        , "stun"        , POS_FIGHTING, do_stun         , 0          , 0              },
	{ "suicid"		, "suicid"		, POS_DEAD	  , do_suicid		, 0			 , 0			  },
    { "suicide"     , "suicide"     , POS_DEAD    , do_suicide      , 0          , 0              },
    { "syslog"      , "syslog"      , POS_DEAD    , do_toggle       , LVL_GOD    , SCMD_SYSLOG    },

    { "tell"        , "t"           , POS_DEAD    , do_tell         , 0          , 0              },
    { "tag"         , "tag"         , POS_STANDING, do_tag          , 0          , 0              },
    { "take"        , "ta"          , POS_RESTING , do_get          , 0          , 0              },
    { "taste"       , "tas"         , POS_RESTING , do_eat          , 0          , SCMD_TASTE     },
	{ "target"		, "target"		, POS_STANDING, do_aim			, 0			 , 0			  },
    { "teleport"    , "tele"        , POS_DEAD    , do_teleport     , LVL_GOD    , 0              },
    { "tedit"       , "tedit"       , POS_DEAD    , do_tedit        , LVL_GOD    , 0              },
//    { "tattach"     , "tattach"     , POS_DEAD    , do_tattach      , LVL_BUILDER, 0              },
//    { "tdetach"     , "tdetach"     , POS_DEAD    , do_tdetach      , LVL_BUILDER, 0              },
    { "thaw"        , "thaw"        , POS_DEAD    , do_wizutil      , LVL_FREEZE , SCMD_THAW      },
	{ "throw"		, "throw"		, POS_FIGHTING, do_throw		, 0			 , 0			  },
    { "title"       , "title"       , POS_DEAD    , do_title        , 0          , 0              },
    { "time"        , "time"        , POS_DEAD    , do_time         , 0          , 0              },
    { "toggle"      , "toggle"      , POS_DEAD    , do_toggle       , 0          , 0              },
    { "track"       , "track"       , POS_STANDING, do_track        , 0          , 0              },
    { "transfer"    , "transfer"    , POS_SLEEPING, do_trans        , LVL_IMMORT , 0              },
    { "trigedit"    , "trigedit"    , POS_DEAD    , do_oasis_trigedit, LVL_BUILDER, 0             },
    { "typo"        , "typo"        , POS_DEAD    , do_ibt    , 0          , SCMD_TYPO      },
    { "tlist"       , "tlist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_TLIST },
    { "tcopy"       , "tcopy"       , POS_DEAD    , do_oasis_copy   , LVL_GOD    , CON_TRIGEDIT   },
    { "tstat"       , "tstat"       , POS_DEAD    , do_tstat        , LVL_BUILDER, 0              },
//    { "tag"         , "tag"         , POS_STANDING, do_tag          , 0          , 0              },
    { "transport"   , "transport"   , POS_SLEEPING, do_transport    , 0          , 0              },

    { "unlock"      , "unlock"      , POS_SITTING , do_gen_door     , 0          , SCMD_UNLOCK    },
    { "ungroup"     , "ungroup"     , POS_DEAD    , do_ungroup      , 0          , 0              },
    { "unban"       , "unban"       , POS_DEAD    , do_unban        , LVL_GRGOD  , 0              },
    { "unaffect"    , "unaffect"    , POS_DEAD    , do_wizutil      , LVL_GOD    , SCMD_UNAFFECT  },
	{ "uninsure"	, "uninsure"	, POS_DEAD	  , do_uninsure	    , 0			 , 0			  },
    { "unwield"      , "unw"         , POS_RESTING , do_unwield       , 0          , 0              },

    { "uptime"      , "uptime"      , POS_DEAD    , do_date         , LVL_IMMORT , SCMD_UPTIME    },
    { "use"         , "use"         , POS_SITTING , do_use          , 1          , SCMD_USE       },
    { "users"       , "users"       , POS_DEAD    , do_users        , LVL_GOD    , 0              },
    { "unaffect_room", "unaffect_room", POS_DEAD  , do_wizutil      , LVL_GOD    , SCMD_UNAFFECTROOM },
    { "units"       , "units"       , POS_FOCUSING, do_units        , 0          , 0              },
    { "unload"      , "unload"      , POS_FOCUSING, do_unload       , 0          , 0              },
    { "upgrade"     , "upgrade"     , POS_STANDING, do_not_here     , 0          , 0              },

    { "value"       , "val"         , POS_STANDING, do_not_here     , 0          , 0              },
    { "version"     , "ver"         , POS_DEAD    , do_gen_ps       , 0          , SCMD_VERSION   },
    { "visible"     , "vis"         , POS_RESTING , do_visible      , 1          , 0              },
    { "vnum"        , "vnum"        , POS_DEAD    , do_vnum         , LVL_IMMORT , 0              },
    { "vstat"       , "vstat"       , POS_DEAD    , do_vstat        , LVL_IMMORT , 0              },
    { "vdelete"     , "vdelete"     , POS_DEAD    , do_vdelete      , LVL_BUILDER, 0              },
    { "vobj"        , "vobj"        , POS_DEAD    , do_vobj         , LVL_BUILDER, 0              },
	{ "vrestore"	, "vrestore"	, POS_DEAD	  , do_vrestore		, LVL_IMPL	 , 0			  },
    { "vwear"       , "vw"          , POS_DEAD    , do_vwear        , LVL_BUILDER, 0              },
    { "ventriloquate" , "ventril"   , POS_SITTING , do_ventriloquate, 0          , 0              },

    { "wake"        , "wake"        , POS_SLEEPING, do_wake         , 0          , 0              },
    { "wear"        , "wea"         , POS_RESTING , do_wear         , 0          , 0              },
    { "weather"     , "weather"     , POS_RESTING , do_weather      , 0          , 0              },
    { "who"         , "wh"          , POS_DEAD    , do_who          , 0          , 0              },
    { "whoami"      , "whoami"      , POS_DEAD    , do_gen_ps       , 0          , SCMD_WHOAMI    },
    { "whois"       , "whoi"        , POS_DEAD    , do_whois        , 0          , 0              },
    { "where"       , "where"       , POS_RESTING , do_where        , 1          , 0              },
    { "whisper"     , "whisper"     , POS_RESTING , do_spec_comm    , 0          , SCMD_WHISPER   },
    { "wield"       , "wie"         , POS_RESTING , do_wield        , 0          , 0              },
    { "withdraw"    , "withdraw"    , POS_STANDING, do_not_here     , 1          , 0              },
    { "wiznet"      , "wiz"         , POS_DEAD    , do_wiznet       , LVL_IMMORT , 0              },
    { ";"           , ";"           , POS_DEAD    , do_wiznet       , LVL_IMMORT , 0              },
    { "wizhelp"     , "wizhelp"     , POS_SLEEPING, do_commands     , LVL_IMMORT , SCMD_WIZHELP   },
    { "wizlist"     , "wizlist"     , POS_DEAD    , do_gen_ps       , 0          , SCMD_IMMLIST   },
    { "wizupdate"   , "wizupde"     , POS_DEAD    , do_wizupdate    , LVL_GRGOD  , 0              },
    { "wizlock"     , "wizlock"     , POS_DEAD    , do_wizlock      , LVL_CIMP   , 0              },
    { "write"       , "write"       , POS_STANDING, do_write        , 1          , 0              },
    { "wimpy"       , "wimpy"       , POS_DEAD    , do_toggle       , 0          , SCMD_WIMPY     },
	{ "whistle"		, "whistle"		, POS_STANDING, do_whistle		, 0			 , 0			  },

    { "zreset"      , "zreset"      , POS_DEAD    , do_zreset       , LVL_BUILDER, 0              },
    { "zedit"       , "zedit"       , POS_DEAD    , do_oasis_zedit  , LVL_BUILDER, 0              },
    { "zlist"       , "zlist"       , POS_DEAD    , do_oasis_list   , LVL_BUILDER, SCMD_OASIS_ZLIST },
    { "zlock"       , "zlock"       , POS_DEAD    , do_zlock        , LVL_GOD    , 0              },
    { "zunlock"     , "zunlock"     , POS_DEAD    , do_zunlock      , LVL_GOD    , 0              },
    { "zcheck"      , "zcheck"      , POS_DEAD    , do_zcheck       , LVL_GOD    , 0              },
    { "zpurge"      , "zpurge"      , POS_DEAD    , do_zpurge       , LVL_BUILDER, 0              },
	{ "."			, "."			, POS_DEAD	  , do_drive		, 0			 , 0			  },
    // this must be last
    { "\n"          , "zzzzzzz"     , 0           , 0               , 0          , 0              }
};

  /* Thanks to Melzaren for this change to allow DG Scripts to be attachable
   *to player's while still disallowing them to manually use the DG-Commands. */
  const struct mob_script_command_t mob_script_commands[] = {

  /* DG trigger commands. minimum_level should be set to -1. */
  { "masound"  , do_masound  , 0 },
  { "mkill"    , do_mkill    , 0 },
  { "mjunk"    , do_mjunk    , 0 },
  { "mdamage"  , do_mdamage  , 0 },
  { "mdoor"    , do_mdoor    , 0 },
  { "mecho"    , do_mecho    , 0 },
  { "mrecho"   , do_mrecho   , 0 },
  { "mechoaround", do_mechoaround , 0 },
  { "msend"    , do_msend    , 0 },
  { "mload"    , do_mload    , 0 },
  { "mpurge"   , do_mpurge   , 0 },
  { "mgoto"    , do_mgoto    , 0 },
  { "mat"      , do_mat      , 0 },
  { "mteleport", do_mteleport, 0 },
  { "mforce"   , do_mforce   , 0 },
  { "mhunt"    , do_mhunt    , 0 },
  { "mremember", do_mremember, 0 },
  { "mforget"  , do_mforget  , 0 },
  { "mtransform", do_mtransform , 0 },
  { "mzoneecho", do_mzoneecho, 0 },
  { "mfollow"  , do_mfollow  , 0 },
  { "\n" , do_not_here , 0 } };

int script_command_interpreter(struct char_data *ch, char *arg) {
  /* DG trigger commands */

  int i;
  char first_arg[MAX_INPUT_LENGTH];
  char *line;

  skip_spaces(&arg);
  if (!*arg)
  return 0;

  line = any_one_arg(arg, first_arg);

  for (i = 0; *mob_script_commands[i].command_name != '\n'; i++)
  if (!str_cmp(first_arg, mob_script_commands[i].command_name))
  break; // NB - only allow full matches.

  if (*mob_script_commands[i].command_name == '\n')
  return 0; // no matching commands.

  /* Poiner to the command? */
  ((*mob_script_commands[i].command_pointer) (ch, line, 0,
  mob_script_commands[i].subcmd));
  return 1; // We took care of execution. Let caller know.
}

const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

static int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;  /* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


/* This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /* special case to handle one-character, non-alphanumeric commands; requested
   * by many people so "'hi" or ";godnet test" is possible. Patch sent by Eric
   * Green and Stefan Wasilewski. */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. Otherwise, find the command. */
  {
    int cont;                                            /* continue the command checks */
    cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
    if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
    if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
    if (cont) return;                                    /* yes, command trigger took over */
  }

  /* Allow IMPLs to switch into mobs to test the commands. */
   if (IS_NPC(ch) && ch->desc && GET_LEVEL(ch->desc->original) >= LVL_IMPL) {
     if (script_command_interpreter(ch, argument))
       return;
   }

  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if(complete_cmd_info[cmd].command_pointer != do_action &&
       !strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
        break;

  /* it's not a 'real' command, so it's a social */

  if(*complete_cmd_info[cmd].command == '\n')
    for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (complete_cmd_info[cmd].command_pointer == do_action &&
          !strncmp(complete_cmd_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
          break;

  if (*complete_cmd_info[cmd].command == '\n') {
    int found = 0;
    send_to_char(ch, "Invalid command, please type 'commands' for more info.\r\n");
	mudlog(CMP, LVL_IMMORT, TRUE, "Invalid command %s by %s.", argument, GET_NAME(ch));
    for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    {
      if (*arg != *cmd_info[cmd].command || cmd_info[cmd].minimum_level > GET_LEVEL(ch))
        continue;

      /* Only apply levenshtein counts if the command is not a trigger command. */
      if ( (levenshtein_distance(arg, cmd_info[cmd].command) <= 2) &&
           (cmd_info[cmd].minimum_level >= 0) )
      {
        if (!found)
        {
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", cmd_info[cmd].command);
      }
    }
  }
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (AFF_FLAGGED(ch, AFF_STUN) && (GET_LEVEL(ch) < (LVL_IMMORT)))
    send_to_char(ch, "@WYou see nothing but stars!@n\r\n");
  else if (AFF_FLAGGED(ch, AFF_PARALYZE) || AFF_FLAGGED(ch, AFF_PETRIFY))
	send_to_char(ch, "@WYou are paralyzed and cannot do anything!@n\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
	case POS_FOCUSING:
      send_to_char(ch, "You are too focused to do anything!\r\n");
      break;
	case POS_MEDITATING:
      send_to_char(ch, "But that would disturb your meditation!\r\n");
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
}

/* Routines to handle aliasing. */
static struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}

void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      save_char(ch);
      send_to_char(ch, "Alias ready.\r\n");
    }
  }
}

/* Valid numeric replacements are only $1 .. $9 (makes parsing a little easier,
 * and it's not that much of a limitation anyway.)  Also valid is "$*", which
 * stands for the entire original line after the alias. ";" is used to delimit
 * commands. */
#define NUM_TOKENS       9

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}

/* Given a character and a string, perform alias replacement on it.
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue. */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}

/* Various other parsing utilities. */

/* Searches an array of strings for a target string.  "exact" can be 0 or non-0,
 * depending on whether or not the match must be exact for it to be returned.
 * Returns -1 if not found; 0..n otherwise.  Array must be terminated with a
 * '\n' so it knows to stop searching. */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae. */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int is_number(const char *str)
{
  if(*str == '-')
     str++;
  if(!*str)
     return (0);
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}

/* Given a string, change all instances of double dollar signs ($$) to single
 * dollar signs ($).  When strings come in, all $'s are changed to $$'s to
 * avoid having users be able to crash the system if the inputted string is
 * eventually sent to act().  If you are using user input to produce screen
 * output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE act() FUNCTION
 * (i.e., do_gecho, do_title, but NOT do_say), you can call
 * delete_doubledollar() to make the output look correct.
 * Modifies the string in-place. */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}

int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}

int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}

/* Copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/* one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word. No longer ignores fill words.  -dak */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}

/* Same as one_argument except that it doesn't ignore fill words. */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/* Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

/* Determine if a given string is an abbreviation of another.
 * Returns 1 if arg1 is an abbreviation of arg2. */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/* Return first space-delimited token in arg1; remainder of string in arg2.
 * NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}

int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}

/* Stuff for controlling the non-playing sockets (get name, pwd etc).
 * This function needs to die. */
static int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}

#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
static int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;
  int pref_temp = 0; /* for "last" log */
  int id = GET_IDNUM(d->character);

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      pref_temp=GET_PREF(k->character);
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */
      pref_temp=GET_PREF(k->character);

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /* Now, go through the character list, deleting all characters that are not
  * already marked for deletion from the above step (i.e., in the CON_HANGUP
  * state), and have not already been selected as a target for switching into.
  * In addition, if we haven't already found a target, choose one if one is
  * available (while still deleting the other duplicates, though theoretically
  * none should be able to exist). */
  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      pref_temp = GET_PREF(ch);
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target) {
    GET_PREF(d->character) = rand_number(1, 128000);
    if (GET_HOST(d->character))
      free(GET_HOST(d->character));
    GET_HOST(d->character) = strdup(d->host);
    return 0;
  }

  if (GET_HOST(target)) free(GET_HOST(target));
  GET_HOST(target) = strdup(d->host);

  GET_PREF(target) = pref_temp;
  add_llog_entry(target, LAST_RECONNECT);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;
  MXPSendTag( d, "<VERSION>" );

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(0, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    if (has_mail(GET_IDNUM(d->character)))
      write_to_output(d, "You have mail waiting.\r\n");
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* New Char dupe-check called at the start of character creation */
/*static bool perform_new_char_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  bool found = FALSE;

  * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. *

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

     Do the player names match?
    if (!strcmp(GET_NAME(k->character), GET_NAME(d->character))) {
       Check the other character is still in creation? *
      if ((STATE(k) > CON_PLAYING) && (STATE(k) < CON_QCLASS)) {
         Boot the older one *
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        mudlog(NRM, LVL_GOD, TRUE, "Multiple logins detected in char creation for %s.", GET_NAME(d->character));

        found = TRUE;
      } else {
         Something went VERY wrong, boot both chars *
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        d->character->desc = NULL;
        d->character = NULL;
        d->original = NULL;
        write_to_output(d, "\r\nSorry, due to multiple connections, all your connections are being closed.\r\n");
        write_to_output(d, "\r\nPlease reconnect.\r\n");
        STATE(d) = CON_CLOSE;

        mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Multiple logins with 1st in-game and the 2nd in char creation.");

        found = TRUE;
      }
    }
  }
  return (found);
}*/
int enter_player_game_death(struct descriptor_data *d)
{
	int load_room;
	load_room = rand_number(400, 409);
    char_to_room(d->character, real_room(load_room));

	if (INSURANCE(d->character) == 5) {
		GET_HIT(d->character) = (GET_MAX_HIT(d->character) / 3);
		GET_PSI(d->character) = (GET_MAX_PSI(d->character) / 3);
		GET_MOVE(d->character) = (GET_MAX_MOVE(d->character) / 3);
	}
	if (INSURANCE(d->character) == 4) {
		GET_HIT(d->character) = (GET_MAX_HIT(d->character) / 4);
		GET_PSI(d->character) = (GET_MAX_PSI(d->character) / 4);
		GET_MOVE(d->character) = (GET_MAX_MOVE(d->character) / 4);
	}
	if (INSURANCE(d->character) == 3) {
		GET_HIT(d->character) = (GET_MAX_HIT(d->character) / 6);
		GET_PSI(d->character) = (GET_MAX_PSI(d->character) / 6);
		GET_MOVE(d->character) = (GET_MAX_MOVE(d->character) / 6);
	}
	if (INSURANCE(d->character) == 2) {
		GET_HIT(d->character) = (GET_MAX_HIT(d->character) / 8);
		GET_PSI(d->character) = (GET_MAX_PSI(d->character) / 8);
		GET_MOVE(d->character) = (GET_MAX_MOVE(d->character) / 8);
	}
	if (INSURANCE(d->character) == 1) {
		GET_HIT(d->character) = (GET_MAX_HIT(d->character) / 10);
		GET_PSI(d->character) = (GET_MAX_PSI(d->character) / 10);
		GET_MOVE(d->character) = (GET_MAX_MOVE(d->character) / 10);
	}
	if (INSURANCE(d->character) == 0) {
		GET_HIT(d->character) = 1;
		GET_PSI(d->character) = 1;
		GET_MOVE(d->character) = 1;
	}
    look_at_room(d->character, 0);
	return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
    int load_result;
    room_vnum load_room;
	bool foundskillmem = FALSE;
	struct char_data *ch = d->character;
	struct gain_data *glist = global_gain;

    reset_char(d->character);

	// Skill/Psionic Memory to protect against botting - Gahan

	if (!IS_NPC(ch)) {
		struct gain_data *curr = global_gain;
	    struct gain_data *tmpgain;
		while(glist && !foundskillmem) {
			if(glist->playerid == GET_IDNUM(ch)) {
			    foundskillmem = TRUE;
			}
			curr = glist;
			glist = glist->next;
		}
		if (foundskillmem == FALSE) {
			tmpgain = (struct gain_data*)malloc(sizeof(struct gain_data));
			tmpgain->playerid = GET_IDNUM(ch);
			tmpgain->playerlevel = GET_LEVEL(ch);
			tmpgain->use = NULL;
			tmpgain->next = NULL;
			if(!global_gain)
			    global_gain = tmpgain;
			else
			    curr->next = tmpgain;
		}
	}

    if (PLR_FLAGGED(d->character, PLR_INVSTART))
        GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
    // We have to place the character in a room before equipping them
    // or equip_char() will gripe about the person in NOWHERE
    if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
        load_room = real_room(load_room);

    // If char was saved with NOWHERE, or real_room above failed...
    if (load_room == NOWHERE) {
        if (GET_LEVEL(d->character) >= LVL_IMMORT)
            load_room = r_immort_start_room;
        else
            load_room = r_mortal_start_room;
    }

    if (PLR_FLAGGED(d->character, PLR_FROZEN))
        load_room = r_frozen_start_room;
    // copyover
    GET_ID(d->character) = GET_IDNUM(d->character);

    // find_char helper
    add_to_lookup_table(GET_ID(d->character), (void *)d->character);

    // After moving saving of variables to the player file, this should only
    // be called in case nothing was found in the pfile. If something was
    // found, SCRIPT(ch) will be set.
    if (!SCRIPT(d->character))
        read_saved_vars(d->character);

    d->character->next = character_list;
    character_list = d->character;
    char_to_room(d->character, load_room);
    load_result = Crash_load(d->character);
	// purge all old bionics
	//bionicspurge(d->character);
    // restore bionics
    bionics_restore(d->character);
    save_char(d->character);

    // Check for a login trigger in the players' start room
    login_wtrigger(&world[IN_ROOM(d->character)], d->character);

   //MXPSendTag( d, "<VERSION>" ); 

    return (load_result);
}

EVENTFUNC(get_protocols)
{
	struct descriptor_data *d;
	struct mud_event_data *pMudEvent;
	char buf[MAX_STRING_LENGTH];
	char greet_copy[MAX_STRING_LENGTH];
	int len;
	
	if (event_obj == NULL)
	  return 0;
	  
	pMudEvent = (struct mud_event_data *) event_obj;
	d = (struct descriptor_data *) pMudEvent->pStruct;  
	
	/* Clear extra white space from the "protocol scroll" */
	write_to_output(d, "[H[J");
	*greet_copy = '\0';
	len = snprintf(buf, MAX_STRING_LENGTH,   "\tO[\toClient\tO] \tw%s\tn | ", d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);
	
	if (d->pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt)
	  len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \tw256\tn | ");
	else if (d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
      len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \twAnsi\tn | ");
	else
      len += snprintf(buf + len, MAX_STRING_LENGTH - len, "[Colors] No Color | ");
 
	len += snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toMXP\tO] \tw%s\tn | ", d->pProtocol->bMXP ? "Yes" : "No");
	len += snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toMSDP\tO] \tw%s\tn | ", d->pProtocol->bMSDP ? "Yes" : "No");	  
	len += snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toATCP\tO] \tw%s\tn\r\n\r\n", d->pProtocol->bATCP ? "Yes" : "No");
		 
	write_to_output(d, buf, 0);	 
		  
	sprintf(greet_copy, "%s", GREETINGS0);
    proc_colors(greet_copy, MAX_STRING_LENGTH, TRUE);
    write_to_output(d, "%s", greet_copy);

	STATE(d) = CON_GET_NAME;
    free_mud_event(pMudEvent);
	return 0;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result;	/* Overloaded variable */
  int player_i;
  struct descriptor_data *d2;
  int num_links = 0;
  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char *);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_CEDIT, cedit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_AEDIT, aedit_parse },
    { CON_HEDIT, hedit_parse },
    { CON_QEDIT, qedit_parse },
    { CON_PREFEDIT, prefedit_parse },
    { CON_IBTEDIT, ibtedit_parse },
	{ CON_CUSTOM, custom_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

  /* Quick check for the OLC states. */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
    switch (STATE(d)) {
		case CON_GET_PROTOCOL:
			write_to_output(d, "Collecting Protocol Information... Please Wait.\r\n"); 
			return;
		break;   
        case CON_GET_NAME:        /* wait for input of name */
            if (d->character == NULL) {
                CREATE(d->character, struct char_data, 1);
                clear_char(d->character);
                CREATE(d->character->player_specials, struct player_special_data, 1);
                GET_HOST(d->character) = strdup(d->host);
                d->character->desc = d;
            }

            if (!*arg)
                STATE(d) = CON_CLOSE;
            else {
                char buf[MAX_INPUT_LENGTH];
                char tmp_name[MAX_INPUT_LENGTH];

                if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
                    strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
                    fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {    /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
                    write_to_output(d, "@RInvalid name, please try another.@n\r\n@DName:@n \r\n");
                    return;
                }

                if ((player_i = load_char(tmp_name, d->character)) > -1) {
                    GET_PFILEPOS(d->character) = player_i;

                    if (PLR_FLAGGED(d->character, PLR_DELETED)) {

                        // Make sure old files are removed so the new player doesn't get the
                        // deleted player's equipment
                        if ((player_i = get_ptable_by_name(tmp_name)) >= 0) {
                            log("removing old player file %s so new player doesn't get deleted player's equipment.", GET_NAME(d->character));
                            remove_player(player_i);
                        }

                        // We get a false positive from the original deleted character
                        free_char(d->character);

                        // Check for multiple creations
                        if (!valid_name(tmp_name)) {
                            write_to_output(d, "Invalid name, please try another.\r\nName: \r\n");
                            return;
                        }

                        CREATE(d->character, struct char_data, 1);
                        clear_char(d->character);
                        CREATE(d->character->player_specials, struct player_special_data, 1);

                        if (GET_HOST(d->character))
                            free(GET_HOST(d->character));

                        GET_HOST(d->character) = strdup(d->host);

                        d->character->desc = d;
                        CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
                        strcpy(d->character->player.name, CAP(tmp_name));    /* strcpy: OK (size checked above) */
                        GET_PFILEPOS(d->character) = player_i;

                        write_to_output(d, "@WDid I get that right, @R%s @W(@CY@W/@CN@W)?@n \r\n", tmp_name);
                        STATE(d) = CON_NAME_CNFRM;
                    }
                    else {
                        // undo it just in case they are set
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
                        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
                        REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
                        d->character->player.time.logon = time(0);
                        write_to_output(d, "@RPassword:@n \r\n");
                        echo_off(d);
                        d->idle_tics = 0;
                        STATE(d) = CON_PASSWORD;
                    }
                }
                else {
                    // player unknown -- make new character

                    // Check for multiple creations of a character
                    if (!valid_name(tmp_name)) {
                        write_to_output(d, "@RInvalid name, please try another.\r\n@DName:@n \r\n");
                        return;
                    }
//                        SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_1);
//                        SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_2);
//bosstone 1/24/2017 color off for creation test
                    CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
                    strcpy(d->character->player.name, CAP(tmp_name));    /* strcpy: OK (size checked above) */

                    write_to_output(d, "@DDid I get that right,@w %s @D(@CY@D/@CN@D)??@n \r\n", tmp_name);
                    STATE(d) = CON_NAME_CNFRM;
                }
            }
            break;

        case CON_NAME_CNFRM:        /* wait for conf. of new name    */
            if (UPPER(*arg) == 'Y') {

                if (isbanned(d->host) >= BAN_NEW) {
                    mudlog(NRM, LVL_GOD, TRUE, "@RRequest for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
                    write_to_output(d, "Sorry, new characters are not allowed from your site at this time!\r\n");
                    write_to_output(d, "If you would like to make a new character, please send an email to\r\n");
                    write_to_output(d, "cyber@cyberassault.org or if you already have a character\r\n");
                    write_to_output(d, "on CyberASSAULT, simply log on and ask one of the Imms.@n\r\n");
                    STATE(d) = CON_CLOSE;
                    return;
                }

                if (circle_restrict) {
                    write_to_output(d, "@RSorry, new players can't be created at the moment.@n\r\n");
                    mudlog(NRM, LVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
                    STATE(d) = CON_CLOSE;
                    return;
                }
				perform_dupe_check(d);
                write_to_output(d, "@GNew character.@n\r\n@DGive me a password for @w%s@n@D:@n \r\n", GET_PC_NAME(d->character));
                echo_off(d);
                STATE(d) = CON_NEWPASSWD;
            }
            else if (*arg == 'n' || *arg == 'N') {
                write_to_output(d, "@DOkay, what IS it, then? @n\r\n");
                free(d->character->player.name);
                d->character->player.name = NULL;
                STATE(d) = CON_GET_NAME;
            }
            else
                write_to_output(d, "@RPlease type Yes or No:@n ");
            break;

        case CON_PASSWORD:        /* get pwd for known player      */
            // To really prevent duping correctly, the player's record should be reloaded
            // from disk at this point (after the password has been typed).  However I'm
            // afraid that trying to load a character over an already loaded character is
            // going to cause some problem down the road that I can't see at the moment.
            // So to compensate, I'm going to (1) add a 15 or 20-second time limit for
            // entering a password, and (2) re-add the code to cut off duplicates when a
            // player quits.  JE 6 Feb 96

            echo_on(d);    /* turn echo back on */

            // New echo_on() eats the return on telnet. Extra space better than none
            write_to_output(d, "\r\n");

            if (!*arg)
                STATE(d) = CON_CLOSE;
            else {
                if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                    mudlog(BRF, LVL_GOD, TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
                    GET_BAD_PWS(d->character)++;
                    save_char(d->character);

                    if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {    /* 3 strikes and you're out. */
                        write_to_output(d, "@RWrong password... disconnecting.@n\r\n");
                        STATE(d) = CON_CLOSE;
                    }
                    else {
                        write_to_output(d, "@RWrong password or that player name is already taken.\r\nPassword: @n\r\n");
                        echo_off(d);
                    }

                    return;
                }

                // Password was correct
                load_result = GET_BAD_PWS(d->character);
                GET_BAD_PWS(d->character) = 0;
                d->bad_pws = 0;

                if (isbanned(d->host) == BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
                    write_to_output(d, "@RSorry, this char has not been cleared for login from your site!@n\r\n");
                    STATE(d) = CON_CLOSE;
                    mudlog(NRM, LVL_GOD, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
                    return;
                }

                if (GET_LEVEL(d->character) < circle_restrict) {
                    write_to_output(d, "@RThe game is temporarily restricted.. try again later.@n\r\n");
                    STATE(d) = CON_CLOSE;
                    mudlog(NRM, LVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
                    return;
                }

                // check and make sure no other copies of this player are logged in
                if (perform_dupe_check(d))
                    return;

                if (GET_LEVEL(d->character) >= LVL_IMMORT)
                    write_to_output(d, "%s", imotd);
                else
                    write_to_output(d, "%s", motd);

                if (GET_INVIS_LEV(d->character))
                    mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s has connected. (invis %d)", GET_NAME(d->character), GET_INVIS_LEV(d->character));
                else
                    mudlog(BRF, LVL_IMMORT, TRUE, "%s has connected.", GET_NAME(d->character));

                if (load_result) {
                    write_to_output(d, "\r\n\r\n\007\007\007%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
                        CCRED(d->character, C_SPR), load_result, (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
                    GET_BAD_PWS(d->character) = 0;
                }


                // special things done for Cyber "just to make sure"

                // this shoulda been done all along in class.c, leave it here for awhile 5/6/01 -Lump
                if (siteok_everyone)
                    SET_BIT_AR(PLR_FLAGS(d->character), PLR_SITEOK);

                // special quest shit that should be removed when they login next time... -Lump
                REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_TEMP_MORTAL); /* remove temporary mortal flag */
                REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_FROZEN);      /* remove freezetag flag */

                write_to_output(d, "\r\n@G*** PRESS RETURN:@n ");
                STATE(d) = CON_RMOTD;
            }
            break;

        case CON_NEWPASSWD:
        case CON_CHPWD_GETNEW:
            if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 || !str_cmp(arg, GET_PC_NAME(d->character))) {
                write_to_output(d, "\r\n@RIllegal password.\r\nPassword:@n \r\n");
                return;
            }

            strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);    /* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
            *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

            write_to_output(d, "\r\n@RPlease retype password: @n\r\n");

            if (STATE(d) == CON_NEWPASSWD)
                STATE(d) = CON_CNFPASSWD;
            else
                STATE(d) = CON_CHPWD_VRFY;
            break;

        case CON_CNFPASSWD:
        case CON_CHPWD_VRFY:
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                write_to_output(d, "\r\n@RPasswords don't match... start over.\r\nPassword:@n \r\n");

                if (STATE(d) == CON_CNFPASSWD)
                    STATE(d) = CON_NEWPASSWD;
                else
                    STATE(d) = CON_CHPWD_GETNEW;
                return;
            }

            echo_on(d);

            if (STATE(d) == CON_CNFPASSWD) {
                write_to_output(d, "\r\n@DWhat is your sex (@CM@D/@CF@D)?@n ");
                STATE(d) = CON_QSEX;
            }
            else {
                save_char(d->character);
                write_to_output(d, "\r\n@RDone.@n\r\n%s", CONFIG_MENU);
                STATE(d) = CON_MENU;
            }
            break;

        case CON_QSEX:        /* query sex of new user         */
            switch (*arg) {

                case 'm':
                case 'M':
                    d->character->player.sex = SEX_MALE;
                    break;

                case 'f':
                case 'F':
                    d->character->player.sex = SEX_FEMALE;
                    break;

                default:
                    write_to_output(d, "@RThat is not a valid sex..\r\nWhat IS your sex?@n ");
                    return;
            }

            write_to_output(d, "%s\r\n@DClass:@n ", class_menu);
            STATE(d) = CON_QCLASS;
            break;

        case CON_QCLASS:
				
            load_result = parse_class(arg, FALSE);

            if (load_result == CLASS_UNDEFINED) {
                write_to_output(d, "\r\n@RThat's not a valid class, please type the entire class name to select a class\r\n@DClass: @n");
                return;
            }
			if (load_result >=7) {
                write_to_output(d, "\r\n@RThat's not a valid class, please type the entire class name to select a class\r\n@DClass: @n");
                return;
			}
            else
                GET_CLASS(d->character) = load_result;

            if (d->olc) {
                free(d->olc);
                d->olc = NULL;
            }

            set_basestats(d->character);
            d->character->aff_abils = d->character->real_abils;

            if (GET_PFILEPOS(d->character) < 0)
                GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));

			if (IS_BORG(d->character))
				write_to_output(d, "%s", borg_classes);
			if (IS_MERCENARY(d->character))
				write_to_output(d, "%s", mercenary_classes);
			if (IS_CRAZY(d->character))
				write_to_output(d, "%s", crazy_classes);
			if (IS_CALLER(d->character))
				write_to_output(d, "%s", caller_classes);
			if (IS_STALKER(d->character))
				write_to_output(d, "%s", stalker_classes);
			if (IS_PREDATOR(d->character))
				write_to_output(d, "%s", predator_classes);
			if (IS_HIGHLANDER(d->character))
				write_to_output(d, "%s", highlander_classes);

			write_to_output(d, "\r\n@RReview the class progression and type 'yes' to continue, or 'no' to choose another class.@d\r\n");
			STATE(d) = CON_RUSURE;
			break;

		case CON_RUSURE:

			if (!strcmp(arg, "no") || !strcmp(arg, "NO")) {
				write_to_output(d, "%s\r\n@DClass:@n ", class_menu);
				STATE(d) = CON_QCLASS;
			}

			else if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
				// Now GET_NAME() will work properly
				init_char(d->character);
				save_char(d->character);
				save_player_index();

	            // make sure the last log is updated correctly
		        GET_PREF(d->character)= rand_number(1, 128000);
			    GET_HOST(d->character)= strdup(d->host);

	            write_to_output(d, "\r\n@WModifying your Stats\r\n\r\n");
		        write_to_output(d, "@CStr\r\n@W--- More strength increases your damroll and how much you can carry.\r\n");
			    write_to_output(d, "@CInt\r\n@W--- More intelligence increases your psi points and practice sessions.\r\n");
				write_to_output(d, "@CPcn\r\n@W--- More perception increases your hitroll.\r\n");
	            write_to_output(d, "@CDex\r\n@W--- More dexterity increases practice sessions and the effects of some skills.\r\n");
		        write_to_output(d, "@CCon\r\n@W--- More constitution increases hitpoints.@n\r\n");
			    write_to_output(d, "@CCha\r\n@W--- More charisma increases charming ability@n\r\n");

	            write_to_output(d, "\r\n Your stats are Str: [@C%d/%d@n] Int: [@C%d@n] Pcn: [@C%d@n] Dex: [@C%d@n] Con: [@C%d@n] Cha: [@C%d@n]",
		            GET_STR(d->character), GET_STR_ADD(d->character), GET_INT(d->character),
			        GET_PCN(d->character), GET_DEX(d->character), GET_CON(d->character), GET_CHA(d->character));

			    write_to_output(d, "\r\n\r\n @WYou now have @R8@W additional points with which you can allot in any manner\r\n");
				write_to_output(d, "to your stats.  Type the first three letters of the stat to which\r\n");
	            write_to_output(d, "you would like to add a point.  The maximum number for a stat is 20.@n\r\n");

		        d->character->player_specials->saved.attrib_add = 8;
			    save_char(d->character);
				STATE(d) = CON_QATTRIB;
				write_to_output(d, "\r\n@DStat to modify:@n \r\n");
			}
			else
				write_to_output(d, "\r\n@RPlease type 'yes' to continue, or 'no' to choose another class.@d\r\n");
			
			break;

        case CON_QATTRIB:
            if (d->character->player_specials->saved.attrib_add > 0) {
                d->character->player_specials->saved.attrib_add--;

                // skip white spaces
                for (; isspace(*arg); arg++);
                    *arg = LOWER(*arg);

                if (!strncmp(arg, "str", 3))
                    if (GET_STR(d->character) < 29)
                        d->character->real_abils.str++;
                    else
                        d->character->real_abils.str_add += 10;
                else if (!strncmp(arg, "STR", 3) && GET_STR(d->character) < 29)
                    d->character->real_abils.str++;
                else if (!strncmp(arg, "s", 3) && GET_STR(d->character) < 29)
                    d->character->real_abils.str++;
                else if (!strncmp(arg, "S", 3) && GET_STR(d->character) < 29)
                    d->character->real_abils.str++;
                else if (!strncmp(arg, "int", 3) && GET_INT(d->character) < 29)
                    d->character->real_abils.intel++;
                else if (!strncmp(arg, "INT", 3) && GET_INT(d->character) < 29)
                    d->character->real_abils.intel++;
                else if (!strncmp(arg, "I", 3) && GET_INT(d->character) < 29)
                    d->character->real_abils.intel++;
                else if (!strncmp(arg, "i", 3) && GET_INT(d->character) < 29)
                    d->character->real_abils.intel++;
                else if (!strncmp(arg, "pcn", 3) && GET_PCN(d->character) < 29)
                    d->character->real_abils.pcn++;
                else if (!strncmp(arg, "PCN", 3) && GET_PCN(d->character) < 29)
                    d->character->real_abils.pcn++;
                else if (!strncmp(arg, "p", 3) && GET_PCN(d->character) < 29)
                    d->character->real_abils.pcn++;
                else if (!strncmp(arg, "P", 3) && GET_PCN(d->character) < 29)
                    d->character->real_abils.pcn++;
                else if (!strncmp(arg, "dex", 3) && GET_DEX(d->character) < 29)
                    d->character->real_abils.dex++;
                else if (!strncmp(arg, "DEX", 3) && GET_DEX(d->character) < 29)
                    d->character->real_abils.dex++;
                else if (!strncmp(arg, "D", 3) && GET_DEX(d->character) < 29)
                    d->character->real_abils.dex++;
                else if (!strncmp(arg, "d", 3) && GET_DEX(d->character) < 29)
                    d->character->real_abils.dex++;
                else if (!strncmp(arg, "con", 3) && GET_CON(d->character) < 29)
                    d->character->real_abils.con++;
                else if (!strncmp(arg, "CON", 3) && GET_CON(d->character) < 29)
                    d->character->real_abils.con++;
                else if (!strncmp(arg, "c", 3) && GET_CON(d->character) < 29)
                    d->character->real_abils.con++;
                else if (!strncmp(arg, "C", 3) && GET_CON(d->character) < 29)
                    d->character->real_abils.con++;
                else if (!strncmp(arg, "ch", 3) && GET_CHA(d->character) < 29)
                    d->character->real_abils.cha++;
                else if (!strncmp(arg, "cha", 3) && GET_CHA(d->character) < 29)
                    d->character->real_abils.cha++;
                else if (!strncmp(arg, "CH", 3) && GET_CHA(d->character) < 29)
                    d->character->real_abils.cha++;
                else if (!strncmp(arg, "CHA", 3) && GET_CHA(d->character) < 29)
                    d->character->real_abils.cha++;
                else {
                    write_to_output(d, "\r\n@RThat is not a proper stat.@n\r\n");
                    d->character->player_specials->saved.attrib_add++;
                }

                d->character->aff_abils = d->character->real_abils;

                write_to_output(d, "\r\n@WYou now have @R%d@W left to add to stats.@n\r\n", d->character->player_specials->saved.attrib_add);
                write_to_output(d, "\n\r Your stats are now: Str: [@C%d/%d@n] Int: [@C%d@n] Pcn: [@C%d@n] Dex: [@C%d@n] Con: [@C%d@n] Cha: [@C%d@n]\r\n",
                    GET_STR(d->character), GET_STR_ADD(d->character), GET_INT(d->character),
                    GET_PCN(d->character), GET_DEX(d->character), GET_CON(d->character), GET_CHA(d->character));
            }

            if (d->character->player_specials->saved.attrib_add == 0) {

                STATE(d) = CON_QEXTRA;

                // what other options can we offer? bionics?
                write_to_output(d, "\n\r\n\r\n\r@WAdvanced Character Options:@n\n\r");
                write_to_output(d, "  @C1) Add 1 Point to Strength\n\r");
                write_to_output(d, "  2) Add 1 Point to Intelligence\n\r");
                write_to_output(d, "  3) Add 1 Point to Perception\n\r");
                write_to_output(d, "  4) Add 1 Point to Dexterity\n\r");
                write_to_output(d, "  5) Add 1 Point to Constitution\n\r");
                write_to_output(d, "  6) Start with 30 More Hit Points\n\r");
                write_to_output(d, "  7) Start with 30 More Psi Points\n\r");
                write_to_output(d, "  8) Start with 30 More Movement\n\r");
                write_to_output(d, "  9) Start with 30 More Practice Sessions\n\r");
                write_to_output(d, "  0) Add 1 Point to Charisma\n\r");
                write_to_output(d, "\n\r@WEnter Option:@n ");

            }
            else {
                write_to_output(d, "\r\n@DStat to modify: @n\r\n");
                STATE(d) = CON_QATTRIB;
            }

            break;

        case CON_QEXTRA:

            // skip whitespaces
            for (; isspace(*arg); arg++);

            *arg = LOWER(*arg);

            switch(atoi(arg)) {

                case 1:
                    if (GET_STR(d->character) < 29)
                        d->character->real_abils.str++;
                    else
                        d->character->real_abils.str_add += 10;
                    STATE(d) = CON_MENU;
                    break;

                case 2:
                    if (GET_INT(d->character) >= 29) {
                        write_to_output(d, "@RAttributes can't be more than 29!@n\r\n");
                        write_to_output(d, "\n\r@WEnter Option: @n\r\n");
                        return;
                    }
                    else {
                        d->character->real_abils.intel++;
                        STATE(d) = CON_MENU;
                    }
                    break;

                case 3:
                    if (GET_PCN(d->character) >= 29) {
                        write_to_output(d, "@RAttributes can't be more than 29!@n\r\n");
                        write_to_output(d, "\n\r@WEnter Option: @n\r\n");
                        return;
                    }
                    else {
                        d->character->real_abils.pcn++;
                        STATE(d) = CON_MENU;
                    }
                    break;

                case 4:
                    if (GET_DEX(d->character) >= 29) {
                        write_to_output(d, "@RAttributes can't be more than 29!@n\r\n");
                        write_to_output(d, "\n\r@WEnter Option:@n \r\n");
                        return;
                    }
                    else {
                        d->character->real_abils.dex++;
                        STATE(d) = CON_MENU;
                    }
                    break;

                case 5:
                    if (GET_CON(d->character) >= 29) {
                        write_to_output(d, "@RAttributes can't be more than 29!@n\r\n");
                        write_to_output(d, "\n\r@WEnter Option: @n\r\n");
                        return;
                    }
                    else {
                        d->character->real_abils.con++;
                        STATE(d) = CON_MENU;
                    }
                    break;

                case 6:
                    GET_MAX_HIT_PTS(d->character) += 60;
                    STATE(d) = CON_MENU;
                    break;

                case 7:
                    GET_MAX_PSI_PTS(d->character) += 60;
                    STATE(d) = CON_MENU;
                    break;

                case 8:
                    GET_MAX_MOVE(d->character) += 60;
                    STATE(d) = CON_MENU;
                    break;

                case 9:
                    GET_PRACTICES(d->character) += 35;
                    STATE(d) = CON_MENU;
                    break;

                case 0:
                    if (GET_CHA(d->character) >= 29) {
                        write_to_output(d, "@RAttributes can't be more than 29!@n\r\n");
                        write_to_output(d, "\n\r@WEnter Option: @n\r\n");
                        return;
                    }
                    else {
//                        d->character->real_abils.cha++;
                // what other options can we offer? bionics?
                write_to_output(d, "\n\r@RPlease make a valid choice.:@n\n\r");
                write_to_output(d, "\n\r\n\r\n\r@WAdvanced Character Options:@n\n\r");
                write_to_output(d, "  @C1) Add 1 Point to Strength\n\r");
                write_to_output(d, "  2) Add 1 Point to Intelligence\n\r");
                write_to_output(d, "  3) Add 1 Point to Perception\n\r");
                write_to_output(d, "  4) Add 1 Point to Dexterity\n\r");
                write_to_output(d, "  5) Add 1 Point to Constitution\n\r");
                write_to_output(d, "  6) Start with 60 More Hit Points\n\r");
                write_to_output(d, "  7) Start with 60 More Psi Points\n\r");
                write_to_output(d, "  8) Start with 60 More Movement\n\r");
                write_to_output(d, "  9) Start with 35 More Practice Sessions\n\r");
                write_to_output(d, "  0) Add 1 Point to Charisma\n\r");
                write_to_output(d, "\n\r@WEnter Option:@n \r\n");

                        STATE(d) = CON_QEXTRA;
                    }
                    break;

                default:
                    write_to_output(d, "\n\r@RThat's Not A Menu Choice!@n\n\r");
                    write_to_output(d, "@WEnter Option:@n \r\n");
                    break;
            }

/*            if (STATE(d) == CON_QCONSENT) {
                mudlog(NRM, LVL_GOD, TRUE, "%s [%s] new player.", GET_NAME(d->character), d->host);
                Crash_delete_file(GET_NAME(d->character));
                post_init_char(d->character);

                write_to_output(d, "\n\r@r*** Emergency Action Message *** Set DefCon 2 *** Emergency Action Message ***@n\n\n\r"
                        "@WCyberASSAULT is aimed for mature players.  As such, it deals with themes\n\r"
                        "which are of an adult nature.  These include but are not limited to warfare\n\r"
                        "in all its forms, death, comradery, friendship, and so on.  If you have\n\r"
                        "a condition (or other interference) which prevents you from separating\n\r"
                        "this fantasy game from reality, you must leave now.  We will not be held\n\r"
                        "responsible for any actions taken as a result of this fantasy game.\n\r");

                write_to_output(d, "\n\r@DCONSENT FORM:@n\n\r"
                        "@WBy entering this game, you agree to the following:\n\r"
                        "1) I am a legal adult (18 in USA), or play with consent of a legal guardian.\n\r"
                        "2) I realize any actions I take are my sole responsibility.\n\r"
                        "3) I realize I may be banned from the game if I violate the rules set\n\r"
                        "   out in the command 'policy'.\n\r"
                        "4) I know that racial insults and other such behavior is not tolerated.\n\r"
                        "5) I will agree to drink all of my milk.\n\r"
                        "6) I understand that CA addicts people for their own good.\r\n"
                        "Type '@Rconsent@n'@W to agree to the rules and enter the game:@n \r\n");
            }
            break;

        case CON_QCONSENT:
            // skip whitespaces
            for (; isspace(*arg); arg++);

            *arg = LOWER(*arg);
            if (!strcmp(arg, "consent")) {
                write_to_output(d, "\n\r");
                STATE(d) = CON_RMOTD;
            }
            else {
                write_to_output(d, "\n\r@RSorry, but all players must consent to the above paragraph\r\n"
                    "and agree to play by OUR rules.@n\n\r");
                SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
                save_char(d->character);
                Crash_delete_file(GET_NAME(d->character));
                STATE(d) = CON_CLOSE;
            }

            if (STATE(d) == CON_RMOTD) {
                write_to_output(d, "%s\r\n", nmotd);
                write_to_output(d, "@R**** PRESS RETURN:@n \r\n");
            }
            break;
*/
        case CON_RMOTD:
    if (IS_HAPPYHOUR > 0){
      write_to_output(d, "\r\n");
      write_to_output(d, "@CThere is currently a Happyhour!@n\r\n");
      write_to_output(d, "@Csee @Yhappyhour show@C for more info!@n\r\n");
    }
            // read CR after printing motd
            write_to_output(d, "%s\r\n", CONFIG_MENU);
            add_llog_entry(d->character, LAST_CONNECT);
            STATE(d) = CON_MENU;
            break;

        case CON_MENU:
            // get selection from main menu
            switch (*arg) {

                case '0':
                    write_to_output(d, "@WGoodbye.@n\r\n");
                    add_llog_entry(d->character, LAST_QUIT);
                    STATE(d) = CON_CLOSE;
                    break;

                case '1':
                    load_result = enter_player_game(d);
                    send_to_char(d->character, "%s", CONFIG_WELC_MESSG);

                    // Clear their load room if it's not persistant
                    if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
                        GET_LOADROOM(d->character) = 3001;

                    greet_mtrigger(d->character, -1);
                    greet_memory_mtrigger(d->character);

                    act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
                    save_char(d->character);

                    STATE(d) = CON_PLAYING;

                    if (GET_LEVEL(d->character) == 0) {
                        do_start(d->character);
                        send_to_char(d->character, "%s", CONFIG_START_MESSG);
                        char_from_room(d->character);
						char_to_room(d->character, real_room(4125));
						clear_quest(d->character);
                        do_prompt(d->character, "%hH %pP %vV> [%AA]", 0, 0);
                        send_to_char(d->character, "\r\n""Your prompt has been set up to display HPV and ammo, type display to see pre-configured prompts or help prompt to design your own.\r\n""\r\n");
                    }
                    /* Check for a login trigger in the players' start room */
                    login_wtrigger(&world[IN_ROOM(d->character)], d->character);

                    look_at_room(d->character, 0);
					// Gahanupdate
					send_to_char(d->character, "\r\nThere are new changes as of June 13th.  Type 'changeb read 94' for more details!\r\n");
					
                    if (has_mail(GET_IDNUM(d->character)))
                        send_to_char(d->character, "@CYou have mail waiting.@n\r\n");

                    // did char lose rented items due to no rent?
                    if (load_result == 2) {
                        send_to_char(d->character, "\r\n@RYou could not afford your rent!\r\n"
                        "Your posessions have been donated to the Salvation Army!@n\r\n");

                        // code that placed character into a death trap room removed -- why was this done?
                    }

                    d->has_prompt = 0;

                    // We've updated to 3.1 - some bits might be set wrongly:
                    REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
                    REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
                    // if they're no longer allowed on the frequency, then kick em off
                    if (!check_frequency(d->character, GET_FREQ(d->character)))
                        GET_FREQ(d->character) = 1;

                    // modified anti-multiplaying snippet from circle list -Lump
                    for (d2 = descriptor_list; d2; d2 = d2->next) {
                        if (d2->character) {

                            if (GET_LEVEL(d2->character) > LVL_GRGOD)
                                continue;

                            if (PLR_FLAGGED(d2->character, PLR_MULTI))
                                continue;

                            if (!str_cmp(d2->host, d->host))
                                num_links++;
                        }
                    }

                    if (num_links >= 3)
                        mudlog(BRF, LVL_GRGOD, TRUE, "WARNING: Multiple Login Detected: %d connections from %s (%s)",
                            num_links, d->host, GET_NAME(d->character));

                    break;

                case '2':
                    if (d->character->player.description) {
                        write_to_output(d, "Current description:\r\n%s\r\n", d->character->player.description);
                        // Don't free this now... so that the old description gets loaded as the
                        // current buffer in the editor.  Do setup the ABORT buffer here, however. */
                        d->backstr = strdup(d->character->player.description);
                    }

                    write_to_output(d, "@REnter the new text you'd like others to see when they look at you.@n\r\n");
                    send_editor_help(d);
                    d->str = &d->character->player.description;
                    d->max_str = PLR_DESC_LENGTH;
                    STATE(d) = CON_PLR_DESC;
                    break;

                case '3':
                    page_string(d, background, 0);
                    STATE(d) = CON_RMOTD;
                    break;

                case '4':
                    write_to_output(d, "\r\n@rEnter your old password:@n \r\n");
                    echo_off(d);
                    STATE(d) = CON_CHPWD_GETOLD;
                    break;

                default:
                    write_to_output(d, "\r\n@rThat's not a menu choice!@n\r\n%s\r\n", CONFIG_MENU);
                    break;
            }
            break;

        case CON_CHPWD_GETOLD:
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                echo_on(d);
                write_to_output(d, "\r\n@RIncorrect password.@n\r\n%s\r\n", CONFIG_MENU);
                STATE(d) = CON_MENU;
            }
            else {
                write_to_output(d, "\r\n@REnter a new password: @n\r\n");
                STATE(d) = CON_CHPWD_GETNEW;
            }
            return;

        case CON_DELCNF1:
            echo_on(d);
            if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
                write_to_output(d, "\r\n@rIncorrect password.@n\r\n%s\r\n", CONFIG_MENU);
                STATE(d) = CON_MENU;
            }
            else {
                write_to_output(d, "\r\n@RYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
                    "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
                    "Please type \"yes\" to confirm:@n \r\n");
                STATE(d) = CON_DELCNF2;
            }
            break;

        case CON_DELCNF2:
            if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {

                if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
                    write_to_output(d, "@RYou try to kill yourself, but the @Cice@R stops you.\r\n"
                        "Character not deleted.@n\r\n\r\n");
                    STATE(d) = CON_CLOSE;
                    return;
                }

                if (GET_LEVEL(d->character) < LVL_GRGOD)
                    SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);

				delete_storage(d->character);
                save_char(d->character);
                Crash_delete_file(GET_NAME(d->character));

                // If the selfdelete_fastwipe flag is set (in config.c), remove all the
                // player's immediately
                if (selfdelete_fastwipe)
                    if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
                        SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
                        log("removing player %s immediately", GET_NAME(d->character));
                        remove_player(player_i);
                    }

                delete_variables(GET_NAME(d->character));
                write_to_output(d, "@RCharacter '@Y%s@R' deleted! Goodbye.@n\r\n", GET_NAME(d->character));
                mudlog(NRM, LVL_GOD, TRUE, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
                STATE(d) = CON_CLOSE;
                return;

            }
            else {
                write_to_output(d, "\r\n@rCharacter @Rnot @rdeleted.@n\r\n%s", CONFIG_MENU);
                STATE(d) = CON_MENU;
            }
            break;

        // It is possible, if enough pulses are missed, to kick someone off while they
        // are at the password prompt. We'll let the game_loop()axe them
        case CON_CLOSE:
            break;

        case CON_IDENT:
            // do nothing
            break;

        default:
            log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
            STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
            STATE(d) = CON_DISCONNECT;    /* Safest to do. */
            break;
    }
}
