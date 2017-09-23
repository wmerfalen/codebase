/**************************************************************************
*  File: psionics.parser.c                                 Part of tbaMUD *
*  Usage: Top-level psionic routines;                                     *
*    outside points of entry to psionic sys.                              *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __PSIONIC_PARSER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "psionics.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"  /* for hit() */
#include "skilltree.h"

#define PINFO psionics_info[psi_num]

// Global Variables definitions, used elsewhere
struct psionics_info_type psionics_info[TOP_PSIONIC_DEFINE + 1];
struct skills_info_type skills_info[TOP_SKILL_DEFINE + 1];
char cast_arg2[MAX_INPUT_LENGTH];
const char *unused_psiname = "!UNUSED!"; /* So we can get &unused_psiname */
const char *unused_skillname = "!UNUSED!"; /* So we can get &unused_skillname */

// Local (File Scope) Function Prototypes
static void assign_psionic(int psi_num, const char *name, int level, int psi, int minpos, int targets, int violent, int routines, const char *wearoff, const char *rwearoff, int prac_cost, int tree, int prereq, int prereqid, const char *display, int pcclass, int tier);

// This function should be used anytime you are not 100% sure that you have
// a valid psionic number.  A typical for () loop would not need to use
// this because you can guarantee > 0 and <= TOP_PSIONIC_DEFINE
const char *psionic_name(int num)
{
    if (num > 0 && num <= TOP_PSIONIC_DEFINE)
        return (psionics_info[num].name);
    else if (num == -1)
        return ("UNUSED");
    else
        return ("UNDEFINED");
}

const char *skill_name(int num)
{
    if (num > 0 && num <= TOP_SKILL_DEFINE)
        return (skills_info[num].name);
    else if (num == -1)
        return ("UNUSED");
    else
        return ("UNDEFINED");
}

int find_psionic_num(char *name)
{
    int index;
    int ok;
    char *temp;
    char *temp2;
    char first[256];
    char first2[256];
    char tempbuf[256];

    for (index = 1; index <= TOP_PSIONIC_DEFINE; index++) {

        if (is_abbrev(name, psionics_info[index].name))
            return (index);

        ok = TRUE;
        strlcpy(tempbuf, psionics_info[index].name, sizeof(tempbuf));    /* strlcpy: OK */
        temp = any_one_arg(tempbuf, first);
        temp2 = any_one_arg(name, first2);

        while (*first && *first2 && ok) {

            if (!is_abbrev(first2, first))
                ok = FALSE;

            temp = any_one_arg(temp, first);
            temp2 = any_one_arg(temp2, first2);
        }

        if (ok && !*first2)
            return (index);
    }

    return (-1);
}

int find_skill_num(char *name)
{
    int skindex;
    int ok;
    char *temp;
    char *temp2;
    char first[256];
    char first2[256];
    char tempbuf[256];

    for (skindex = 1; skindex <= TOP_SKILL_DEFINE; skindex++) {

        if (is_abbrev(name, skills_info[skindex].name))
            return (skindex);

        ok = TRUE;
        strlcpy(tempbuf, skills_info[skindex].name, sizeof(tempbuf));    /* strlcpy: OK */
        temp = any_one_arg(tempbuf, first);
        temp2 = any_one_arg(name, first2);

        while (*first && *first2 && ok) {

            if (!is_abbrev(first2, first))
                ok = FALSE;

            temp = any_one_arg(temp, first);
            temp2 = any_one_arg(temp2, first2);
        }

        if (ok && !*first2)
            return (skindex);
    }

    return (-1);
}

// This function is the very heart of the entire psionic system.  All invocations
// of all types of psionics -- objects, spoken and unspoken PC and NPC psionics, the
// works -- all come through this function eventually. This is also the entry
// point for non-spoken or unrestricted psionics. psi_num 0 is legal but silently
// ignored here, to make callers simpler

