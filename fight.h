/**
* @file fight.h
* Fighting and violence functions and variables.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _FIGHT_H_
#define _FIGHT_H_

/* Structures and defines */
/* Attacktypes with grammar */
struct attack_hit_type {
    const char *singular;
    const char *plural;
	const char *onehit;
	const char *threehit;
	const char *fivehit;
	const char *tenhit;
};

/* Functions available in fight.c */
void make_corpse(struct char_data *ch);
void update_combo(struct char_data *ch, struct char_data *victim, int skill);
void reset_combo(struct char_data *ch);
void group_gain(struct char_data *ch, struct char_data *victim);
void appear(struct char_data *ch);
void check_killer(struct char_data *ch, struct char_data *vict);
int compute_armor_class(struct char_data *ch);
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int damagetype);
void death_cry(struct char_data *ch);
void die(struct char_data *ch, struct char_data *killer, bool gibblets);
void free_messages(void);
void hit(struct char_data *ch, struct char_data *victim, int type, int thaco, int range);
void rangedhit(struct char_data *ch, struct char_data *victim, int type, int thaco);
void load_messages(void);
void perform_violence(void);
struct obj_data *make_body_part(struct char_data *ch, int part_location);
void remove_body_part(struct char_data *ch, int location, bool death);
void raw_kill(struct char_data *ch, struct char_data *killer, bool gibblets);
void  set_fighting(struct char_data *ch, struct char_data *victim);
void stop_ranged_fighting(struct char_data *ch);
void set_ranged_fighting(struct char_data *ch, struct char_data *vict);
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype);
void  stop_fighting(struct char_data *ch);
void wpn_psionic(struct char_data *ch, struct char_data *victim, struct obj_data *obj, int which_psionic, int skill_level);
int PK_OK(struct char_data *ch, struct char_data *victim);
int obj_damage_char(struct obj_data *obj, struct char_data *victim, int dam);
int get_dice(struct char_data *ch);
void activate_chip(struct char_data *ch);
void deactivate_chip(struct char_data *ch);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void end_rangedcombat(struct char_data *ch);
void exec_delay_hit(
	struct char_data *ch,
	char *victim_name,
	int wait_time, 
	int damage, 
	int damage_type, 
	int psi_num,
	int psi_level,
	bool psi_ack,
	char *hit_attacker_msg, 
	char *hit_victim_msg, 
	char *hit_room_msg, 
	char *af_attacker_msg, 
	char *af_victim_msg, 
	char *af_room_msg,
	char *echo_attacker_msg, 
	char *echo_victim_msg, 
	char *echo_room_msg);

/* Global variables */
#ifndef __FIGHT_C__
extern struct attack_hit_type attack_hit_text[];
extern struct char_data *combat_list;
extern struct char_data *ranged_combat_list;
#endif /* __FIGHT_C__ */

#endif /* _FIGHT_H_*/
