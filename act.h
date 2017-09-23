/**
* @file act.h
* Header file for the core act* c files.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Utility functions that could easily be moved elsewhere have been
* marked. Suggest a review of all utility functions (aka. non ACMDs) and
* determine if the utility functions should be placed into a lower level
* (non-ACMD focused) shared module.
*
*/
#ifndef _ACT_H_
#define _ACT_H_

#include "utils.h" /* for the ACMD macro */

/*****************************************************************************
 * Begin Functions and defines for act.comm.c
 ****************************************************************************/
/* functions with subcommands */
/* do_gen_comm */
#define NEWB_RADIO 10
ACMD(do_event);
ACMD(do_gen_comm);
#define SCMD_HOLLER   0
#define SCMD_SHOUT    1
#define SCMD_GOSSIP   2
#define SCMD_AUCTION  3
#define SCMD_GRATZ    4
#define SCMD_NEWBIE   5
#define SCMD_RADIO    6
#define SCMD_GEMOTE   7
/* do_qcomm */
ACMD(do_qcomm);
#define SCMD_QSAY     0
#define SCMD_QECHO    1
/* do_spec_com */
ACMD(do_spec_comm);
#define SCMD_WHISPER  0
#define SCMD_ASK      1
/* functions without subcommands */
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_page);
ACMD(do_reply);
ACMD(do_tell);
ACMD(do_ignore);
ACMD(do_write);
ACMD(do_radio);
ACMD(do_market);
ACMD(do_happyhour);
/*****************************************************************************
 * Begin Functions and defines for act.informative.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Move to a utility library */
char *find_exdesc(char *word, struct extra_descr_data *list);
/** @todo Move to a mud centric string utility library */
void space_to_minus(char *str);
/** @todo Move to a help module? */
int search_help(const char *argument, int level);
void free_history(struct char_data *ch, int type);
void free_recent_players(void);
void auction_channel(char *msg);
void auction_update(void);
void show_obj_stats(struct char_data *ch, struct obj_data *obj);
long radio_find_alias(char *arg);
int check_frequency(struct char_data *ch, int freq);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
void do_auto_exits(struct char_data *ch);

/* functions with subcommands */
/* do_commands */
ACMD(do_commands);
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS  1
#define SCMD_WIZHELP  2
/* do_gen_ps */
ACMD(do_gen_ps);
#define SCMD_INFO      0
#define SCMD_HANDBOOK  1
#define SCMD_CREDITS   2
#define SCMD_WIZLIST   3
#define SCMD_POLICIES  4
#define SCMD_VERSION   5
#define SCMD_IMMLIST   6
#define SCMD_MOTD      7
#define SCMD_IMOTD     8
#define SCMD_CLEAR     9
#define SCMD_WHOAMI    10
#define SCMD_NMOTD     11
#define SCMD_CHANGES   12
/* do_look */
ACMD(do_look);
#define SCMD_LOOK 0
#define SCMD_READ 1
/* functions without subcommands */
ACMD(do_areas);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_units);
ACMD(do_help);
ACMD(do_history);
ACMD(do_inventory);
ACMD(do_levels);
ACMD(do_score);
ACMD(do_who);
ACMD(do_oldscore);
ACMD(do_oldequipment);
ACMD(do_oldwho);
ACMD(do_newscore);
ACMD(do_newwho);
ACMD(do_affections);
ACMD(do_time);
ACMD(do_toggle);
ACMD(do_users);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_whois);
ACMD(do_dossier);
ACMD(do_search);
ACMD(do_psionics);
ACMD(do_skills);
ACMD(do_bionics);
ACMD(do_anatomy);
ACMD(do_alias);
ACMD(do_hitroll);
ACMD(do_armor);
ACMD(do_exp);
ACMD(do_uninsure);