// Added the parameter 'ack' to control if the psionic generates a message or not - Tails 12/24/08
//  ... currently only used by psi_affects to keep equipment psionics quiet
int call_psionic(struct char_data *ch, struct char_data *victim, struct obj_data *obj, int psi_num, int psi_level, int casttype, bool ack)
{

    if (psi_num < 1 || psi_num > TOP_PSIONIC_DEFINE)
        return (0);

    if (!cast_wtrigger(ch, victim, obj, psi_num))
        return (0);

    if (!cast_otrigger(ch, obj, psi_num))
        return (0);

    if (!cast_mtrigger(ch, victim, psi_num))
        return (0);

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_PSIONICS) && !obj) {
		act("Your psionic fizzles out and dies.\r\n", FALSE, ch, 0, 0, TO_CHAR, 2);
        act("$n's psionic fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM, 2);
        return (0);
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL) && (PINFO.violent || IS_SET(PINFO.routines, PSI_DAMAGE))) {
        send_to_char(ch, "A flash of white light fills the room, countering your violent psionic!\r\n");
        act("White light from no particular source suddenly fills the room, then vanishes.", FALSE, ch, 0, 0, TO_ROOM);
        return (0);
    }

    if (IS_SET(PINFO.routines, PSI_DAMAGE))
        if (psi_damage(psi_level, ch, victim, psi_num, obj) == -1)
            return (-1);    /* Successful and target died, don't cast again. */

    if (IS_SET(PINFO.routines, PSI_AFFECTS))
        psi_affects(psi_level, ch, victim, psi_num, ack, obj);

    if (IS_SET(PINFO.routines, PSI_UNAFFECTS))
        psi_unaffects(psi_level, ch, victim, psi_num, obj);

     if (IS_SET(PINFO.routines, PSI_POINTS))
        psi_points(psi_level, ch, victim, psi_num, obj);

    if (IS_SET(PINFO.routines, PSI_ALTER_OBJS))
        psi_alter_objs(psi_level, ch, obj, psi_num);

    if (IS_SET(PINFO.routines, PSI_GROUPS))
        psi_groups(psi_level, ch, psi_num, obj);

    if (IS_SET(PINFO.routines, PSI_MASSES))
        psi_masses(psi_level, ch, psi_num, obj);

    if (IS_SET(PINFO.routines, PSI_AREAS))
        psi_areas(psi_level, ch, psi_num, obj);

    if (IS_SET(PINFO.routines, PSI_SUMMONS))
        psi_summons(psi_level, ch, obj, psi_num);

    if (IS_SET(PINFO.routines, PSI_CREATIONS))
        psi_creations(psi_level, ch, psi_num, obj);

    if (IS_SET(PINFO.routines, PSI_MANUAL)) {

        switch (psi_num) {
            case PSIONIC_ASTRAL_PROJECTION: MANUAL_PSI(psionic_astral_projection); break;
            case PSIONIC_CHARM:             MANUAL_PSI(psionic_charm); break;
            case PSIONIC_COMMUNE:           MANUAL_PSI(psionic_commune); break;
            case PSIONIC_DETOX:             MANUAL_PSI(psionic_detox); break;
            case PSIONIC_DUPLICATES:        MANUAL_PSI(psionic_duplicates); break;
            case PSIONIC_FARSEE:            MANUAL_PSI(psionic_farsee); break;
            case PSIONIC_LOCATE_OBJECT:     MANUAL_PSI(psionic_locate_object); break;
            case PSIONIC_PACIFY:            MANUAL_PSI(psionic_pacify); break;
            case PSIONIC_PANIC:             MANUAL_PSI(psionic_panic); break;
            case PSIONIC_PORTAL:            MANUAL_PSI(psionic_portal); break;
            case PSIONIC_PSYCHIC_LEECH:     MANUAL_PSI(psionic_psychic_leech); break;
            case PSIONIC_PSYCHIC_STATIC:    MANUAL_PSI(psionic_psychic_static); break;
            case PSIONIC_SUMMON:            MANUAL_PSI(psionic_summon); break;
            case PSIONIC_TELEPORT:          MANUAL_PSI(psionic_teleport); break;
            case PSIONIC_PSIONIC_CHANNEL:   MANUAL_PSI(psi_psionic_channel); break;
            case PSIONIC_FLASH_GRENADE:     MANUAL_PSI(psi_flash_grenade); break;
            case PSIONIC_ULTRA_STUN:        MANUAL_PSI(psi_ultra_stun); break;
            case PSIONIC_GAS_GRENADE:       MANUAL_PSI(psi_gas_grenade); break;
            case PSIONIC_IDENTIFY:          MANUAL_PSI(psionic_identify); break;
            case PSIONIC_FIRE:              MANUAL_PSI(psi_fire); break;
            case PSIONIC_SMOKE:             MANUAL_PSI(psi_smoke); break;
            case PSIONIC_LAG:               MANUAL_PSI(psi_lag); break;
			case PSIONIC_LIGHTNING_FLASH:	MANUAL_PSI(psionic_lightning_flash); break;
			case PSIONIC_PSI_BULLET:		MANUAL_PSI(psionic_psi_bullet); break;
            case PSIONIC_FIRE_BLADE:        MANUAL_PSI(psi_fire_blade); break; 
            case PSIONIC_ICE_BLADE:         MANUAL_PSI(psi_ice_blade); break; 
			case PSIONIC_TIGERCLAW:		MANUAL_PSI(psionic_tigerclaw); break;
			case PSIONIC_TURRET:		MANUAL_PSI(psi_turret); break;

        }
    }

  return (1);
}

