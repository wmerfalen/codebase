/**************************************************************************
*  File: fight.c                                           Part of tbaMUD *
*  Usage: Combat system.                                                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __FIGHT_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "psionics.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"
#include "bionics.h"
#include "drugs.h"
#include "graph.h"
#include "events.h"
#include "skilltree.h"
#include "craft.h"
#include "mud_event.h"



/* head of l-list of fighting chars */
struct char_data *combat_list = NULL;
/* linked list of ranged combat fighting - Gahan */
struct char_data *ranged_combat_list = NULL;
/* Saving throw damage calculator - Gahan */
int get_savingthrow(struct char_data *ch, struct char_data *victim, int dam, int damagetype);
static void execute_combo(struct char_data *ch, struct char_data *victim);
void exec_delayed(struct char_data *ch);

/* Weapon attack texts - Gahan 2011*/
struct attack_hit_type attack_hit_text[] =
{
    {"hit",		"hits",		"hit",		"series of hits",		"fury of hits",		"frenzy of hits"},    /* 0 */
    {"sting",	"stings",	"sting",	"series of stings",		"fury of stings",	"frenzy of stings"},
    {"whip",	"whips",	"whip",		"series of whips",		"fury of whips",	"frenzy of whips"},
    {"slash",	"slashes",	"slash",	"series of slashes",	"fury of slashes",	"frenzy of slashes"},
    {"bite",	"bites",	"bite",		"series of bites",		"fury of bites",	"frenzy of bites"},
    {"bludgeon","bludgeons","bludgeon",	"series of smashes",	"fury of smashes",	"frenzy of smashes"},  /* 5 */
    {"crush",	"crushes",	"crush",	"series of crushes",	"fury of crushes",	"frenzy of crushes"},
    {"pound",	"pounds",	"pound",	"series of pounds",		"fury of pounds",	"frenzy of pounds"},
    {"claw",	"claws",	"claw",		"series of claws",		"fury of claws",	"frenzy of claws"},
    {"maul",	"mauls",	"maul",		"series of mauls",		"fury of mauls",	"frenzy of mauls"},
    {"thrash",	"thrashes", "thrash",	"series of thrashes",	"fury of thrashes",	"frenzy of thrashes"}, /* 10 */
    {"pierce",	"pierces",	"pierce",	"series of pierces",	"fury of pierces",	"frenzy of pierces"},
    {"blast",	"blasts",	"blast",	"series of blasts",		"fury of blasts",	"frenzy of blasts"},
    {"punch",	"punches",	"punch",	"series of punches",	"fury of punches",	"frenzy of punches"},
    {"stab",	"stabs",	"stab",		"series of stabs",		"fury of stabs",	"frenzy of stabs"},
    {"bash",	"bashes",	"bash",		"series of bashes",		"fury of bashes",	"frenzy of bashes"},	/*15 */
	{"shot",	"shoots",	"shot",		"series of shots",		"fury of shots",	"frenzy of shots"},
	{"explode", "explodes", "explosion","series of explosions",	"fury of explosions","frenzy of explosions"},
	{"chop",	"chops",	"chop",		"series of chops",		"fury of chops",	"frenzy of chops"},
	{"flail",	"flails",	"flail",	"series of flails",		"fury of flails",	"frenzy of flails"},
	{"slice",	"slices",	"slice",	"series of slices",		"fury of slices",	"frenzy of slices"}, /*20*/
	{"splat",	"splats",	"splat",	"series of splats",		"fury of splats",	"frenzy of splats"},
	{"attack",	"attacks",	"attack",	"series of attacks",	"fury of attacks",	"frenzy of attacks"},
	{"scratch",	"scratches","scratch",	"series of scratches",	"fury of scratches","frenzy of scratches"}, 
	{"cleave",	"cleaves",	"cleave",	"series of cleaves",	"fury of cleaves",	"frenzy of cleaves"},
	{"hack",	"hacks",	"hack",		"series of hacks",		"fury of hacks",	"frenzy of hacks"}, /*25*/
	{"saw",		"saws",		"saw",		"series of saws",		"fury of saws",		"frenzy of saws"},
	{"gouge",	"gouges",	"gouge",	"series of gouges",		"fury of gouges",	"frenzy of gouges"},
	{"grind",	"grinds",	"grind",	"series of grinds",		"fury of grinds",	"frenzy of grinds"},
	{"drill",	"drills",	"drill",	"series of drills",		"fury of drills",	"frenzy of drills"},
	{"maim",	"maims",	"maim",		"series of maims",		"fury of maims",	"frenzy of maims"}, /*30*/
	{"slap",	"slaps",	"slap",		"series of slaps",		"fury of slaps",	"frenzy of slaps"},
	{"kick",	"kicks",	"kick",		"series of kicks",		"fury of kicks",	"frenzy of kicks"}
};
/* Gun attack types - Gahan 2011 */
struct attack_hit_type attack_gun_hit_text[] =
{
	{"shot",			"shoots",	"shot",				"short burst of auto-fire",		"burst of auto-fire",			"long burst of auto-fire"},
	{"energy bolt",		"sears",	"energy blast",		"focused burst of energy",		"concentrated burst of energy",	"destructive burst of energy"},
	{"shot",			"shoots",	"shot",				"short burst of auto-fire",		"burst of auto-fire",			"long burst of auto-fire"},
	{"slug",			"blasts",	"buck shot",		"spread shot",					"semi automatic shellfire",		"fully automatic shellfire"},
	{"blast",			"bombards",	"blast",			"group of explosives",			"volley of explosives",			"salvo of explosives"},
	{"stream of flame",	"immolates","blast",			"concentrated burst of flame",	"stream of flame",				"firestorm"},
	{"rocket",			"explodes",	"rocket",			"group of rockets",				"cluster of rockets",			"shower of rockets"},
	{"shot",			"shoots",	"arrow",			"band of arrows",				"rain of arrows",				"slew of arrows"},
	{"shot",			"shoots",	"bolt",				"handful of bolts",				"rapid fire bolts",				"sleet of bolts"}
};
/* Damage type inserts for damage messages - Gahan 2013 */
const char *damagetype_insert[] = {
	"",
	" poisonous",
	" drug infected",
	" firey",
	" icey",
	" electrifying",
	" explosive",
	" psionic",
	" ethereal",
};
/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;
static struct char_data *next_ranged_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
static void free_messages_type(struct msg_type *msg);
//static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular, const char *damage_type);

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
/* The Fight related routines */
void appear(struct char_data *ch)
{
    if (affected_by_psionic(ch, PSIONIC_INVISIBILITY))
        affect_from_char(ch, PSIONIC_INVISIBILITY, TRUE);

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}

void subtract_ammo(struct obj_data *wielded, struct char_data *ch)
{
    struct obj_data *ammo;
	int j;
	bool ack = false;

    ammo = GET_AMMO_LOADED(wielded);
    ammo->obj_ammo.ammo_count--;

    if (GET_LOADED_AMMO_COUNT(wielded) == 0) {
	  for (j = 0; j < MAX_OBJ_APPLY; j++)
		affect_modify_ar(ch, GET_OBJ_APPLY_LOC(ammo, j), GET_OBJ_APPLY_MOD(ammo, j), GET_OBJ_AFFECT(ammo), FALSE);

		// now apply the affects
		for (j = 0; j < NUM_AFF_FLAGS; j++)
		  if (OBJAFF_FLAGGED(ammo, j))
            obj_affect_to_char(ch, j, ack, ammo);

	  affect_total(ch);
	  GET_AMMO_LOADED(wielded) = NULL;
      obj_from_obj(ammo);
      extract_obj(ammo);
    }
}

static void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}

void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}

void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128], *buf;

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  while (!feof(fl)) {
    buf = fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      buf = fgets(chk, 128, fl);

    while (*chk == 'M') {
      buf = fgets(chk, 128, fl);
      sscanf(chk, " %d\n", &type);
      for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
         (fight_messages[i].a_type); i++);
      if (i >= MAX_MESSAGES) {
        log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
        exit(1);
      }
      CREATE(messages, struct message_type, 1);
      fight_messages[i].number_of_attacks++;
      fight_messages[i].a_type = type;
      messages->next = fight_messages[i].msg;
      fight_messages[i].msg = messages;

      messages->die_msg.attacker_msg = fread_action(fl, i);
      messages->die_msg.victim_msg = fread_action(fl, i);
      messages->die_msg.room_msg = fread_action(fl, i);
      messages->miss_msg.attacker_msg = fread_action(fl, i);
      messages->miss_msg.victim_msg = fread_action(fl, i);
      messages->miss_msg.room_msg = fread_action(fl, i);
      messages->hit_msg.attacker_msg = fread_action(fl, i);
      messages->hit_msg.victim_msg = fread_action(fl, i);
      messages->hit_msg.room_msg = fread_action(fl, i);
      messages->god_msg.attacker_msg = fread_action(fl, i);
      messages->god_msg.victim_msg = fread_action(fl, i);
      messages->god_msg.room_msg = fread_action(fl, i);
      buf  = fgets(chk, 128, fl);
      while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        buf  = fgets(chk, 128, fl);
    }
  }
  fclose(fl);
}

void update_pos(struct char_data *victim)
{
    if (!victim)
        return;
	if(GET_POS(victim) == -1)
		return;

    if (GET_HIT(victim) > 0) {
		if (GET_POS(victim) == POS_SLEEPING)
			return;
        if (GET_POS(victim) > POS_STUNNED)
            return;
        else
            GET_POS(victim) = POS_STANDING;
    }
    else if (GET_HIT(victim) <= -21)
        GET_POS(victim) = POS_DEAD;
    else if (GET_HIT(victim) <= -16)
        GET_POS(victim) = POS_MORTALLYW;
    else if (GET_HIT(victim) <= -10)
        GET_POS(victim) = POS_INCAP;
    else
        GET_POS(victim) = POS_STUNNED;
}

void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (IS_HIGHLANDER(ch) && IS_HIGHLANDER(vict))
    return;

  // if both are players, and they can pk, its ok
  if (!IS_NPC(ch) && !IS_NPC(vict) && PK_OK(ch, vict))
    return;

  // if room is a PK room, anything goes
  if (PK_OK_ROOM(ch))
    return;
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}
void set_ranged_fighting(struct char_data *ch, struct char_data *vict)
{
	if (!ch || !vict)
		return;
	// Sanity checks are always good..
	if (ch == vict)
		return;
	// No way should a character be allowed to engage in ranged fighting if they're already in melee range
	if (FIGHTING(ch))
		return;

	if (!(get_lineofsight(ch, vict, NULL, NULL))) {
		end_rangedcombat(vict);
		if (RANGED_FIGHTING(ch))
			stop_ranged_fighting(ch);
	}

	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
	REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

	ch->next_ranged_fighting = ranged_combat_list;
	ranged_combat_list = ch;

	RANGED_FIGHTING(ch) = vict;
	RANGED_COMBAT_TICKS(ch) = 0;
}
/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  struct char_data *k = NULL;
  struct follow_type *f = NULL;
  if (ch == vict)
    return;

  if (FIGHTING(ch))
    return;

  ch->next_fighting = combat_list;
  combat_list = ch;

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;
  COMBAT_TICKS(ch) = 0;
  activate_chip(ch); /* advanced skull bionic expansion slot */
  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
  if (AFF_FLAGGED(ch, AFF_GROUP)) {
    // todo: perhaps only the group leader should get the auto-assist?
	// This is a little too robotic for my taste too, but this seems to be a common theme.
	if (ch->master)
      k = ch->master;
    else
	  k = ch;
	
	if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch) && !FIGHTING(k) &&
		PRF_FLAGGED(k, PRF_AUTOASSIST) && IN_ROOM(k) == IN_ROOM(ch))
      do_assist(k, GET_NAME(ch), 0, 0);
    for (f = k->followers; f; f = f->next) {
		if ((f->follower != ch) && AFF_FLAGGED(f->follower, AFF_GROUP) && (IN_ROOM(f->follower) == IN_ROOM(ch))) {
			// group auto-assist
            if (!IS_NPC(f->follower) && PRF_FLAGGED(f->follower, PRF_AUTOASSIST) && FIGHTING(f->follower) == NULL) {
				COMBAT_TICKS(f->follower) = rand_number(0, 3);
                do_assist(f->follower, GET_NAME(ch), 0, 0);
				set_fighting(f->follower, vict);
			}
			/* pet auto-assist! -DH */
			if (IS_NPC(f->follower) && AFF_FLAGGED(f->follower, AFF_CHARM) && FIGHTING(f->follower) == NULL) {
				COMBAT_TICKS(f->follower) = rand_number(0,3);
                do_assist(f->follower, GET_NAME(k), 0, 0);
				set_fighting(f->follower, vict);
			}
        }
    }
  }
}
void stop_ranged_fighting(struct char_data *ch)
{
	struct char_data *temp;

	if (!ch)
		return;
    if (ch == next_ranged_combat_list)
        next_ranged_combat_list = ch->next_ranged_fighting;

    REMOVE_FROM_LIST(ch, ranged_combat_list, next_ranged_fighting);
    ch->next_ranged_fighting = NULL;
    RANGED_FIGHTING(ch) = NULL;
    GET_POS(ch) = POS_STANDING;
    update_pos(ch);
	RANGED_COMBAT_TICKS(ch) = 0;
}
/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
  COMBAT_TICKS(ch) = 0;
  GET_CIRCLETICKS(ch) = 0;
  deactivate_chip(ch);
  reset_combo(ch);
}

/* determine damage type - Gahan 2013 */

int get_damagetype(struct char_data *ch)
{
	int type = 0;
	if (IS_NPC(ch)) {
		if (GET_DAMTYPE(ch) > 0) {
			type = GET_DAMTYPE(ch);
		}
	}
	else {
		if (ch->equipment[WEAR_WIELD]) {
			/* If weapon is not a gun */
			if (!IS_GUN(ch->equipment[WEAR_WIELD]))
				type = GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 0);
			/* If weapon is a gun */
			if (IS_GUN(ch->equipment[WEAR_WIELD]))
				if (IS_LOADED(ch->equipment[WEAR_WIELD]))
					type = GET_AMMO_DAMTYPE(ch->equipment[WEAR_WIELD]);
		}
	}
	/* If no weapon, fists are considered normal damage, return the default 0 */
	return(type);
}

void remove_body_part(struct char_data *ch, int location, bool death)
{
    struct obj_data *part = NULL;
    struct obj_data *device = NULL;
    struct bionic_data *temp;
    int i;

    struct associated_parts_info {
        int location;
        int others[1];
    } associated_parts[] = {

        { BODY_PART_RIGHT_ARM, {BODY_PART_RIGHT_HAND}},
        { BODY_PART_LEFT_ARM , {BODY_PART_LEFT_HAND}},
        { BODY_PART_RIGHT_LEG, {BODY_PART_RIGHT_FOOT}},
        { BODY_PART_LEFT_LEG , {BODY_PART_LEFT_FOOT}},
        { -1, {-1}}

    };

    // has the part already been removed?
    if (PART_MISSING(ch, location)) return;

    // does the character instantly die?
    if (death) {
        die(ch, NULL, FALSE);
        return;
    }

    // set the part condition to missing
    BODYPART_CONDITION(ch, location) = 0;
    SET_BODYPART_CONDITION(ch, location, RIPPED);

    // make the body part
    part = make_body_part(ch, location);

    if (!part) {
        log("..generation of body part in remove_body_part failed for location (%d)", location);
        return;
    }

    // do we need to remove the bionic?
    if ((device = remove_bionic(ch, location))) {
       // put the bionic in the body part
        CREATE(temp, struct bionic_data, 1);
        temp->device = device;
        temp->next = part->bionics;
        part->bionics = temp;
        device->in_obj = part;
    }

    // are there associated body parts?
    for (i = 0; associated_parts[i].location != -1; i++) {

        if (associated_parts[i].location == location) {

            int j = 0;
            int num_parts = sizeof(associated_parts[i].others)/sizeof(int);
            int next = -1;

            for (j = 0; j < num_parts; j++) {

                next = associated_parts[i].others[j];

                if (!PART_MISSING(ch, next)) {

                    BODYPART_CONDITION(ch, next) = 0;
                    SET_BODYPART_CONDITION(ch, next, RIPPED);

                    // do we need to remove a bionic?
                    if ((device = remove_bionic(ch, next))) {
                        // put the bionic in the body part
                        CREATE(temp, struct bionic_data, 1);
                        temp->device = device;
                        temp->next = part->bionics;
                        part->bionics = temp;
                        device->in_obj = part;
                    }
                }
            }
        }
    }

    // check for bad side effects
    if (GET_BODYPART_CONDITION(ch, BODY_PART_LEFT_LEG, RIPPED) && GET_BODYPART_CONDITION(ch, BODY_PART_RIGHT_LEG, RIPPED))
        GET_POS(ch) = POS_SITTING;

    if (GET_BODYPART_CONDITION(ch, BODY_PART_LEFT_EYE, RIPPED) && GET_BODYPART_CONDITION(ch, BODY_PART_RIGHT_EYE, RIPPED))
        SET_BIT_AR(AFF_FLAGS(ch), AFF_BLIND);

}

// when the char loses a body part only some body parts are handled here
struct obj_data *make_body_part(struct char_data *ch, int part_location)
{
    char buf[MAX_STRING_LENGTH];
    struct obj_data *body_part;
    int room = IN_ROOM(ch);