/*****************************************************************************
 * Begin Functions and defines for act.item.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Compare with needs of find_eq_pos_script. */
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void weight_change_object(struct obj_data *obj, int weight);
void perform_do_unload(struct char_data *ch, struct obj_data *obj);
int can_take_obj(struct char_data *ch, struct obj_data *obj);
int check_restrictions(struct char_data *ch, struct obj_data *obj);
ACMD(do_behead);
/* functions with subcommands */
/* do_drop */
ACMD(do_drop);
#define SCMD_DROP   0
#define SCMD_JUNK   1
#define SCMD_DONATE 2
/* do_eat */
ACMD(do_eat);
#define SCMD_EAT    0
#define SCMD_TASTE  1
#define SCMD_DRINK  2
#define SCMD_SIP    3
/* do_pour */
ACMD(do_pour);
#define SCMD_POUR  0
#define SCMD_FILL  1
/* functions without subcommands */
ACMD(do_drink);
ACMD(do_get);
ACMD(do_give);
ACMD(do_grab);
ACMD(do_put);
ACMD(do_remove);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_load);
ACMD(do_unload);
ACMD(do_assemble);
ACMD(do_pull);
ACMD(do_inject);
ACMD(do_light);
ACMD(do_arm);
ACMD(do_disarm);
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_throw);
ACMD(do_rupture);


/*****************************************************************************
 * Begin Functions and defines for act.movement.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_gen_door */
void get_encumbered(struct char_data*ch);
ACMD(do_gen_door);
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4
/* Functions without subcommands */
ACMD(do_enter);
ACMD(do_follow);
ACMD(do_leave);
ACMD(do_move);
ACMD(do_rest);
ACMD(do_sit);
ACMD(do_sleep);
ACMD(do_stand);
ACMD(do_wake);
ACMD(do_focus);
ACMD(do_meditate);
ACMD(do_drag);
ACMD(do_noleave);

/* Global variables from act.movement.c */
#ifndef __ACT_MOVEMENT_C__
extern const char *cmd_door[];
#endif /* __ACT_MOVEMENT_C__ */


/*****************************************************************************
 * Begin Functions and defines for act.offensive.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_hit */
ACMD(do_hit);
ACMD(do_attack);
#define SCMD_HIT    0
/* Functions without subcommands */
ACMD(do_assist);
ACMD(do_backstab);
ACMD(do_flee);
ACMD(do_kill);
ACMD(do_order);
ACMD(do_rescue);
ACMD(do_behead);
ACMD(do_slay);
ACMD(do_berserk);
ACMD(do_ambush);
ACMD(do_flank);
ACMD(do_charge);
ACMD(do_slash);
ACMD(do_dodge);
ACMD(do_protect);
ACMD(do_coverfire);
ACMD(do_stun);
ACMD(do_bodyblock);
ACMD(do_circle);
ACMD(do_behead);
ACMD(do_scavenger);
ACMD(do_blast);
ACMD(do_recon);
ACMD(do_retreat);
ACMD(do_circle);
ACMD(do_electrify);
ACMD(do_opticblast);
ACMD(do_drain);
ACMD(do_leapattack);
ACMD(do_aim);
ACMD(do_snipe);
ACMD(do_assassinate);
#define SCMD_BLAST    1

/*****************************************************************************
 * Begin Functions and defines for act.other.c
 ****************************************************************************/
/* Functions with subcommands */
int perform_group(struct char_data *ch, struct char_data *vict);
void list_char_to_char(struct char_data *list, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);

/* do_gen_tog */
ACMD(do_gen_tog);
ACMD(do_hp);
ACMD(do_cp);
ACMD(do_mv);
ACMD(do_qp);
ACMD(do_dash);
ACMD(do_fame);
ACMD(do_toggle);
ACMD(do_recover);
ACMD(do_affections);
/* do_gen_write */
ACMD(do_gen_write);
ACMD(do_hph);
ACMD(do_shards);