// psi_object_psionics: This is the entry-point for all psionic items.  This should
// only be called by the 'quaff', 'use', 'recite', etc. routines.
// For reference, object values 0-3:
// staff  - [0]    level    [1] max charges    [2] num charges    [3] psi num
// wand   - [0]    level    [1] max charges    [2] num charges    [3] psi num
// scroll - [0]    level    [1] psi num        [2] psi num        [3] psi num
// potion - [0]    level    [1] psi num        [2] psi num        [3] psi num
// Staves and wands will default to level 14 if the level is not specified; the
// DikuMUD format did not specify staff and wand levels in the world files */
void psi_object_psionics(struct char_data *ch, struct obj_data *obj, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int i;
    int k;
    struct char_data *tch = NULL;
    struct char_data *next_tch;
    struct obj_data *tobj = NULL;

    one_argument(argument, arg);

    k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);

    switch (GET_OBJ_TYPE(obj)) {
        case ITEM_STAFF:

            act("You activate $p.", FALSE, ch, obj, 0, TO_CHAR);

            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
            else
                act("$n activates $p.", FALSE, ch, obj, 0, TO_ROOM);

            if (GET_OBJ_VAL(obj, 2) <= 0) {
                send_to_char(ch, "It seems powerless.\r\n");
                act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
            }
            else {
                GET_OBJ_VAL(obj, 2)--;
                GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

                // Level to cast psionic at
                k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;

                // Area/mass psionics on staves can cause crashes. So we use special cases for those psionics here
                if (HAS_PSIONIC_ROUTINE(GET_OBJ_VAL(obj, 3), PSI_MASSES | PSI_AREAS)) {

                    for (i = 0, tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
                        i++;

                    while (i-- > 0)
                        call_psionic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF, TRUE);
                }
                else {
                    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
                        next_tch = tch->next_in_room;
                        if (ch != tch)
                            call_psionic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF, TRUE);
						if (ch == tch)
							call_psionic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF, TRUE);
                    }
                }
            }
            break;

        case ITEM_WAND:
            if (k == FIND_CHAR_ROOM) {
                if (tch == ch) {
                    act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
                    act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
                }
                else {
                    act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);

                    if (obj->action_description)
                        act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
                    else
                        act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
                }
            }
            else if (tobj != NULL) {
                act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);

                if (obj->action_description)
                    act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
                else
                    act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
            }
            else if (IS_SET(psionics_info[GET_OBJ_VAL(obj, 3)].routines, PSI_AREAS | PSI_MASSES)) {

                // Wands with area psionics don't need to be pointed
                act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
                act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
            }
            else {
                act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
                return;
            }

            if (GET_OBJ_VAL(obj, 2) <= 0) {
                send_to_char(ch, "It seems powerless.\r\n");
                act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
                return;
            }

            GET_OBJ_VAL(obj, 2)--;
            GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

            if (GET_OBJ_VAL(obj, 0))
                call_psionic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 0), CAST_WAND, TRUE);
            else
                call_psionic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), DEFAULT_WAND_LVL, CAST_WAND, TRUE);
            break;

        case ITEM_SCROLL:
            if (*arg) {
                if (!k) {
                    act("There is nothing here to affect with $p.", FALSE, ch, obj, NULL, TO_CHAR);
                    return;
                }
            }
            else
                tch = ch;
			
			for (i = 0; i < NUM_WEARS; i++)
				if (GET_EQ(ch, i))
					if (obj == GET_EQ(ch, i))
						obj_to_char(unequip_char(ch, i, FALSE), ch);
				

			obj_from_char(obj);
            act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);

            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            else
                act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);

            GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

            for (i = 1; i <= 4; i++)
                if (call_psionic(ch, tch, tobj, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_SCROLL, TRUE) <= 0)
                    break;

            if (obj != NULL)
                extract_obj(obj);
            break;

        case ITEM_POTION:
            tch = ch;

            if (!consume_otrigger(obj, ch, OCMD_QUAFF))  /* check trigger */
                return;

            act("You put your lips up to $p and begin to drink...", FALSE, ch, obj, NULL, TO_CHAR);
			obj_from_char(obj);

            if (obj->action_description)
                act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
            else
                act("$n begins to drink $p.", TRUE, ch, obj, NULL, TO_ROOM);

            GET_WAIT_STATE(ch) = PULSE_VIOLENCE;

            for (i = 1; i <= 4; i++)
                if (call_psionic(ch, ch, obj, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_POTION, TRUE) <= 0)
                    break;

            if (obj != NULL)
                extract_obj(obj);
            break;

        case ITEM_DRUG:
            break;

        default:
            log("SYSERR: Unknown object_type %d in psi_object_psionics.", GET_OBJ_TYPE(obj));
            break;
    }
}

