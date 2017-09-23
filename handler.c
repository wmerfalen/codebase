/**************************************************************************
*  File: handler.c                                         Part of tbaMUD *
*  Usage: Internal funcs: moving and finding chars/objs.                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "psionics.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "quest.h"
#include "graph.h"
#include "constants.h"
#include "class.h"
#include "bionics.h"
#include "skilltree.h"
#include "protocol.h"

/* local file scope variables */
static int extractions_pending = 0;
//static int extractions_pending_death = 0;

/* local file scope functions */
static int apply_ac(struct char_data *ch, int eq_pos);
static void update_object(struct obj_data *obj, int use);

char *fname(const char *namelist)
{
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* Leave this here even if you put in a newer isname().  Used for OasisOLC. */
int is_name(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  if (!str || !namelist || !*str || !*namelist)
    return (0);

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    /* skip to next name */
   for (; isalpha(*curname); curname++);
     if (!*curname)
       return (0);
    curname++;                  /* first char of new name */
  }
}

/* allow abbreviations */
#define WHITESPACE " \t"
int isname(const char *str, const char *namelist)
{
  char *newlist;
  char *curtok;

  if (!str || !*str || !namelist || !*namelist)
    return 0;

  if (!strcmp(str, namelist)) /* the easy way */
    return 1;

  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */
  for(curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE))
    if(curtok && is_abbrev(str, curtok)) {
      /* Don't allow abbreviated numbers. - Sryth */
      if (isdigit(*str) && (atoi(str) != atoi(curtok)))
        return 0;
      free(newlist);
      return 1;
    }
  free(newlist);
  return 0;
}

void aff_apply_modify(struct char_data *ch, byte loc, sh_int mod, char *msg)
{
    int skill_level = 0;

    switch (loc) {
        case APPLY_NONE:
            break;
        case APPLY_STR:
            GET_STR(ch) += mod;
            break;
        case APPLY_DEX:
            GET_DEX(ch) += mod;
            break;
        case APPLY_INT:
            GET_INT(ch) += mod;
            break;
        case APPLY_PCN:
            GET_PCN(ch) += mod;
            break;
        case APPLY_CON:
            GET_CON(ch) += mod;
            break;
        case APPLY_CHA:
            GET_CHA_ABIL(ch) += mod;
            break;
        case APPLY_AGE:
            GET_AGE_MOD(ch) += (mod * SECS_PER_MUD_YEAR);
            break;
        case APPLY_CHAR_WEIGHT:
            GET_WEIGHT(ch) += mod;
            break;
        case APPLY_CHAR_HEIGHT:
            GET_HEIGHT(ch) += mod;
            break;
        case APPLY_PSI:
            GET_MAX_PSI_PTS(ch) += mod;
            break;
        case APPLY_HIT:
            GET_MAX_HIT_PTS(ch) += mod;
            break;
        case APPLY_MOVE:
            GET_MAX_MOVE(ch) += mod;
            break;
        case APPLY_AC:
            GET_AC(ch) += mod;
            break;
        case APPLY_HITROLL:
            GET_HITROLL(ch) += mod;
            break;
        case APPLY_DAMROLL:
            GET_DAMROLL(ch) += mod;
            break;
        case APPLY_SAVING_NORMAL:
            GET_SAVE(ch, SAVING_NORMAL) += mod;
            break;
        case APPLY_SAVING_POISON:
            GET_SAVE(ch, SAVING_POISON) += mod;
            break;
        case APPLY_SAVING_DRUG:
            GET_SAVE(ch, SAVING_DRUG) += mod;
            break;
        case APPLY_SAVING_FIRE:
            GET_SAVE(ch, SAVING_FIRE) += mod;
            break;
        case APPLY_SAVING_COLD:
            GET_SAVE(ch, SAVING_COLD) += mod;
            break;
        case APPLY_SAVING_ELECTRICITY:
            GET_SAVE(ch, SAVING_ELECTRICITY) += mod;
            break;
        case APPLY_SAVING_EXPLOSIVE:
            GET_SAVE(ch, SAVING_EXPLOSIVE) += mod;
            break;
        case APPLY_SAVING_PSIONIC:
            GET_SAVE(ch, SAVING_PSIONIC) += mod;
            break;
        case APPLY_SAVING_NONORM:
            GET_SAVE(ch, SAVING_NONORM) += mod;
            break;
        case APPLY_SKILL:
            if (IS_NPC(ch))
                return;
            // mod is the skill number
            if (mod < 0) {
                skill_level = 0;
                mod = -mod;
            }
            else
                skill_level = 6;
            // we assume players can only learn up to skill level 5 (see spec_procs.c)
            // otherwise skill level 6 to 100 is learned through equipment apply only -Lump
            if ((mod < 0 || mod > TOP_SKILL_DEFINE) || !strcmp("!UNUSED!", skills_info[mod].name)) {
                send_to_char(ch, "Unrecognized skill! Please report as a bug!!!\r\n");
                break;
            }
			// todo: Only set the skill maybe if its already 0.
            else if (GET_SKILL_LEVEL(ch, mod) == 0 || GET_SKILL_LEVEL(ch, mod) > 5)
                    SET_SKILL(ch, mod, skill_level);
            break;
		case APPLY_PSIONIC:
			if (IS_NPC(ch))
				return;
			if (mod < 0) {
				skill_level = 0;
				mod = -mod;
			}
			else
				skill_level = 6;
            if ((mod < 0 || mod > TOP_SKILL_DEFINE) || !strcmp("!UNUSED!", skills_info[mod].name)) {
                send_to_char(ch, "Unrecognized skill! Please report as a bug!!!\r\n");
                break;
            }
            else if (GET_PSIONIC_LEVEL(ch, mod) == 0) { 
                    SET_PSIONIC(ch, mod, skill_level);
			}
            else {
                if (PRF_FLAGGED(ch, PRF_SHOWDBG))
                    send_to_char(ch, "DBG: would set skill %d to level: %d but you are already learned.\r\n", mod, skill_level);
            }
            break;
        case APPLY_ALL_STATS:
            GET_STR(ch) += mod;
            GET_DEX(ch) += mod;
            GET_INT(ch) += mod;
            GET_PCN(ch) += mod;
            GET_CON(ch) += mod;
            GET_CHA_ABIL(ch) += mod;
            break;
        case APPLY_PSI_MASTERY:
            GET_PSIMASTERY(ch) += mod;
            break;
        case APPLY_PSI_REGEN:
            GET_PSI_REGEN_RAW(ch) += mod;
            break;
        case APPLY_HIT_REGEN:
            GET_HIT_REGEN(ch) += mod;
            break;
        case APPLY_MOVE_REGEN:
            GET_MOVE_REGEN(ch) += mod;
            break;
        case APPLY_HITNDAM:
            GET_HITROLL(ch) += mod;
            GET_DAMROLL(ch) += mod;
            break;
        case APPLY_REGEN_ALL:
            GET_ALL_REGEN(ch) += mod;
            break;
        case APPLY_HPV:
            GET_MAX_PSI_PTS(ch) += mod;
            GET_MAX_HIT_PTS(ch) += mod;
            GET_MAX_MOVE(ch) += mod;
            break;
        case APPLY_PSI2HIT:
            GET_MAX_PSI_PTS(ch)  -= mod;
            GET_MAX_HIT_PTS(ch)  += mod;
            break;
        default:
            log("SYSERR: Unknown apply adjust %d attempt (%s, aff_apply_modify).", loc, __FILE__);
            break;
    }
}

void affect_modify_ar(struct char_data * ch, byte loc, sh_int mod, int bitv[], bool add)
{
  int i , j;

  if (add) {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j))
          SET_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
  } else {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j))
          REMOVE_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
    mod = -mod;
  }

  aff_apply_modify(ch, loc, mod, "affect_modify_ar");
}

/* This updates a character by subtracting everything he is affected by
 * restoring original abilities, and then affecting all again. */