#define SCMD_BRIEF          0
#define SCMD_COMPACT        1
#define SCMD_NOSHOUT        2
#define SCMD_NOTELL         3
#define SCMD_DISPHP         4
#define SCMD_DISPPSI        5
#define SCMD_DISPMOVE       6
#define SCMD_DISPAMMO       7
#define SCMD_NOHASSLE       8
#define SCMD_QUEST          9
#define SCMD_NOSUMMON       10
#define SCMD_NOREPEAT       11
#define SCMD_HOLYLIGHT      12
#define SCMD_COLOR          13
#define SCMD_COLOR2         14
#define SCMD_NOWIZ          15
#define SCMD_MORTLOG        16 
#define SCMD_SYSLOG         17
#define SCMD_NOAUCT         18
#define SCMD_NOGOSS         19
#define SCMD_NOGRATZ        20
#define SCMD_SHOWVNUMS      21
#define SCMD_AFK            22
#define SCMD_AUTOCON        23
#define SCMD_AUTOEXIT       24
#define SCMD_AUTOEQUIP      25
#define SCMD_AUTOLOOT       26
#define SCMD_AUTOSPLIT      27
#define SCMD_AUTOASSIST     28
#define SCMD_AUTOGAIN       29
#define SCMD_AUTODAM        30
#define SCMD_AUTOUNIT       31
#define SCMD_AUTOMAP        32
#define SCMD_AUTOKEY        33
#define SCMD_AUTODOOR       34
#define SCMD_LDFLEE         35
#define SCMD_NONEWBIE       36
#define SCMD_CLS            37
#define SCMD_BUILDWALK      38
#define SCMD_RADIOSNOOP     39
#define SCMD_SPECTATOR      40
#define SCMD_TEMP_MORTAL    41
#define SCMD_FROZEN         42
#define SCMD_OLDSCORE       43
#define SCMD_NOWHO          44
#define SCMD_SHOWDBG        45
#define SCMD_SET_STEALTH    46
#define SCMD_NO_TYPO        47

// special player preferences with values
#define SCMD_PAGELENGTH     48
#define SCMD_SCREENWIDTH    49
#define SCMD_WIMPY          50

// not player preferences but game preferences
#define SCMD_SLOWNS         51
#define SCMD_TRACK          52
#define SCMD_AUTOSCAVENGER  53
#define SCMD_NOFIGHTSPAM	54
#define SCMD_OLDWHO         55
#define SCMD_OLDEQ          56
#define SCMD_NOMARKET		57
#define SCMD_BLIND			58
#define SCMD_SPAM			59

/* do_quit */
ACMD(do_quit);
#define SCMD_QUI  0
#define SCMD_QUIT 1
/* do_use */
ACMD(do_use);
#define SCMD_USE  0
#define SCMD_QUAFF  1
#define SCMD_RECITE 2
/* Functions without subcommands */
ACMD(do_display);
ACMD(do_group);
ACMD(do_hide);
ACMD(do_not_here);
ACMD(do_practice);
ACMD(do_skilltree);
ACMD(do_report);
ACMD(do_save);
ACMD(do_sneak);
ACMD(do_split);
ACMD(do_title);
ACMD(do_ungroup);
ACMD(do_visible);
ACMD(do_medkit);
ACMD(do_scan);
ACMD(do_recall);
ACMD(do_depress);
ACMD(do_transport);
ACMD(do_run);
ACMD(do_suicid);
ACMD(do_suicide);
ACMD(do_ventriloquate);
ACMD(do_god_rent);
ACMD(do_gain);
ACMD(do_increase);
ACMD(do_qcreate);
ACMD(do_prompt);
ACMD(do_scavenger);


/*****************************************************************************
 * Begin Functions and defines for act.social.c
 ****************************************************************************/
/* Utility Functions */
void free_social_messages(void);
/** @todo free_action should be moved to a utility function module. */
void free_action(struct social_messg *mess);
/** @todo command list functions probably belong in interpreter */
void free_command_list(void);
/** @todo command list functions probably belong in interpreter */
void create_command_list(void);
/* Functions without subcommands */
ACMD(do_action);
ACMD(do_gmote);



/*****************************************************************************
 * Begin Functions and defines for act.wizard.c
 ****************************************************************************/
/* Utility Functions */
/** @todo should probably be moved to a more general file handler module */
void clean_llog_entries(void);
void bionicspurge(struct char_data *victim);
/** @todo This should be moved to a more general utility file */
int script_command_interpreter(struct char_data *ch, char *arg);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name);
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr);
/* Functions with subcommands */
/* do_date */
ACMD(do_date);
#define SCMD_DATE   0
#define SCMD_UPTIME 1
/* do_echo */
ACMD(do_echo);
#define SCMD_ECHO   0
#define SCMD_EMOTE  1
/* do_last */
ACMD(do_last);
#define SCMD_LIST_ALL 1
/* do_shutdown */
ACMD(do_shutdown);
#define SCMD_SHUTDOW   0
#define SCMD_SHUTDOWN  1
/* do_wizutil */
ACMD(do_wizutil);
#define SCMD_REROLL   0
#define SCMD_PARDON   1
#define SCMD_NOTITLE  2
#define SCMD_MUTE     3
#define SCMD_FREEZE   4
#define SCMD_THAW     5
#define SCMD_UNAFFECT 6