// cast_psionic is used generically to cast any spoken psionic, assuming we already
// have the target char/obj and psionic number.  It checks all restrictions,
// prints the words, etc. Entry point for NPC casts.  Recommended entry point
// for psionics cast by NPCs via specprocs
int cast_psionic(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int psi_num)
{
    int level;

    if (psi_num < 0 || psi_num > TOP_PSIONIC_DEFINE) {
        log("SYSERR: cast_psionic trying to call psi_num %d/%d.", psi_num, TOP_PSIONIC_DEFINE);
        return (0);
    }

    if (GET_POS(ch) < PINFO.min_position) {
        switch (GET_POS(ch)) {

            case POS_SLEEPING:
                send_to_char(ch, "You dream about great psionic powers.\r\n");
                break;

            case POS_RESTING:
                send_to_char(ch, "You cannot concentrate while resting.\r\n");
                break;

            case POS_FOCUSING:
                send_to_char(ch, "Your concentration is not focused on that matter.\r\n");
                break;

            case POS_MEDITATING:
                send_to_char(ch, "Just relax and breathe slowly, empty your mind.\r\n");
                break;

            case POS_SITTING:
                send_to_char(ch, "You can't do this sitting!\r\n");
                break;

            case POS_FIGHTING:
                send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
                break;

            default:
                send_to_char(ch, "You can't do much of anything like this!\r\n");
                break;
        }

        return (0);
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch)) {
        send_to_char(ch, "You are afraid you might hurt your master!\r\n");
        return (0);
    }

    if ((tch != ch) && IS_SET(PINFO.targets, TAR_SELF_ONLY)) {
        send_to_char(ch, "You can only psi this upon yourself!\r\n");
        return (0);
    }

    if ((tch == ch) && IS_SET(PINFO.targets, TAR_NOT_SELF)) {
        send_to_char(ch, "You cannot psi this upon yourself!\r\n");
        return (0);
    }

    if (IS_SET(PINFO.routines, PSI_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch, "You can't psi this if you're not in a group!\r\n");
        return (0);
    }

    send_to_char(ch, "%s", CONFIG_OK);

    // todo: why are NPCs limited?
    if (IS_NPC(ch))
        level = 5;
    else
        level = GET_PSIONIC_LEVEL(ch, psi_num);

    return (call_psionic(ch, tch, tobj, psi_num, level, CAST_PSIONIC, TRUE));
}