    static struct chop_mesg_type {
        char *desc;
        char *short_desc;
        char *to_char;
        char *to_room;
    } chop_mesg[] = {

{
  "RESERVED",
  "RESERVED",                                       /* reserved */
  "RESERVED",
  "RESERVED"
},
{
  "The head of %s.",
  "the decapitated head",                           /* severed head (eyes, ears, mouth) */
  "Your head drops onto the ground!",
  "$n's head drops onto the ground!"
},
{
  "the detached left eye of %s.",
  "the left eye",                                   /* left eye */
  "OUCH!  Your left eye was just pried from its socket!",
  "$n's left eye is pried from its socket!"
},
{
  "the detached right eye of %s.",
  "the right eye",                                  /* right eye */
  "OUCH!  Your right eye was just pried from its socket!",
  "$n's rightt eye is pried from its socket!"
},
{
  "%s's mouth!",
  "the mouth",                                       /* mouth */
  "Your entire mouth is removed from your face!",
  "$n's mouth is removed from their face!"
},
{
  "The head of %s.",
  "the neck and severed head",                      /* neck and severed head (neck1 and neck2) */
  "Your head is seperated from your shoulders!",
  "$n's head is seperated from $s shoulders!"
},
{
  "%s's severed arm.",
  "the severed left arm",                           /* left arm, hand, finger, wrist, hold */
  "Your left limb is severed from your body!",
  "$n's left limb is completely severed!"
},
{
  "%s's severed arm.",
  "the severed right arm",                          /* right arm, hand, finger, wrist, wield */
  "Your right limb is severed from your body!",
  "$n's right limb is completely severed!"
},
{
  "%s's wrist.",
  "the left wrist",                                   /* just an ear */
  "Your left wrist falls onto the ground!",
  "$n's left wrist falls onto the ground!"
},
{
  "%s's wrist.",
  "the right wrist",                                  /* just an ear */
  "Your right wrist falls onto the ground!",
  "$n's right wrist falls onto the ground!"
},
{
  "A hand belonging to %s.",
  "the chopped off left hand",                      /* left hand, finger, wrist, hold */
  "Your left hand is severed from your arm!",
  "$n's left hand is completely severed!"
},
{
  "A hand belonging to %s.",
  "the chopped off right hand",                     /* right hand, finger, wrist, wield */
  "Your right hand is severed from your arm!",
  "$n's right hand is completely severed!"
},
{
  "%s's severed leg.",
  "the severed left leg",                           /* left leg, feet */
  "Your left leg is severed from your body!",
  "$n's left leg is completely severed!"
},
{
  "%s's severed leg.",
  "the severed right leg",                          /* right leg, feet */
  "Your right leg is severed from your body!",
  "$n's right leg is completely severed!"
},
{
  "%s's foot that must have been lopped off.",
  "the chopped off left foot",                      /* left feet */
  "Your left foot is severed from your leg!",
  "$n's left foot is completely severed!"
},
{
  "%s's foot that must have been lopped off.",
  "the chopped off right foot",                     /* right feet */
  "Your right foot is severed from your leg!",
  "$n's right foot is completely severed!"
},
{
  "%s's upper torso that must have been severed.",
  "the upper torso",                                /* chest */
  "Your upper torso is wrenched from your body!",
  "$n's upper torso seperates from $s body!"
},
{
  "%s's lower torso that must have been severed.",
  "the lower torso",                                /* abdomen */
  "Your lower torso is ripped from your body!",
  "$n's lower torso seperates from $s body!"
},
{
  "Yuck!  %s's back half lies on the ground.",
  "the back",                                       /* back */
  "You are cleaved in half and your back falls to the ground!",
  "$n is cleaved in half and their back falls to the ground!"
},
};

    const int part_weight[] =
    { 0, 5, 1, 1, 0, 0, 0, 6, 20, 20, 2, 2, 40, 40, 3, 3, 25, 35, 60};

    // create the base object and initialize
    body_part = create_obj();
    body_part->item_number = NOTHING;
    IN_ROOM(body_part) = NOWHERE;

    // set some basic item stats
    GET_OBJ_TYPE(body_part) = ITEM_BODY_PART;
    SET_BIT_AR(GET_OBJ_WEAR(body_part), ITEM_WEAR_TAKE);
    SET_BIT_AR(GET_OBJ_EXTRA(body_part), ITEM_NORENT | ITEM_NOSELL);
    GET_OBJ_VAL(body_part, 0) = part_location;
    GET_OBJ_WEIGHT(body_part) = part_weight[part_location];
    //GET_OBJ_DURABILITY(body_part) = 3;
    //GET_OBJ_MAX_DAM(body_part) = 50;

    // drop any items worn at this location into the victim inventory
    switch(part_location) {

    case BODY_PART_NECK:
        if (GET_EQ(ch, WEAR_NECK_1))     obj_to_char(unequip_char(ch, WEAR_NECK_1, TRUE), ch);
        if (GET_EQ(ch, WEAR_NECK_2))     obj_to_char(unequip_char(ch, WEAR_NECK_2, TRUE), ch);
    case BODY_PART_HEAD:
        if (GET_EQ(ch, WEAR_HEAD))       obj_to_char(unequip_char(ch, WEAR_HEAD, TRUE), ch);
        if (GET_EQ(ch, WEAR_FACE))       obj_to_char(unequip_char(ch, WEAR_FACE, TRUE), ch);
        break;
    case BODY_PART_RIGHT_ARM:
        // todo: assume the right arm is used for wielding - check with handedness later
        if (GET_EQ(ch, WEAR_ARMS))       obj_to_char(unequip_char(ch, WEAR_ARMS, TRUE), ch);
	case BODY_PART_LEFT_WRIST:
		if (GET_EQ(ch, WEAR_WRIST_L))	 obj_to_char(unequip_char(ch, WEAR_WRIST_L, TRUE), ch);
	case BODY_PART_RIGHT_WRIST:
		if (GET_EQ(ch, WEAR_WRIST_R))	 obj_to_char(unequip_char(ch, WEAR_WRIST_R, TRUE), ch);
    case BODY_PART_RIGHT_HAND:
        if (GET_EQ(ch, WEAR_HANDS))      obj_to_char(unequip_char(ch, WEAR_HANDS, TRUE), ch);
        if (GET_EQ(ch, WEAR_WRIST_R))    obj_to_char(unequip_char(ch, WEAR_WRIST_R, TRUE), ch);
        if (GET_EQ(ch, WEAR_RING_FIN_R)) obj_to_char(unequip_char(ch, WEAR_RING_FIN_R, TRUE), ch);
        if (GET_EQ(ch, WEAR_WIELD))      obj_to_char(unequip_char(ch, WEAR_WIELD, TRUE), ch);
        break;
    case BODY_PART_LEFT_ARM:
        // todo: assume the left arm is used for holding - check with handedness later
        if (GET_EQ(ch, WEAR_ARMS))       obj_to_char(unequip_char(ch, WEAR_ARMS, TRUE), ch);
    case BODY_PART_LEFT_HAND:
        // todo: assume the left hand is used for holding - check with handedness later
        if (GET_EQ(ch, WEAR_HANDS))      obj_to_char(unequip_char(ch, WEAR_HANDS, TRUE), ch);
        if (GET_EQ(ch, WEAR_WRIST_L))    obj_to_char(unequip_char(ch, WEAR_WRIST_L, TRUE), ch);
        if (GET_EQ(ch, WEAR_RING_FIN_L)) obj_to_char(unequip_char(ch, WEAR_RING_FIN_L, TRUE), ch);
        if (GET_EQ(ch, WEAR_HOLD))       obj_to_char(unequip_char(ch, WEAR_HOLD, TRUE), ch);
        break;
    case BODY_PART_LEFT_LEG:
    case BODY_PART_RIGHT_LEG:
        if (GET_EQ(ch, WEAR_LEGS))       obj_to_char(unequip_char(ch, WEAR_LEGS, TRUE), ch);
    case BODY_PART_LEFT_FOOT:
    case BODY_PART_RIGHT_FOOT:
        if (GET_EQ(ch, WEAR_FEET))       obj_to_char(unequip_char(ch, WEAR_FEET, TRUE), ch);
        break;
    case BODY_PART_ABDOMAN:
    case BODY_PART_CHEST:
    case BODY_PART_MOUTH:
    case BODY_PART_LEFT_EYE:
    case BODY_PART_RIGHT_EYE:
    case BODY_PART_BACK:
        break;
    default:
        log("SYSERR: Illegal body part [%s] in make_body_parts().", body_parts[part_location]);
        return (NULL);
        break;
    }

    // set the name and short description and description
    snprintf(buf, sizeof(buf), "%s bodypart", body_parts[part_location]);
    body_part->name = strdup(buf);

    snprintf(buf, sizeof(buf), "%s of %s", chop_mesg[part_location].short_desc, GET_NAME(ch));
    body_part->short_description = strdup(buf);

    snprintf(buf, sizeof(buf), chop_mesg[part_location].desc, GET_NAME(ch));
    body_part->description = strdup(buf);

    // generate messages
    act(chop_mesg[part_location].to_char, TRUE, ch, 0, 0, TO_CHAR);
    act(chop_mesg[part_location].to_room, TRUE, ch, body_part, 0, TO_NOTVICT);

    // now drop the body part into the room
    obj_to_room(body_part, room);

    return(body_part);
}
void make_gibblets(struct char_data *ch)
{
    // try to generate a head
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_HEAD);

    // try to generate left arm or left hand
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_LEFT_ARM);
    else if (rand_number (1, 2) == 1)
        make_body_part(ch, BODY_PART_LEFT_HAND);

    // try to generate right arm or right hand
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_RIGHT_ARM);
    else if (rand_number (1, 2) == 1)
        make_body_part(ch, BODY_PART_RIGHT_HAND);

    // try to generate left leg or left foot
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_LEFT_LEG);
    else if (rand_number (1, 2) == 1)
        make_body_part(ch, BODY_PART_LEFT_FOOT);

    // try to generate right leg or right foot
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_RIGHT_LEG);
    else if (rand_number (1, 2) == 1)
        make_body_part(ch, BODY_PART_RIGHT_FOOT);

    // try to generate an upper torso
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_CHEST);

    // try to generate a lower torso
    if (rand_number(1, 2) == 1)
        make_body_part(ch, BODY_PART_ABDOMAN);

}
void make_corpse(struct char_data *ch)
{
  char buf[MAX_NAME_LENGTH + 64];
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o, *obj;
  struct obj_data *money;
  int i, x, y;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("xxcorpsexx corpse");

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", GET_NAME(ch));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for(x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_VAL(corpse, 4) = GET_IDNUM(ch); /* to identify the body */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;

	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_UNDEAD)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_UNDEAD);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_HUMANOID)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_HUMANOID);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_MECHANICAL) 
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_MECHANICAL);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_HIGHLANDER)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_HIGHLANDER);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_ANIMAL)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_ANIMAL);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_CANINEFELINE)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_CANFEL);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_INSECT)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_INSECT);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_REPTILE)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_REPTILE);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_AQUATIC)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_AQUATIC);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_ALIEN)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_ALIEN);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_MUTANT)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_MUTANT);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_PLANT)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_PLANT);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_ETHEREAL) {
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_ETHEREAL);
	    corpse->name = strdup("small pile dust xxcorpsexx corpse");

		GET_OBJ_WEIGHT(corpse) = 1;

	    snprintf(buf, sizeof(buf), "A small pile of dust is lying on the ground here.");
	    corpse->description = strdup(buf);

		snprintf(buf, sizeof(buf), "A small pile of dust");
		corpse->short_description = strdup(buf);
	}
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_SHELL)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_SHELL);
	if (IS_NPC(ch) && GET_MOB_CLASS(ch) == MOB_CLASS_BIRD)
		SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_CORPSE_BIRD);

    if (IS_NPC(ch))
        GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
    else
        GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;

    corpse->obj_flags.timer_on = TRUE;

    /* Code to remove ammo from weapons */
    if (ch->equipment[WEAR_WIELD])
        if (IS_GUN(ch->equipment[WEAR_WIELD]))
            if (IS_LOADED(ch->equipment[WEAR_WIELD])) {
                obj_from_obj(GET_AMMO_LOADED(ch->equipment[WEAR_WIELD]));
                obj_to_char(GET_AMMO_LOADED(ch->equipment[WEAR_WIELD]), ch);
                GET_AMMO_LOADED(ch->equipment[WEAR_WIELD]) = NULL;
            }

	if (!OBJ_FLAGGED(corpse, ITEM_CORPSE_ETHEREAL)) {
		/* transfer character's inventory to the corpse */
		corpse->contains = ch->carrying;
		for (o = corpse->contains; o != NULL; o = o->next_content)
			o->in_obj = corpse;
		object_list_new_owner(corpse, NULL);

		/* transfer character's equipment to the corpse */
		for (i = 0; i < NUM_WEARS; i++)
		if (GET_EQ(ch, i)) {
			remove_otrigger(GET_EQ(ch, i), ch);
			obj_to_obj(unequip_char(ch, i, FALSE), corpse);
		}

		/* transfer gold */
		if (GET_UNITS(ch) > 0) {
		/* following 'if' clause added to fix gold duplication loophole. The above
     * line apparently refers to the old "partially log in, kill the game
     * character, then finish login sequence" duping bug. The duplication has
     * been fixed (knock on wood) but the test below shall live on, for a
     * while. -gg 3/3/2002 */
			if (IS_NPC(ch) || ch->desc) {
				money = create_money(GET_UNITS(ch));
				obj_to_obj(money, corpse);
			}
		GET_UNITS(ch) = 0;
		}
	}
	else {
		for (i = 0; i < NUM_WEARS; i++)
			if (GET_EQ(ch, i)) {
				remove_otrigger(GET_EQ(ch, i), ch);
				obj_to_char(unequip_char(ch, i, FALSE), ch);
			}
		while (ch->carrying) {
			obj = ch->carrying;
			obj_from_char(obj);
			obj_to_room(obj, IN_ROOM(ch));
		}
		if (GET_UNITS(ch) > 0) {
			money = create_money(GET_UNITS(ch));
			obj_to_room(money, IN_ROOM(ch));
		}
	}

	ch->carrying = NULL;
	IS_CARRYING_N(ch) = 0;
	IS_CARRYING_W(ch) = 0;
	get_encumbered(ch);

	obj_to_room(corpse, IN_ROOM(ch));
}

void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < DIR_COUNT; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}

void raw_kill(struct char_data * ch, struct char_data * killer, bool gibblets)
{
	struct char_data *newtarget = NULL;
	struct char_data *temp;

    if (FIGHTING(ch))
      stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* To make ordinary commands work in scripts.  welcor*/
  GET_POS(ch) = POS_STANDING;
  
  if (GET_DELAY_HIT_WAIT(ch) >= 0  && GET_DELAY_HIT(ch) != NULL) {
    GET_DELAY_HIT_WAIT(ch) = -1;
	exec_delayed(ch);
  }
  if (LOCKED(ch)) {
	newtarget = LOCKED(ch);
	if (RANGED_FIGHTING(newtarget))
	  stop_ranged_fighting(newtarget);
	if (LOCKED(ch)->char_specials.targeted != NULL)
		LOCKED(ch)->char_specials.targeted = NULL;
	LOCKED(ch) = NULL;

  }
  // Stop protecting
  if (ch->char_specials.protecting) {
      temp = ch->char_specials.protecting;
      send_to_char(temp, "%s is no longer protecting you.\n\r", GET_NAME(ch));
      temp->char_specials.protector = NULL;
      FIGHTING(ch) = temp; //berserk fix
  }
  ch->char_specials.protecting = NULL;
  if (ch->char_specials.protector) {
      temp = ch->char_specials.protector;
      send_to_char(temp, "You are no longer protecting %s.\n\r", GET_NAME(ch));
      temp->char_specials.protecting = NULL;
  }
  ch->char_specials.protector = NULL;

  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

	if (AFF_FLAGGED(ch, AFF_INFINITYLIFE))
		INSURANCE(ch) = 5;
	else if (AFF_FLAGGED(ch, AFF_PLATINUMLIFE))
		INSURANCE(ch) = 4;
	else if (AFF_FLAGGED(ch, AFF_GOLDLIFE))
		INSURANCE(ch) = 3;
	else if (AFF_FLAGGED(ch, AFF_SILVERLIFE))
		INSURANCE(ch) = 2;
	else if (AFF_FLAGGED(ch, AFF_BRONZELIFE))
		INSURANCE(ch) = 1;
	else
		INSURANCE(ch) = 0;

  if (killer) 
    autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
  if (gibblets)
   make_gibblets(ch);
  update_pos(ch);
  save_char(ch);
  if (IS_NPC(ch)) {
	make_corpse(ch);	
	extract_char(ch);
  }
  if (!IS_NPC(ch)) {
	  extract_char_death(ch);
// clear_char_event_list(ch);
  }

  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }
}

void die(struct char_data * ch, struct char_data * killer, bool gibblets)
{
    int lose = 0;

    lose = 10000 * (GET_LEVEL(ch));
    if (GET_LEVEL(ch) < 11)
        lose = lose/100;

    if (!IS_NPC(ch)) gain_exp(ch, -lose, FALSE);
 
  if (!IS_NPC(ch)) {
    GET_NUM_DEATHS(ch) = GET_NUM_DEATHS(ch) + 1;
	REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
//  clear_char_event_list(ch);
    if (!IS_STALKER(ch))
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_DODGE);
  }
  raw_kill(ch, killer, gibblets);
}

void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
	int share, hap_share;

    share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base));
	if (share > exp_cap[GET_LEVEL(ch)].exp_cap)
		share = exp_cap[GET_LEVEL(ch)].exp_cap;
	if ((IS_HAPPYHOUR) && (IS_HAPPYEXP)) {
		/* This only reports the correct amount - the calc is done in gain_exp */
		hap_share = share + (int)((float)share * ((float)HAPPY_EXP / (float)(100)));
		share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, hap_share));
	}
    if (share > 1) {
        
		send_to_char(ch, "You receive %d experience.\r\n", share);
	}
    else
        send_to_char(ch, "You receive one measly experience point.\r\n");
    gain_exp(ch, share, FALSE);
}

void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members, base, charmies = 0;
  int tot_gain = 0;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
		if (!IS_NPC(f->follower))
	      tot_members++;
		else
			charmies++;
	}
  /* round up to the nearest tot_members */
  if (tot_members > 0) {
	  base = ((float)GET_EXP(victim) * (1 +(0.05*(float)(GET_LEVEL(victim)-GET_LEVEL(k) - 5))));
	  tot_gain = (base / (tot_members + charmies));
  }

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) && IS_CALLER(k)) {
	  if (charmies == 1)
		  tot_gain *= 1.5;
	  else if (charmies > 1)
		  tot_gain *= 1.8;
  }

  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch))
    perform_group_gain(k, tot_gain, victim);
  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
      perform_group_gain(f->follower, tot_gain, victim);
}

