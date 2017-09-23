/**
* @file spec_procs.h
* Header file for special procedure modules. This file groups a lot of the
* legacy special procedures found in spec_procs.c and castle.c.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* 
*/
#ifndef _SPEC_PROCS_H_
#define _SPEC_PROCS_H_

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c 
 ****************************************************************************/
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);

/*****************************************************************************
 * Begin Functions and defines for spec_procs.c 
 ****************************************************************************/
/* Utility functions */
void sort_psionics(void);
void sort_skills(void);
void list_skills(struct char_data *ch);
char *how_good(int level);
void new_practice_list(struct char_data *ch, int tree);
/* Special functions */
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(stalker);
SPECIAL(mercenary);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(healer);
SPECIAL(guard_east);
SPECIAL(guard_west);
SPECIAL(guard_south);
SPECIAL(guard_north);
SPECIAL(guardian);
SPECIAL(imm_museum_guard);
SPECIAL(bouncer);
SPECIAL(dummy);
SPECIAL(justice);
SPECIAL(scratch_ticket);
SPECIAL(mortuary);
SPECIAL(bionics_shop);
SPECIAL(bionics_upgrade);
SPECIAL(bionics_doctor);
SPECIAL(quest_store);
SPECIAL(quest_store2);
SPECIAL(lotto);
SPECIAL(aggr_15);
SPECIAL(aggr_20);


#endif /* _SPEC_PROCS_H_ */