// do_psi is the entry point for PC-casted psionics.  It parses the arguments,
// determines the psionic number and finds a target, throws the die to see if
// the psionic can be cast, checks for sufficient mana and subtracts it, and
// passes control to cast_psionic(). */
ACMD(do_psi)
{
    struct char_data *tch = NULL;
    struct obj_data *tobj = NULL;
    char *s;
    char *t;
    int psi_pts;
    int psi_num;
    int i;
    int target = 0;

    // tbaMUD doesn't seem to want mobs to cast psionics.  But we do.  Tails 1/09/2009
    // if (IS_NPC(ch))
    //    return;

    // get: blank, psionic name, target name
    s = strtok(argument, "'");

    if (s == NULL) {
        send_to_char(ch, "Psi what where?\r\n");
        return;
    }

    s = strtok(NULL, "'");
    if (s == NULL) {
        send_to_char(ch, "Psionic names must be enclosed in the Psi Symbols: \'\r\n");
        return;
    }

    t = strtok(NULL, "\0");

    skip_spaces(&s);

    // psi_num = search_block(s, psionics, 0);
    psi_num = find_psionic_num(s);

    if ((psi_num < 1) || (psi_num > TOP_PSIONIC_DEFINE) || !*s) {
        send_to_char(ch, "Psi what?!?\r\n");
        return;
    }

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NO_PSIONICS)) {
        send_to_char(ch, "Your psionics fizzle out and die.\r\n");
        return;
    }

    if (!IS_NPC(ch) && GET_LEVEL(ch) < PINFO.min_level) {
        send_to_char(ch, "You are not a high enough level to use that psionic!\r\n");
        return;
    }

    if (!IS_NPC(ch) && GET_PSIONIC_LEVEL(ch, psi_num) == 0) {
        send_to_char(ch, "You have no idea how to use that psionic.\r\n");
        return;
    }

    // this is also in the NPC/PC cast_psionic above, but we need it here before we subtract psi -Lump
    if (GET_POS(ch) < PINFO.min_position) {
        switch (GET_POS(ch)) {

            case POS_SLEEPING:
                send_to_char(ch, "You dream about great psionic powers.\r\n");
                break;

            case POS_RESTING:
                send_to_char(ch, "You cannot concentrate while resting.\r\n");
                break;

            case POS_FOCUSING:
                send_to_char(ch, "Your concentration is not focused on that matter.\r\n");
                break;

            case POS_MEDITATING:
                send_to_char(ch, "Just relax and breath slowly, empty your mind.\r\n");
                break;

            case POS_SITTING:
                send_to_char(ch, "You can't do this sitting!\r\n");
                break;

            case POS_FIGHTING:
                send_to_char(ch, "Impossible!  You can't concentrate enough!\r\n");
                break;

            default:
                send_to_char(ch, "You can't do much of anything like this!\r\n");
                break;
        }
        return;
    }

    // Find the target
    if (t != NULL) {

        char arg[MAX_INPUT_LENGTH];

        strlcpy(arg, t, sizeof(arg));
        one_argument(arg, t);
        skip_spaces(&t);

        // Copy target to global cast_arg2, for use in psionics like locate object
        strcpy(cast_arg2, t);
    }

    if (IS_SET(PINFO.targets, TAR_IGNORE))
        target = TRUE;
    else if (t != NULL && *t) {

        if (!target && (IS_SET(PINFO.targets, TAR_CHAR_ROOM))) {
            if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_ROOM)) != NULL)
                target = TRUE;
        }

        if (!target && IS_SET(PINFO.targets, TAR_CHAR_WORLD))
            if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD)) != NULL)
                target = TRUE;

        if (!target && IS_SET(PINFO.targets, TAR_OBJ_INV))
            if ((tobj = get_obj_in_list_vis(ch, t, NULL, ch->carrying)) != NULL)
                target = TRUE;

        //if (!target && IS_SET(PINFO.targets, TAR_OBJ_EQUIP)) {
        //    for (i = 0; !target && i < NUM_WEARS; i++)
        //        if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name)) {
        //           tobj = GET_EQ(ch, i);
        //            target = TRUE;
        //        }
        //}

        //if (!target && IS_SET(PINFO.targets, TAR_OBJ_ROOM))
        //    if ((tobj = get_obj_in_list_vis(ch, t, NULL, world[IN_ROOM(ch)].contents)) != NULL)
        //        target = TRUE;

        if (!target && IS_SET(PINFO.targets, TAR_OBJ_WORLD))
            if ((tobj = get_obj_vis(ch, t, NULL)) != NULL)
                target = TRUE;

    }
    else {
        // if target string is empty
        if (!target && IS_SET(PINFO.targets, TAR_FIGHT_SELF))
            if (FIGHTING(ch) != NULL) {
                tch = ch;
                target = TRUE;
            }

        if (!target && IS_SET(PINFO.targets, TAR_FIGHT_VICT))
            if (FIGHTING(ch) != NULL) {
                tch = FIGHTING(ch);
                target = TRUE;
            }

        // if no target specified, and the psionic isn't violent, default to self
        if (!target && IS_SET(PINFO.targets, TAR_CHAR_ROOM) && !PINFO.violent) {
            tch = ch;
            target = TRUE;
        }

        if (!target && IS_SET(PINFO.targets, TAR_CHAR_SELF) && !PINFO.violent) {
            tch = ch;
            target = TRUE;
        }

        if (!target) {
            send_to_char(ch, "Upon %s should the psionic be used?\r\n",
                IS_SET(PINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
            return;
        }
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && PINFO.violent) {
        send_to_char(ch, "Possibly you should leave the nasty psi's to your master.\r\n");
        return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM) && (tch != ch)) {
        send_to_char(ch, "Hmm, this might be unbalancing to psi things on other people.\r\n");
        return;
    }

    if (target && (tch == ch) && PINFO.violent) {
        send_to_char(ch, "You shouldn't psi that on yourself -- could be bad for your health!\r\n");
        return;
    }

    if (!target) {
        if (psi_num == PSIONIC_LOCATE_OBJECT) {
            // Added here so they still lose psi and invisible imms inv will not generate different results
            psi_pts = PINFO.psi;
            if ((psi_pts > 0) && (GET_PSI(ch) < psi_pts) && (GET_LEVEL(ch) < LVL_IMMORT)) {
                send_to_char(ch, "You haven't the energy to psi that!\r\n");
                return;
            }

            GET_PSI(ch) -= MAX(0, psi_pts);
            send_to_char(ch, "Okay.\r\nYou sense nothing.\r\n");
            GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
            return;
        }
		if (psi_num == PSIONIC_IMBUE_ESSENCE) {
			send_to_char(ch, "You cannot find that item in your inventory.\r\n");
			return;
		}
        else
            send_to_char(ch, "Cannot find the target of your psionic!\r\n");

        return;
    }

    psi_pts = PINFO.psi;

    if ((psi_pts > 0) && (GET_PSI(ch) < psi_pts) && (GET_LEVEL(ch) < LVL_IMMORT) && (psi_num != PSIONIC_THERAPY)) {
//    if ((psi_pts > 0) && (GET_PSI(ch) < psi_pts) && (GET_LEVEL(ch) < LVL_IMMORT) && (psi_num != PSIONIC_THERAPY) && !IS_NPC(ch)) {
        send_to_char(ch, "You haven't the energy to psi that!\r\n");
        return;
    }
    else if ((psi_pts > 0) && (GET_MOVE(ch) < psi_pts) && (GET_LEVEL(ch) < LVL_IMMORT) && (psi_num == PSIONIC_THERAPY)) {
        send_to_char(ch, "You haven't the energy to psi that!\r\n");
        return;
    }

    if (psi_pts > 0 && psi_num != PSIONIC_THERAPY)
//    if (psi_pts > 0 && psi_num != PSIONIC_THERAPY && !IS_NPC(ch))
        GET_PSI(ch) -= MAX(0, psi_pts);

    // You throws the dice and you takes your chances.. 101% is total failure
    // todo: huh?  the above comment is no longer applicable?
    cast_psionic(ch, tch, tobj, psi_num);
    GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
}