/*static void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp, happy_exp;

  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

   Calculate level-difference bonus 
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);

  if (IS_HAPPYHOUR && IS_HAPPYEXP) {
    happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
    exp = MAX(happy_exp, 1);
  }

  gain_exp(ch, exp, TRUE);
}
*/
static char *replace_string(const char *str, const char *weapon_singular, const char *dam_type)
{
  static char buf[256];
  char *cp = buf;

  if (dam_type == NULL)
	return(buf);

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'd':
	for (; *dam_type; *(cp++) = *(dam_type++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;


    *cp = 0;
  }				/* For */


  return (buf);
}


/* message for doing damage with a weapon - XORG START MELEE*/
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type)
{
	struct obj_data *wield;
    char *buf;
    char *buf2;
    int msgnum;

    static struct dam_weapon_type {
        const char *to_room;
        const char *to_char;
        const char *to_victim;
    } dam_weapons[] = {

        /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

        {
            "$n's#d #w misses $N.",                     /* 0: 0 */
                "Your #d #w misses $N.",
                "$n's#d #w misses you."
        },

        {
            "$n's#d #w scratches $N.",    /* 1: 1..2  */
                "Your#d #w scratches $N.",
                "You brush off $n's poorly executed#d #w, more annoyed than hurt."
            },

            {
                "$n's#d #w grazes $N, barely hurting $S.",                     /*2: 2 - 4 */
                    "Your#d #w is aimed well at $N, but $S moves at the last second and you only graze $S.",
                    "You see $n's#d #w coming and try to move out of the way, avoiding most of the damage."
            },

            {
                "$n lands a well-placed#d #w on $N.",        /*3: 5 - 10  */
                    "Your well-aimed#d #w drives $N back a step.",
                    "You stumble slightly as $n's well-placed#d #w hits you."
                },

                {
                    "$n staggers $N back with a powerful#d #w.",            /*4: 10 - 25 */
                        "Your powerful#d #w staggers $N.",
                        "You stagger as $n's powerful#d #w hits you."
                },

                {
                    "$n's#d #w opens a nasty wound on $N.",            /*5: 26 - 40  */
                        "Your#d #w inflicts a nasty wound on $N.",
                        "$n's#d #w opens a nasty wound on your torso."
                    },

                    {
						"$n's#d #w hits $N so hard that you can hear things breaking.",        /* 6: 41..60  */
                            "Your#d #w hits so hard that you hear something break inside $N.",
                            "You feel something breaking as $n's#d #w hits."
                    },

                    {
                        "$n maims $N with a horrible#d #w.",    /* 7: 61..100  */
                            "Your horrible#d #w maims $N badly.",
                            "You are maimed by $n's horrible#d #w!"
                        },

                        {
                            "$N crumples at $n's powerful#d #w.",             /*8: 101..130 */
                                "You land a#d #w so powerful that $N crumples.",
                                "You double over in pain as $n's#d #w hits you."
                        },

                        {
                            "$n's mighty#d #w nearly severs $N in half.",               /*9: 131..160 */
                                "Your mighty#d #w nearly severs $N in half.",
                                "You are almost severed in half by $n's mighty#d #w."
                            },

                            {
                                "$n sends $N flying back with a demolishing#d #w.",    /* 10: 161..190 */
                                    "Your demolishing#d #w sends $N flying back.",
                                    "You tumble back as $n's demolishing#d #w hits you."
                            },

                            {
								"$n gives a mighty roar and $s#d #w goes clean through $N!",    /* 11: 191..220 */
                                    "Giving a mighty roar, your#d #w goes clean through $N!",
                                    "$n roars and $s#d #w goes clean through you!"
                                },

                                {
                                    "$n's deadly#d #w nearly sends $N to $S maker!",              /* 12; 221..250 */
                                        "You nearly ended the fight with your deadly#d #w!",
                                        "You cling to life desperately before $n's deadly#d #w!"
                                },

                                {
                                    "Sparks fly as $n deals $N a devastating#d #w!",        /*13: 251..300 */
                                        "Your devastating#d #w sends sparks flying as it hits $N!",
                                        "Sparks fly around you as $n's devastating#d #w hits you squarely!"
                                    },

                                    {
                                        "Blue flames dance around $n as $s#d #w hits $N!",             /*14: 301..250 */
                                            "Blue flames surround you as your#d #w presses $N!",
                                            "Blue flames surround $n as $s#d #w hits $N!"
                                    },

                                    {
                                        "$n's incredible#d #w nearly tears $N apart!",       /*15: 351..400 */
                                            "Your incredible#d #w nearly tears $N apart!",
                                            "You almost fall to pieces as $n's incredible#d #w hits you!"
                                        },

                                        {
                                            "A golden aura surrounds $n as $s#d #w strikes $N!",              /*16: 401..450 */
                                                "A golden aura surrounds you as your#d #w strikes $N!",
                                                "A golden aura surrounds $n as $s#d #w strikes you!"
                                        },

                                        {
                                            "$n's thundering#d #w floors $N!",         /*17: 451..500 */
                                                "You floor $N with a thundering#d #w!",
                                                "You are floored by $n's thundering#d #w!"
                                            },

                                            {
                                                "$n powers up and deals $N a planet-shaking#d #w!",             /*18: 501..550 */
                                                    "Powering up, you deal $N a planet-shaking#d #w!",
                                                    "$n's planet-shaking#d #w spells your doom!"
                                            },

                                            {
                                                "$N gets hit by $n's#d #w with all $s might!",              /*19: 551..600 */
                                                    "Your#d #w hits $N with all your might!",
                                                    "$n's#d #w hits you with all $s might!"
                                                },

                                                {
                                                    "$n deals $N a nearly fatal#d #w!",           /*20: 601..1000 */
                                                        "You deal $N a nearly fatal#d #w!",
                                                        "You see your life flicker before your eyes as $n's#d #w hits you."
                                                },
												
                                                {
                                                    "$n pushes forward at $N with a determined#d #w!",           /*21:*/
                                                        "You push forward at $N with a determined#d #w!",
														"You are driven back by $n's determined#d #w!"
                                                },
												
                                                {
                                                    "$n delivers a frantic#d #w on $N!",           /*22*/
                                                        "You vent anger and frustration through $N with a frantic#d #w!",
														"Screams and roars deafen you as $n attacks you with a frantic#d #w!"
                                                },
												
                                                {
                                                    "$n powers into $N with a dreadful#d #w!",           /*23*/
                                                        "You power into $N with your dreadful#d #w!",
                                                        "$n powers into you with a dreadful#d #w!"
                                                },
												
                                                {
													"A shockwave seems to radiate outward as $n's#d #w impacts $N!",           /*24 */
														"You feel the impact go through you as you#d #w $N!",
														"You are shaken to the core as $n's#d #w impacts you!"
                                                },
												
                                                {
													"$n seems to blur out of existence while attacking $N with a deadly#d #w!",           /*25*/
														"Time seems to stop as you execute a masterful#d #w!",
                                                        "$n seems to blur out of existence while attacking you!"
                                                },

                                                {
                                                    "The world shakes as $n smites $N with a god-like#d #w!",        /*26*/
                                                        "The world shakes as you smite $N with a god-like#d #w!",
                                                        "Your world shakes as $n smites you with a god-like#d #w!"
                                                    },

                                                    {
                                                        "$n strikes $N center mass. $E FUCKING IMPLODES!!",     /*27: 25000+*/
                                                            "You strike $N center mass. $E FUCKING IMPLODES!!",
                                                            "$n strikes you dead center. YOU FUCKING IMPLODE!!"
                                                    }

    };

    w_type -= TYPE_HIT;        /* Change to base of table with text */
    if (dam == 0) msgnum = 0;
    else if (dam <= 2) msgnum = 1;
    else if (dam <= 4) msgnum = 2;
    else if (dam <= 10) msgnum = 3;
	else if (dam <= 25) msgnum = 4;
	else if (dam <= 40) msgnum = 5;
	else if (dam <= 60) msgnum = 6;
	else if (dam <= 100) msgnum = 7;
	else if (dam <= 130) msgnum = 8;
	else if (dam <= 160) msgnum = 9;
	else if (dam <= 190) msgnum = 10;
    else if (dam <= 220) msgnum = 11;
	else if (dam <= 250) msgnum = 12;
	else if (dam <= 300) msgnum = 13;
	else if (dam <= 350) msgnum = 14;
	else if (dam <= 400) msgnum = 15;
	else if (dam <= 450) msgnum = 16;
	else if (dam <= 500) msgnum = 17;
	else if (dam <= 550) msgnum = 18;
	else if (dam <= 600) msgnum = 19;
	else if (dam <= 1000) msgnum = 20;
	else if (dam <= 2000) msgnum = 21;
	else if (dam <= 4000) msgnum = 22;
	else if (dam <= 7000) msgnum = 23;
	else if (dam <= 9000) msgnum = 24;
	else if (dam <= 15000) msgnum = 25;
	else if (dam <= 25000) msgnum = 26;
    else                msgnum = 27;

    /* melee */
	wield = ch->equipment[WEAR_WIELD];
	const char *w_mess = "";
	const char *dam_type = "";
	
	if (get_damagetype(ch) > 0)
		dam_type = damagetype_insert[get_damagetype(ch)];
	
	int attacks = attack_rate[ATTACKS_PER_ROUND(ch)].fire_rate;

	if (attacks == 1)
		w_mess = attack_hit_text[w_type].onehit;
	else if (attacks == 3)
		w_mess = attack_hit_text[w_type].threehit;
	else if (attacks == 5)
		w_mess = attack_hit_text[w_type].fivehit;
	else if (attacks == 10)
		w_mess = attack_hit_text[w_type].tenhit;
	else 
		w_mess = attack_hit_text[w_type].onehit;

    /* damage message to onlookers */
	char *w_name = "";

	if(wield != NULL)
		w_name = wield->short_description;

    buf = replace_string(dam_weapons[msgnum].to_room, w_mess, dam_type);
	act(buf, FALSE, ch, NULL, victim, TO_NOTVICT, 3);

    if (!IS_NPC(ch)) {

        /* damage message to damager - ch */
        send_to_char(ch, CCYEL(ch, C_NRM));
        if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
            send_to_char(ch, "[%d] ", dam);

		buf = replace_string(dam_weapons[msgnum].to_char, w_mess, dam_type);
        act(buf, FALSE, ch, NULL, victim, TO_CHAR, 0);
        send_to_char(ch, CCNRM(ch, C_NRM));
    }

    if (!IS_NPC(victim)) {

        /* damage message to damagee - victim*/
        send_to_char(victim, CCRED(victim, C_NRM));
        if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
            send_to_char(victim, "[%d] ", dam);

        buf2 = replace_string(dam_weapons[msgnum].to_victim, w_mess, dam_type);
        act(buf2, FALSE, ch, NULL, victim, TO_VICT, 0);
        send_to_char(victim, CCNRM(victim, C_NRM));
    }
}
// XORG START RANGED (GUN)
static void dam_message_shooting(int dam, struct char_data *ch, struct char_data *victim, int w_type)
{
    struct obj_data *wield;
    int  msgnum;
    char *buf;

    static struct dam_weapon_type {
        char      *to_room;
        char      *to_char;
        char      *to_victim;
    } dam_weapons[] = {

        {
			"$n's #w flies harmlessly past $N.",       /* 0: 0 */
                "Your #w flies harmlessly past $N.",
                "$n fires a #w, which flies harmlessly past you."
        },

        {
            "$n's shot mostly deflects off of $N!",       /* 1: 1 - 20 */
                "Your shot mostly deflects off of $N!",
                "$n's shot mostly deflects off of you!"
            },

            {
                "$n's wild #w hits $N.",      /* 2: 21 - 50 */
                    "Your wild #w hits $N.",
                    "$n's wild #w hits you."
            },

            {
                "$n hits $N with a lucky #w.",  /* 3: 51 - 60 */
                    "You hit $N with a lucky #w.",
                    "You are hit by $n's lucky #w."
                },

                {
                    "$n's #w hits $N.",      /* 4: 61 - 80 */
						"Your #w hits $N.",
                        "$n's #w hits you."
                },

                {
                    "$n's #w staggers $N.",       /* 5: 81 - 110 */
                        "Your #w staggers $N.",
                        "$n's #w staggers you."
                    },

                    {
                        "Diving to one side, $n fires a #d at $N!",   /* 6: 111 - 150 */
							"You dive to one side while firing a #d at $N!",
                            "$n dives to one side while firing at you!"
                    },

                    {
                        "$n knocks $N down with $s #w.",   /* 7: 151 - 190  */
                            "You knock $N down with your #w.",
                            "You are knocked down by $n's #w."
                        },

                        {
                            "$n's deadly #w pierces $N",    /* 8: 191 - 260 */
                                "Your deadly #w pierces $N.",
                                "$n's deadly #w pierces your defenses!"
                        },

                        {
                            "$n snaps off a #w which hits $N squarely.",       /* 9: 261 - 280 */
                                "You snap off a #w which hits $N squarely.",
                                "$n's #w hits you squarely."
                            },

                            {
                                "$n fires, $s #w tearing through $N!", /* 10: 281 - 300 */
                                    "You fire a #w which tears through $N!",
                                    "$n's #w tears through you."
                            },

                            {
                                "$n quickly fires a lethal #w at $N, blood splatters across the ground!",/* 11: 301 - 320 */
                                    "You quickly fire a lethal #w at $N, blood splatters across the ground!",
                                    "$n quickly fires a lethal #w at you.  You're blood is sent flying across the room!"
                                },

                                {
                                    "Sidestepping an attack, $n fires off a quick #w which passes cleanly through $N!",      /* 12: 321 - 340 */
                                        "You sidestep $N's attack and fire off a #w which passes right through $M!",
                                        "You feel pain engulf your body as $n's return fire passes through you."
                                },

                                {
                                    "$n's well-placed #w rips off a chunk from $N!",   /* 13: 341 - 360 */
                                        "Your well-placed #w rips a chunk off $N!",
                                        "$n's #w tears a chunk of flesh from you!"
                                    },

                                    {
                                        "$n steps right in front of $N and pulls off a #w!",       /* 14: 361 - 390 */
                                            "You step in front of $N and shoot $S point blank!",
                                            "$n steps in front of you and $s #w hits you point blank!"
                                    },

                                    {
                                        "$n spins $s weapon expertly and staggers $N with a skilled #w!",  /* 15: 391 - 420 */
                                            "You spin your weapon expertly and stagger $N with a skilled #w!",
                                            "Spinning $s weapon, $n staggers you with a skilled #w!"
                                        },

                                        {
                                            "$n squeezes off a powerful #w which throws $N back a step!",        /* 16: 421 - 450 */
                                                "You squeeze off a powerful #w which throws $N back a step!",
                                                "You stumble back a step as $n's powerful #w slams into you."
                                        },

                                        {
                                            "$n aims low and cripples $N with $s #w.",    /* 17: 451 - 490 */
                                                "Aiming low, you cripple $N with your #w!",
                                                "$n's #w cripples you."
                                            },

                                            {
                                                "Ducking low, $n aims up and hits $N with a nearly decapitating #w!",      /* 18: 491 - 520*/
                                                    "You duck low and fires upward at $N with a nearly decapitating #w!",
                                                    "Ducking low, $n hits you with a #w that nearly takes off your head!"
                                            },

                                            {
                                                "$n's powerful #w takes $N dead-center!",      /* 19: 521 - 560 */
                                                    "The powerful #w from your weapon takes $N dead-center!",
                                                    "$n hits you dead-center with a powerful #w!"
                                                },

                                                {
                                                    "$n slips behind $N and fires a #w from behind!",     /* 20: 561 - 590 */
                                                        "You step around $N and shoot $M from behind!",
                                                        "$n suddenly steps around you and the #w hits you in the spine!"
                                                },

                                                {
                                                    "$n takes careful aim and hits $N with a devastating #w!", /* 21: 591 - 650 */
                                                        "Taking careful aim, you hit $N with a devastating #w!",
                                                        "$n's carefully aimed #w devastates you!"
                                                    },

                                                    {
                                                        "$n's powerful #w explodes through $N!",   /* 22: 650 - 690 */
                                                            "Your powerful #w explodes through $N!",
                                                            "$n's powerful #w explodes through you!"
                                                    },

                                                    {
                                                        "$n leaps into the air and pumps a #w into $N before landing.", /* 23: 691 - 740*/
                                                            "You leap into the air and shoot at $N before landing.",
                                                            "$n leaps up and hits you with a mighty #w before landing."
                                                        },

                                                        {
                                                            "$n's weapon glows with blue flame as $n fires at $N!",  /* 24: 741 - 780 */
                                                                "Your weapon glows with blue flame as you fire at $N!",
                                                                "You see $n's weapon glowing with blue f as $n's #w tears into you!"
                                                        },

                                                        {
                                                            "$n's deadly #w inflicts a nearly fatal wound on $N!", /* 25: 781 - 820 */
                                                                "Your deadly #w inflicts a nearly fatal wound on $N!",
                                                                "$n's deadly #w inflicts you with a nearly fatal wound!"
                                                            },

                                                            {
                                                                "$n's #w passes into $N and explodes out $S back!", /* 26: 821 - 920 */
                                                                    "Your #w passes through $N and explodes out $S back!",
                                                                    "$n's #w passes through you and explodes out your back!"
                                                            },

                                                            {
                                                                "$N is almost cut in two by $n's #w!", /* 27: 921 - 1000 */
                                                                    "Your #w nearly cuts $N in two!",
                                                                    "$n's #w hits you, nearly cutting you in two!"
                                                                },

                                                                {
																	"$N is obliterated by $n's perfectly aimed #w!", /* 28: 1000 - 25000 */
																		"Your perfectly aimed #w obliterates $N!",
																		"$n obliterates you with a perfectly aimed #w!"
                                                                },

                                                                {
                                                                    "$n's #w tears a FUCKING BLACK HOLE through $N!!", /* 29: 25000+ */
                                                                        "Your #w tears a FUCKING BLACK HOLE through $N!!",
                                                                        "$n's #w tears a FUCKING BLACK HOLE through you!!"
                                                                    }

    };

    wield = ch->equipment[WEAR_WIELD];
    if (dam == 0) msgnum = 0;
    else if (dam <= 20) msgnum = 1;
	else if (dam <= 50) msgnum = 2;
	else if (dam <= 60) msgnum = 3;
	else if (dam <= 80) msgnum = 4;
	else if (dam <= 120) msgnum = 5;
	else if (dam <= 180) msgnum = 6;
	else if (dam <= 250) msgnum = 7;
	else if (dam <= 300) msgnum = 8;
	else if (dam <= 350) msgnum = 9;
	else if (dam <= 400) msgnum = 10;
	else if (dam <= 450) msgnum = 11;
	else if (dam <= 500) msgnum = 12;
	else if (dam <= 550) msgnum = 13;
	else if (dam <= 600) msgnum = 14;
	else if (dam <= 650) msgnum = 15;
	else if (dam <= 700) msgnum = 16;
	else if (dam <= 750) msgnum = 17;
	else if (dam <= 800) msgnum = 18;
	else if (dam <= 900) msgnum = 19;
	else if (dam <= 1000) msgnum = 20;
	else if (dam <= 1500) msgnum = 21;
	else if (dam <= 2000) msgnum = 22;
	else if (dam <= 2500) msgnum = 23;
	else if (dam <= 3000) msgnum = 24;
	else if (dam <= 4000) msgnum = 25;
	else if (dam <= 5000) msgnum = 26;
	else if (dam <= 7500) msgnum = 27;
	else if (dam <= 25000) msgnum = 28;
    else                   msgnum = 29;

    /* Guns */
	char *w_name = "";
	const char *w_mess = "";
	const char *dam_type = "";
	
	if (get_damagetype(ch) > 0)
		dam_type = damagetype_insert[get_damagetype(ch)];

	int attacks = attack_rate[ATTACKS_PER_ROUND(ch)].fire_rate;

	if (attacks == 1)
		w_mess = attack_gun_hit_text[w_type].onehit;
	else if (attacks == 3)
		w_mess = attack_gun_hit_text[w_type].threehit;
	else if (attacks == 5)
		w_mess = attack_gun_hit_text[w_type].fivehit;
	else if (attacks == 10)
		w_mess = attack_gun_hit_text[w_type].tenhit;
	else 
		w_mess = attack_gun_hit_text[w_type].onehit;

	if(wield != NULL)
		w_name = wield->short_description;
    // damage message to onlookers
    buf = replace_string(dam_weapons[msgnum].to_room, w_mess, dam_type);
   	act(buf, FALSE, ch, NULL, victim, TO_NOTVICT, 3);

    if (!IS_NPC(ch)) {
        // damage message to damager - ch
        send_to_char(ch, CCYEL(ch, C_NRM));
        if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
            send_to_char(ch, "[%d] ", dam);
	
		buf = replace_string(dam_weapons[msgnum].to_char, w_mess, dam_type);
        act(buf, FALSE, ch, wield, victim, TO_CHAR, 0);
        send_to_char(ch, CCNRM(ch, C_NRM));
    }

    if (!IS_NPC(victim)) {

        // damage message to damagee - victim
        send_to_char(victim, CCRED(victim, C_NRM));
        if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
            send_to_char(victim, "[%d] ", dam);
		buf = replace_string(dam_weapons[msgnum].to_victim, w_mess, dam_type);	        
		act(buf, FALSE, ch, wield, victim, TO_VICT, 0);
        send_to_char(victim, CCNRM(victim, C_NRM));
    }
}
// XORG FINISH
/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
	    send_to_char(ch, CCYEL(ch, C_CMP));
	    act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	    send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
	  send_to_char(ch, CCYEL(ch, C_CMP));
	  act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(ch, CCNRM(ch, C_CMP));
        }

	send_to_char(vict, CCRED(vict, C_CMP));
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(vict, CCNRM(vict, C_CMP));

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