void affect_total(struct char_data *ch)
{
  struct affected_type *af;
  struct affected_type *afdrug;
  int i, j, k, maxabil;
  
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_APPLY; j++)
	    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(GET_EQ(ch, i), j),
              GET_OBJ_APPLY_MOD(GET_EQ(ch, i), j),
              GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);

  }

  for (i = 0; i < NUM_BODY_PARTS; i++) {
	if (BIONIC_DEVICE(ch, i))
	  for (j = 0; j < MAX_OBJ_APPLY; j++)
		affect_modify_ar(ch, GET_OBJ_APPLY_LOC(BIONIC_DEVICE(ch, i), j),
			GET_OBJ_APPLY_MOD(BIONIC_DEVICE(ch, i), j),
			GET_OBJ_AFFECT(BIONIC_DEVICE(ch, i)), FALSE);
  }

  if (ch->equipment[WEAR_WIELD]) {
	if (IS_GUN(ch->equipment[WEAR_WIELD]))
      if (IS_LOADED(ch->equipment[WEAR_WIELD]))
		  for (j = 0; j < MAX_OBJ_APPLY; j++)
			affect_modify_ar(ch, GET_OBJ_APPLY_LOC(GET_AMMO(ch->equipment[WEAR_WIELD]), j),
		      GET_OBJ_APPLY_MOD(GET_AMMO(ch->equipment[WEAR_WIELD]), j),
			  GET_OBJ_AFFECT(GET_AMMO(ch->equipment[WEAR_WIELD])), FALSE);
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
  
  for (afdrug = ch->drugs_used; afdrug; afdrug = afdrug->next)
	affect_modify_ar(ch, afdrug->location, afdrug->modifier, afdrug->bitvector, FALSE);

  for (i = 0; i < NUM_SKILLS; i++) {
	  if (GET_SKILL_LEVEL(ch, i) < 0 && GET_SKILL_LEVEL(ch, i) > 5)
		  if (GET_LEVEL(ch) < LVL_IMMORT)
			  SET_SKILL(ch, i, 0);
  }
  for (i = 0; i < NUM_PSIONICS; i++) {
//	  if (GET_PSIONIC_LEVEL(ch, i) < 0 || GET_PSIONIC_LEVEL(ch, i) > 5)
	  if (GET_PSIONIC_LEVEL(ch, i) < 0 && GET_PSIONIC_LEVEL(ch, i) > 5)
		  if (GET_LEVEL(ch) < LVL_IMMORT)
			  SET_PSIONIC(ch, i, 0);
  }
  ch->aff_abils = ch->real_abils;
  
  //if (GET_LEVEL(ch) < LVL_IMMORT) {
  //	GET_HITROLL(ch) = 0;
  //	GET_DAMROLL(ch) = 0;
  //	GET_AC(ch) = 0;
  //}
  
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_APPLY; j++)
	    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(GET_EQ(ch, i), j),
              GET_OBJ_APPLY_MOD(GET_EQ(ch, i), j),
              GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);

  }

  if (ch->equipment[WEAR_WIELD]) {
	if (IS_GUN(ch->equipment[WEAR_WIELD]))
      if (IS_LOADED(ch->equipment[WEAR_WIELD]))
		  for (j = 0; j < MAX_OBJ_APPLY; j++) {
			affect_modify_ar(ch, GET_OBJ_APPLY_LOC(GET_AMMO(ch->equipment[WEAR_WIELD]), j),
		      GET_OBJ_APPLY_MOD(GET_AMMO(ch->equipment[WEAR_WIELD]), j),
			  GET_OBJ_AFFECT(GET_AMMO(ch->equipment[WEAR_WIELD])), TRUE);
		  }
  }

	for (i = 0; i < NUM_BODY_PARTS; i++) {
		if (BIONIC_DEVICE(ch, i))
			for (j = 0; j < MAX_OBJ_APPLY; j++)
				affect_modify_ar(ch, GET_OBJ_APPLY_LOC(BIONIC_DEVICE(ch, i), j),
					GET_OBJ_APPLY_MOD(BIONIC_DEVICE(ch, i), j),
					GET_OBJ_AFFECT(BIONIC_DEVICE(ch, i)), TRUE);
	}
  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  for (afdrug = ch->drugs_used; afdrug; afdrug = afdrug->next)
	affect_modify_ar(ch, afdrug->location, afdrug->modifier, afdrug->bitvector, TRUE);

	// Cant have 100% damage resistance
    for (k = 0; k < MAX_SAVING_THROW; k++)
		GET_SAVE(ch, k) = MAX(-100, MIN(GET_SAVE(ch, k), 90));
    // Make certain values are between 0..30, not < 0 and not > 30!
	maxabil = 30;

    GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), maxabil));
    GET_INT(ch) = MAX(0, MIN(GET_INT(ch), maxabil));
    GET_PCN(ch) = MAX(0, MIN(GET_PCN(ch), maxabil));
    GET_CON(ch) = MAX(0, MIN(GET_CON(ch), maxabil));
    GET_CHA_ABIL(ch) = MAX(0, MIN(GET_CHA_ABIL(ch), maxabil));
    GET_STR(ch) = MAX(0, MIN(GET_STR(ch), maxabil));
	get_encumbered(ch);
}

/* Insert an affect_type in a char_data structure. Automatically sets
 * apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;
    // This will prevent softskills from removing hard-earned skills! -Lump 03/07
    // assuming the only way to get 'perfect' skill level is by chip bionic
    if (af->location == APPLY_SKILL && GET_SKILL_LEVEL(ch, af->modifier) < 100 && GET_SKILL_LEVEL(ch, af->modifier) > 0) {
        send_to_char(ch, "You already know that skill, know-it-all!\r\n");
        af->location = 0;
        af->modifier = 0;
        return;
    }

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  affect_total(ch);
}

/* Remove an affected_type structure from a char (called when duration reaches
 * zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply */
void affect_remove(struct char_data *ch, struct affected_type *af, ...)
{
  struct affected_type *temp;
  int boolmsg = 0;
  va_list exargs;
  va_start(exargs,af);
  boolmsg = va_arg(exargs, int);

  const char *to_room = NULL;
  if (ch->affected == NULL) {
    core_dump();
    return;
  }
    // This will prevent softskills from removing hard-earned skills! -Lump 03/07
    // assuming the only way to get 'perfect' skill level is by chip bionic
    if (af->location != APPLY_SKILL)
        affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
    else if (GET_SKILL_LEVEL(ch, af->modifier) > 100 || GET_SKILL_LEVEL(ch, af->modifier) < 0)
        affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
    
	if (af->type > 0 && af->type <= TOP_PSIONIC_DEFINE) {
		if (psionics_info[af->type].wear_off_msg && boolmsg == 1)
			send_to_char(ch, "%s\r\n", psionics_info[af->type].wear_off_msg);
		if (psionics_info[af->type].rwear_off_msg != NULL && boolmsg == 1) {
			to_room = psionics_info[af->type].rwear_off_msg;
		if (to_room != NULL)
			act(to_room, FALSE, ch, 0, ch, TO_ROOM);
		}
	}

  REMOVE_FROM_LIST(af, ch->affected, next);
  free(af);
  affect_total(ch);
}

/* Call affect_remove with every affect from the spell "type" */
void affect_from_char(struct char_data *ch, int type, bool ack)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp);
  }
}

/* Return TRUE if a char is affected by a spell (SPELL_XXX), FALSE indicates
 * not affected. */
bool affected_by_psionic(struct char_data *ch, int type)
{
    struct affected_type *hjp;

    for (hjp = ch->affected; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return (TRUE);

    return (FALSE);
}

void affect_join(struct char_data *ch, struct affected_type *af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
	af->duration += hjp->duration;
      else if (avg_dur)
        af->duration = (af->duration+hjp->duration)/2;
      if (add_mod)
	af->modifier += hjp->modifier;
      else if (avg_mod)
        af->modifier = (af->modifier+hjp->modifier)/2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

/* move a player out of a room */
void char_from_room(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);
 
  char_from_furniture(ch);

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light is ON */
	world[IN_ROOM(ch)].light--;

  REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room);
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}

/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room)
{
  if (ch == NULL || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		room, top_of_world, ch);
  else {
	if (room == 0)
		mudlog(CMP, LVL_IMMORT, TRUE, "Char_to_room called for room void on %s.", GET_NAME(ch));
		
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    IN_ROOM(ch) = room;
    autoquest_trigger_check(ch, 0, 0, AQ_ROOM_FIND);
    autoquest_trigger_check(ch, 0, 0, AQ_MOB_FIND);

    if (GET_EQ(ch, WEAR_LIGHT))
      if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light ON */
	  world[room].light++;

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
  }
}

/* Give an object to a char. */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
    struct obj_data *temp_obj = NULL;
    struct obj_data *temp_fol = NULL;
    bool found = FALSE;

    if (object && ch) {

        // object->next_content = ch->carrying;
        // ch->carrying = object;
        // object->carried_by = ch;
	
        if (ch->carrying) {
            temp_fol = ch->carrying;
            if (GET_OBJ_VNUM(temp_fol) != GET_OBJ_VNUM(object))
                temp_obj = temp_fol->next_content;
        }

        for (; found != TRUE && temp_obj != NULL; temp_obj = temp_obj->next_content) {

            if (GET_OBJ_VNUM(temp_obj) == GET_OBJ_VNUM(object)) {
                found = TRUE;
                object->next_content = temp_obj;
                temp_fol->next_content = object;
            }

            temp_fol = temp_obj;
        }

        if (!found) {
            object->next_content = ch->carrying;
            ch->carrying = object;
        }

        IN_ROOM(object) = NOWHERE;
        object->carried_by = ch;
        object->worn_on = NOWHERE;
        IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
        IS_CARRYING_N(ch)++;

        autoquest_trigger_check(ch, NULL, object, AQ_OBJ_FIND);

        // set flag for crash-save system, but not on mobs!
        if (!IS_NPC(ch))
            SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
    }
    else
        log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
}

/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  if (!object->carried_by) {
	  log("GAHAN: Object %s tried to be removed from a NULL character.", object->short_description);
	  return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos)
{
  int factor;

  if (GET_EQ(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  switch (eq_pos) {

  case WEAR_BODY:
    factor = 3;
    break;			/* 30% */
  case WEAR_HEAD:
    factor = 2;
    break;			/* 20% */
  case WEAR_LEGS:
    factor = 2;
    break;			/* 20% */
  default:
    factor = 1;
    break;			/* all others 10% */
  }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos, bool ack)
{
  int j;

  if (pos < 0 || pos >= NUM_WEARS) {
    obj_to_char(obj, ch);
	send_to_char(ch, "Something went wrong, notify Gahan with this error code for a free shard: eqc01\r\n");
    return;
  }

  if (GET_EQ(ch, pos)) {
    log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    return;
  }
  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (IN_ROOM(obj) != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (!IS_NPC(ch) && invalid_class(ch, obj)) {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }
        if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_GOD) {
            if ((OBJ_FLAGGED(obj, ITEM_BORG_ONLY) && !IS_BORG(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_CRAZY_ONLY) && !IS_CRAZY(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_MERCENARY_ONLY) && !IS_MERCENARY(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_STALKER_ONLY) && !IS_STALKER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_CALLER_ONLY) && !IS_CALLER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_HIGHLANDER_ONLY) && !IS_HIGHLANDER(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_PREDATOR_ONLY) && !IS_PREDATOR(ch)) ||
                (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY) && !IS_REMORT(ch))) {
                    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
                    obj_to_char(obj, ch);
                    return;
                }
        }

        // level restrictions */
        if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_GOD) {
            if ((OBJ_FLAGGED(obj, ITEM_MINLEV15) && (GET_LEVEL(ch) < 15)) ||
                (OBJ_FLAGGED(obj, ITEM_MINLEV25) && (GET_LEVEL(ch) < 25)) ||
                (OBJ_FLAGGED(obj, ITEM_IMM_ONLY) && (GET_LEVEL(ch) < LVL_IMMORT)) ||
                (OBJ_FLAGGED(obj, ITEM_REMORT_ONLY) && (GET_REMORTS(ch) < GET_OBJ_REMORT(obj)))) {
                    act("You are too inexperienced to use this item.\n\r", FALSE, ch, 0, 0, TO_CHAR);
                    obj_to_char(obj, ch);
                    return;
                }
        }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) += apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[IN_ROOM(ch)].light++;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", GET_NAME(ch));

  for (j = 0; j < MAX_OBJ_APPLY; j++)
    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(obj, j), GET_OBJ_APPLY_MOD(obj, j), GET_OBJ_AFFECT(obj), TRUE);

    // now apply the affects
    for (j = 0; j < NUM_AFF_FLAGS; j++)
        if (OBJAFF_FLAGGED(obj, j))
            obj_affect_to_char(ch, j, ack, obj);

  affect_total(ch);
}