// Assign the psionics on boot up
static void assign_psionic(int psi_num, const char *name, int level, int psi, int minpos, int targets, int violent, int routines,
    const char *wearoff, const char *rwearoff, int prac_cost, int tree, int prereq, int prereqid, const char *display, int pcclass, int tier)
{
    if (psi_num < 0 || psi_num > TOP_PSIONIC_DEFINE) {
        log("SYSERR: attempting assign to illegal psi_num %d/%d", psi_num, TOP_PSIONIC_DEFINE);
        return;
    }

    psionics_info[psi_num].min_level = level;
    psionics_info[psi_num].psi = psi;
    psionics_info[psi_num].min_position = minpos;
    psionics_info[psi_num].targets = targets;
    psionics_info[psi_num].violent = violent;
    psionics_info[psi_num].routines = routines;
    psionics_info[psi_num].name = name;
    psionics_info[psi_num].wear_off_msg = wearoff;
	psionics_info[psi_num].rwear_off_msg = rwearoff;
	psionics_info[psi_num].prac_cost = prac_cost;
	psionics_info[psi_num].tree = tree;
	psionics_info[psi_num].prereq = prereq;
	psionics_info[psi_num].prereqid = prereqid;
	psionics_info[psi_num].display = display;
	psionics_info[psi_num].pcclass = pcclass;
	psionics_info[psi_num].tier = tier;
}