int get_savingthrow(struct char_data *ch, struct char_data *victim, int dam, int damagetype)
{
	float newdam = (float)dam;
	int i;
	for (i = 0; i <= MAX_SAVING_THROW; i++) {
		if (GET_SAVE(victim, i) != 0 && damagetype == i) {
			if (GET_SAVE(victim, i) > 0)
				newdam = (1 - (float)GET_SAVE(victim, i) / 100) * (float)dam;
			else if (GET_SAVE(victim, i) < 0)
				newdam = (1 + (((float)GET_SAVE(victim, i) * -1) / 100)) * (float)dam;
		}
	}
	return ((int)newdam);
}

/* This function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done. */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int damagetype)
{
    struct message_type *messages;
    char buf[MAX_STRING_LENGTH];
    int  i;
    int j;
    int nr;
    int exp;
    struct obj_data *wielded = 0;
    byte behead = FALSE;
    long local_units = 0, happy_units = 0;
    int happy_exp;
    char local_buf[256];
	int newdam;

    // you cannot damage a dead person
    if (GET_POS(victim) <= POS_DEAD) {

        if (IS_HIGHLANDER(ch) && IS_HIGHLANDER(victim) && ch != victim) {
            send_to_char(ch, "\r\nThere can be only one, behead %s!\n\r", GET_NAME(victim));
            return (-1);
        }
        else {
            /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
            if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
                return (-1);

            log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
                GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
            die(victim, ch, FALSE);
            return (-1);
        }
    }

    // peaceful rooms
    if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
        ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
            send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
            return (0);
    }
	if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL)) {
		if (ch != victim)
			return (0);
	}
    // shopkeeper protection
    if (!ok_damage_shopkeeper(ch, victim))
        return (0);

    // If you do damage of any type, YOU LOSE SNEAK!!! -- unless it is through an explosive
    if (!IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SNEAK) && attacktype != TYPE_EXPLOSIVE)
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);

    wielded = ch->equipment[WEAR_WIELD];

    // You can't damage an immortal!
    if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT && !PRF_FLAGGED(victim, PRF_TEMP_MORTAL) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
        dam = 0;
	
	// You can't harm an ethereal mob with normal damage
	if (IS_NPC(victim) && GET_MOB_CLASS(victim) == MOB_CLASS_ETHEREAL && damagetype != DMG_ETHEREAL)
		return (0);

    if (affected_by_psionic(victim, PSIONIC_SANCTUARY))
        dam = dam / 2;  // 1/2 damage when sanctuary
    else if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT))
        dam = dam / 3;  // 1/3 damage when super sancted
    else if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT))
        dam = dam / 4;  // 1/4 damage when mega sancted

	newdam = (get_savingthrow(ch, victim, dam, damagetype));


    /* neatly done set_fighting routine 11/11/97 Tails */
    if (victim != ch) {
        if (GET_POS(ch) > POS_SLEEPING) {

            if (!FIGHTING(ch) && attacktype != -1)
                set_fighting(ch, victim);
			if (!RANGED_FIGHTING(ch) && attacktype == -1)
				if (get_lineofsight(ch, victim, NULL, NULL))
					set_ranged_fighting(ch, victim);
            //See if an attacked mob switches to the master
            if (IS_NPC(ch) && IS_NPC(victim) && victim->master && !rand_number(0, 4) && AFF_FLAGGED(victim, AFF_CHARM) && (IN_ROOM(victim->master) == IN_ROOM(ch)) && FIGHTING(ch) && FIGHTING(ch) == victim) {

                // so we're switching from the charmie to the master.  ignore this attack,
                // and let the ch hit the master
                // todo:  this completely ignores what caused this damage
                // todo:  we should hit the victim and then switch to the master
                FIGHTING(ch) = victim->master;
				int thaco = rand_number(1,30);
                hit(ch, victim->master, TYPE_UNDEFINED, thaco, 0);
                return (-1);
            }

        }

        // Start the victim fighting the attacker
        if (GET_POS(victim) > POS_SLEEPING) {

            if (!FIGHTING(victim) && attacktype != -1)
                set_fighting(victim,ch);
			if (!RANGED_FIGHTING(victim) && attacktype == -1)
				if (get_lineofsight(victim, ch, NULL, NULL))
					set_ranged_fighting(victim, ch);

            // the mob should remember who has attacked it
            if (MOB_FLAGGED(victim, MOB_MEMORY)) {

                if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT)
                    remember(victim, ch);
            }

            // the mob should hunt who has attacked it
            if (MOB_FLAGGED(victim, MOB_HUNTER)) {

                if (!IS_NPC(ch) && GET_LEVEL(ch) < LVL_IMMORT)
                    HUNTING(victim) = ch;
                else if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && GET_LEVEL(ch->master) < LVL_IMMORT)
                    HUNTING(victim) = ch->master;
            }
        }
    }

    // If you attack a pet, it hates your guts
    if (victim->master == ch)
        stop_follower(victim);

    // If the attacker is invisible, he becomes visible
    if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_STEALTH)))
        appear(ch);

    // highlanders do not die normally.  set the lowest hp to -150 so they dont take so long to wake up
    GET_HIT(victim) = MAX(-150, GET_HIT(victim) - newdam);
    update_pos(victim);

    // you get experience for being in the battle
    if (ch != victim && newdam > 0) {

		int expmax = exp_cap[GET_LEVEL(ch)].exp_cap / 10;

        exp = newdam;

        if (IS_NPC(ch)) exp = 0;
		else {
			if (exp > expmax)
				exp = expmax;
		}
        gain_exp(ch, exp, FALSE);
    }

    // generate the damage messages
    if ((attacktype >= TYPE_MIN) && (attacktype <= TYPE_MAX) && (attacktype != TYPE_SHOOT)) {
        if (!IS_NPC(ch) && !ch->equipment[WEAR_WIELD])
            dam_message(newdam, ch, victim, TYPE_HIT);
        else
            dam_message(newdam, ch, victim, attacktype);
    }
    else if (attacktype == TYPE_SHOOT) {
		if (wielded) {
			if (!IS_NPC(ch) && IS_GUN(wielded)) {
		        if (IS_LOADED(ch->equipment[WEAR_WIELD]))
					dam_message_shooting(newdam, ch, victim, GET_GUN_TYPE(ch->equipment[WEAR_WIELD]));
				if (!IS_NPC(ch) && !IS_LOADED(ch->equipment[WEAR_WIELD]))
		            send_to_char(ch, "Click!\r\n");
			}
			else if (IS_NPC(ch) && IS_GUN(wielded))
				dam_message_shooting(newdam, ch, victim, GET_GUN_TYPE(ch->equipment[WEAR_WIELD]));
			else
				return (0);
        }
    }
	else if (attacktype == TYPE_UNDEFINED) {
	}
	else if (attacktype == -1) {
		
		if (!IS_NPC(ch) && !IS_LOADED(ch->equipment[WEAR_WIELD])) {
            send_to_char(ch, "Click!\r\n");
			return (0);
		}
		if (newdam > 0) {  
			send_to_char(ch, "@y[%d] You compress the trigger, firing a round of bullets at %s to the %s.@n\r\n", newdam, GET_NAME(victim), dirs[ch->char_specials.tmpdir]);
			act("$n fires a bullet at $N from a distance!", FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
			send_to_char(victim, "@r[%d] You stagger back and begin to bleed as a bullet tears into you!\r\n", newdam);
			act("$n begins to stagger back after a bullet hits $m squarely, blood erupts from the wound.\r\n", FALSE, victim, NULL, victim, TO_NOTVICT);
		}
		if (newdam <= 0) {
			send_to_char(ch, "@y[%d] You compress the trigger, firing a round of bullets at %s to the %s.@n\r\n", newdam, GET_NAME(victim), dirs[ch->char_specials.tmpdir]);
			act("$n fires a bullet at $N from a distance but all you can hear is the air moving.", FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
			send_to_char(victim, "@rA bullet just whizzes by you from a distance!\r\n");
			act("$n narrowly avoids death as a bullet just misses $M head.\r\n", FALSE, victim, NULL, victim, TO_NOTVICT);
		}
	}
    else {
        for (i = 0; i < MAX_MESSAGES; i++) {
            if (fight_messages[i].a_type == attacktype) {

                nr = dice(1, fight_messages[i].number_of_attacks);

                for (j = 1, messages = fight_messages[i].msg; (j < nr) && (messages); j++)
                    messages = messages->next;

                if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT)) {
                    act(messages->god_msg.attacker_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_CHAR);
                    act(messages->god_msg.victim_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_VICT);
					act(messages->god_msg.room_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
                }
                else if (newdam > 0) {

                    if (GET_POS(victim) <= POS_DEAD) {
						if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
							send_to_char(ch, "[%d] ", newdam);
						if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
							send_to_char(victim, "[%d] ", newdam);
                        act(messages->die_msg.attacker_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_CHAR);
                        act(messages->die_msg.victim_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_VICT);
						act(messages->die_msg.room_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
                    }
                    else {
						if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
							send_to_char(ch, "[%d] ", newdam);
						if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
							send_to_char(victim, "[%d] ", newdam);
                        act(messages->hit_msg.attacker_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_CHAR);
                        act(messages->hit_msg.victim_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_VICT);
						act(messages->hit_msg.room_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
                    }
                }
                else { /* dam == 0 */
						if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
							send_to_char(ch, "[%d] ", newdam);
						if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
							send_to_char(victim, "[%d] ", newdam);
                    act(messages->miss_msg.attacker_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_CHAR);
                    act(messages->miss_msg.victim_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_VICT);
					act(messages->miss_msg.room_msg, FALSE, ch, ch->equipment[WEAR_WIELD], victim, TO_NOTVICT);
                }
            }
        }
    }

    /* Use send_to_char -- act() doesn't send message if you are DEAD. */
    switch (GET_POS(victim)) {

    case POS_MORTALLYW:
        act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
        break;

    case POS_INCAP:
        act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
        break;

    case POS_STUNNED:
        act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
        break;

    case POS_DEAD:
       if (IS_HIGHLANDER(victim) && !PRF_FLAGGED(victim, PRF_TEMP_MORTAL)) {
            send_to_char(ch, "\r\nYour opponent is a Highlander. Will you show mercy or behead your victim!?.\n\r");
            act("$n lies prone on the ground in a death-like state... awaiting your blade. ", TRUE, victim, 0, 0, TO_ROOM);
            send_to_char(victim, "You are dead ... for now.\n\r");
            if (IS_NPC(ch)) {
				if (GET_MOB_CLASS(ch) == MOB_CLASS_HIGHLANDER)
			        behead = TRUE;
			}
        }
        else {

            // clear the fight action cache
            //fight_send_messages(ch);
			int randomdeath = rand_number(0,3);
            act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
			if (randomdeath == 0)
				send_to_char(victim, "Your spine has seperated at the neck after that final deathly blow.\r\n");
			if (randomdeath == 1)
				send_to_char(victim, "The attack penetrates your skull and your vision goes black instantly.\r\n");
			if (randomdeath == 2)
				send_to_char(victim, "You collapse to the ground and convulse, the pain slowly fades away.\r\n");
			if (randomdeath == 3)
				send_to_char(victim, "A flash of white lights encompass your vision as you feel you are floating away.\r\n");
            send_to_char(victim, "You are DEAD!  Sorry...\r\n");
        }
        break;

    default:            /* >= POSITION SLEEPING */
        if (newdam > (GET_MAX_HIT(victim) >> 2))
            send_to_char(victim, "That really did HURT!\r\n");


        if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
			if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOFIGHTSPAM))
				send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n", CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
		}

        if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) && GET_HIT(victim) < GET_WIMP_LEV(victim)) {
            send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
            do_flee(victim, "", 0, 0);
        }

        break;
    }

    // Help out poor linkless people who are attacked
    if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED && PRF_FLAGGED(victim, PRF_LDFLEE)) {

        do_flee(victim, NULL, 0, 0);

        if (!FIGHTING(victim)) {
            act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
            GET_WAS_IN(victim) = IN_ROOM(victim);
            char_from_room(victim);
            char_to_room(victim, real_room(3001));
        }
    }

    if (!AWAKE(victim)) {
        if (FIGHTING(victim))
            stop_fighting(victim);
		if (RANGED_FIGHTING(victim))
			stop_ranged_fighting(victim);
	}


    // stop someone from fighting if they're stunned or worse
    if (GET_POS(victim) <= POS_STUNNED && (FIGHTING(victim) != NULL || RANGED_FIGHTING(victim) != NULL)) {
        if (FIGHTING(victim))
			stop_fighting(victim);
		if (RANGED_FIGHTING(victim))
			stop_ranged_fighting(victim);
	}

    // Uh oh.  Victim died.
    if (GET_POS(victim) == POS_DEAD  && (!IS_HIGHLANDER(victim) || PRF_FLAGGED(victim, PRF_TEMP_MORTAL))) {

        if (ch != victim && (IS_NPC(victim) || victim->desc)) {
            if (AFF_FLAGGED(ch, AFF_GROUP)  && (ch->followers || ch->master))
                group_gain(ch, victim);
            else {

                /* Calculate level-difference bonus for experience
                Ideal solo level is mob + 5 = pc level, so a level 25 killing a
                level 20 mob gets no bonus, but a level 25 killing a level 25 mob
                gets a 5% bonus for every level below the ideal.
                */

                exp =((float)GET_EXP(victim) * (1 +(0.05*(float)(GET_LEVEL(victim)-GET_LEVEL(ch) - 5))));

                if (IS_NPC(ch))
                    exp = 1;

                else if (!IS_NPC(victim))
                    exp = 10000;

                exp = MAX(exp, 1); /* this was set at x2 ?? */

				if (exp > exp_cap[GET_LEVEL(ch)].exp_cap)
					exp = exp_cap[GET_LEVEL(ch)].exp_cap;

                if (IS_HAPPYHOUR && IS_HAPPYEXP) {
                
					happy_exp = exp + (int)((float)exp * ((float)HAPPY_EXP / (float)(100)));
					exp = MAX(happy_exp, 1);
				}
                
                if (ch != victim)
                    gain_exp(ch, exp, TRUE);
            }
        }

        if (!IS_NPC(victim)) {

            mudlog(BRF, 0, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);

            if (MOB_FLAGGED(ch, MOB_MEMORY))
                forget(ch, victim);

            if (HUNTING(ch) == victim)
                HUNTING(ch) = NULL;
        }

		if (LOCKED(ch) == victim)
			LOCKED(ch) = NULL;
		if (RANGED_FIGHTING(ch) == victim)
			stop_ranged_fighting(ch);

		if (IS_NPC(victim)) {
			if ((IS_HAPPYHOUR) && (IS_HAPPYUNITS))
			{
				happy_units = (long)(GET_UNITS(victim) * (((float)(happy_units))/(float)100));
				happy_units = MAX(0, happy_units);
				increase_units(victim, happy_units);
			}
			
			local_units = GET_UNITS(victim);
			sprintf(local_buf,"%ld", (long)local_units);
		}

        die(victim, ch, FALSE);

        if (IS_NPC(victim) && !IS_NPC(ch) && attacktype != -1) {
    
			if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOUNITS))
				do_get(ch, "all.unit corpse", 0, 0);

			if (PRF_FLAGGED(ch, PRF_AUTOLOOT)) 
				do_get(ch, "all xxcorpsexx", 0, 0);

			if (PRF_FLAGGED(ch, PRF_AUTOSCAVENGER))
				do_scavenger(ch, "xxcorpsexx", 0, 0);

	        if (PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
				sprintf(buf, "%ld", local_units);
			    do_split(ch, buf, 0, 0);
			}
			if (IS_MOB(victim))
				if ((GET_MOB_VNUM(victim) > 10800) && (GET_MOB_VNUM(victim) < 10900))
					load_prize(ch);
		}
	
    }

    // PC_NAME is also the mob keyword, so highlander mobs can behead each other
    if (behead) {
        do_behead(ch, GET_PC_NAME(victim), 0, 0);
        return (0);
    }
    return (newdam);
}
// this function allows any object (mainly explosives) to do damage to a character
// as such, it doesn't need some of the checks found in the basic damage() function
int obj_damage_char(struct obj_data *obj, struct char_data *victim, int dam)
{
    float percent;
    float exp_removed;

	// Can't kill anyone who isnt in the room?
	if (!victim)
		return(-1);

    // you cannot damage a dead person
    if (GET_POS(victim) <= POS_DEAD) {

        if (IS_HIGHLANDER(victim))
            return (-1);
        else {

            if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
                return (-1);

            log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
                GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), obj->short_description);
            die(victim, NULL, TRUE);
            return (-1);
        }
    }

    // peaceful rooms
    if (ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL))
        return (0);

    // You can't damage an immortal!
    if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT && !PRF_FLAGGED(victim, PRF_TEMP_MORTAL) && PRF_FLAGGED(victim, PRF_NOHASSLE)))
        dam = 0;

    if (affected_by_psionic(victim, PSIONIC_SANCTUARY))
        dam = dam / 2;  // 1/2 damage when sanctuary
    else if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT))
        dam = dam / 3;  // 1/3 damage when super sancted
    else if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT))
        dam = dam / 4;  // 1/4 damage when mega sancted

    // take into account armor
    dam -= GET_AC(victim);

    // Set the maximum damage per round and subtract the hit points
    dam = MAX(dam, 0);

    // highlanders do not die normally.  set the lowest hp to -150 so they dont take so long to wake up
    GET_HIT(victim) = MAX(-150, GET_HIT(victim) - dam);

    update_pos(victim);

    /* Use send_to_char -- act() doesn't send message if you are DEAD. */
    switch (GET_POS(victim)) {

    case POS_MORTALLYW:
        act("$n is mortally wounded, and will die soon if not aided.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You are mortally wounded, and will die soon if not aided.\r\n");
        break;

    case POS_INCAP:
        act("$n is incapacitated and will slowly die if not aided.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You are incapacitated and will slowly die if not aided.\r\n");
        break;

    case POS_STUNNED:
        act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
        send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
        break;

    case POS_DEAD:
        if (IS_HIGHLANDER(victim) && !PRF_FLAGGED(victim, PRF_TEMP_MORTAL)) {
            act("$n lies prone on the ground in a death-like state.", TRUE, victim, 0, 0, TO_ROOM);
            send_to_char(victim, "You are dead ... for now.\n\r");
        }
        else {
            act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
            send_to_char(victim, "You are dead!  Sorry...\r\n");
        }
        break;

    default:            /* >= POSITION SLEEPING */
        if (dam > (GET_MAX_HIT(victim) >> 2))
            send_to_char(victim, "That really did HURT!\r\n");

        if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2))
            send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n", CCRED(victim, C_SPR), CCNRM(victim, C_SPR));

        if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && GET_HIT(victim) < GET_WIMP_LEV(victim)) {
            send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
            do_flee(victim, "", 0, 0);
        }

        break;
    }

    // Help out poor linkless people who are attacked
    if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED && PRF_FLAGGED(victim, PRF_LDFLEE)) {

        do_flee(victim, NULL, 0, 0);

        if (!FIGHTING(victim)) {
            act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
            GET_WAS_IN(victim) = IN_ROOM(victim);
            char_from_room(victim);
            char_to_room(victim, real_room(3001));
        }
    }

    // stop someone from fighting if they're stunned or worse
    if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
        stop_fighting(victim);

    // check to see if victim is a mob, reduce the exp so players cannot gain exp from 1-hitting mobs
    if (IS_NPC(victim) && dam > 0) {

        percent = (float)dam/(float)GET_MAX_HIT(victim);

        // remove this same percentage from exp
        exp_removed = percent * GET_MAX_EXP(victim);

        // remove the exp
        GET_EXP(victim) = MAX(0, GET_EXP(victim) - (int)exp_removed);
    }



    // Uh oh.  Victim died.
    if (GET_POS(victim) == POS_DEAD  && (!IS_HIGHLANDER(victim) || PRF_FLAGGED(victim, PRF_TEMP_MORTAL))) {

        if (!IS_NPC(victim))
            mudlog(BRF, 0, TRUE, "%s killed by %s at %s", GET_NAME(victim), obj->short_description, world[IN_ROOM(victim)].name);

		if (GET_OBJ_TYPE(obj) == ITEM_EXPLOSIVE)
	        die(victim, NULL, TRUE);
		else
			die(victim, NULL, FALSE);

        return (-1);
    }

    // if we reach here, the victim has not died and damage was done
    return (dam);
}