struct obj_data *unequip_char(struct char_data *ch, int pos, bool ack)
{
  int j;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL) {
    //core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) -= apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[IN_ROOM(ch)].light--;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  // remove the location and modifiers here
  for (j = 0; j < MAX_OBJ_APPLY; j++)
    affect_modify_ar(ch, GET_OBJ_APPLY_LOC(obj, j), GET_OBJ_APPLY_MOD(obj, j), GET_OBJ_AFFECT(obj), FALSE);

  // remove the perm affects
  for (j = 0; j < NUM_AFF_FLAGS; j++)
    if (OBJAFF_FLAGGED(obj, j))
      obj_affect_from_char(ch, j, ack);

  affect_total(ch);
  get_encumbered(ch);
  return (obj);
}

int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL) {
    *ppos++ = '\0';
    strlcpy(number, *name, sizeof(number));
    strcpy(*name, ppos);	/* strcpy: OK (always smaller) */

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return (0);

    return (atoi(number));
  }
  return (1);
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int *number, room_rnum room)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = world[room].people; i && *number; i = i->next_in_room)
    if (isname(name, i->player.name))
      if (--(*number) == 0)
	return (i);

  return (NULL);
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room)
{
    struct obj_data *temp_obj = NULL;
    struct obj_data *temp_fol = NULL;
    bool found = FALSE;

    if (!object || room == NOWHERE || room > top_of_world)
        log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)", room, top_of_world, object);
    else {

        temp_fol = world[room].contents;
        if (temp_fol != NULL && GET_OBJ_VNUM(temp_fol) != GET_OBJ_VNUM(object))
            temp_obj = temp_fol->next_content;

        for (; found != TRUE && temp_obj != NULL; temp_obj = temp_obj->next_content) {

            if (GET_OBJ_VNUM(temp_obj) == GET_OBJ_VNUM(object)) {
                found = TRUE;
                temp_fol->next_content = object;
                object->next_content = temp_obj;
            }

            temp_fol = temp_obj;
        }

        if (!found) {
            object->next_content = world[room].contents;
            world[room].contents = object;
        }

        IN_ROOM(object) = room;
        object->carried_by = NULL;
        if (ROOM_FLAGGED(room, ROOM_HOUSE))
            SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

    }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;
  struct char_data *t, *tempch;

  if (!object || IN_ROOM(object) == NOWHERE) {
    log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	object, IN_ROOM(object));
    return;
  }

   /* if people are sitting in it, toss their butt to the ground */
  if (OBJ_SAT_IN_BY(object) != NULL) {
    for (tempch = OBJ_SAT_IN_BY(object); tempch; tempch = t) {
       t = NEXT_SITTING(tempch);
       SITTING(tempch) = NULL;
       NEXT_SITTING(tempch) = NULL;
    }
  }

  REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content);

  if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
    SET_BIT_AR(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
  IN_ROOM(object) = NOWHERE;
  object->next_content = NULL;
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
	obj, obj, obj_to);
    return;
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  tmp_obj = obj->in_obj;

  /* Add weight to container, unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

    /* top level object.  Subtract weight from inventory if necessary. */
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
    if (tmp_obj->carried_by)
      IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

    /* Subtract weight from char that carries the object */
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
    if (temp->carried_by)
      IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  }
  obj->in_obj = NULL;
  obj->next_content = NULL;
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}

/* Extract an object from the world */
void extract_obj(struct obj_data *obj)
{
  struct char_data *ch, *next = NULL;
  struct obj_data *temp, *plastique;
  struct bionic_data *bionic;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on, TRUE) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  if (OBJ_SAT_IN_BY(obj)){
    for (ch = OBJ_SAT_IN_BY(obj); OBJ_SAT_IN_BY(obj); ch = next){
      if (!NEXT_SITTING(ch))
        OBJ_SAT_IN_BY(obj) = NULL;
      else
        OBJ_SAT_IN_BY(obj) = (next = NEXT_SITTING(ch));
      SITTING(ch) = NULL;
      NEXT_SITTING(ch) = NULL;
    }
  }

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

    if (obj->plastique) {
        plastique = obj->plastique;
        obj->plastique = NULL;
        extract_obj(plastique);
    }

    // remove bionics, if any (corpses)
    bionic = obj->bionics;
    while (bionic) {
        temp = bionic->device;
        bionic = bionic->next;
        extract_obj(temp);
    }

  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) != NOTHING)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  free_obj(obj);
}

static void update_object(struct obj_data *obj, int use)
{
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}

void update_char_objects(struct char_data *ch)
{
  int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0) {
	i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
	if (i == 1) {
	  send_to_char(ch, "Your light begins to flicker and fade.\r\n");
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (i == 0) {
	  send_to_char(ch, "Your light sputters out and dies.\r\n");
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[IN_ROOM(ch)].light--;
	}
      }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      update_object(GET_EQ(ch, i), 2);

  if (ch->carrying)
    update_object(ch->carrying, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch)
{
  struct char_data *k, *temp;
  struct descriptor_data *d;
  struct obj_data *obj, *part;
  int i;

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
        GET_NAME(ch), __FILE__);
    exit(1);
  }

  /* We're booting the character of someone who has switched so first we need
   * to stuff them back into their own body.  This will set ch->desc we're
   * checking below this loop to the proper value. */
  if (!IS_NPC(ch) && !ch->desc) {
    for (d = descriptor_list; d; d = d->next)
      if (d->original == ch) {
	do_return(d->character, NULL, 0, 0);
        break;
      }
  }

  if (ch->desc) {
    /* This time we're extracting the body someone has switched into (not the
     * body of someone switching as above) so we need to put the switcher back
     * to their own body. If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu. */
    if (ch->desc->original)
      do_return(ch, NULL, 0, 0);
    else {
      /* Now we boot anybody trying to log in with the same character, to help
       * guard against duping.  CON_DISCONNECT is used to close a descriptor
       * without extracting the d->character associated with it, for being
       * link-dead, so we want CON_CLOSE to clean everything up. If we're
       * here, we know it's a player so no IS_NPC check required. */
      for (d = descriptor_list; d; d = d->next) {
        if (d == ch->desc)
          continue;
        if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
          STATE(d) = CON_CLOSE;
      }
      STATE(ch->desc) = CON_MENU;
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }

  /* On with the character's assets... */
  if (ch->followers || ch->master)
    die_follower(ch);

  /* transfer objects to room, if any */
  while (ch->carrying) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, IN_ROOM(ch));
  }

  /* transfer equipment to room, if any */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i, FALSE), IN_ROOM(ch));

  for (i = 1; i < NUM_BODY_PARTS; i++) {
    if (BIONIC_DEVICE(ch, i)) {
		part = (BIONIC_DEVICE(ch, i));
		extract_obj(part);
        BIONIC_DEVICE(ch, i) = NULL;
    }
  }

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }
  /* we can't forget the hunters either... */
  for (temp = character_list; temp; temp = temp->next)
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;

  char_from_room(ch);
  // if ch has a room occupied as a vehicle, open it up
  if (ch->char_specials.vehicle.vehicle_room_vnum != 0)
	REMOVE_BIT_AR(ROOM_FLAGS(real_room(ch->char_specials.vehicle.vehicle_room_vnum)), ROOM_OCCUPIED);
  if (IS_NPC(ch)) {
    if (GET_MOB_RNUM(ch) != NOTHING)	/* prototyped */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);

    if (SCRIPT(ch))
      extract_script(ch, MOB_TRIGGER);

    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));
  } else {
    save_char(ch);
    Crash_delete_crashfile(ch);
  }

  /* If there's a descriptor, they're in the menu now. */
  if (IS_NPC(ch) || !ch->desc)
    free_char(ch);
}

/* Why do we do this? Because trying to iterate over the character list with
 * 'ch = ch->next' does bad things if the current character happens to die. The
 * trivial workaround of 'vict = next_vict' doesn't work if the _next_ person
 * in the list gets killed, for example, by an area spell. Why do we leave them
 * on the character_list? Because code doing 'vict = vict->next' would get
 * really confused otherwise. */
void extract_char(struct char_data *ch)
{
  char_from_furniture(ch);

  if (IS_NPC(ch))
    SET_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  else
    SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);

  extractions_pending++;
}

/* I'm not particularly pleased with the MOB/PLR hoops that have to be jumped
 * through but it hardly calls for a completely new variable. Ideally it would
 * be its own list, but that would change the '->next' pointer, potentially
 * confusing some code. -gg This doesn't handle recursive extractions. */
void extract_pending_chars(void)
{
  struct char_data *vict, *next_vict, *prev_vict;

  if (extractions_pending < 0)
    log("SYSERR: Negative (%d) extractions pending.", extractions_pending);

  for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict) {
    next_vict = vict->next;

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    extract_char_final(vict);
    extractions_pending--;

    if (prev_vict)
      prev_vict->next = next_vict;
    else
      character_list = next_vict;
  }

  if (extractions_pending > 0)
    log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);

  extractions_pending = 0;
}