void assign_skill(int skill, const char *name, int level, int prac_cost, int tree, int prereq, int prereqid, const char *display, int pcclass, int tier)
{
    if (skill < 0 || skill > TOP_SKILL_DEFINE) {
        log("SYSERR: attempting assign to illegal skillnum %d/%d", skill, TOP_SKILL_DEFINE);
        return;
    }

    skills_info[skill].min_level = level;
    skills_info[skill].name = name;
	skills_info[skill].prac_cost = prac_cost;
	skills_info[skill].tree = tree;
	skills_info[skill].prereq = prereq;
	skills_info[skill].prereqid = prereqid;
	skills_info[skill].display = display;
	skills_info[skill].pcclass = pcclass;
	skills_info[skill].tier = tier;
}

void unused_psionic(int psi_num)
{
    psionics_info[psi_num].min_level = LVL_IMPL + 1;
    psionics_info[psi_num].psi = 0;
    psionics_info[psi_num].min_position = 0;
    psionics_info[psi_num].targets = 0;
    psionics_info[psi_num].violent = 0;
    psionics_info[psi_num].routines = 0;
    psionics_info[psi_num].name = unused_psiname;
    psionics_info[psi_num].remort = 0;
}

void unused_skill(int skillnum)
{
    skills_info[skillnum].min_level = LVL_IMPL + 1;
    skills_info[skillnum].name = unused_skillname;
    skills_info[skillnum].min_level = 0;
    skills_info[skillnum].remort = 0;
}

// Re-Written by Gahan 2012
// Basically what this does, uniformly lump all tree skills in to one large tree for sorting and statistical purposes
void psi_assign_psionics(void)
{
	int i;
	int j;

	for (i = 0; i <= TOP_PSIONIC_DEFINE ; i++)
		unused_psionic(i);

	for (j = 0; j <= (MAX_SKILLSNPSIONIC - 1); j++) {
		if (full_skill_tree[j].psi_or_skill == 0)
			assign_psionic(full_skill_tree[j].skillnum, full_skill_tree[j].skill_name, full_skill_tree[j].skill_level, full_skill_tree[j].point_cost, full_skill_tree[j].minpos, full_skill_tree[j].targets, full_skill_tree[j].violent, full_skill_tree[j].routines, full_skill_tree[j].wearoff, full_skill_tree[j].rwearoff, full_skill_tree[j].practice_cost, full_skill_tree[j].tree, full_skill_tree[j].prereq, full_skill_tree[j].prereqid, full_skill_tree[j].display, full_skill_tree[j].pcclass, full_skill_tree[j].tier);
	}
}

// Re-Written by Gahan 2012
// Basically what this does, uniformly lump all tree skills in to one large tree for sorting and statistical purposes
void skill_assign_skills(void)
{
    int i;
	int j;

    // Do not change the loop below
    for (i = 0; i <= TOP_SKILL_DEFINE; i++)
        unused_skill(i);

	for (j = 0; j <= (MAX_SKILLSNPSIONIC - 1); j++) {
		if (full_skill_tree[j].psi_or_skill == 1)
			assign_skill(full_skill_tree[j].skillnum, full_skill_tree[j].skill_name, full_skill_tree[j].skill_level, full_skill_tree[j].practice_cost, full_skill_tree[j].tree, full_skill_tree[j].prereq, full_skill_tree[j].prereqid, full_skill_tree[j].display, full_skill_tree[j].pcclass, full_skill_tree[j].tier);
	}
}
	

