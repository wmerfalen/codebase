/**************************************************************************
*  File: spec_assign.c                                     Part of tbaMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
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
#include "db.h"
#include "interpreter.h"
#include "spec_procs.h"
#include "ban.h" /* for SPECIAL(gen_board) */
#include "boards.h"
#include "mail.h"

SPECIAL(questmaster); 
SPECIAL(shop_keeper);

/* local (file scope only) functions */
static void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));
extern void assign_statues(void);
/* functions to perform assignments */
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
    mob_rnum rnum;

    if ((rnum = real_mobile(mob)) != NOBODY)
        mob_index[rnum].func = fname;
    else if (!mini_mud)
        log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

static void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

static void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}

/* Assignments */
/* assign special procedures to mobiles. Guildguards, snake, thief, magic user,
 * puff, fido, janitor, and cityguards are now implemented via triggers. */
void assign_mobiles(void)
{
  assign_statues();
    ASSIGNMOB(3062, fido);
    ASSIGNMOB(3066, fido);
    ASSIGNMOB(21754, fido);
    ASSIGNMOB(21755, fido);

    ASSIGNMOB(3005, receptionist);
    ASSIGNMOB(3095, cryogenicist);
    ASSIGNMOB(3010, postmaster);

    ASSIGNMOB(3020, guild);
    ASSIGNMOB(31099, guild);
    ASSIGNMOB(3021, guild);
    ASSIGNMOB(3022, guild);
    ASSIGNMOB(3023, guild);
    ASSIGNMOB(6018, guild);
    ASSIGNMOB(13609, guild);
    ASSIGNMOB(21512, guild);
	ASSIGNMOB(21601, guild);
	ASSIGNMOB(4102, guild);
	ASSIGNMOB(3044, guild);
	ASSIGNMOB(3099, healer);
    ASSIGNMOB(3069, bouncer);

    ASSIGNMOB(8020, mayor);

    ASSIGNMOB(7009, mercenary);
    ASSIGNMOB(7006, snake);
    ASSIGNMOB(13001, dummy);
    ASSIGNMOB(13002, dummy);
    ASSIGNMOB(13003, dummy);
    ASSIGNMOB(13050, guardian);

    /* New Stuff for Immortal Shrine (MacLeod) */
    ASSIGNMOB(9900, guard_west);
    ASSIGNMOB(9901, guard_south);
    ASSIGNMOB(9902, guard_north);
    ASSIGNMOB(9903, guard_east);
    ASSIGNMOB(9904, guard_west);
    ASSIGNMOB(9905, guard_south);
    ASSIGNMOB(9906, guard_north);
    ASSIGNMOB(9907, guard_east);
    ASSIGNMOB(9908, guard_north);
    ASSIGNMOB(9909, guard_east);

    ASSIGNMOB(106, guard_east);
    ASSIGNMOB(107, guard_west);
    ASSIGNMOB(101, guard_north);

    ASSIGNMOB(1201, justice);         /* Enforcer of the law */
    ASSIGNMOB(800, imm_museum_guard);  /* IMM Museum entrance Guard*/
}

/* assign special procedures to objects */
void assign_objects(void)
{
    ASSIGNOBJ(3093, gen_board);   /* mort idea board */
    ASSIGNOBJ(3094, gen_board);   /* mort bug board */
    ASSIGNOBJ(3095, gen_board);   /* mort quest board */
    ASSIGNOBJ(3096, gen_board);    /* social board (gone?) */
    ASSIGNOBJ(3097, gen_board);    /* freeze board */
    ASSIGNOBJ(3098, gen_board);    /* immortal board */
    ASSIGNOBJ(3099, gen_board);    /* mortal board */
    ASSIGNOBJ(1211, gen_board);   /* voting board*/
    ASSIGNOBJ(1212, gen_board);   /* rep board*/
    ASSIGNOBJ(1213, gen_board);   /* auction board*/
    ASSIGNOBJ(1299, gen_board);   /* imp board*/
    ASSIGNOBJ(10090, gen_board);  /* area board*/
    ASSIGNOBJ(10091, gen_board);  /* playtest board */
    ASSIGNOBJ(10092, gen_board);  /* code board */
    ASSIGNOBJ(10093, gen_board);  /* logistics board */
    ASSIGNOBJ(10094, gen_board);  /* immquest board */
	ASSIGNOBJ(1244, gen_board);   /*changebard*/

    ASSIGNOBJ(3034, bank);    /* atm */
    ASSIGNOBJ(198, bank);    /* atm */
    ASSIGNOBJ(1216, scratch_ticket); /* instant win scratch ticket */
}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

    ASSIGNROOM(3031, pet_shops);
    ASSIGNROOM(14210, pet_shops);
    ASSIGNROOM(3099, mortuary);
    ASSIGNROOM(12, bionics_shop);
    ASSIGNROOM(12, bionics_upgrade);
    ASSIGNROOM(3097, bionics_doctor);
    ASSIGNROOM(4019, quest_store2);
    ASSIGNROOM(3094, quest_store);
    ASSIGNROOM(4000, quest_store);
    ASSIGNROOM(4001, quest_store);
    ASSIGNROOM(4002, quest_store);
    ASSIGNROOM(4003, quest_store);
    ASSIGNROOM(4004, quest_store);
    ASSIGNROOM(4005, quest_store);
    ASSIGNROOM(4006, quest_store);
    ASSIGNROOM(4007, quest_store);
    ASSIGNROOM(4008, quest_store);
    ASSIGNROOM(4009, quest_store);
    ASSIGNROOM(4010, quest_store);
    ASSIGNROOM(4011, quest_store);
    ASSIGNROOM(4012, quest_store);
    ASSIGNROOM(4013, quest_store);
    ASSIGNROOM(4014, quest_store);
    ASSIGNROOM(4015, quest_store);
    ASSIGNROOM(4016, quest_store);
    ASSIGNROOM(4017, quest_store);
    ASSIGNROOM(4018, quest_store);
    ASSIGNROOM(3096, lotto);

  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}

struct spec_func_data { 
   char *name; 
   SPECIAL(*func); 
}; 

struct spec_func_data spec_func_list[] = { 
    { "Mayor",          mayor },
    { "Snake",          snake },
    { "Puff",           puff },
    { "Fido",           fido },
    { "Janitor",        janitor },
    { "Cityguard",      cityguard },
    { "Postmaster",     postmaster },
    { "Receptionist",   receptionist },
    { "Cryogenicist",   cryogenicist},
    { "Bulletin Board", gen_board },
    { "Bank",           bank },
    { "Pet Shop",       pet_shops },
    { "Dump",           dump },
    { "Guildmaster",    guild },
    { "Questmaster",    questmaster },
    { "Shopkeeper",     shop_keeper },
	{ "Healer",			healer },
    { "\n", NULL}
}; 

const char *get_spec_func_name(SPECIAL(*func)) 
{ 
  int i; 
  for (i=0; *(spec_func_list[i].name) != '\n'; i++) { 
    if (func == spec_func_list[i].func) return (spec_func_list[i].name); 
  } 
  return NULL; 
} 