#define SCMD_MORTB      0
#define SCMD_QUESTB     1
#define SCMD_IMMB       2
#define SCMD_BUGB       3
#define SCMD_CODEB      4
#define SCMD_FREEZEB    5
#define SCMD_IDEAB      6
#define SCMD_IMMQB      7
#define SCMD_REPB    8
#define SCMD_AUCB    9
#define SCMD_AREAB    10
#define SCMD_PLAYTESTB    11
#define SCMD_CHANGEB	12
/* do_set */
#define SET_AC          1
#define SET_AFK         2
#define SET_AGE         3
#define SET_ATTR        4
#define SET_BANK        5    
#define SET_BIONIC      6
#define SET_BRIEF       7
#define SET_CHA         8
#define SET_CLASS       9   
#define SET_COLOR       10
#define SET_CON         11
#define SET_DAMROLL     12
#define SET_DELETE      13
#define SET_DEX         14
#define SET_DRUNK       15
#define SET_DTS         16
#define SET_ESSENCES    17
#define SET_EXP         18
#define SET_FREQ        19
#define SET_FROZEN      20
#define SET_UNITS       21
#define SET_HEIGHT      22
#define SET_HIT         23
#define SET_HITROLL     24
#define SET_HUNGER      25
#define SET_IDNUM       26
#define SET_INT         27
#define SET_INVIS       28
#define SET_ISTART      29
#define SET_KILLER      30
#define SET_LEVEL       31
#define SET_LDROOM      32
#define SET_PSI         33
#define SET_MAXHIT      34
#define SET_MAXPSI      35
#define SET_MAXMOVE     36
#define SET_MOVE        37
#define SET_MULTI       38
#define SET_NAME        39
#define SET_NODELETE    40 
#define SET_NOHASS      41
#define SET_NOSUMM      42
#define SET_NOWHO       43
#define SET_NOWIZ       44
#define SET_OLC         45
#define SET_PASS        46
#define SET_PCN         47
#define SET_PK          48
#define SET_POOFIN      49
#define SET_POOFOUT     50 
#define SET_PRAC        51
#define SET_PSIMAST     52
#define SET_QUEST       53
#define SET_QPS         54
#define SET_QHIST       55
#define SET_REMRTPRAC   56
#define SET_REP         57
#define SET_ROOM        58
#define SET_SCREEN      59
#define SET_SEX         60 
#define SET_SHOWVNUMS   61
#define SET_SITEOK      62
#define SET_SPECT       63
#define SET_STR         64
#define SET_STRADD      65
#define SET_TEAM        66
#define SET_TEMPMORT    67
#define SET_THIEF       68
#define SET_THIRST      69
#define SET_TITLE       70 
#define SET_VAR         71
#define SET_WEIGHT      72
#define SET_REMORTS     73
#define SET_FAME		74
/* Functions without subcommands */
ACMD(do_advance);
ACMD(do_at);
ACMD(do_checkloadstatus);
ACMD(do_copyover);
ACMD(do_copyto);
ACMD(do_dc);
ACMD(do_demote);
ACMD(do_dig);
ACMD(do_changelog);
ACMD(do_file);
ACMD(do_findkey);
ACMD(do_finddoor);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_goto);
ACMD(do_invis);
ACMD(do_links);
ACMD(do_create);
ACMD(do_peace);
ACMD(do_plist);
ACMD(do_purge);
ACMD(do_reimp);
ACMD(do_recent);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_saveall);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_snoop);
ACMD(do_stat);
ACMD(do_switch);
ACMD(do_teleport);
ACMD(do_trans);
ACMD(do_tstat);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_zcheck);
ACMD(do_zlock);
ACMD(do_zpurge);
ACMD(do_zreset);
ACMD(do_zunlock);
ACMD(do_board);
ACMD(do_remort);
ACMD(do_eqload);
ACMD(do_linkdead);
ACMD(do_forcerent);
ACMD(do_lotto);
ACMD(do_confiscate);
ACMD(do_vwear);
ACMD(do_vobj);
ACMD(do_cleave);
ACMD(do_glist);
// todo: what do these do?
#define SCMD_SQUELCH       3
#define SCMD_UNAFFECTROOM  7
#endif /* _ACT_H_ */