/* Here follows high-level versions of some earlier routines, ie functions
 * which incorporate the actual player-data */
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (str_cmp(i->player.name, name)) /* If not same, continue */
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}

struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room)
    if (isname(name, i->player.name))
      if (CAN_SEE(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return get_player_vis(ch, name, NULL, 0);

  for (i = character_list; i && *number; i = i->next) {
    if (IN_ROOM(ch) == IN_ROOM(i))
      continue;
    if (!isname(name, i->player.name))
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;

    return (i);
  }
  return (NULL);
}

struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(ch, name, number);
  else
    return (NULL);
}

struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = list; i && *number; i = i->next_content)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *number)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, number, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, number, world[IN_ROOM(ch)].contents)) != NULL)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && *number; i = i->next)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (NULL);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (equipment[j]);

  return (NULL);
}

int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (j);

  return (-1);
}

const char *money_desc(int amount)
{
  int cnt;
  struct {
    int limit;
    const char *description;
  } money_table[] = {
    {          1, "a single resource unit"			},
    {        100, "a tiny pile of resource units"		},
    {        500, "a handful of resource units"		},
    {       1000, "a little pile of resource units"		},
    {       5000, "a small pile of resource units"		},
    {      10000, "a pile of resource units"		},
    {      25000, "a big pile of resource units"		},
    {      50000, "a large heap of resource units"		},
    {      75000, "a huge mound of resource units"		},
    {     100000, "an enormous mound of resource units"	},
    {     250000, "a small mountain of resource units"	},
    {     500000, "a mountain of resource units"		},
    {     750000, "a huge mountain of resource units"	},
    {    1000000, "an enormous mountain of resource units"	},
    {          0, NULL					},
  };

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of resource units");
}

struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];
  int y;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("resource unit");
    obj->short_description = strdup("a resource unit");
    obj->description = strdup("One lousy resource unit is lying here.");
    new_descr->keyword = strdup("resource units");
    new_descr->description = strdup("It's just one lousy resource unit.");
  } else {
    obj->name = strdup("resource units");
    obj->short_description = strdup(money_desc(amount));
    snprintf(buf, sizeof(buf), "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("resource units");
    if (amount < 10)
      snprintf(buf, sizeof(buf), "There are %d units.", amount);
    else if (amount < 100)
      snprintf(buf, sizeof(buf), "There are about %d units.", 10 * (amount / 10));
    else if (amount < 1000)
      snprintf(buf, sizeof(buf), "It looks to be about %d units.", 100 * (amount / 100));
    else if (amount < 100000)
      snprintf(buf, sizeof(buf), "You guess there are, maybe, %d units.",
	      1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
    else
      strcpy(buf, "There are a LOT of units.");	/* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->obj_flags.wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
}

/* Generic Find, designed to find any object orcharacter.
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in. */
int generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		     struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int i, found, number;
  char name_val[MAX_INPUT_LENGTH];
  char *name = name_val;

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);
  if (!(number = get_number(&name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0) {
	*tar_obj = GET_EQ(ch, i);
	found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4);	/* strcpy: OK (always less) */
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}

// return if a room is affected by a psionic, NULL indicates not affected */
bool room_affected_by_psionic(room_rnum room, sh_int type)
{
    struct affected_type *affect;

    if (room == NOWHERE)
        return (FALSE);

    for (affect = world[room].affected; affect; affect = affect->next)
        if (affect->type == type)
            return (TRUE);

    return (FALSE);
}

void affect_to_room(room_rnum room, struct affected_type *af)
{
    struct affected_type *affected_alloc;
    bool found;
    struct affected_rooms *room_af_temp;
    struct affected_rooms *room_af_add;

    CREATE(affected_alloc, struct affected_type, 1);
    *affected_alloc = *af;
    affected_alloc->next = world[room].affected;
    world[room].affected = affected_alloc;

    // flag is already set!
    if (!ROOM_FLAGGED(room, af->bitvector[0]))
        SET_BIT_AR(ROOM_FLAGS(room), af->bitvector[0] -1);
	else
		REMOVE_BIT_AR(ROOM_FLAGS(room), af->bitvector[0] -1);

    found = FALSE;
    for (room_af_temp = room_af_list; room_af_temp; room_af_temp = room_af_temp->room_next)
        if (room_af_temp->room_number == room)
            found = TRUE;

    if (found == FALSE) {
        CREATE(room_af_add, struct affected_rooms, 1);
        room_af_add->room_number = room;
        room_af_add->room_next = room_af_list;
        room_af_list = room_af_add;
    }
}

// todo: this does not seem to be called
void affect_from_room(room_rnum room, byte room_affection)
{
    struct affected_type *temp;

    for (temp = world[room].affected; temp; temp = temp->next)
        if (temp->type == room_affection)
            room_affect_remove(room, temp);
}
void room_affect_modify(room_rnum room, int bitv, bool add)
{
    if (add)
        SET_BIT_AR(ROOM_FLAGS(room), bitv);
    else
        REMOVE_BIT_AR(ROOM_FLAGS(room), bitv);
}
//void affect_modify(struct char_data * ch, byte loc, sh_int mod, long bitv[], bool add)
//{
//    if (add) {
//		        if(IS_SET_AR(bitv, (i*32)+j))
//          SET_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
//        // todo: is this the best way to prevent AFF_DONTUSE from being used?
//        if (bitv != AFF_DONTUSE)
//            SET_BIT_AR(AFF_FLAGS(ch), bitv);
//    }
//    else {
//        REMOVE_BIT_AR(AFF_FLAGS(ch), bitv);
//        mod = -mod;
//    }
//
//    aff_apply_modify(ch, loc, mod, "affect_modify");
//}

void room_affect_remove(room_rnum room, struct affected_type *af)
{
    struct affected_type *temp;
    struct affected_rooms *room_af_temp1;
    struct affected_rooms *room_af_temp2;

    assert(world[room].affected);

    room_affect_modify(room, af->bitvector[0], FALSE);

    // remove structure from the room affected list
    if (world[room].affected == af)
        world[room].affected = af->next;
    else {

        for (temp = world[room].affected; (temp->next) && (temp->next != af); temp = temp->next);

        if (temp->next != af) {
            log("SYSERR: ERROR: Could not locate affected_type in world->affected. (handler.c)");
            exit(1);
        }

        temp->next = af->next;
    }

    free(af);

    // if removing this affect makes the room completely unaffected, remove it from the global list
    if (world[room].affected == NULL) {

        room_af_temp1 = room_af_list;
        room_af_temp2 = room_af_temp1;

        if (room_af_temp1->room_number == room)
            room_af_list = room_af_temp1->room_next;
        else {
            for (; (room_af_temp1->room_next) && (room_af_temp1->room_number != room); room_af_temp1 = room_af_temp1->room_next)
                room_af_temp2 = room_af_temp1;

            if (room_af_temp1->room_number != room) {
                log("SYSERR: ERROR: Could not locate room in the room affections list (handler.c)");
                exit(1);
            }

            room_af_temp2->room_next = room_af_temp1->room_next;
        }

        free(room_af_temp1);
    }
}
// Based on a combination of affect_to_char, Tails' drug names, Mac's inject code
void drug_to_char(struct char_data *ch, struct affected_type * af)
{
    struct affected_type *affected_alloc;

    CREATE(affected_alloc, struct affected_type, 1);

    *affected_alloc = *af;
    affected_alloc->next = ch->drugs_used;
    ch->drugs_used = affected_alloc;

    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
    affect_total(ch);
}

// Remove a affected_type structure from a char (called when duration reaches zero).
// Pointer *af must never be NIL!  Frees mem and calls affect_location_apply
void drug_remove(struct char_data * ch, struct affected_type * af)
{
    struct affected_type *temp;

    assert(ch->drugs_used);

    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);

    REMOVE_FROM_LIST(af, ch->drugs_used, next);
    free(af);
    affect_total(ch);
}

// Call drug_remove with every match of drug type
void drug_from_char(struct char_data * ch, int type)
{
    struct affected_type *hjp;
    struct affected_type *next;

    for (hjp = ch->drugs_used; hjp; hjp = next) {
        next = hjp->next;
        if (hjp->type == type)
            drug_remove(ch, hjp);
    }
}

// Return if a char is affected by a drug (DRUG_XXX), NULL indicates not affected
bool affected_by_drug(struct char_data * ch, int type)
{
    struct affected_type *hjp;

    for (hjp = ch->drugs_used; hjp; hjp = hjp->next)
        if (hjp->type == type)
            return (TRUE);

    return (FALSE);
}

void drug_join(struct char_data * ch, struct affected_type * af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
    struct affected_type *hjp;
    bool found = FALSE;

    for (hjp = ch->drugs_used; !found && hjp; hjp = hjp->next) {

        if ((hjp->type == af->type) && (hjp->location == af->location)) {
			if (add_dur)
				af->duration += hjp->duration;
			else if (avg_dur)
		        af->duration = (af->duration+hjp->duration)/2;
			if (add_mod)
				af->modifier += hjp->modifier;
			else if (avg_mod)
				af->modifier = (af->modifier+hjp->modifier)/2;

            drug_remove(ch, hjp);
            drug_to_char(ch, af);
            found = TRUE;
        }
    }

    if (!found)
        drug_to_char(ch, af);
}
// GAHANTODO: Fix this embarassing shit
void obj_affect_to_char(struct char_data *ch, int i, bool ack, struct obj_data *obj)
{
    switch (i) {

        case AFF_SANCT:
            call_psionic(ch, ch, obj, PSIONIC_SANCTUARY, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_SUPER_SANCT:
            call_psionic(ch, ch, obj, PSIONIC_SUPER_SANCT, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_ARMORED_AURA:
            call_psionic(ch, ch, obj, PSIONIC_ARMORED_AURA, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_PSI_SHIELD:
            call_psionic(ch, ch, obj, PSIONIC_PSI_SHIELD, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_INVISIBLE:
            call_psionic(ch, ch, obj, PSIONIC_INVISIBILITY, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_DETECT_INVIS:
            call_psionic(ch, ch, obj, PSIONIC_DETECT_INVISIBILITY, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_LEVITATE:
            call_psionic(ch, ch, obj, PSIONIC_LEVITATE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_HASTE:
            call_psionic(ch, ch, obj, PSIONIC_HASTE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_BIO_TRANCE:
            call_psionic(ch, ch, obj, PSIONIC_BIO_TRANCE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_SENSE_LIFE:
            call_psionic(ch, ch, obj, PSIONIC_SENSE_LIFE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_THERMAL_AURA:
            call_psionic(ch, ch, obj, PSIONIC_THERMAL_AURA, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_COOL_AURA:
            call_psionic(ch, ch, obj, PSIONIC_AURA_OF_COOL, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_MEGA_SANCT:
            call_psionic(ch, ch, obj, PSIONIC_MEGA_SANCT, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_SECOND_SIGHT:
            call_psionic(ch, ch, obj, PSIONIC_SECOND_SIGHT, EQ_PSI_LEVEL, 0, ack);
            break;

		case AFF_INFRAVISION:
			call_psionic(ch, ch, obj, PSIONIC_INFRAVISION, EQ_PSI_LEVEL, 0, ack);
			break;

        case AFF_DUPLICATES:
            call_psionic(ch, ch, obj, PSIONIC_DUPLICATES, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_INDESTRUCT_AURA:
            call_psionic(ch, ch, obj, PSIONIC_INDESTRUCTABLE_AURA, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_INSPIRE:
            call_psionic(ch, ch, obj, PSIONIC_INSPIRE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_VORPAL_SPEED:
            call_psionic(ch, ch, obj, PSIONIC_VORPAL_SPEED, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_FANATICAL_FURY:
            call_psionic(ch, ch, obj, PSIONIC_FANATICAL_FURY, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_LETH_IMMUNE:
            call_psionic(ch, ch, obj, PSIONIC_LETHARGY_IMMUNE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_BLIND_IMMUNE:
            call_psionic(ch, ch, obj, PSIONIC_BLIND_IMMUNE, EQ_PSI_LEVEL, 0, ack);
            break;

        case AFF_COURAGE:
            call_psionic(ch, ch, obj, PSIONIC_COURAGE, EQ_PSI_LEVEL, 0, ack);
            break;

		case AFF_BLEEDING:
			call_psionic(ch, ch, obj, PSIONIC_BLEED, EQ_PSI_LEVEL, 0, ack);
			break;

		case AFF_BIO_REGEN:
			call_psionic(ch, ch, obj, PSIONIC_BIO_REGEN, EQ_PSI_LEVEL, 0, ack);
			break;

		case AFF_COMBAT_MIND:
			call_psionic(ch, ch, obj, PSIONIC_COMBAT_MIND, EQ_PSI_LEVEL, 0, ack);
			break;

        default:
            // must not be a psionic
            if (!AFF_FLAGGED(ch, i))
                SET_BIT_AR(AFF_FLAGS(ch), i);
            break;

    }
}

// todo: this should only remove psionics put on the char by the equipment
// instead it will remove a psionic no matter how it got there.
void obj_affect_from_char(struct char_data *ch, int i, bool ack)
{
    switch (i) {

        case AFF_SANCT:
            if (affected_by_psionic(ch, PSIONIC_SANCTUARY))
                affect_from_char(ch, PSIONIC_SANCTUARY, ack);
            break;

        case AFF_SUPER_SANCT:
            if (affected_by_psionic(ch, PSIONIC_SUPER_SANCT))
                affect_from_char(ch, PSIONIC_SUPER_SANCT, ack);
            break;

        case AFF_ARMORED_AURA:
            if (affected_by_psionic(ch, PSIONIC_ARMORED_AURA))
                affect_from_char(ch, PSIONIC_ARMORED_AURA, ack);
            break;

        case AFF_PSI_SHIELD:
            if (affected_by_psionic(ch, PSIONIC_PSI_SHIELD))
                affect_from_char(ch, PSIONIC_PSI_SHIELD, ack);
            break;

        case AFF_INVISIBLE:
            if (affected_by_psionic(ch, PSIONIC_INVISIBILITY))
                affect_from_char(ch, PSIONIC_INVISIBILITY, ack);
            break;

        case AFF_DETECT_INVIS:
            if (affected_by_psionic(ch, PSIONIC_DETECT_INVISIBILITY))
                affect_from_char(ch, PSIONIC_DETECT_INVISIBILITY, ack);
            break;

        case AFF_LEVITATE:
            if (affected_by_psionic(ch, PSIONIC_LEVITATE))
                affect_from_char(ch, PSIONIC_LEVITATE, ack);
            break;

        case AFF_HASTE:
            if (affected_by_psionic(ch, PSIONIC_HASTE))
                affect_from_char(ch, PSIONIC_HASTE, ack);
            break;

        case AFF_BIO_TRANCE:
            if (affected_by_psionic(ch, PSIONIC_BIO_TRANCE))
                affect_from_char(ch, PSIONIC_BIO_TRANCE, ack);
            break;

        case AFF_SENSE_LIFE:
            if (affected_by_psionic(ch, PSIONIC_SENSE_LIFE))
                affect_from_char(ch, PSIONIC_SENSE_LIFE, ack);
            break;

        case AFF_THERMAL_AURA:
            if (affected_by_psionic(ch, PSIONIC_THERMAL_AURA))
                affect_from_char(ch, PSIONIC_THERMAL_AURA, ack);
            break;

        case AFF_COOL_AURA:
            if (affected_by_psionic(ch, PSIONIC_AURA_OF_COOL))
                affect_from_char(ch, PSIONIC_AURA_OF_COOL, ack);
            break;

        case AFF_MEGA_SANCT:
            if (affected_by_psionic(ch, PSIONIC_MEGA_SANCT))
                affect_from_char(ch, PSIONIC_MEGA_SANCT, ack);
            break;

        case AFF_SECOND_SIGHT:
            if (affected_by_psionic(ch, PSIONIC_SECOND_SIGHT))
                affect_from_char(ch, PSIONIC_SECOND_SIGHT, ack);
            break;

		case AFF_INFRAVISION:
			if (affected_by_psionic(ch, PSIONIC_INFRAVISION))
				affect_from_char(ch, PSIONIC_INFRAVISION, ack);
			break;

        case AFF_DUPLICATES:
            //       if (affected_by_psionic(ch, PSIONIC_DUPLICATES))
            //         affect_from_char(ch, PSIONIC_DUPLICATES, ack);
            // Dupes stay on, to prevent re-casting with save
            break;

        case AFF_INDESTRUCT_AURA:
            if (affected_by_psionic(ch, PSIONIC_INDESTRUCTABLE_AURA))
                affect_from_char(ch, PSIONIC_INDESTRUCTABLE_AURA, ack);
            break;

        case AFF_INSPIRE:
            if (affected_by_psionic(ch, PSIONIC_INSPIRE))
                affect_from_char(ch, PSIONIC_INSPIRE, ack);
            break;

        case AFF_VORPAL_SPEED:
            if (affected_by_psionic(ch, PSIONIC_VORPAL_SPEED))
                affect_from_char(ch, PSIONIC_VORPAL_SPEED, ack);
            break;

        case AFF_FANATICAL_FURY:
            if (affected_by_psionic(ch, PSIONIC_FANATICAL_FURY))
                affect_from_char(ch, PSIONIC_FANATICAL_FURY, ack);
            break;

        case AFF_LETH_IMMUNE:
            if (affected_by_psionic(ch, PSIONIC_LETHARGY_IMMUNE))
                affect_from_char(ch, PSIONIC_LETHARGY_IMMUNE, ack);
            break;

        case AFF_BLIND_IMMUNE:
            if (affected_by_psionic(ch, PSIONIC_BLIND_IMMUNE))
                affect_from_char(ch, PSIONIC_BLIND_IMMUNE, ack);
            break;

        case AFF_COURAGE:
            if (affected_by_psionic(ch, PSIONIC_COURAGE))
                affect_from_char(ch, PSIONIC_COURAGE, ack);
            break;

		case AFF_BLEEDING:
			if (affected_by_psionic(ch, PSIONIC_BLEED))
				affect_from_char(ch, PSIONIC_BLEED, ack);
			break;

		case AFF_BIO_REGEN:
			if (affected_by_psionic(ch, PSIONIC_BIO_REGEN))
				affect_from_char(ch, PSIONIC_BIO_REGEN, ack);
			break;
		case AFF_COMBAT_MIND:
			if (affected_by_psionic(ch, PSIONIC_COMBAT_MIND))
				affect_from_char(ch, PSIONIC_COMBAT_MIND, ack);
			break;

        default:
            // must not be a psionic
            if (AFF_FLAGGED(ch, i))
                REMOVE_BIT_AR(AFF_FLAGS(ch), i);
            break;
   }
}

struct char_data *get_lineofsight(struct char_data *ch, struct char_data *victim, char *name, char *direction)
{
    struct char_data *i;
	struct obj_data *obj;
	int aim_type;
	int is_in, dir, dis, maxdis, found = 0;

	obj = GET_EQ(ch, WEAR_WIELD);

	if (obj == NULL) {
		send_to_char(ch, "You need to be wielding a ranged weapon in order to fight back!\r\n");
		if (RANGED_FIGHTING(ch))
			stop_ranged_fighting(ch);
		return (NULL);
	}
	else {
		maxdis = GET_RANGE(obj);
		
		if (maxdis <= 0) {
			send_to_char(ch, "You need a weapon with better range to hit your target from here.\r\n");
			return (NULL);
		}
	}
	if (IS_NPC(ch) && !IS_NPC(victim)) {
		maxdis = GET_RANGE(obj);
		if (maxdis < 9)
			maxdis = 9;
	}

	is_in = ch->in_room;

	if (direction == NULL) {

		if (ch == victim) {
			for (dir = 0; dir < NUM_OF_DIRS; dir++) {
				ch->in_room = is_in;
				for (dis = 0; dis <= maxdis; dis++) {
					if (((dis == 0) && (dir == 0)) || (dis > 0) || found == 0) {
						for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
							if (!((ch == i) && (dis == 0)) && found == 0) {
								if (isname(name, i->player.name)) {
									if (CAN_SEE(ch, i)) {
										ch->char_specials.tmpdir = dir;
										ch->in_room = is_in;
										return (i);
									}
								}
							}
						}
					}
						
					if (!CAN_GO(ch, dir) || (world[ch->in_room].dir_option[dir]->to_room== is_in))
						break;
					else if (ROOM_FLAGGED(world[ch->in_room].dir_option[dir]->to_room, ROOM_NOTRACK)) {
						send_to_char(ch, "Interference begins to scramble your targeting systems.\r\n");
						ch->in_room = is_in;
						return (NULL);
					}
					else
						ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
				}
			}
		ch->in_room = is_in;
		return (NULL);
		}
		else {
			for (dir = 0; dir < NUM_OF_DIRS; dir++) {
				ch->in_room = is_in;
				for (dis = 0; dis <= maxdis; dis++) {
					if (((dis == 0) && (dir == 0)) || (dis > 0) || found == 0) {
						for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
							if (!((ch == i) && (dis == 0)) && found == 0) {
								if (i == victim) {
									if (CAN_SEE(ch, i)) {
										ch->in_room = is_in;
										ch->char_specials.tmpdir = dir;
										return (i);
									}
								}
							}
						}
					}
					if (!CAN_GO(ch, dir) || (world[ch->in_room].dir_option[dir]->to_room== is_in))
						break;

					else if (ROOM_FLAGGED(world[ch->in_room].dir_option[dir]->to_room, ROOM_NOTRACK)) {
						send_to_char(ch, "Interference begins to scramble your targeting systems.\r\n");
						ch->in_room = is_in;
						return (NULL);
					}
					else
						ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
				}
			}
		ch->in_room = is_in;
		return (NULL);
		}
	}
	else {
		aim_type = search_block(direction, dirs, FALSE);
		dir = aim_type;
		if (aim_type >= 0) {
			while (aim_type == dir) {
				ch->in_room = is_in;
				for (dis = 0; dis <= maxdis; dis++) {
					if (((dis == 0) && (dir == 0)) || (dis > 0) || found == 0) {
						for (i = world[IN_ROOM(ch)].people; i; i = i->next_in_room) {
							if (!((ch == i) && (dis == 0)) && found == 0) {
								if (isname(name, i->player.name)) {
									if (CAN_SEE(ch, i)) {
										ch->char_specials.tmpdir = dir;
										ch->in_room = is_in;
										return (i);
									}
								}
							}
						}
					}
						
					if (!CAN_GO(ch, dir) || (world[ch->in_room].dir_option[dir]->to_room== is_in))
						break;
					else if (ROOM_FLAGGED(world[ch->in_room].dir_option[dir]->to_room, ROOM_NOTRACK)) {
						send_to_char(ch, "Interference begins to scramble your targeting systems.\r\n");
						ch->in_room = is_in;
						return (NULL);
					}
					else
						ch->in_room = world[ch->in_room].dir_option[dir]->to_room;
				}
				ch->in_room = is_in;
				return (NULL);
			}
			ch->in_room = is_in;
			return (NULL);
		}
		else {
			send_to_char(ch, "You have specified an invalid direction.\r\n");
			return (NULL);
		}
	}
}

// New fight system TAG: NEWFIGHT
void execute_special_psionics(void)
{
    struct char_data *ch;
    struct char_data *victim = NULL;
	int dam = 0;
	
	for (ch = character_list; ch; ch = ch->next) {
		// Look for characters with queue'd up shots
		if (GET_POS(ch) >= POS_FIGHTING) {
			if (GET_PSIBULLETS(ch) > 0) {
				if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
					send_to_char(ch, "You can't seem to keep your psionic bullets targeted on your victim.\r\n");
					GET_PSIBULLETS(ch) = 0;
				}
				else {
					dam = GET_LEVEL(ch) * (rand_number(20,60));
					victim = FIGHTING(ch);
					if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
						send_to_char(ch, "[%d] ", dam);
					act("$n releases a glowing hot bullet of pure energy at $N knocking them back!", FALSE, ch, 0, victim, TO_ROOM);
					act("You release a glowing hot bullet of pure energy at $N knocking them back!", FALSE, ch, 0, victim, TO_CHAR);
					act("You are knocked back as $n fires a glowing hot bullet of pure energy at you!", FALSE, ch, 0, victim, TO_VICT);
					GET_PSIBULLETS(ch)--;
					damage(ch, victim, dam, PSIONIC_PSI_BULLET, DMG_PSIONIC);
					update_pos(victim);
					return;
				}
			}
		}
		GET_PSIBULLETS(ch) = 0;
	}
}
void execute_special_psionics_caller(void)
{
    struct char_data *ch;
    struct char_data *victim = NULL;
	int dam = 0;

	for (ch = character_list; ch; ch = ch->next) {
		// Look for characters with queue'd up shots
		if (GET_POS(ch) >= POS_FIGHTING) {
			if (GET_CLAWS(ch) > 0) {
				if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
					send_to_char(ch, "You can't seem to find your victim.\r\n");
					GET_CLAWS(ch) = 0;
				}
				else {


	        if (GET_EQ(ch, WEAR_HOLD) && (GET_EQ(ch, WEAR_WIELD))) {
           send_to_char(ch, " \t[f420]You must have a free hand to morph a tigerclaw.@n\r\n");
           GET_CLAWS(ch) = 0;
           return;
        }


					dam = GET_LEVEL(ch) * (rand_number(20,60));
					victim = FIGHTING(ch);
					if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
						send_to_char(ch, "[%d] ", dam);

//        act("$n swipes $s \t[f420]Tiger Claw@n at $N, Cutting deeply!", FALSE, ch, 0, victim, TO_ROOM);
//        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
//        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);
  switch (rand_number(0, 6)) {
  case 0:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 1:
        act("$N runs and dives at $n, slicing and clawing $s face to shreds!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You run and dive at $N, @Rslicing and clawing@n $S face to shreds!", FALSE, ch, 0, victim, TO_CHAR);
        act("$n runs and dives at you, @Rslicing@n and @Rclawing@n your face to shreds!", FALSE, ch, 0, victim, TO_VICT);
    break;
  case 2:
        act("$N brandishes the claw of a tiger and pounds $S sharp claws into $n's chin with a @Ytiger uppercut!@n",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You pound your \t[f420]Tiger Claw@n perfectly under $N's chin. $E is shaken from the uppercut!", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel @WPhenomenal pain@n as $n pounds you with a \t[f420]Tiger Claw@n uppercut.", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 3:
        act("$N stretches out $S \t[f420]Tiger Claw@n and takes a broad swipe at $n. ",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You stretch out your \t[f420]Tiger Claw@n and take a broad swipe at $N. That looks painful!", FALSE, ch, 0, victim, TO_CHAR);
        act("You feel wounds opening up as $n takes a broad swipe with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 4:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 5:
        act("$N leaps into the air with $S \t[f420]Tiger Claw@n flailing at $n, Blood covers the room!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You leap into the air with your \t[f420]Tiger Claw!@n flailing at $N, @RBlood covers the room!.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("You notice $n leaping at you with $s \t[f420]Tiger Claw!@n flailing, blood covers the room as you feel the sudden pain from the cuts!", FALSE, ch, 0, victim, TO_VICT);

    break;
  case 6:
        act("$N spins in a circle with $S \t[f420]Tiger Claw@n targeting $n, slice and diced!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You hold out your \t[f420]Tiger Claw!@n and spin in a circle towards $N. Slice and Diced!", FALSE, ch, 0, victim, TO_CHAR);
        act("Spinning towards you, $n's \t[f420]Tiger Claw!@n cuts you multiple times!", FALSE, ch, 0, victim, TO_VICT);

    break;
  default:
        act("$N swipes $S \t[f420]Tiger Claw@n at $n, Cutting deeply!",FALSE, victim, 0, ch, TO_NOTVICT);
        act("You @Wswipe ferociously@n at $N with your \t[f420]Tiger Claw!@n @RBlood oozes from the cuts.@n", FALSE, ch, 0, victim, TO_CHAR);
        act("@RBlood oozes@n from your lacerations as $n swipes you with $s \t[f420]Tiger Claw!@n", FALSE, ch, 0, victim, TO_VICT);

    break;
  }

					GET_CLAWS(ch)--;
					damage(ch, victim, dam, TYPE_CLAW, DMG_PSIONIC);
					update_pos(victim);
					return;
				}
			}
		}
		GET_CLAWS(ch) = 0;
	}
}

void events_per_second(void)
{
	struct char_data *ch;

	for (ch = character_list; ch; ch = ch->next) {

		if (IS_NPC(ch) && FUN_HUNTING(ch)) {
			int roll = rand_number(1,100);
			if (roll >= 50)
				fun_hunt(ch);
		}
		
		if (!IS_NPC(ch) && AFF_FLAGGED(ch, AFF_AUTOPILOT)) {
			if (!IN_VEHICLE(ch)) {
				send_to_char(ch, "Somehow you're vehicle has been lost during the autopilot process.  Please report the incident to Gahan.\r\n");
				REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
			}
			if (real_room(AUTOPILOT_ROOM(ch)) != WINDOW_VNUM(ch))
				autopilot_move(ch);
			else {
				REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_AUTOPILOT);
				send_to_char(ch, "The voice through your GPS navigation says, 'You've arrived at your destination'\r\n");
			}
		}
		
		if (!IS_NPC(ch) && !ch->desc) {
			if (ch->char_specials.timer >= 10) {
				struct follow_type *f;
				struct follow_type *next_fol;
				bool grouped = TRUE;

                act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);

	            Crash_rentsave(ch, 0);

				if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
					grouped = FALSE;
				}

				if (grouped == TRUE) {
					for (f = ch->followers; f; f = next_fol) {

						next_fol = f->next;

						if (AFF_FLAGGED(f->follower, AFF_GROUP)) {

							REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
							act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
				
							if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM)) {
								extract_char(f->follower);
								send_to_room(IN_ROOM(f->follower), "%s races out of the room after being free'd by its master!\r\n", f->follower->player.short_descr);
							}
							else 
								stop_follower(f->follower);
						}
					}
				}

			REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
			extract_char(ch);        /* Char is saved before extracting. */
			}
		}
	}
}
/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */
void extract_char_death(struct char_data *ch)
{
    struct char_data *temp;
	struct obj_data *obj;

	GET_WAIT_STATE(ch) = PULSE_SECOND;

	// we can't forget the hunters either...
    for (temp = character_list; temp; temp = temp->next)
        if (HUNTING(temp) == ch)
            HUNTING(temp) = NULL;

	char_from_furniture(ch);
	int load_room = rand_number(400, 409);
	if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 4100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 4199) {
		char_from_room(ch);
		char_to_room(ch, real_room(4111));
	}
	else {
		char_from_room(ch);
		char_to_room(ch, real_room(load_room));
	}

	if (INSURANCE(ch) == 5) {
		GET_HIT(ch) = (GET_MAX_HIT(ch) / 3);
		GET_PSI(ch) = (GET_MAX_PSI(ch) / 3);
		GET_MOVE(ch) = (GET_MAX_MOVE(ch) / 3);
	}
	if (INSURANCE(ch) == 4) {
		GET_HIT(ch) = (GET_MAX_HIT(ch) / 4);
		GET_PSI(ch) = (GET_MAX_PSI(ch) / 4);
		GET_MOVE(ch) = (GET_MAX_MOVE(ch) / 4);
	}
	if (INSURANCE(ch) == 3) {
		GET_HIT(ch) = (GET_MAX_HIT(ch) / 6);
		GET_PSI(ch) = (GET_MAX_PSI(ch) / 6);
		GET_MOVE(ch) = (GET_MAX_MOVE(ch) / 6);
	}
	if (INSURANCE(ch) == 2) {
		GET_HIT(ch) = (GET_MAX_HIT(ch) / 8);
		GET_PSI(ch) = (GET_MAX_PSI(ch) / 8);
		GET_MOVE(ch) = (GET_MAX_MOVE(ch) / 8);
	}
	if (INSURANCE(ch) == 1) {
		GET_HIT(ch) = (GET_MAX_HIT(ch) / 10);
		GET_PSI(ch) = (GET_MAX_PSI(ch) / 10);
		GET_MOVE(ch) = (GET_MAX_MOVE(ch) / 10);
	}
	if (INSURANCE(ch) == 0) {
		GET_HIT(ch) = 1;
		GET_PSI(ch) = 1;
		GET_MOVE(ch) = 1;
	}
    look_at_room(ch, 0);
	GET_POS(ch) = POS_RESTING;
	if (GET_EQ(ch, WEAR_MEDICAL)) {
		obj = GET_EQ(ch, WEAR_MEDICAL);
		if (GET_OBJ_VAL(GET_EQ(ch, WEAR_MEDICAL), 0) > 0) {
			send_to_char(ch, "@RYour medical bracelet flashes red for a brief second, before returning to normal.@n\r\n");
			GET_OBJ_VAL(GET_EQ(ch, WEAR_MEDICAL), 0) -= 1;
		}
		else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_MEDICAL), 0) == 0) {
			send_to_char(ch, "@WYour medical bracelet falls from your wrist and shatters into a million pieces.@n\r\n");
			send_to_char(ch, "@GYou are now uninsured! Please remember to pick up another insurance policy!@n\r\n");
			obj_to_char(unequip_char(ch, WEAR_MEDICAL, FALSE), ch);
			extract_obj(obj);
		}	
	}
	else {
		send_to_char(ch, "\r\nWithout insurance, the doctors were forced to make quick work of you and messed something up.\r\n");
		if (GET_LEVEL(ch) > 10) {
			psi_affects(40, ch, ch, PSIONIC_RESURRECTION_FATIGUE, TRUE, NULL);
		}
	}
}

void bionics_affect(struct char_data *ch)
{
    int i = 0;
    int j = 0;
    int maxabil = 0;

    if (IS_NPC(ch))
        return;

    // Make certain values are between 0..25, not < 0 and not > 25!
    maxabil = 30;

    for (i = 0; i < MAX_BIONIC; i++) {
        if (HAS_BIONIC(ch, i)) {
            if (BIONIC_LEVEL(ch, i) == BIONIC_BASIC) {

                switch (i) {

                    case BIO_CHASSIS:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor   */
                        break;

                    case BIO_SPINE:
                        GET_PERCENT_MACHINE(ch) += 4;                       /* 4% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor   */
                        if (GET_CON(ch) < maxabil)                           /* +1 con     */
                            GET_CON(ch) += 1;
                        break;

                    case BIO_SHOULDER:
                        GET_PERCENT_MACHINE(ch) += 7;                       /* 7% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor   */
                        break;

                    case BIO_RIBCAGE:
                        GET_PERCENT_MACHINE(ch) += 4;                       /* 4% machine */
                        GET_AC(ch) += 5;                                    /* +3 armor   */
                        GET_MAX_HIT_PTS(ch) += 10;                              /* +10 hp     */
                        break;

                    case BIO_HIP:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor   */
                        break;

                    case BIO_ARMS:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        GET_AC(ch) += 5;                                    /* +2 armor   */
                        j = 0;
                        while (j < 2) {                                     /* +2 str     */
                            if (GET_STR(ch) < maxabil)
                                GET_STR(ch) += 1;
                            else
                                GET_STR_ADD(ch) += 10;
                            j++;
                        }
                        break;

                    case BIO_NECK:
                        GET_PERCENT_MACHINE(ch) += 2;                       /* 2% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor   */
                        break;

                    case BIO_LUNG:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        if (!AFF_FLAGGED(ch, AFF_BIO_TRANCE))               /* perm water breathe */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_BIO_TRANCE);
                        break;

                    case BIO_LEGS:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        GET_AC(ch) += 5;                                    /* +2 armor   */
                        GET_MAX_MOVE(ch) += 50;                             /* +50 move   */
                        break;

                    case BIO_SKULL:
                        GET_PERCENT_MACHINE(ch) += 8;                       /* 8% machine */
                        GET_AC(ch) += 5;                                    /* +1 armor */
                        break;

                    case BIO_CORE:
                        GET_PERCENT_MACHINE(ch) += 4;                       /* 4% machine */
                        GET_MAX_HIT_PTS(ch) += 40;                              /* +40 hp */
                        GET_HIT_REGEN(ch) += 10;                            /* +10 HP rege */
                        break;

                    case BIO_BLASTER:
                        GET_DAMROLL(ch) += 11;                              /* +11 damroll */
                        break;

                    case BIO_BLADES:
                        GET_DAMROLL(ch) += 4;                               /* +4 damroll */
                        break;

                    case BIO_EXARMS:
                        /* +1 attack handled by fight.c */
                        break;

                    case BIO_EYE:
                        GET_PERCENT_MACHINE(ch) += 2;                       /* 2% machine */
                        GET_HITROLL(ch) += 10;                              /* +10 hitroll */
                        if (!AFF_FLAGGED(ch, AFF_DETECT_INVIS))             /* Perm see invis */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_DETECT_INVIS);
                        if (!AFF_FLAGGED(ch, AFF_SENSE_LIFE))               /* Perm sense life */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_SENSE_LIFE);
                        break;

                    case BIO_JACK:
                        GET_PERCENT_MACHINE(ch) += 2;                       /* 2% machine */
                        for (j = 0; GET_PCN(ch) < maxabil && j < 3; j++)    /* +3 PCN */
                            GET_PCN(ch) += 1;
                        break;

                    case BIO_VOICE:
                        GET_PERCENT_MACHINE(ch) += 2;                       /* 2% machine */
                        /* Perm ventriloquate */
                        break;

                    case BIO_ARMOR:
                        GET_AC(ch) += 10;                                    /* +7 armor */
                        GET_MAX_HIT_PTS(ch) += 50;                              /* +50 hp */
                        if (!AFF_FLAGGED(ch, AFF_COOL_AURA))                /* Perm heat resist */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_COOL_AURA);
                        if (!AFF_FLAGGED(ch, AFF_THERMAL_AURA))             /* Perm cold resist */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_THERMAL_AURA);
                        break;

                    case BIO_JET:
                        if (!AFF_FLAGGED(ch, AFF_LEVITATE))                 /* Perm fly */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_LEVITATE);
                        break;

                    case BIO_FOOTJET:
                        GET_MAX_HIT_PTS(ch) += 50;
                        GET_DAMROLL(ch) += 10;
                        //  ch->player_specials->saved.char_psi_regen += 25;
                        break;

                    case BIO_MATRIX:
                        GET_MAX_HIT_PTS(ch) += 25;
                        GET_DAMROLL(ch) += 10;
                        GET_HITROLL(ch) += 10;
                        GET_MOVE_REGEN(ch) -= 10;
                        break;

                    case BIO_CHIPJACK:
                        GET_PERCENT_MACHINE(ch) += 1;
                        break;

                }
            }
            else if (BIONIC_LEVEL(ch, i) == BIONIC_ADVANCED) {
                switch (i) {
                    case BIO_INTERFACE:
                        GET_PERCENT_MACHINE(ch) += 2;                       /* 2% machine */
                        if (GET_INT(ch) < maxabil)                           /* +1 int */
                            GET_INT(ch) += 1;
                        break;

                    case BIO_SKULL:
                        if (GET_EQ(ch, WEAR_EXPANSION))
                            if (GET_POS(ch) == POS_FIGHTING)
                                activate_chip(ch);

                        GET_PERCENT_MACHINE(ch) += 10;                      /* 10% machine */
                        //if (ch->aff_abils.pcn < maxabil)                   /* +1 pcn */
                        //   ch->aff_abils.pcn += 1;
                        //if (ch->aff_abils.intel > 0)                       /* -1 int */
                        //   ch->aff_abils.intel -= 1;
                        //this is buggy, whacks out max for int to 24, not good.
                        //ch->player_specials->saved.char_psi_regen += 30;  /* +30 psi regen */
                        GET_MAX_HIT_PTS(ch) += 50; //Temp, no idea what to do here
                        GET_AC(ch) += 10;
                        break;

                    case BIO_CORE:
                        GET_PERCENT_MACHINE(ch) += 6;                       /* 6% machine */
                        GET_MAX_HIT_PTS(ch) += 80;                              /* +80 hp */
                        GET_HIT_REGEN(ch) += 30;                            /* +30 HP rege */
                        break;

                    case BIO_JET:
                        if (!AFF_FLAGGED(ch, AFF_LEVITATE))                 /* Perm fly */
                            SET_BIT_AR(AFF_FLAGS(ch), AFF_LEVITATE);

                        if (GET_DEX(ch) < maxabil)                           /* +1 dex */
                            GET_DEX(ch) += 1;

                        GET_MAX_MOVE(ch) += 60;                             /* +60 moves */
                        break;

                }
            }
            else if (BIONIC_LEVEL(ch, i) == BIONIC_REMORT) {
                // there are no remort bionics yet, so set it to basic!
                BIONIC_LEVEL(ch, i) = BIONIC_BASIC;
            }
            else {
                // there must be some problem, so set it to basic!
                mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Reached unknown bionic level %d by %s.", i, GET_NAME(ch));
                BIONIC_LEVEL(ch, i) = BIONIC_BASIC;
            }
        }
    }
}

void bionics_unaffect(struct char_data *ch)
{
    int i = 0;
    int j = 0;
    int maxabil = 0;

    if (IS_NPC(ch))
        return;

    // Make certain values are between 0..25, not < 0 and not > 25!
    maxabil = (GET_LEVEL(ch) >= 25 || IS_REMORT(ch)) ? 25 : 20;

    for (i = 0; i < MAX_BIONIC; i++) {
        if (HAS_BIONIC(ch, i)) {
            if (BIONIC_LEVEL(ch, i) == BIONIC_BASIC) {

                switch (i) {

                    case BIO_CHASSIS:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor   */
                        break;

                    case BIO_SPINE:
                        GET_PERCENT_MACHINE(ch) -= 4;                       /* 4% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor   */
                        if (GET_CON(ch) > 0)                                 /* +1 con     */
                            GET_CON(ch) -= 1;
                        break;

                    case BIO_SHOULDER:
                        GET_PERCENT_MACHINE(ch) -= 7;                       /* 7% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor   */
                        break;

                    case BIO_RIBCAGE:
                        GET_PERCENT_MACHINE(ch) -= 4;                       /* 4% machine */
                        GET_AC(ch) -= 5;                                    /* +3 armor   */
                        GET_MAX_HIT_PTS(ch) -= 10;                              /* +10 hp     */
                        break;

                    case BIO_HIP:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor   */
                        break;

                    case BIO_ARMS:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        GET_AC(ch) -= 5;                                    /* +2 armor   */
                        j = 0;
                        while (j < 2) {                                     /* +2 str */

                            if (GET_STR(ch) == maxabil) {
                                if (GET_STR_ADD(ch) >= 10)
                                    GET_STR_ADD(ch) -= 10;
                                else
                                    GET_STR(ch) -= 1;
                            }
                            else if (GET_STR(ch) > 0 && GET_STR(ch) < maxabil)
                                GET_STR(ch) -= 1;

                            j++;
                        }
                        break;

                    case BIO_NECK:
                        GET_PERCENT_MACHINE(ch) -= 2;                       /* 2% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor   */
                        break;

                    case BIO_LUNG:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        if (AFF_FLAGGED(ch, AFF_BIO_TRANCE))               /* perm water breathe */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BIO_TRANCE);
                        break;

                    case BIO_LEGS:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        GET_AC(ch) -= 5;                                    /* +2 armor   */
                        GET_MAX_MOVE(ch) -= 50;                             /* +50 move   */
                        break;

                    case BIO_SKULL:
                        GET_PERCENT_MACHINE(ch) -= 8;                       /* 8% machine */
                        GET_AC(ch) -= 5;                                    /* +1 armor */
                        break;

                    case BIO_CORE:
                        GET_PERCENT_MACHINE(ch) -= 4;                       /* 4% machine */
                        GET_MAX_HIT_PTS(ch) -= 40;                              /* +40 hp */
                        GET_HIT_REGEN(ch) -= 10;                            /* +10 HP rege */
                        break;

                    case BIO_BLASTER:
                        GET_DAMROLL(ch) -= 11;                              /* +11 damroll */
                        break;

                    case BIO_BLADES:
                        GET_DAMROLL(ch) -= 4;                               /* +4 damroll */
                        break;

                    case BIO_EXARMS:
                        /* +1 attack handled by fight.c */
                        break;

                    case BIO_EYE:
                        GET_PERCENT_MACHINE(ch) -= 2;                       /* 2% machine */
                        GET_HITROLL(ch) -= 10;                              /* +10 hitroll */
                        if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))             /* Perm see invis */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DETECT_INVIS);
                        if (AFF_FLAGGED(ch, AFF_SENSE_LIFE))               /* Perm sense life */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SENSE_LIFE);
                        break;

                    case BIO_JACK:
                        GET_PERCENT_MACHINE(ch) -= 2;                       /* 2% machine */
                        for (j = 0; GET_PCN(ch) > maxabil && j < 3; j++)    /* +3 PCN */
                            GET_PCN(ch) -= 1;
                        break;

                    case BIO_VOICE:
                        GET_PERCENT_MACHINE(ch) -= 2;                       /* 2% machine */
                        /* Perm ventriloquate */
                        break;

                    case BIO_ARMOR:
                        GET_AC(ch) -= 10;                                    /* +7 armor */
                        GET_MAX_HIT_PTS(ch) -= 50;                              /* +50 hp */
                        if (AFF_FLAGGED(ch, AFF_COOL_AURA))                /* Perm heat resist */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_COOL_AURA);
                        if (AFF_FLAGGED(ch, AFF_THERMAL_AURA))             /* Perm cold resist */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_THERMAL_AURA);
                        break;

                    case BIO_JET:
                        if (AFF_FLAGGED(ch, AFF_LEVITATE))                 /* Perm fly */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_LEVITATE);
                        break;

                    case BIO_FOOTJET:
                        GET_MAX_HIT_PTS(ch) -= 50;
                        GET_DAMROLL(ch) -= 10;
                        //  ch->player_specials->saved.char_psi_regen += 25;
                        break;

                    case BIO_MATRIX:
                        GET_MAX_HIT_PTS(ch) -= 25;
                        GET_DAMROLL(ch) -= 10;
                        GET_HITROLL(ch) -= 10;
                        GET_MOVE_REGEN(ch) += 10;
                        break;

                    case BIO_CHIPJACK:
                        GET_PERCENT_MACHINE(ch) -= 1;
                        break;

                }
            }
            else if (BIONIC_LEVEL(ch, i) == BIONIC_ADVANCED) {
                switch (i) {
                    case BIO_INTERFACE:
                        GET_PERCENT_MACHINE(ch) -= 2;                       /* 2% machine */
                        if (GET_INT(ch) > 0)                                 /* +1 int */
                            GET_INT(ch) -= 1;
                        break;

                    case BIO_SKULL:
                        if (GET_EQ(ch, WEAR_EXPANSION))
                            if (GET_POS(ch) == POS_FIGHTING)
                                deactivate_chip(ch);

                        GET_PERCENT_MACHINE(ch) -= 10;                      /* 10% machine */
                        //if (ch->aff_abils.pcn < maxabil)                   /* +1 pcn */
                        //   ch->aff_abils.pcn += 1;
                        //if (ch->aff_abils.intel > 0)                       /* -1 int */
                        //   ch->aff_abils.intel -= 1;
                        //this is buggy, whacks out max for int to 24, not good.
                        //ch->player_specials->saved.char_psi_regen += 30;  /* +30 psi regen */
                        GET_MAX_HIT_PTS(ch) -= 50; //Temp, no idea what to do here
                        GET_AC(ch) -= 10;
                        break;

                    case BIO_CORE:
                        GET_PERCENT_MACHINE(ch) -= 6;                       /* 6% machine */
                        GET_MAX_HIT_PTS(ch) -= 80;                              /* +80 hp */
                        GET_HIT_REGEN(ch) -= 30;                            /* +30 HP rege */
                        break;

                    case BIO_JET:
                        if (AFF_FLAGGED(ch, AFF_LEVITATE))                 /* Perm fly */
                            REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_LEVITATE);

                        if (GET_DEX(ch) > 0)                           /* +1 dex */
                            GET_DEX(ch) -= 1;

                        GET_MAX_MOVE(ch) -= 60;                             /* +60 moves */
                        break;

                }
            }
            else if (BIONIC_LEVEL(ch, i) == BIONIC_REMORT) {
                // there are no remort bionics yet, so set it to basic!
                BIONIC_LEVEL(ch, i) = BIONIC_BASIC;
            }
            else {
                // there must be some problem, so set it to basic!
                mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Reached unknown bionic level %d by %s.", i, GET_NAME(ch));
                BIONIC_LEVEL(ch, i) = BIONIC_BASIC;
            }
        }
    }
}
