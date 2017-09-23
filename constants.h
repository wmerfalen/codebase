/**
* @file constants.h
* Declares the global constants defined in constants.c.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

extern const char *tbamud_version;
extern const char *dirs[];
extern const char *autoexits[];
extern const char *room_bits[];
extern const char *zone_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *affected_bits[];
extern const char *combat_bits[];
extern const char *connected_types[];
extern const char *wear_where[];
extern const char *equipment_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *weekdays[];
extern const char *teams[];
extern const char *biotypes[];
extern const char *month_name[];
extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct pcn_app_type pcn_app[];
extern const struct attack_rate_type attack_rate[];
extern int rev_dir[];
extern int movement_loss[];
extern int drink_aff[][3];
extern const char *trig_types[];
extern const char *otrig_types[];
extern const char *wtrig_types[];
extern const char *history_types[];
extern const char *ibt_bits[];
extern size_t room_bits_count;
extern size_t action_bits_count;
extern size_t affected_bits_count;
extern size_t extra_bits_count;
extern size_t wear_bits_count;
extern size_t zone_bits_count;
extern const char *mercenary_classes;
extern const char *crazy_classes;
extern const char *stalker_classes;
extern const char *borg_classes;
extern const char *caller_classes;
extern const char *predator_classes;
extern const char *highlander_classes;
/* Cyberassault Externs */
extern const char *gun_types[];
extern const char *explosive_types[];
extern const char *subexplosive_types[];
extern const char *wpn_psionics[];
extern const char *drug_names[];
extern const char *where_body_parts[];
extern const char *part_condition[]; 
extern const char *bionic_names[][2];
extern const char *bionic_level[];
extern const char *body_parts[];
extern const char *npc_class_types[];
extern const char *vehicle_types[];
extern const char *vehicle_names[];
extern const char *damagetype_bits[];
#endif /* _CONSTANTS_H_ */