int gun_prof(struct char_data *ch, int type)
{

    int modifier = 0;

    switch(type) {

    case GUN_NONE:
        break;

    case GUN_CLIP:
        if (GET_SKILL_LEVEL(ch, SKILL_AUTO_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

    case GUN_ENERGY:
        if (GET_SKILL_LEVEL(ch, SKILL_ENERGY_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

    case GUN_SHELL:
        if (GET_SKILL_LEVEL(ch, SKILL_SHELL_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

    case GUN_ROCKET:
        if (GET_SKILL_LEVEL(ch, SKILL_ROCKET_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

    case GUN_AREA:
        if (GET_SKILL_LEVEL(ch, SKILL_EXPLO_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

    case GUN_FUEL:
        if (GET_SKILL_LEVEL(ch, SKILL_FUEL_WPN_PRO))
            modifier = 4*(GET_LEVEL(ch)/5);
        break;

	case GUN_BOLT:
		if (GET_SKILL_LEVEL(ch, SKILL_ARCHERY_WPN_PRO))
			modifier = 4*(GET_LEVEL(ch)/5);
		break;

	case GUN_ARROW:
		if (GET_SKILL_LEVEL(ch, SKILL_ARCHERY_WPN_PRO))
			modifier = 4*(GET_LEVEL(ch)/5);
		break;

    default:
        log("SYSERR: Illegal GUN type in fight.c gun_prof");
        break;
    }

    return (modifier);
}

// Ranged combat, we will incorporate this into the standard hit function after it works properly - Gahan
void rangedhit(struct char_data *ch, struct char_data *victim, int type, int thaco)
{
	struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
	struct obj_data *obj;
	struct obj_data *ammo;
	int w_type;
	int dam = 0;
	int diceroll;
	int which_psionic;
	int skill_level;
	int calc_hitroll = 0;
	int calc_damroll = 0;
	int victim_save;
	int raw_damage;
	int fire = 0;
	int dam_absorbed;
	int backstab_mult(int level);
	int attack_rates;
	int attacks;
	int hitdice;
	int CMBMND = 0;
	int miss = 0;
	int crit = 0;
	int dupehit = 0;
	int j = 0;
	int i = 0;
	int num_attacks = 1;
	byte behead = FALSE;
	int totaldam = 0;


    if (!ch || !victim) {
		stop_ranged_fighting(ch);
		return;
	}

	if (GET_POS(victim) == POS_DEAD) {
		stop_ranged_fighting(ch);
		return;
	}

	w_type = TYPE_SHOOT;

	if (wielded) {
		if (IS_GUN(wielded)) {
		    num_attacks = GET_SHOTS_PER_ROUND(wielded);

			if (IS_LOADED(ch->equipment[WEAR_WIELD]) && !IS_NPC(ch)) {
				if (GET_LOADED_AMMO_COUNT(ch->equipment[WEAR_WIELD]) <= fire)
					num_attacks = GET_LOADED_AMMO_COUNT(wielded);
			}
		}
		else
			num_attacks = 1;
    }
    else
		num_attacks = 1; //This shouldn't happen, but just in case!

	if (IS_NPC(ch)) {
		if (!(get_lineofsight(ch, victim, NULL, NULL)))
			end_rangedcombat(victim);
	}

	if (GET_DELAY_HIT_WAIT(ch) > 0) num_attacks = 0;
	
	if (num_attacks > 30)
		num_attacks = 30;
	
	// Figure out the delay for each round of attacks //
	ATTACKS_PER_ROUND(ch) = num_attacks;

	attacks = attack_rate[ATTACKS_PER_ROUND(ch)].fire_rate;
	attack_rates = attack_rate[ATTACKS_PER_ROUND(ch)].num_attacks;
	TICK_TO_HIT(ch) = attack_rates;

	for (j = 1; j <= attacks; j++) {
        // did the victim die before we were done?
        if (!victim) return;

        // check for highlander and dead victims
        if (GET_POS(victim) == POS_DEAD) {
            if (IS_HIGHLANDER(victim) && !PRF_FLAGGED(victim, PRF_TEMP_MORTAL)) {
                act("$N is lying on the ground prone.  Why don't you try beheading $M?", FALSE, ch, 0, victim, TO_CHAR);
                if (IS_NPC(ch) && IS_HIGHLANDER(ch))
                    behead = TRUE;
            }
            stop_ranged_fighting(ch);
            break;
        }
		if (thaco == 0)
			thaco = rand_number(1,30);

		hitdice = rand_number(1, 30);
        diceroll = thaco + hitdice;
        calc_hitroll = GET_DEX(ch);
        calc_hitroll += diceroll;

        if (GET_SKILL_LEVEL(ch, SKILL_CRITICAL_KNOWLEDGE))
	        calc_damroll += 3;

		calc_hitroll += gun_prof(ch, GET_GUN_TYPE(wielded));
        calc_hitroll += (GET_PCN(ch));

        if (GET_SKILL_LEVEL(ch, SKILL_MARKSMANSHIP))
			calc_hitroll += 3;
        
		if (GET_SKILL_LEVEL(ch, SKILL_SNIPER))
			calc_hitroll += 7;
        if (GET_SKILL_LEVEL(ch, SKILL_DEADLY_AIM))
			calc_damroll += 7;
                
		calc_hitroll *= GET_HITROLL(ch);

        if (!IS_NPC(victim))
            victim_save = GET_LEVEL(victim);
        
		else
            victim_save = GET_LEVEL(victim) * 2;

        victim_save += GET_DEX(victim);
        victim_save += GET_CON(victim);
        victim_save *= GET_AC(victim);
        victim_save = victim_save - (victim_save / rand_number(1,10));

        raw_damage = (GET_PCN(ch) - 10) * 4;

        // double stat bonus damage if combat mind is on
        if (affected_by_psionic(ch, PSIONIC_COMBAT_MIND))
            raw_damage *= 2;

        // add base damage to damroll
        raw_damage += GET_DAMROLL(ch);

        // add any bonus from skills above
        raw_damage += calc_damroll;

		// Randomize damage a bit... Todo: lets do something better in the future
        raw_damage -= rand_number(GET_DAMROLL(ch)/10, GET_DAMROLL(ch)/3);
        raw_damage += rand_number(GET_DAMROLL(ch)/10, GET_DAMROLL(ch)/4);

        // check for gun damage

	    if (IS_GUN(wielded)) {
			raw_damage += gun_prof(ch, GET_GUN_TYPE(wielded));
			if (IS_LOADED(ch->equipment[WEAR_WIELD]) && !IS_NPC(ch))
				subtract_ammo(ch->equipment[WEAR_WIELD], ch);
		}

		raw_damage += get_dice(ch);

        if (GET_POS(victim) < POS_FIGHTING)
            raw_damage *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

		if (IS_NPC(ch))
			dam_absorbed = (GET_AC(victim) / 5);
		else
			dam_absorbed = (GET_AC(victim) / 2);

        if (!IS_NPC(ch) && GET_SKILL_LEVEL(ch, SKILL_CRITICAL_KNOWLEDGE) != 0 && diceroll >= 53)
            diceroll = 60;

        if ((diceroll <= 56) && (diceroll > 7)) {
            if (( calc_hitroll > victim_save)) { // Hit
                if (victim_save * 2 > calc_hitroll)
                    dam = raw_damage / 2;
                else if (victim_save * 4 > calc_hitroll)
                    dam = raw_damage - (dam_absorbed / 4);
                else
                    dam = raw_damage - (dam_absorbed / 7);

                if (dam  < 5) {
					miss++;
                    dam = 1;
                    diceroll = 1;
				}
            }
            else
            {
				if (IS_NPC(ch) && !IS_NPC(victim)) // Mobs need more help hitting players with crazy saves
				{
					if (GET_REMORTS(victim) > 2) {
						int randchance = rand_number(0,10);
						if (randchance > 7)
							dam = raw_damage - (dam_absorbed * 1.5);
					}
				}
				else {
					miss++;
					dam = 0;
					diceroll = 1;
				}
            }
        } 
        else 
        {  // Critical hit stuff 

            if ((diceroll > 56)) 
            {	// Critical Hit
                crit++;
				calc_hitroll = victim_save + 10;  // makes sure you hit 
                i = victim->points.hit * .5;
				if (IS_NPC(victim)) {
	                dam = raw_damage * 2;  // critical hit means ac absorbs nothing
	                if (dam > i)
		                dam = i + (raw_damage / 5);
				}
				else {
					dam = (raw_damage * 2) - dam_absorbed;
					if (dam > (GET_MAX_HIT(victim) / 2))
						dam = (GET_MAX_HIT(victim) / 2);
				}
            } 

            if (diceroll <= 7) 
			{
                // Critical Miss 
                if (affected_by_psionic(ch, PSIONIC_COMBAT_MIND) && rand_number(0,5) < 3) 
                {
                    //Saved!
					CMBMND++;
                    calc_hitroll = victim_save + 10;  // makes sure you hit
                    i = victim->points.hit * .25; //not as hard as a real crit, but nice
                    dam = raw_damage * 2;  // critical hit means ac absorbs nothing

                    if(dam > i)
                        dam = i + raw_damage;

                    diceroll = 60; //prevent multipliers
                }
                else 
                {
					// You critically miss here.
					miss++;
                    calc_hitroll = victim_save - 10; // makes sure you miss
                    dam = 0;

					int criticalsave = rand_number(1,4);

					switch(criticalsave) {

					case 1:
						act("@RThe pressure to hit your mark throws you off, you completely miss $N!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						break;
					case 2:
						act("@RSomeone startles you as you are about to pull the trigger, and the shot goes completely wild.@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						break;
					case 3:
						act("@YYou weren't properly prepared to fire, and the gun's kickback hits your face!@n\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
						act("@YThe kickback on $n's weapon hits $s in the face, how embarrassing!@n", TRUE, ch, 0, victim, TO_NOTVICT, 3);
						break;
					case 4:
						act("@YYou try and slow your breathing down to aim properly but get light headed and fire at the ground!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						act("@Y$n fires a gun at the ground, thats embarrassing!@n", TRUE, ch, 0, 0, TO_ROOM, 3);
						break;
					}

                    if (!IS_NPC(ch) && criticalsave == 3)
                    {

						GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
						GET_HIT(ch) = GET_HIT(ch) - 100;
							
						if (GET_HIT(ch) < 0)
							GET_HIT(ch) = -1;

						j = attacks;
						break;
					}
                    
					else {
						GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
						j = attacks;
						break;
                    }
                } // END OF CRITICAL MISS (DAMAGE/LOSS OF TURN)
            } // END OF CRITICAL MISS CHECKS
        } // END OF CRITICAL CHECKS

		if ((diceroll != 60) && AFF_FLAGGED(victim, AFF_STUN))
			dam = 2*dam;

		if ((victim->char_specials.saved.duplicates > 0 && dam > 2 )) {
			if (victim->char_specials.saved.duplicates > 0 && dam > 2) 
			{

                // Added code to randomize 'hits' on hallucinations. Rationally, we would
                // expect an opponent to correctly choose the 'real' opponent sometimes
                // and when that happens, we assume his mind is strong enough to destroy
                // the hallucinations.  I'm keeping it simple for now, but might add to it later.

				int rand_num = rand_number(1, 100);
				int dupe = victim->char_specials.saved.duplicates;
				int mob_goal = 100/(1 + dupe) + GET_LEVEL(ch) - GET_LEVEL(victim);
				int int_diff = GET_INT(victim) - GET_INT(ch);
				int pcn_diff = GET_PCN(victim) - GET_PCN(ch);
				int lvl_diff = GET_LEVEL(victim) - GET_LEVEL(ch);
				int player_goal = 100/(1 + dupe) + (int_diff + pcn_diff + lvl_diff)/4;

				if (!IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SECOND_SIGHT))
					player_goal += 15;
				if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SECOND_SIGHT))
					mob_goal += 10;

	            circle_srandom(time(0));

	            if ((IS_NPC(ch) && rand_num < mob_goal)  || (!IS_NPC(ch) && rand_num < player_goal))
		        {

					act("You luckily hit $N instead of $S hallucinations!", FALSE, ch, 0,victim, TO_CHAR);
					act("$n luckily hits you instead of your hallucinations!", FALSE, ch, 0, victim, TO_VICT);
					dupehit = 0;
					damage(ch, victim,  dam, w_type, DMG_NORMAL);

				}
				else 
				{
                    // opponent hits hallucination - shatter a duplicate
					victim->char_specials.saved.duplicates--;
					dupehit++;
					if ((victim->char_specials.saved.duplicates == 0) && (affected_by_psionic(victim, PSIONIC_DUPLICATES)))
						affect_from_char(victim, PSIONIC_DUPLICATES, TRUE);

				}
		    }
		}
		totaldam += dam;
	} // END OF ATTACK LOOP

	if (crit >= 1)
		act("@W$N staggers back as a bullet catches $M in a weak spot!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
	if (CMBMND >= 1)
		act("@YYou manage to dodge $N's attack, and quickly fire off a round leaving $M off guard!\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
	if (dupehit >= 1) {
		if (!PRF_FLAGGED(ch, PRF_NOFIGHTSPAM))
			send_to_char(ch, "@GYou watch %d images of your target flicker out of existence!\r\n@n", dupehit);
		if (!PRF_FLAGGED(victim, PRF_NOFIGHTSPAM))
			send_to_char(victim, "@G%s hits %d of your duplicates, they flicker out of existence!\r\n@n", GET_NAME(ch), dupehit);
		dam = 0;
	}

	damage(ch, victim, totaldam, -1, get_damagetype(ch));
	update_pos(victim);

    // auto con and weapon psionic
    if (GET_POS(victim) > POS_DEAD) {

        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOCON) && GET_POS(ch) > POS_DEAD) {
            diag_char_to_char(victim, ch);
			send_to_char(ch, "\r\n");
		}

        // can't we just use 'wielded' here?
        obj = ch->equipment[WEAR_WIELD];

        // perform weapon psionic effect
        if (obj && obj->obj_wpnpsi.which_psionic > 0) {
			int randweapon = rand_number(1,100);
			if (randweapon >= 77) {
				which_psionic = obj->obj_wpnpsi.which_psionic;
				skill_level = obj->obj_wpnpsi.skill_level;
				wpn_psionic(ch, victim, obj, which_psionic, skill_level);
			}
        }

        //New: Ammo weapon psionics! -Lump
        if (obj && IS_GUN(obj) && IS_LOADED(obj)) {
            ammo = GET_AMMO_LOADED(obj);

            // perform ammo weapon psionic effect
            if (ammo->obj_wpnpsi.which_psionic > 0) {
                which_psionic = ammo->obj_wpnpsi.which_psionic;
                skill_level = ammo->obj_wpnpsi.skill_level;
                wpn_psionic(ch, victim, obj, which_psionic, skill_level);
            }
        }
    }
}
void hit(struct char_data *ch, struct char_data *victim, int type, int thaco, int range)
{
	struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
	int w_type;
	int wpn;
	int dam = 0;
	int diceroll;
	int which_psionic;
	int skill_level;
	int calc_hitroll = 0;
	int calc_damroll = 0;
	int victim_save = 0;
	int raw_damage;
	int fire;
	int dam_absorbed;
	int backstab_mult(int level);
	int attack_rates;
	int attacks;
	int hitdice;
	int CMBMND = 0;
	int miss = 0;
	int crit = 0;
	int dupehit = 0;
	int j = 0;
	int i = 0;
	int num_attacks = 1;
	struct obj_data *obj;
	struct obj_data *ammo;
	byte behead = FALSE;

    if (!ch || !victim) {
		stop_fighting(ch);
        return;
	}
	if (GET_POS(victim) == POS_DEAD && !IS_HIGHLANDER(victim)) {
		stop_fighting(ch);
		return;
	}
    // Do some sanity checking, in case someone flees, etc.
    if (IN_ROOM(ch) != IN_ROOM(victim)) {
        if (FIGHTING(ch) && FIGHTING(ch) == victim)
            stop_fighting(ch);
        GET_TOTDAM(ch) = 0;
        GET_TOTHITS(ch) = 0;
        GET_TOTDAM(victim) = 0;
        GET_TOTHITS(victim) = 0;
        return;
    }


/* 10/26/16 to fix hiding mobs from staying hidden? */
    REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_HIDE);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);


  if (!IS_NPC(victim) && ((GET_SKILL_LEVEL(victim, SKILL_REBOOT)) != 0) && !char_has_mud_event(victim, eD_REBOOT) && char_has_mud_event(victim, eD_ROLL)) {
      GET_HIT(victim)  = GET_MAX_HIT(victim); 
    act("\tOSystem Emergency Reboot Complete......Maximum HP Restored."
            " \tn$N looks confused\tO!\tn", FALSE, victim, NULL, ch, TO_CHAR);
    act("$n \tOperforms a reboot of bionic systems. They look refreshed!!\tn",
            FALSE, victim, NULL, ch, TO_VICT | TO_SLEEP);
    act("$n performs an emergency \tOreboot\tn they look refreshed!\tn", FALSE, victim, NULL, ch, TO_NOTVICT);
    attach_mud_event(new_mud_event(eD_REBOOT, victim, NULL),(PULSE_IDLEPWD * 20));
   return ;
}

    // Find the weapon type (for display purposes only)
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
        w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
    else if (wielded && GET_OBJ_TYPE(wielded) == ITEM_GUN) {
        if (IS_NPC(ch) || IS_LOADED(wielded)) w_type = TYPE_SHOOT;
		else w_type = TYPE_BASH;
	}
    else {
        if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
            w_type = ch->mob_specials.attack_type + TYPE_HIT;
        else
            w_type = TYPE_HIT;
    }
    // base calculation of player number of attacks
    if (!IS_NPC(ch)) {
        if (AFF_FLAGGED(ch, AFF_EXARMS)) num_attacks += 1;
        if (AFF_FLAGGED(ch, AFF_FOOTJET)) num_attacks += 1;
        if (AFF_FLAGGED(ch, AFF_MATRIX)) num_attacks += 1;
    }
    // base calculation of mob number of attacks
    if (IS_NPC(ch)) {
        if (GET_MOB_ATTACKS(ch) == -1) { // old-style haste flags
            if (MOB_FLAGGED(ch, MOB_HASTE_2)) num_attacks += 1;
            if (MOB_FLAGGED(ch, MOB_HASTE_3)) num_attacks += 3;
            if (MOB_FLAGGED(ch, MOB_HASTE_4)) num_attacks += 4;
            if (MOB_FLAGGED(ch, MOB_PACIFIST)) num_attacks = 0;
        }
        else
            num_attacks = (int) GET_MOB_ATTACKS(ch);
    }
    // calculation of shared player and mob number of attacks modifiers
    if (affected_by_psionic(ch, PSIONIC_LETHARGY) && num_attacks > 1) num_attacks--;
    if (affected_by_psionic(ch, PSIONIC_HASTE)) num_attacks++;
    if (affected_by_drug(ch, DRUG_FLASH)) num_attacks++;
    if (GET_SKILL_LEVEL(ch, SKILL_DOUBLE_HIT))
		num_attacks++;
    // Lets limit the number of player attacks
    // the max attacks for a PC is 5 with vorpal or 6 if they're predator with vorpal, geeze
    if (!IS_NPC(ch)) {
        if (num_attacks > 4)
            num_attacks = 4;
        if (affected_by_psionic(ch, PSIONIC_VORPAL_SPEED))
            num_attacks += rand_number(0, 2);
		if (AFF_FLAGGED(ch, AFF_MANIAC)) {
			if ((wielded) && !IS_GUN(wielded))
				num_attacks += rand_number(1, 3);
		}
        // predators are better in combat
        if (GET_CLASS(ch) == CLASS_PREDATOR)
            num_attacks++;
    }
    if (w_type == TYPE_SHOOT && !IS_NPC(ch)) {
		if (wielded) {
			if (GET_OBJ_TYPE(wielded) != ITEM_GUN)
				fire = 0;
			else
		        fire = GET_SHOTS_PER_ROUND(wielded);
		}
		else
			fire = 0;

		if (wielded) {
	        if (!IS_NPC(ch) && IS_LOADED(ch->equipment[WEAR_WIELD])) {
		        if (GET_LOADED_AMMO_COUNT(ch->equipment[WEAR_WIELD]) <= fire)
			        num_attacks = GET_LOADED_AMMO_COUNT(wielded);
				else
					num_attacks = fire;
			}
        }
    }
	
	if (GET_DELAY_HIT_WAIT(ch) > 0) num_attacks = 0;
	
	if (num_attacks > 30)
		num_attacks = 30;
	
	// Figure out the delay for each round of attacks //
	ATTACKS_PER_ROUND(ch) = num_attacks;

	if (!IS_NPC(ch)) {
		attacks = attack_rate[ATTACKS_PER_ROUND(ch)].fire_rate;
		attack_rates = attack_rate[ATTACKS_PER_ROUND(ch)].num_attacks;
		TICK_TO_HIT(ch) = attack_rates;
	}
	if (IS_NPC(ch)) {
		attacks = attack_rate[ATTACKS_PER_ROUND(ch)].fire_rate;
		attack_rates = attack_rate[ATTACKS_PER_ROUND(ch)].num_attacks;
		TICK_TO_HIT(ch) = attack_rates;
	}
	
	if (!FIGHTING(victim))
		set_fighting(victim, ch);

	// this will be the new beginning of the fight loop
	if (type == TYPE_UNDEFINED)
		type = TYPE_HIT;
		
	for (j = 1; j <= attacks; j++) {
        // did the victim die before we were done?
        if (!victim) {
			stop_fighting(ch);
			return;
		}
        // check for highlander and dead victims
        if (GET_POS(victim) == POS_DEAD) {
            if (IS_HIGHLANDER(victim)) {
                act("$N is lying on the ground prone.  Why don't you try beheading $M?", FALSE, ch, 0, victim, TO_CHAR);
                if (IS_NPC(ch) && IS_HIGHLANDER(ch))
                    behead = TRUE;
            }
            stop_fighting(ch);
            break;
        }
		if (thaco == 0)
			thaco = rand_number(1,30);

		int falldam = 0;
		hitdice = rand_number(1, 30);
        diceroll = thaco + hitdice;
        calc_hitroll = GET_DEX(ch);
        calc_hitroll += diceroll;

        // if you are using a gun, you might get a bonus to hitroll or damroll based on skills
        if (!IS_NPC(ch))
        {
            if (wielded) 
            {
                if (GET_SKILL_LEVEL(ch, SKILL_CRITICAL_KNOWLEDGE))
                    calc_damroll += 3;
                if (IS_GUN(wielded)) {
                    calc_hitroll += gun_prof(ch, GET_GUN_TYPE(wielded));
                    calc_hitroll += (GET_PCN(ch));

                    if (GET_SKILL_LEVEL(ch, SKILL_MARKSMANSHIP))
                        calc_hitroll += 3;
                    if (GET_SKILL_LEVEL(ch, SKILL_SNIPER))
                        calc_hitroll += 7;
                    if (GET_SKILL_LEVEL(ch, SKILL_DEADLY_AIM))
                        calc_damroll += 7;
				} 
				else {
                    if (GET_SKILL_LEVEL(ch, SKILL_MELEE_WPN_PRO)) {
                        calc_hitroll += 4*(GET_LEVEL(ch)/5);
                        calc_damroll += 1;
                    }
                    if ((GET_SKILL_LEVEL(ch, SKILL_STRONGARM)) && !IS_REMORT(ch)) {
                        calc_hitroll += ((GET_LEVEL(ch) /2 * 1));
                        calc_damroll += ((GET_LEVEL(ch) /2 * 1));
                    }
                    if ((GET_SKILL_LEVEL(ch, SKILL_STRONGARM)) && IS_REMORT(ch)) {
                        calc_hitroll += ((GET_LEVEL(ch) /2 * GET_REMORTS(ch)));
                        calc_damroll += ((GET_LEVEL(ch) /2 * GET_REMORTS(ch)));
                    }
                    calc_hitroll += (GET_STR(ch));
                }
            }
        }
        
		calc_hitroll += GET_HITROLL(ch);

        // todo: weapons base damage is based on perception?
        // todo: only guns. melee is str -bossy 2016
        if ((wielded) && IS_GUN(wielded))
            raw_damage = (GET_PCN(ch) - 10) * 4;
        else {
            raw_damage = str_app[GET_STR(ch)].todam;
			raw_damage += (IS_WEARING_W(ch) / 5);
		}

        // double stat bonus damage if combat mind is on
        if (affected_by_psionic(ch, PSIONIC_COMBAT_MIND))
            raw_damage *= 2;
		// weapon upgrade check
		for (wpn = 1; wpn < NUM_WEARS; wpn++)
			if (GET_EQ(ch, wpn))
				if (GET_OBJ_TYPE(GET_EQ(ch, wpn)) == ITEM_WEAPONUPG)
					raw_damage += dice(GET_OBJ_VAL(GET_EQ(ch, wpn), 1), GET_OBJ_VAL(GET_EQ(ch, wpn), 2));

        // add base damage to damroll
        raw_damage += rand_number((GET_DAMROLL(ch)/2),GET_DAMROLL(ch));
        // add any bonus from skills above
        raw_damage += calc_damroll;
        // check for gun damage
        if (!IS_NPC(ch)) {
            if (wielded) {
                if (IS_GUN(wielded)) {
                    raw_damage += gun_prof(ch, GET_GUN_TYPE(wielded));
					if (IS_LOADED(ch->equipment[WEAR_WIELD]))
						subtract_ammo(ch->equipment[WEAR_WIELD], ch);
				}
                else {
                    if (GET_SKILL_LEVEL(ch, SKILL_MELEE_WPN_PRO))
                        raw_damage += 4*(GET_LEVEL(ch)/4);
                }
            }
        }

		raw_damage += get_dice(ch);		
        if (GET_POS(victim) < POS_FIGHTING)
            raw_damage *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
        if (IS_NPC(ch))
			dam_absorbed = GET_AC(victim) / 2;
		else
			dam_absorbed = GET_AC(victim) / 2 + (GET_REMORTS(ch) * 50);

		// wearing more weight increases your ability to absorb attacks
		dam_absorbed += (IS_WEARING_W(victim) / 3);

        if (!IS_NPC(ch) && GET_SKILL_LEVEL(ch, SKILL_CRITICAL_KNOWLEDGE) != 0 && diceroll >= 55)
            diceroll = 60;

		int chancetohit = (-2*(GET_LEVEL(victim) - GET_LEVEL(ch))) + (GET_DEX(victim) - GET_DEX(victim)) + rand_number( (-2*MIN(0,GET_INT(victim) - GET_INT(ch))),GET_HITROLL(ch));
		
		if ((diceroll <= 57) && (diceroll > 7)) {
            if ((chancetohit > 0)) { //hit
                if (chancetohit < 50)
                    dam = raw_damage - dam_absorbed;
                else if (chancetohit < 250)
                    dam = raw_damage - (dam_absorbed / 3);
                else
                    dam = raw_damage - (dam_absorbed / 6);

                if (dam  < 3) {
					miss++;
                    dam = 0;
                    diceroll = 1;
					if (!FIGHTING(victim))
						set_fighting(victim,ch);
                }
            }
            else {
				if (IS_NPC(ch) || !IS_NPC(victim)) { // Mobs need more help hitting players with crazy saves
						int randchance = rand_number(0,10);
						if (randchance > 8) {
							dam = rand_number((GET_LEVEL(ch)), GET_LEVEL(ch) * 3);
					}
				}
				else {
					miss++;
					dam = 0;
					diceroll = 1;
					if (!FIGHTING(victim))
						set_fighting(victim,ch);
				}
            }
        } 
        else {  /* Critical hit stuff */
            if ((diceroll > 56)) {	/* Critical Hit */
                crit++;
				calc_hitroll = victim_save + 10;  /* makes sure you hit */
                i = victim->points.max_hit * .5;
				if (!IS_NPC(victim)) {
	                dam = raw_damage * 1.5;  /* critical hit means ac absorbs nothing */
	                if (dam > i)
		                dam = i + (raw_damage / 5);
				}
				else
					dam = (raw_damage * 2);
            } 

            if (diceroll <= 7) {
                /* Critical Miss */
                if (affected_by_psionic(ch, PSIONIC_COMBAT_MIND) && rand_number(0,5) < 3) {
                    //Saved!
					CMBMND++;
                    calc_hitroll = victim_save + 10;  /* makes sure you hit */
                    i = victim->points.hit * .25; //not as hard as a real crit, but nice
                    dam = raw_damage * 2;  /* critical hit means ac absorbs nothing */
                    if(dam > i)
                        dam = i + raw_damage;
                    diceroll = 60; //prevent multipliers
                }
                else {
					// You critically miss here.
					miss++;
                    calc_hitroll = victim_save - 10; /* makes sure you miss */
                    dam = 0;

					if (!FIGHTING(victim))
						set_fighting(victim,ch);

					int criticalsave = rand_number(1,4);

					switch(criticalsave) {

					case 1:
						act("@RYou make a critical error in judgement, and the fight against $N takes a turn for the worse!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						break;
					case 2:
						act("@RYou overestimate your combat skills, completely missing $N@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						break;
					case 3:
						act("@RYou fall on your ass, flailing at $N!@n\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
						act("@R$n falls on $s ass, flailing at $N!@n\r\n", TRUE, ch, 0, victim, TO_NOTVICT, 3);
						act("@R$n falls on $s ass, flailing at you!@n\r\n", TRUE, ch, 0, 0, TO_VICT, 3);
						break;
					case 4:
						act("You trip over your own feet, hitting the ground hard!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
						act("$n trips over his own feet, hitting the ground hard!@n\r\n", TRUE, ch, 0, 0, TO_ROOM, 3);
						break;
					}

                    if (!IS_NPC(ch) && criticalsave >= 2) {
						// When you fall and are too heavy, you take more damage!
						if (ENCUMBRANCE(ch) < 10)
							falldam = 5 + GET_LEVEL(ch);
						if (ENCUMBRANCE(ch) >= 10 && ENCUMBRANCE(ch) < 30)
							falldam = 10 + GET_LEVEL(ch);
						if (ENCUMBRANCE(ch) >= 30 && ENCUMBRANCE(ch) < 50)
							falldam = 25 + GET_LEVEL(ch);
						if (ENCUMBRANCE(ch) >= 50 && ENCUMBRANCE(ch) < 70)
							falldam = 30 + (GET_LEVEL(ch) * 2);
						if (ENCUMBRANCE(ch) >= 70 && ENCUMBRANCE(ch) < 90)
							falldam = 50 + (GET_LEVEL(ch) * 2);
						if (ENCUMBRANCE(ch) >= 90)
							falldam = 75 + (GET_LEVEL(ch) * 2);

						GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
						GET_HIT(ch) = GET_HIT(ch) - falldam;
							
						if (GET_HIT(ch) < 0)
							GET_HIT(ch) = -1;

						j = attacks;
						break;
					}
                    
					else {
						GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
						j = attacks;
						break;
                    }
                } // END OF CRITICAL MISS (DAMAGE/LOSS OF TURN)
            } // END OF CRITICAL MISS CHECKS
        } // END OF CRITICAL CHECKS

		if ((diceroll != 60) && AFF_FLAGGED(victim, AFF_STUN))
			dam = 1.4 * dam;
		if ((type == SKILL_AMBUSH) ||
			(type == SKILL_SLASH) ||
			(type == SKILL_FLANK) || 
			(type == SKILL_CHARGE)) {
				dam = ((backstab_mult(GET_LEVEL(ch)) * dam));
			}

		if (type == SKILL_CIRCLE)
			dam = ((GET_STR(ch)+GET_DEX(ch)+GET_DAMROLL(ch)+get_dice(ch)) * GET_LEVEL(ch) / rand_number(1,6) / (attacks));

		if ((victim->char_specials.saved.duplicates > 0 && dam > 1 )) {
			if (victim->char_specials.saved.duplicates > 0 && dam > 1) {

                // Added code to randomize 'hits' on hallucinations. Rationally, we would
                // expect an opponent to correctly choose the 'real' opponent sometimes
                // and when that happens, we assume his mind is strong enough to destroy
                // the hallucinations.  I'm keeping it simple for now, but might add to it later.

				int rand_num = rand_number(1, 100);
				int dupe = victim->char_specials.saved.duplicates;
				int mob_goal = 100/(1 + dupe) + GET_LEVEL(ch) - GET_LEVEL(victim);
				int int_diff = GET_INT(victim) - GET_INT(ch);
				int pcn_diff = GET_PCN(victim) - GET_PCN(ch);
				int lvl_diff = GET_LEVEL(victim) - GET_LEVEL(ch);
				int player_goal = 100/(1 + dupe) + (int_diff + pcn_diff + lvl_diff)/4;

				if (!IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SECOND_SIGHT))
					player_goal += 15;
				if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_SECOND_SIGHT))
					mob_goal += 10;

	            circle_srandom(time(0));

	            if ((IS_NPC(ch) && rand_num < mob_goal)  || (!IS_NPC(ch) && rand_num < player_goal))
		        {

					act("You luckily hit $N instead of $S hallucinations!", FALSE, ch, 0,victim, TO_CHAR);
					act("$n luckily hits you instead of your hallucinations!", FALSE, ch, 0, victim, TO_VICT);
					dupehit = 0;
					damage(ch, victim,  dam, w_type, DMG_NORMAL);

				}
				else 
				{
                    // opponent hits hallucination - shatter a duplicate
					victim->char_specials.saved.duplicates--;
					dupehit++;
					if ((victim->char_specials.saved.duplicates == 0) && (affected_by_psionic(victim, PSIONIC_DUPLICATES)))
						affect_from_char(victim, PSIONIC_DUPLICATES, TRUE);

				}
		    }
		}

		else { // character does not have any duplicates and diceroll > 1
			GET_TOTDAM(ch) += dam;
			GET_TOTHITS(ch) += 1;
		}


        // code so a fleeing player doesnt get another hit on them
		if (IN_ROOM(ch) != IN_ROOM(victim)) {
			if (FIGHTING(ch) && FIGHTING(ch) == victim) {
				stop_fighting(ch);
				break;
			}
		}
	} // END OF ATTACK LOOP



    if (!victim || !ch)
        return;
	if (crit >= 1) {
		act("@WAdrenaline pumps through your entire body as your attack critically strikes!@n", TRUE, ch, 0, victim, TO_CHAR, 3);
		act("$n's adrenaline spikes as $s critically strikes $N!\r\n", TRUE, ch, 0, victim, TO_NOTVICT, 3);
		act("@W$n's adrenaline spikes as they critically strike you!@n\r\n", TRUE, ch, 0, 0, TO_VICT, 3);
	}
	if (CMBMND >= 1) {
		act("@YYou fake a fall to the ground in attempt to catch your victim off guard!\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
		act("$n fakes a fall to the ground in an attemp to catch $N off guard!\r\n", TRUE, ch, 0, victim, TO_NOTVICT, 3);
		act("@Y$n fakes a fall, and catches you off guard with $m swift attack!\r\n@n", TRUE, ch, 0, 0, TO_VICT, 3);
	}
	if (dupehit >= 1) {
			send_to_char(ch, "@GYou watch %d images of your target flicker out of existence!\r\n@n", dupehit);
			send_to_char(victim, "@G%s hits %d of your duplicates, they flicker out of existence!\r\n@n", GET_NAME(ch), dupehit);
	}
	else {


  /* defensive roll, avoids a lethal blow once every X minutes
   * X = about 1 minutes with current settings
   */
  if (!IS_NPC(victim) && ((GET_HIT(victim) -  GET_TOTDAM(ch)) <= 0) && (GET_SKILL_LEVEL(victim, SKILL_DEFENSE_ROLL) != 0) && !char_has_mud_event(victim, eD_ROLL)) {
    act("\tOYou time a defensive roll perfectly and avoid the attack from"
            " \tn$N\tO!\tn", FALSE, victim, NULL, ch, TO_CHAR);
    act("$n \tOtimes a defensive roll perfectly and avoids your attack!\tn",
            FALSE, victim, NULL, ch, TO_VICT | TO_SLEEP);
    act("$n times a \tOdefensive roll\tn perfectly and avoids an attack "
            "from $N!\tn", FALSE, victim, NULL, ch, TO_NOTVICT);
    attach_mud_event(new_mud_event(eD_ROLL, victim, NULL),(PULSE_IDLEPWD * 4));
   return ;
}

		damage(ch, victim, GET_TOTDAM(ch), w_type, get_damagetype(ch));
                update_pos(victim);


//here
	}

    // auto con and weapon psionic and combo updates
                 if (GET_POS(victim) > POS_DEAD) {
		if (GET_TOTDAM(ch) > 0) {
			if (type > 0 && type <= NUM_SKILLS) {
				update_combo(ch, victim, type);
			}
		}
		else {
			reset_combo(ch);
		}
		
        if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOCON) && GET_POS(ch) > POS_DEAD) {
            diag_char_to_char(victim, ch);
			send_to_char(ch, "\r\n");
		}

        // can't we just use 'wielded' here?
        obj = ch->equipment[WEAR_WIELD];

        // perform weapon psionic effect
        if (obj && obj->obj_wpnpsi.which_psionic > 0) {
			int randweapon = rand_number(1,100);
			if (randweapon >= 77) {
				which_psionic = obj->obj_wpnpsi.which_psionic;
				skill_level = obj->obj_wpnpsi.skill_level;
				wpn_psionic(ch, victim, obj, which_psionic, skill_level);
			}
        }

        //New: Ammo weapon psionics! -Lump
        if (obj && IS_GUN(obj) && IS_LOADED(obj)) {
            ammo = GET_AMMO_LOADED(obj);

            // perform ammo weapon psionic effect
            if (ammo->obj_wpnpsi.which_psionic > 0) {
                which_psionic = ammo->obj_wpnpsi.which_psionic;
                skill_level = ammo->obj_wpnpsi.skill_level;
                wpn_psionic(ch, victim, obj, which_psionic, skill_level);
            }
        }
    }

    if (behead) {
        do_behead(ch, GET_PC_NAME(victim), 0, 0);
        return;
    }

    // check if the victim has a hitprcnt trigger
    hitprcnt_mtrigger(victim);
	dupehit = 0;
	GET_TOTDAM(ch) = 0;
	GET_TOTHITS(ch) = 0;
}

int PK_OK(struct char_data *ch, struct char_data *victim)
{
    // imms can always PK ;)
    if (GET_LEVEL(ch) > LVL_IMMORT)
        return (TRUE);

    // you can PK yourself, sure, go ahead
    if (ch == victim)
        return (TRUE);

    // room is set for PK_OK
    if (PK_OK_ROOM(victim))
        return (TRUE);
    // team code: prevents accidental team-killing
    if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(victim))
        return (FALSE);
    // if both are set to PLR_PK, or the victim is set to KILLER, and they aren't in a NO_PK room...
    if (((PLR_FLAGGED(victim, PLR_PK) && PLR_FLAGGED(ch, PLR_PK)) || PLR_FLAGGED(victim, PLR_KILLER)) && !PK_NOT_OK_ROOM(victim))
        return (TRUE);

    return (FALSE);
}

// todo: clean up this function
void activate_chip(struct char_data *ch)
{

    int mod_ac;
    int mod_hnd;
    int mod_hit;
    int mod_psi;
    int mod_regen;
    int i;
    struct obj_data *chip = NULL;

    if (!(chip = GET_EQ(ch, WEAR_EXPANSION)))
        return;

    if (GET_OBJ_TYPE(chip) != ITEM_EXPANSION)
        return;

    for (i = 0; i < 6; i++) {

        float tmpFloat = (float)GET_OBJ_VAL(chip, (i))/100;

        switch (i) {

        case 0: //regen
            mod_regen = (float)ch->player_specials->saved.char_all_regen * tmpFloat;

            //right now, restricting to a single instance, no stacking
            if (GET_REGEN_MOD(ch) == 0) {
                GET_REGEN_MOD(ch) = mod_regen;
                ch->player_specials->saved.char_all_regen += mod_regen;
            }
            break;

        case 1: //ac
            mod_ac = (float)GET_AC(ch) * tmpFloat;

            if (GET_AC_MOD(ch) == 0) {
                GET_AC_MOD(ch) = mod_ac;
                GET_AC(ch) += mod_ac;
            }
            break;

        case 2: //hnd
            mod_hnd = (float)GET_DAMROLL(ch) * tmpFloat;

            if (GET_HND_MOD(ch) == 0) {
                GET_HND_MOD(ch) = mod_hnd;
                GET_DAMROLL(ch) += mod_hnd;
                GET_HITROLL(ch) += mod_hnd;
            }
            break;

        case 3: //hitpts
            mod_hit = (float)GET_MAX_HIT_PTS(ch) * tmpFloat;

            if (GET_HIT_MOD(ch) == 0) {
                GET_HIT_MOD(ch) = mod_hit;
                GET_MAX_HIT_PTS(ch) += mod_hit;
            }
            break;

        case 4: //psi
            mod_psi = (float)GET_MAX_PSI_PTS(ch) * tmpFloat;

            if (GET_PSI_MOD(ch) == 0) {
                GET_PSI_MOD(ch) = mod_psi;
                GET_MAX_PSI_PTS(ch) += mod_psi;
            }
            break;

        case 5: //unused
        default:
            break;

        }
    }
}

void deactivate_chip(struct char_data *ch)
{

    if (GET_PSI_MOD(ch) != 0) {
        GET_MAX_PSI_PTS(ch) -= GET_PSI_MOD(ch);
        GET_PSI_MOD(ch) = 0;
    }

    if (GET_HIT_MOD(ch) != 0) {
        GET_MAX_HIT_PTS(ch) -= GET_HIT_MOD(ch);
        GET_HIT_MOD(ch) = 0;
    }

    if (GET_HND_MOD(ch) != 0) {
        GET_DAMROLL(ch) -= GET_HND_MOD(ch);
        GET_HITROLL(ch) -= GET_HND_MOD(ch);
        GET_HND_MOD(ch) = 0;
    }

    if (GET_AC_MOD(ch) != 0) {
        GET_AC(ch) -= GET_AC_MOD(ch);
        GET_AC_MOD(ch) = 0;
    }

    if (GET_REGEN_MOD(ch) != 0) {
        ch->player_specials->saved.char_all_regen -= GET_REGEN_MOD(ch);
        GET_REGEN_MOD(ch) = 0;
    }
}

int get_dice(struct char_data *ch)
{
   int dam = 0;
   struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
	
	if (!wielded) 
	{
            if (IS_NPC(ch))
			{
                return dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
			}
    }
    else 
	{
        // melee weapon
        if (!IS_GUN(wielded))
		{
            return dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]) + dam;
		}
        else 
		{
                // it is an unloaded gun
            if (!IS_NPC(ch) && !IS_LOADED(wielded))
			{
                return dice(GET_GUN_BASH_NUM(wielded), GET_GUN_BASH_SIZE(wielded)) + dam;
			}
            else // it is a loaded gun
			{
                return dice(wielded->obj_flags.value[1], wielded->obj_flags.value[2]) + dam;
			}
        }
	}
	
	return 0;
}
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
	char *echo_room_msg)
{
	struct delay_hit *delay_hit;
	CREATE(delay_hit, struct delay_hit, 1);
	
	struct msg_type hit_msg[1];
	struct msg_type af_msg[1];
	struct msg_type echo_msg[1];
	
		
	hit_msg[0].attacker_msg = hit_attacker_msg;
	hit_msg[0].victim_msg = hit_victim_msg;
	hit_msg[0].room_msg = hit_room_msg;
	af_msg[0].attacker_msg = af_attacker_msg;
	af_msg[0].victim_msg = af_victim_msg;
	af_msg[0].room_msg = af_room_msg;
	echo_msg[0].attacker_msg = echo_attacker_msg;
	echo_msg[0].victim_msg = echo_victim_msg;
	echo_msg[0].room_msg = echo_room_msg;
	
		
	delay_hit[0].ch = ch;
	delay_hit[0].victim = victim_name;
	delay_hit[0].hit_msg = hit_msg[0];
	delay_hit[0].af_msg = af_msg[0];
	delay_hit[0].echo_msg = echo_msg[0];
	delay_hit[0].psi_num = psi_num;
	delay_hit[0].psi_level = psi_level;
	delay_hit[0].psi_ack = psi_ack;
	delay_hit[0].dam = damage;
	delay_hit[0].dam_type = damage_type;
	
		
	GET_DELAY_HIT(ch) = delay_hit;
	GET_DELAY_HIT_WAIT(ch) = wait_time;

	if(GET_WAIT_STATE(ch) < wait_time)
	{
		GET_WAIT_STATE(ch) = wait_time;
	}
    if (!ch)
        return;
}
void end_rangedcombat(struct char_data *ch)
{
    struct char_data *ranged;
	struct char_data *temp;

	for (ranged = ranged_combat_list; ranged; ranged = next_ranged_combat_list) {
		next_ranged_combat_list = ranged->next_ranged_fighting;

		if (RANGED_FIGHTING(ranged) == ch) {
			REMOVE_FROM_LIST(ranged, ranged_combat_list, next_ranged_fighting);
			ranged->next_ranged_fighting = NULL;
			RANGED_FIGHTING(ranged) = NULL;
			GET_POS(ranged) = POS_STANDING;
			update_pos(ranged);
			RANGED_COMBAT_TICKS(ranged) = 0;
			send_to_char(ranged, "@RYour victim has escaped!@n\r\n");
		}
		if (ranged->char_specials.targeted != NULL)
			ranged->char_specials.targeted = NULL;
		if (LOCKED(ch) != NULL)
			LOCKED(ch) = NULL;
	}
}
void execute_combat(void)
{
	struct char_data *ranged;
	struct char_data *tmp;
    struct char_data *ch;
    struct char_data *victim;
	struct char_data *comboch;
    struct affected_type af;
	struct obj_data *wielded;
	
	for (ranged = ranged_combat_list; ranged; ranged = next_ranged_combat_list) {
		next_ranged_combat_list = ranged->next_ranged_fighting;

		RANGED_COMBAT_TICKS(ranged)++;

		if (RANGED_FIGHTING(ranged) == NULL) {
			stop_ranged_fighting(ranged);
			RANGED_COMBAT_TICKS(ranged) = 0;
			continue;
		}

		victim = RANGED_FIGHTING(ranged);

		// Can't have ranged combat with someone in the same room.
		if (IS_NPC(victim)) {
			if (IN_ROOM(ranged) == IN_ROOM(victim)) {
				do_say(victim, "Gotcha!", 0, 0);
				stop_ranged_fighting(ranged);
				int thaco = 15;
				hit(victim, ranged, TYPE_UNDEFINED, thaco, 0);
				continue;
			}
		}
		if (!IS_NPC(ranged)) {
			if (IN_ROOM(ranged) == IN_ROOM(RANGED_FIGHTING(ranged))) {
				stop_ranged_fighting(ranged);
				stop_ranged_fighting(RANGED_FIGHTING(ranged));
				continue;
			}
		}

		// Can't have ranged combat with someone, if you're being attacked by another mob.

		if (FIGHTING(ranged)) {
			stop_ranged_fighting(ranged);
			RANGED_COMBAT_TICKS(ranged) = 0;
			continue;
		}
		wielded = GET_EQ(ranged, WEAR_WIELD);

		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_GUN) {
			if (!(get_lineofsight(ranged, victim, NULL, NULL))) {
				send_to_char(ranged, "\r\n@RYour victim is no longer within your line of sight, deactivating targetting systems.@n\r\n");
				end_rangedcombat(victim);
				if (RANGED_FIGHTING(ranged))
					stop_ranged_fighting(ranged);
				continue;
			}
		}
		// If an NPC gets involved in long ranged combat, we must make sure that
		// A) The mob is wielding a Gun.
		// B) If not then it check if it has memory, remember the player.
		// C) Check if it hunts, to hunt the player.
		// D) Otherwise, get the F out of there!

		if (IS_NPC(victim)) {

			if (!RANGED_FIGHTING(victim)) {

				if (MOB_FLAGGED(victim, MOB_MEMORY)) {
					FUN_HUNTING(victim) = ranged;
					remember(ranged, victim);
				}

				if (MOB_FLAGGED(victim, MOB_HUNTER)) {
					FUN_HUNTING(victim) = ranged;
					HUNTING(victim) = ranged;
				}
				if (MOB_FLAGGED(victim, MOB_SENTINEL)) {
					FUN_HUNTING(victim) = ranged;
					remember(ranged, victim);
				}
				else {
					int roll = rand_number(1,100);
					if (roll > 50)
						do_flee(victim, "", 0, 0);
				}
			}
		}



		if (!IS_NPC(ranged)) {
			wielded = GET_EQ(ranged, WEAR_WIELD);
			if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_GUN) {
				send_to_char(ranged, "You need to be wielding a ranged weapon in order to engage in ranged combat.\r\n");
				stop_ranged_fighting(ranged);
			}
		}

		if (GET_POS(ranged) == POS_SLEEPING) {
			send_to_char(ranged, "Hey you're still sleeping!\r\n");
			continue;
		}

        if (IS_NPC(ranged)) {

            if (GET_WAIT_STATE(ranged) > 0) {
                GET_WAIT_STATE(ranged) -= PULSE_VIOLENCE;
                continue;
            }

            GET_WAIT_STATE(ranged) = 0;

            if (GET_POS(ranged) < POS_FIGHTING && GET_POS(ranged) > POS_DEAD) {
                GET_POS(ranged) = POS_FIGHTING;
                act("$n scrambles to $s feet!", TRUE, ranged, 0, 0, TO_ROOM);
            }
        }

        /* PC auto-stand */
        if (!IS_NPC(ranged)) {
            if ((GET_WAIT_STATE(ranged) == 0) && (GET_POS(ranged) < POS_FIGHTING && GET_POS(ranged) > POS_DEAD && GET_POS(ranged) != POS_SLEEPING)) {
                GET_POS(ranged) = POS_FIGHTING;
                act("$n scrambles to $s feet!", TRUE, ranged, 0, 0, TO_ROOM, 3);
                send_to_char(ranged, "You scramble to your feet!\r\n");
            }
        }

        if (GET_POS(ranged) < POS_FIGHTING) {
            continue;
        }

		if (RANGED_COMBAT_TICKS(ranged) >= TICK_TO_HIT(ranged) && GET_POS(victim) > POS_DEAD) {
			
			int chance = rand_number(0,100);
			int thaco = rand_number(1,30);

			if (AFF_FLAGGED(ranged, AFF_BLIND) && chance <= 60) {
				act("$n stumbles around blindly, completely missing $N!", TRUE, ranged, 0, victim, TO_NOTVICT, 3);
				act("$n stumbles around blindly, completely missing you!", TRUE, ranged, 0, victim, TO_VICT, 3);
				act("You stumble around blindly, completely missing $N!", TRUE, ranged, 0, victim, TO_CHAR, 3);

				RANGED_COMBAT_TICKS(ranged) = 0;
				TICK_TO_HIT(ranged) = 16;
			}

			else if (AFF_FLAGGED(ranged, AFF_STUN)) {
				act("$n is too dazed to get $s attack off!", TRUE, ranged, 0, victim, TO_NOTVICT, 3);
				act("$n seems dazed and confused!", TRUE, ranged, 0, victim, TO_VICT, 3);
				act("You can't get your attack off because you are stunned!", TRUE, ranged, 0, victim, TO_CHAR, 3);

				RANGED_COMBAT_TICKS(ranged) = 0;
				TICK_TO_HIT(ranged) = 30;
			}
			
			else {
					
				if (thaco <= 4)
					act("Your inconsistent breathing throws your aim off.\r\n", TRUE, ranged, 0, victim, TO_CHAR, 3);
				if (thaco > 5 && thaco < 10)
					act("You fire your weapon too quickly, trying to gain an advantage.\r\n", TRUE, ranged, 0, victim, TO_CHAR, 3);
				if (thaco >= 10 && thaco < 20)
					act("You patiently aim at your target, attempting for a body shot.\r\n", TRUE, ranged, 0, victim, TO_CHAR, 3);
				if (thaco >= 20 && thaco < 25)
					act("You confidently aim at your target, trying to strike at their vitals.\r\n", TRUE, ranged, 0, victim, TO_CHAR, 3);
				if (thaco >= 25)
					act("@YYour eyes go red momentarily as you lock onto your opponent perfectly!@n\r\n", TRUE, ranged, 0, victim, TO_CHAR, 3);

				RANGED_COMBAT_TICKS(ranged) = 0;
				rangedhit(ranged, victim, TYPE_UNDEFINED, thaco);	
				update_pos(victim);
			}
		}
	}

	for (tmp = character_list; tmp; tmp = tmp->next){
		if (AFF_FLAGGED(tmp, AFF_FLASH))
			fun_hunt(tmp);
	}

    for (ch = combat_list; ch; ch = next_combat_list) {
        next_combat_list = ch->next_fighting;

		COMBAT_TICKS(ch)++;
		TRIGGER_TICKS(ch)++;

        if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
			COMBAT_TICKS(ch) = 0;
			TRIGGER_TICKS(ch) = 0;
            stop_fighting(ch);
            continue;
        }

		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char(ch, "Hey you're still sleeping!\r\n");

        if (IS_NPC(ch)) {

            if (GET_WAIT_STATE(ch) > 0) {
                GET_WAIT_STATE(ch) -= PULSE_VIOLENCE;
                continue;
            }

            GET_WAIT_STATE(ch) = 0;

            if (GET_POS(ch) < POS_FIGHTING) {
                GET_POS(ch) = POS_FIGHTING;
                act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
            }
        }

        /* PC auto-stand */
        if (!IS_NPC(ch)) {
            if ((GET_WAIT_STATE(ch) == 0) && (GET_POS(ch) < POS_FIGHTING && GET_POS(ch) != POS_SLEEPING)) {
                GET_POS(ch) = POS_FIGHTING;
                act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM, 3);
                send_to_char(ch, "You scramble to your feet!\r\n");
            }
        }

        if (GET_POS(ch) < POS_FIGHTING) {
            continue;
        }

        //See if an attacked mob switches to the master
        victim = FIGHTING(ch);
		
		if (CMB_FLAGGED(ch, CMB_COMBO)) {
			REMOVE_BIT_AR(CMB_FLAGS(ch), CMB_COMBO);
			for (comboch = world[IN_ROOM(ch)].people; comboch; comboch = comboch->next_in_room) {
				TICK_TO_HIT(comboch) = 10;
				COMBAT_TICKS(comboch) = 0;
			}
			send_to_room(IN_ROOM(ch), "\r\nA strange energy begins to swirl in the room...\r\n");
			SET_BIT_AR(CMB_FLAGS(ch), CMB_NEXTCOMBO);
		}
		
		if (CMB_FLAGGED(victim, CMB_COMBO) || CMB_FLAGGED(victim, CMB_NEXTCOMBO))
			continue;	
			
		if (CMB_FLAGGED(ch, CMB_NEXTCOMBO) && COMBAT_TICKS(ch) > TICK_TO_HIT(ch)) {
			execute_combo(ch,victim);
			COMBAT_TICKS(ch) = 0;
			COMBAT_TICKS(victim) = 0;
			REMOVE_BIT_AR(CMB_FLAGS(ch), CMB_NEXTCOMBO);
		}
		else if (CMB_FLAGGED(ch, CMB_NEXTCOMBO) && COMBAT_TICKS(ch) <= TICK_TO_HIT(ch)) {
			continue;
		}
		
		if (GET_CIRCLETICKS(ch) > 0) {
			int circledam = ((GET_STR(ch)+GET_DEX(ch)+GET_DAMROLL(ch)+get_dice(ch)) * GET_LEVEL(ch) / rand_number(1,6) / (ATTACKS_PER_ROUND(ch)));
			GET_CIRCLETICKS(ch)++;
			
			if (GET_CIRCLETICKS(ch) == 8) {
				act("$n drives $s weapon into $N causing considerable damage!", FALSE, ch, 0, victim, TO_ROOM, 3);
				act("Seizing an opening, you close the distance and drive your weapon into $N!", FALSE, ch, 0, victim, TO_CHAR);
				act("You scream in pain as $n circles around driving $S weapon into you!", FALSE, ch, 0, victim, TO_VICT);
				if (rand_number(1,20) > 10) {
					af.type = PSIONIC_BLEED;
					af.duration = .2;
					SET_BIT_AR(af.bitvector, AFF_BLEEDING);

					act("@R$N staggers and begins to pour blood on the ground!@n", FALSE, ch, 0, victim, TO_ROOM, 3);
					act("@R$N staggers back as you strike and begins to pour blood on the ground!@n", FALSE, ch, 0, victim, TO_CHAR);
					act("@RYou stagger back and begin to bleed quickly on the ground!@n", FALSE, ch, 0, victim, TO_VICT);
					update_pos(victim);
				}
				else {
					damage(ch, victim, circledam, SKILL_CIRCLE, DMG_NORMAL);
					update_pos(victim);
				}
			}
			else if (GET_CIRCLETICKS(ch) == 17) {
				act("$n circles around $N again, trying to disable $M!", FALSE, ch, 0, victim, TO_ROOM, 3);
				act("You circle around $N again, trying to disable $M!", FALSE, ch, 0, victim, TO_CHAR);
				act("$n circles around you again, trying to disable you!", FALSE, ch, 0, victim, TO_VICT);
				if (rand_number(1,20) > 10) {
					af.type = PSIONIC_PARALYZE;
					af.duration = .2;
					SET_BIT_AR(af.bitvector, AFF_STUN);

					act("@R$n's strike takes $N to the ground!!@n", FALSE, ch, 0, victim, TO_ROOM);
					act("@RYou circle around again, and take $N to the ground with your strike!!@n", FALSE, ch, 0, victim, TO_CHAR);
					act("@R$n circles around you again, $S strike takes you to the ground!!@n", FALSE, ch, 0, victim, TO_VICT);
					update_pos(victim);
				}
				else {
					damage(ch, victim, circledam, SKILL_CIRCLE, DMG_NORMAL);
					update_pos(victim);
				}
			}
			else if (GET_CIRCLETICKS(ch) == 25) {
				act("$n circles around, and drives $s weapon into $N!", FALSE, ch, 0, victim, TO_ROOM, 3);
				act("You circle around, and drive your weapon into $N!", FALSE, ch, 0, victim, TO_CHAR, 0);
				act("$n circles around, and drives $s weapon into you!", FALSE, ch, 0, victim, TO_VICT, 0);
				damage(ch, victim, circledam, SKILL_CIRCLE, DMG_NORMAL);
				update_pos(victim);
				GET_CIRCLETICKS(ch) = 0;
			}
		}


        if (IS_NPC(ch) && IS_NPC(victim) && victim->master && AFF_FLAGGED(victim, AFF_CHARM) && (IN_ROOM(victim->master) == IN_ROOM(ch))) {

			if (COMBAT_TICKS(ch) >= TICK_TO_HIT(ch)) {
				int rnd = rand_number(1,10);
				if (rnd >= 7) {
					act("$n turns around from $s target and jumps at you!", FALSE, ch, 0, 0, TO_VICT, 3);
					int thaco = rand_number(1,30);
					hit(ch, victim->master, TYPE_UNDEFINED, thaco, 0);
					COMBAT_TICKS(ch) = 0;
					return;
				}
			}
        }

        //PROTECT See if an attacked mob switches to the guy protecting the victim
        victim = FIGHTING(ch);
		if(victim) {
        if (IS_NPC(ch) && !IS_NPC(victim) && victim->char_specials.protector && (IN_ROOM(victim->char_specials.protector) == IN_ROOM(ch)) && GET_POS(victim->char_specials.protector) >= POS_FIGHTING) {
			if (FIGHTING(ch) && FIGHTING(ch) == victim) {
				FIGHTING(ch) = victim->char_specials.protector;
//		        stop_fighting(ch);
//			    set_fighting(ch, victim->char_specials.protector);
            }
			if (COMBAT_TICKS(ch) >= TICK_TO_HIT(ch)) {
				act("$n steps in to save $N's ass!!", FALSE, victim->char_specials.protector, 0, victim, TO_NOTVICT, 3);
				act("$n steps in to save your ass!!", FALSE, victim->char_specials.protector, 0, victim, TO_VICT, 3);
				act("You step in and save $N's ass!!", FALSE, victim->char_specials.protector, 0, victim, TO_CHAR, 0);
				int thaco = rand_number(1,30);
				hit(ch, victim->char_specials.protector, TYPE_UNDEFINED, thaco, 0);
				COMBAT_TICKS(ch) = 0;
				return;
			}
		}
		}
		int randomchance = rand_number(1,100);
		
		if (randomchance < 20) {
			COMBAT_TICKS(ch)++;
			TRIGGER_TICKS(ch)--;
		}
		if (randomchance > 80) {
			COMBAT_TICKS(ch)--;
			TRIGGER_TICKS(ch)++;
		}

		if (COMBAT_TICKS(ch) >= TICK_TO_HIT(ch) && GET_POS(victim) > POS_DEAD && GET_CIRCLETICKS(ch) < 1) {
			
			int chance = rand_number(0,100);
			int thaco = rand_number(1,30);

			if ((AFF_FLAGGED(victim, AFF_DODGE) && chance > 90) || (HAS_BIONIC(victim, BIO_FOOTJET) && chance > 90)) {
				act("$n attacks, but $N easily gets out of the way.", FALSE, ch, 0, victim, TO_NOTVICT, 3);
				act("$n barely misses you as you dodge the attack.", FALSE, ch, 0, victim, TO_VICT, 3);
				act("$N narrowly gets out of the way of your attack.", FALSE, ch, 0, victim, TO_CHAR, 3);
			
				if (!FIGHTING(victim))
					set_fighting(victim,ch);

				COMBAT_TICKS(ch) = 0;
				TICK_TO_HIT(ch) = 10;
			}

			else if (AFF_FLAGGED(victim, AFF_BODY_BLOCK && chance > 92)) {
				act("$N can't get $S attack off!", TRUE, ch, 0, victim, TO_NOTVICT, 3);
				act("$N seems to be blocked by you!", TRUE, ch, 0, victim, TO_CHAR, 3);
				act("You can't get your attack off because $n has blocked you!", TRUE, ch, 0, victim, TO_VICT, 3);
				REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_BODY_BLOCK);
				
				if (!FIGHTING(victim))
					set_fighting(victim,ch);

				COMBAT_TICKS(ch) = 0;
				TICK_TO_HIT(ch) = 12;
			}
			
			else if (AFF_FLAGGED(ch, AFF_BLIND) && chance <= 60) {
				act("$n stumbles around blindly, completely missing $N!", TRUE, ch, 0, victim, TO_NOTVICT, 3);
				act("$n stumbles around blindly, completely missing you!", TRUE, ch, 0, victim, TO_VICT, 3);
				act("You stumble around blindly, completely missing $N!", TRUE, ch, 0, victim, TO_CHAR, 3);

				COMBAT_TICKS(ch) = 0;
				TICK_TO_HIT(ch) = 16;
			}

			else if (AFF_FLAGGED(ch, AFF_STUN)) {
				act("$n is too dazed to get $s attack off!", TRUE, ch, 0, victim, TO_NOTVICT, 3);
				act("$n seems dazed and confused!", TRUE, ch, 0, victim, TO_VICT, 3);
				act("You can't get your attack off because you are stunned!", TRUE, ch, 0, victim, TO_CHAR, 3);

				COMBAT_TICKS(ch) = 0;
				TICK_TO_HIT(ch) = 30;
			}
			
			else if (AFF_FLAGGED(ch, AFF_COVER_FIRE)) {
				COMBAT_TICKS(ch) = 0;
				TICK_TO_HIT(ch) = rand_number(10,20);
				REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_COVER_FIRE);
			}
			else {
					
				if (thaco <= 4)
					act("You stumble all over as you quickly try to get your attack off.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
				if (thaco > 5 && thaco < 10)
					act("You attack wildly, attempting to land a lucky hit.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
				if (thaco >= 10 && thaco < 20)
					act("You aim at your target patiently trying to strike at their vitals.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
				if (thaco >= 20 && thaco < 25)
					act("You confidently aim squarely with your exceptional skill.\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);
				if (thaco >= 25)
					act("Your eyes go red momentarily as you lock onto your opponent perfectly!\r\n", TRUE, ch, 0, victim, TO_CHAR, 3);

				hit(ch, FIGHTING(ch), TYPE_UNDEFINED, thaco, 0);
				update_pos(FIGHTING(ch));
				COMBAT_TICKS(ch) = 0;	
			}
		}
		
		if (TRIGGER_TICKS(ch) >= 35) {
			if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
				(mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
			TRIGGER_TICKS(ch) = 0;
			fight_mtrigger(ch);
		}
	}
}
void wpn_psionic(struct char_data *ch, struct char_data *victim, struct obj_data *obj, int which_psionic, int skill_level)
{

    if (!ch || !victim)
        return;

    switch(which_psionic) {
    case 1:
        call_psionic(ch, victim, obj, PSIONIC_BLINDNESS, skill_level, 0, TRUE);
        break;
    case 2:
        call_psionic(ch, victim, obj, PSIONIC_PETRIFY, skill_level, 0, TRUE);
        break;
    case 3:
        call_psionic(ch, victim, obj, PSIONIC_COUNTER_SANCT, skill_level, 0, TRUE);
        break;
    case 4:
        call_psionic(ch, victim, obj, PSIONIC_PACIFY, skill_level, 0, TRUE);
        break;
    case 5:
        call_psionic(ch, victim, obj, PSIONIC_PSI_SHACKLE, skill_level, 0, TRUE);
        break;
    case 6:
        call_psionic(ch, victim, obj, PSIONIC_POISON, skill_level, 0, TRUE);
        break;
    case 7:
        call_psionic(ch, victim, obj, PSIONIC_PSYCHIC_LEECH, skill_level, 0, TRUE);
        break;
    case 8:
        call_psionic(ch, victim, obj, PSIONIC_LETHARGY, skill_level, 0, TRUE);
        break;
    case 9:
        call_psionic(ch, victim, obj, PSIONIC_PSIONIC_CHANNEL, skill_level, 0, TRUE);
        break;
    case 10:
        call_psionic(ch, victim, obj, PSIONIC_FRAG_GRENADE, skill_level, 0, TRUE);
        break;
    case 11:
        call_psionic(ch, victim, obj, PSIONIC_PLASMA_GRENADE, skill_level, 0, TRUE);
        break;
    case 12:
        call_psionic(ch, victim, obj, PSIONIC_THERM_GRENADE, skill_level, 0, TRUE);
        break;
    case 13:
        call_psionic(ch, victim, obj, PSIONIC_FLASH_GRENADE, skill_level, 0, TRUE);
        break;
    case 14:
        call_psionic(ch, victim, obj, PSIONIC_ULTRA_STUN, skill_level, 0, TRUE);
        break;
    case 15:
        call_psionic(ch, victim, obj, PSIONIC_GAS_GRENADE, skill_level, 0, TRUE);
        break;
    case 16:
        call_psionic(ch, victim, obj, PSIONIC_NOVA, skill_level, 0, TRUE);
        break;
    case 17:
        call_psionic(ch, victim, obj, PSIONIC_MEGA_COUNTER, skill_level, 0, TRUE);
        break;
    case 18:
        call_psionic(ch, victim, obj, PSIONIC_DEGRADE, skill_level, 0, TRUE);
        break;
    case 19:
        call_psionic(ch, victim, obj, PSIONIC_MEGA_DEGRADE, skill_level, 0, TRUE);
        break;
    case 20:
        call_psionic(ch, victim, obj, PSIONIC_PSI_HAMMER, skill_level, 0, TRUE);
        break;
    case 21:
        call_psionic(ch, victim, obj, PSIONIC_ELECTROKINETIC_DISCHARGE, skill_level, 0, TRUE);
        break;
    case 22:
        call_psionic(ch, victim, obj, PSIONIC_SNUFF_LIFEFORCE, skill_level, 0, TRUE);
        break;
    case 23:
        call_psionic(ch, victim, obj, PSIONIC_PYROKINETIC_ERUPTION, skill_level, 0, TRUE);
        break;
    case 24:
        call_psionic(ch, victim, obj, PSIONIC_TRANSMUTE, skill_level, 0, TRUE);
        break;
    case 25:
        call_psionic(ch, victim, obj, PSIONIC_DISRUPTION, skill_level, 0, TRUE);
        break;
    case 26:
        call_psionic(ch, victim, obj, PSIONIC_FIRE, skill_level, 0, TRUE);
        break;
    case 27:
        call_psionic(ch, victim, obj, PSIONIC_SMOKE, skill_level, 0, TRUE);
        break;
    case 28:
        call_psionic(ch, victim, obj, PSIONIC_LAG, skill_level, 0, TRUE);
        break;
    case 29:
        call_psionic(ch, victim, obj, PSIONIC_FIRE_BLADE, skill_level, 0, TRUE);
	call_psionic(ch, victim, obj, PSIONIC_FIRE, skill_level, 0, TRUE);
        break;
    case 30:
        call_psionic(ch, victim, obj, PSIONIC_ICE_BLADE, skill_level / 4, 0, TRUE);
	call_psionic(ch, victim, obj, PSIONIC_LETHARGY, skill_level / 4, 0, TRUE);
        break;
    default:
        break;
    }
}

void update_combo(struct char_data *ch, struct char_data *victim, int skill)
{
	int i,j,n=MAX_COMBOLENGTH;
	int x[5];
	
	if (n == MAX_COMBOLENGTH) {
		for (i=1;i<MAX_COMBOLENGTH;i++)
			x[i-1] = COMBOCOUNTER(ch, (i));
		for (j=0;j<(MAX_COMBOLENGTH-1);j++)
			COMBOCOUNTER(ch, j) = x[j];
		COMBOCOUNTER(ch, (MAX_COMBOLENGTH -1)) = skill;
	}

	execute_combo(ch, victim);			
}

static void execute_combo(struct char_data *ch, struct char_data *victim)
{
	int a,i,j,n,x,y,k;
	int z = -1;
	int share;
	int combolookup[MAX_COMBOLENGTH];
	bool combolanded = FALSE;
	bool found = FALSE;
	
	k = (MAX_COMBOLENGTH -1);
	
	for (i = 0; i < MAX_COMBOLENGTH;i++) {
		combolookup[i] = COMBOCOUNTER(ch, k);
		k--;
	}
	
	for (x=0;x < MAX_COMBOS;x++) {
		n = 0;
		for (j=0;j<MAX_COMBOLENGTH;j++)
			if (combolookup[j] == combo[x].combo[j] || combo[x].combo[j] == 0)
				n++;
		
		if (n == 5 && (!CMB_FLAGGED(ch, CMB_COMBO) && !CMB_FLAGGED(ch, CMB_NEXTCOMBO))) {
			SET_BIT_AR(CMB_FLAGS(ch), CMB_COMBO);
			break;
		}
		
		if (n == 5 && CMB_FLAGGED(ch, CMB_NEXTCOMBO)) {
			int combodmg = (GET_LEVEL(ch) * GET_DAMROLL(ch));
			combodmg -= (rand_number(0,(GET_LEVEL(ch)*(GET_DAMROLL(ch)/2))));
			if (!IS_NPC(ch)) {
				// damage message to character
				if (ch && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTODAM) && GET_POS(ch) >= POS_DEAD)
					send_to_char(ch, "@G[%d] ", combodmg);
				act(combo[x].chsend, TRUE, ch, 0, victim, TO_CHAR);
			}
			if (!IS_NPC(victim)) {
				// damage message to victim
				if (victim && !IS_NPC(victim) && PRF_FLAGGED(victim, PRF_AUTODAM) && GET_POS(victim) >= POS_DEAD)
					send_to_char(ch, "@G[%d] ", combodmg);
				act(combo[x].victsend, TRUE, ch, 0, victim, TO_VICT);
			}
			act(combo[x].roomsend, TRUE, ch, 0, victim, TO_NOTVICT);
			damage(ch, victim, combodmg, TYPE_UNDEFINED, DMG_NORMAL);
			update_pos(victim);
			combolanded = TRUE;
			z = x;
			reset_combo(ch);
			REMOVE_BIT_AR(CMB_FLAGS(ch), CMB_NEXTCOMBO);
		}
	}
	if (combolanded == TRUE && z != -1) {
		for (a=0;a<MAX_COMBOLEARNED;a++)
			if (GET_COMBO_LEARNED(ch, a) == combo[z].id)
				found = TRUE;
		if (found == FALSE) {
			for (y=0;y<MAX_COMBOLEARNED;y++) {
				if (!GET_COMBO_LEARNED(ch, y)) {
					SET_COMBO_LEARNED(ch, y, combo[z].id);
					share = ((GET_LEVEL(ch) * GET_LEVEL(ch)) * 300);
					send_to_char(ch, "\r\n@M--------------------------------@n\r\n");
					send_to_char(ch, "@M--- @YYou learned a new combo! @M---@n\r\n");
					send_to_char(ch, "@M--------------------------------@n\r\n\r\n");
					send_to_char(ch, "@GYou gain %d experience!\r\n", share);
					gain_exp(ch, share, FALSE);
					break;

				}
			}
		}
	}
}
void reset_combo(struct char_data *ch)
{
	int i;
	for (i=0;i<MAX_COMBOLENGTH;i++)
		COMBOCOUNTER(ch, i) = 0;
}
