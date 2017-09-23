/**************************************************************************
*  File: core_psionics.c                                   Part of tbaMUD *
*  Usage: Low-level functions for psionics; psionic template code.        *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

// NB: This was originally magic.c

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "psionics.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "class.h"
#include "fight.h"
#include "drugs.h"
#include "shop.h"
#include "skilltree.h"

// local file scope function prototypes
//static int psi_materials(struct char_data *ch, IDXTYPE item0, IDXTYPE item1, IDXTYPE item2, int extract, int verbose);
static void perform_psi_groups(int psi_level, struct char_data *ch, struct char_data *tch, int psi_num, struct obj_data *obj);

struct affected_rooms *room_af_list = NULL;

// Negative apply_saving_throw[] values make saving throws better! So do
// negative modifiers.  Though people may be used to the reverse of that.
// It's due to the code modifying the target saving throw instead of the
// random number of the character as in some other systems. */
// todo: this function needs a re-design for Cyber

int psi_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier)
{
    int save = 0;
	int saveroll = 0;

    save += GET_SAVE(ch, SAVING_PSIONIC);
	save += (GET_LEVEL(ch) - GET_LEVEL(victim)) *2;
    
	saveroll = rand_number(1, 100);

	if (save > saveroll)
		return (TRUE);
	else
		return (FALSE);
}

// affect_update: called from comm.c (causes psionics to wear off)
void affect_update(void)
{
    struct affected_type *af;
    struct affected_type *next;
    struct char_data *i;
	//const char *to_room = NULL;

    for (i = character_list; i; i = i->next) {

        for (af = i->affected; af; af = next) {

            next = af->next;

            if (af->duration >= 1)
                af->duration--;
            else if (af->duration <= -1)    /* No action */
                af->duration = -1;    /* GODs only! unlimited */
            else {
                if ((af->type > 0) && (af->type <= TOP_PSIONIC_DEFINE))
                    if (af->type == PSIONIC_INDUCE_SLEEP) {
                        if (IS_NPC(i)) {
                            send_to_char(i, "You awaken, and sit up.\r\n");
                            act("$n awakens.", TRUE, i, 0, 0, TO_ROOM);
                            GET_POS(i) = POS_STANDING;
                        }
                    }

                    affect_remove(i, af, TRUE);
           }
        }

        // is the character still around?
        if (i == NULL)
            continue;

        // drug updates
        for (af = i->drugs_used; af && i; af = next) {

            next = af->next;

            if (af->duration >= 1)
                af->duration--;
            else if (af->duration == -1)    /* No action */
                af->duration = -1;    /* GODs only! unlimited */
            else {

                if ((af->type > 0) && (af->type <= MAX_DRUGS))
                    if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
                        if (*drug_names[af->type]) {
                            switch (rand_number(1,4)) {

                                case 1:
                                    send_to_char(i, "You've come down off of %s.\r\n", drug_names[af->type]);
                                    break;

                                case 2:
                                    send_to_char(i, "You are no longer feeling the affects of %s.\r\n", drug_names[af->type]);
                                    break;

                                case 3:
                                default:
                                    send_to_char(i, "The %s has worn off...Time to get some more, eh?\r\n", drug_names[af->type]);
                                    break;
                        }
                    }

                drug_remove(i, af);
            }
        }
    }
}

void room_affect_update(void)
{
    static struct affected_type *af;
    static struct affected_type *room_af_next;
    static struct affected_rooms *i;

    for (i = room_af_list; i != NULL; ) {
        for (af = world[i->room_number].affected; af; af = room_af_next) {

            room_af_next = af->next;
            af = world[i->room_number].affected;

            if (af->duration >= 1)
                af->duration--;
            else
    if (room_affected_by_psionic(i->room_number, PSIONIC_TURRET))
        send_to_room(i->room_number, "\r\n@DA large turret explodes into pieces.@n\r\n");

                room_affect_remove(i->room_number, af);


        }

        if (room_af_list == NULL)
            i = NULL;
        else
            i = i->room_next;
    }
}

bool group_member_check(struct char_data *ch, struct char_data *dude)
{
    struct char_data *k;
    struct follow_type *f;

    if (ch == dude)
        return (TRUE);

    if (!AFF_FLAGGED(ch, AFF_GROUP))
        return (FALSE);

    if (!AFF_FLAGGED(dude, AFF_GROUP))
        return (FALSE);

    // setup a search through the follower list
    if (ch->master) {
        k = ch->master;
        if (k == dude)
            return (TRUE);
    }
    else
        k = ch;

    for (f = k->followers; f; f = f->next)
        if (f->follower == dude)
            return (TRUE);

    return (FALSE);
}


// Checks for up to 3 vnums (psionic reagents) in the player's inventory. If
// multiple vnums are passed in, the function ANDs the items together as
// requirements (ie. if one or more are missing, the psionic will not fail).
// @param ch The caster of the psionic.
// @param item0 The first required item of the psionic, NOTHING if not required.
// @param item1 The second required item of the psionic, NOTHING if not required.
// @param item2 The third required item of the psionic, NOTHING if not required.
// @param extract TRUE if psi_materials should consume (destroy) the items in
// the players inventory, FALSE if not. Items will only be removed on a successful cast.
// @param verbose TRUE to provide some generic failure or success messages,
// FALSE to send no in game messages from this function.
// @retval int TRUE if ch has all materials to cast the psionic, FALSE if not.
// todo: this did not exist in cyber
/*static int psi_materials(struct char_data *ch, IDXTYPE item0, IDXTYPE item1, IDXTYPE item2, int extract, int verbose)
{
    // Used for object searches
    struct obj_data *tobj = NULL;

    // Points to found reagents
    struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

    // Check for the objects in the players inventory
    for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {

        if ((item0 != NOTHING) && (GET_OBJ_VNUM(tobj) == item0)) {
            obj0 = tobj;
            item0 = NOTHING;
        }
        else if ((item1 != NOTHING) && (GET_OBJ_VNUM(tobj) == item1)) {
            obj1 = tobj;
            item1 = NOTHING;
        }
        else if ((item2 != NOTHING) && (GET_OBJ_VNUM(tobj) == item2)) {
            obj2 = tobj;
            item2 = NOTHING;
        }
    }

    // If we needed items, but didn't find all of them, then the psionic is a failure
    if ((item0 != NOTHING) || (item1 != NOTHING) || (item2 != NOTHING)) {

        // Generic psionic failure messages
        if (verbose) {
            switch (rand_number(0, 2)) {

                case 0:
                    send_to_char(ch, "A wart sprouts on your nose.\r\n");
                    break;

                case 1:
                    send_to_char(ch, "Your hair falls out in clumps.\r\n");
                    break;

                case 2:
                    send_to_char(ch, "A huge corn develops on your big toe.\r\n");
                    break;
            }
        }

        // Return fales, the material check has failed
        return (FALSE);
    }

    // From here on, ch has all required materials in their inventory and the
    // material check will return a success

    // Extract (destroy) the materials, if so called for
    if (extract) {

        if (obj0 != NULL)
            extract_obj(obj0);

        if (obj1 != NULL)
            extract_obj(obj1);

        if (obj2 != NULL)
            extract_obj(obj2);

        // Generic success messages that signals extracted objects
        if (verbose) {
            send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
            act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
        }
    }

    // Don't extract the objects, but signal materials successfully found
    if (!extract && verbose) {
        send_to_char(ch, "Your pack rumbles.\r\n");
        act("Something rumbles in $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
    }

    // Signal to calling function that the materials were successfully found and processed
    return (TRUE);
}*/


// Every psionic that does damage comes through here.  This calculates the amount
// of damage, adds in any modifiers, determines what the saves are, tests for
// save and calls damage(). -1 = dead, otherwise the amount of damage done. */
int psi_damage(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj)
{
    int dam = 0;
    int modifier = 0;
	int check;
    int level;
	int psi_type;
    const char *to_vict = NULL;
    const char *to_room = NULL;

    if (victim == NULL || ch == NULL)
        return (0);

    // todo: why is the level of the character affecting how much damage is done?
    if (GET_REMORTS(ch) > 0)
			check = GET_LEVEL(ch) + (20 * GET_REMORTS(ch));
    else
        check = GET_LEVEL(ch);

    level = (float)((check / 5 ) + (float)((GET_INT(ch) + GET_PCN(ch))/7) + rand_number(1,20));

    switch (psi_num) {

        case PSIONIC_TRANSMUTE:
            to_room = "$n gets all transmuted. That must have sucked.";
            to_vict = "You've been transmuted. That really sucked.";
            dam = 150 * level;
            modifier = 10;
			psi_type = DMG_ETHEREAL;
            break;

        case PSIONIC_PSI_HAMMER:
            to_room = "$N creates a psionic backlash!";
            //to_vict = "Your mind shatters from the psionic assault!";
            dam = 30 * level;
            modifier = 3;
			psi_type = DMG_PSIONIC;
            break;

        case PSIONIC_ELECTROKINETIC_DISCHARGE:
            to_room = "Several blue streaks of energy form above $n and snaps $m to the ground!";
            to_vict = "Several blue streaks of energy form above your head and snap you to the ground!";
            dam = 50 + (50 * level);
            modifier = 5;
			psi_type = DMG_ELECTRICITY;
            break;

        case PSIONIC_SNUFF_LIFEFORCE:
            to_room = "The hair on your neck stands up!";
            to_vict = "You start to choke!";
            dam = 750 * level;
			psi_type = DMG_PSIONIC;
            break;

        case PSIONIC_DISRUPTION:
			to_room = "The body of $n seems to shift slightly in all directions, causing considerable pain.";
            to_vict = "The molecular structure of your body scrambles slightly, causing considerable pain.";
            dam = (20 * level) + 200 ;
			psi_type = DMG_PSIONIC;
            break;

        case PSIONIC_NOVA:
            to_room = "$N unleashes a ball of searing heat!";
            to_vict = "You are fully engulfed in a ball of burning energy!";
            dam = 1000 * level;
			psi_type = DMG_PSIONIC;
            break;

        case PSIONIC_SUPER_NOVA:
            dam = 2000 * level;
			psi_type = DMG_PSIONIC;
            break;

        case PSIONIC_PYROKINETIC_ERUPTION:
            dam = 300 * level;
			psi_type = DMG_PSIONIC;
            break;

		case PSIONIC_THERM_GRENADE:
			dam = 1000 * level;
			psi_type = DMG_EXPLOSIVE;
			break;

		case PSIONIC_FRAG_GRENADE:
			dam = 500 * level;
			psi_type = DMG_EXPLOSIVE;
			break;

		case PSIONIC_PLASMA_GRENADE:
			dam = 750 * level;
			psi_type = DMG_EXPLOSIVE;
			break;

        default:
            send_to_char(ch, "Psionic unimplemented, it would seem.\r\n");
            log("Unimplemented psionic called inside psi_damage: %d", psi_num);
            return (0);

    }

    // mirror by Jack, fixed by Lump :P, improved efficiency by Tails
    if (AFF_FLAGGED(victim, AFF_PSI_MIRROR) && rand_number(1, 100) <= 60) {

        // give 1% chance per 10 psi mastery to avoid mirror
        if (GET_PSIMASTERY(ch) == 0 && rand_number(1, 100) > 30) {
            struct char_data *temp;
            send_to_char(ch, "An intense blue aura erupts and your psionic is directed back towards you!!\r\n");
            send_to_char(victim, "An intense blue aura erupts about you and a psionic attack is sent back towards your target!!\r\n");
            temp = victim;
            victim = ch;
            ch = temp;
            dam = dam / 3;
        }
        if (GET_PSIMASTERY(ch) == 1 && rand_number(1,100) > 75) {
            struct char_data *temp;
            send_to_char(ch, "A brilliant blue aura erupts and your psionic is directed back towards you!!\r\n");
            send_to_char(victim, "A brilliant blue aura erupts about you and a psionic attack is sent back towards your target!!\r\n");
            temp = victim;
            victim = ch;
            ch = temp;
            dam = dam / 6;
        }
    }

    // todo: this should be re-evaluated
    // cut damage done to players -DH
    if (!IS_NPC(victim))
        dam = dam / 4;

    if (to_vict != NULL)
        act(to_vict, FALSE, victim, 0, ch, TO_CHAR);

    if (to_room != NULL)
        act(to_room, TRUE, victim, 0, ch, TO_ROOM);

    // and finally, inflict the damage
	if (obj == NULL)
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), dam, ch);
	int finaldamage = damage(ch, victim, dam, psi_num, psi_type);
	if (obj == NULL && finaldamage > 0)
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), finaldamage, ch);
    return (finaldamage);
}

// Every psionic that does an affect comes through here.  This determines the
// effect, whether it is added or replacement, whether it is legal or not, etc.
// affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
#define MAX_PSIONIC_AFFECTS 5

// this function now includes a parameter 'ack' which controls if a message is sent to the char and victim
void psi_affects(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, bool ack, struct obj_data *obj)
{
    struct affected_type af[MAX_PSIONIC_AFFECTS];
    bool accum_affect = FALSE;
    bool accum_duration = FALSE;
    const char *to_vict = NULL;
    const char *to_room = NULL;
    int i, j;
    int d_roll;
	int v_roll;
    int num;
    struct char_data *temp;
    float tmpPM = 1.0;
    bool violence = FALSE;

    if (victim == NULL || ch == NULL)
        return;

    // previous checks by code before we get here verified that the room is not peaceful
    // this code checks for PK
    if (psionics_info[psi_num].violent && !PK_OK(ch, victim) && !IS_NPC(ch) && !IS_NPC(victim) && ack) {
        send_to_char(ch, "You may not do that!\n");
        return;
    }

	// Add exp for casting psionic_affects
	if (obj == NULL) {
		int exp = (rand_number(-250,250) + ((psionics_info[psi_num].min_level * 25 * (GET_LEVEL(ch) / 2))));
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), exp, ch);
	}

    // init the affected structure(s)
    for (i = 0; i < MAX_PSIONIC_AFFECTS; i++) {
		new_affect(&(af[i]));
        af[i].type = psi_num;
    }

    switch (psi_num) {

        case PSIONIC_ARMORED_AURA:
            af[0].location = APPLY_AC;
            af[0].modifier = GET_LEVEL(ch)/3+(GET_INT(ch)+GET_PCN(ch))/4 + GET_REMORTS(ch);
            af[0].duration = GET_LEVEL(ch)/3+(GET_INT(ch)+GET_PCN(ch))/2 + GET_REMORTS(ch);
			SET_BIT_AR(af[0].bitvector, AFF_ARMORED_AURA);
            to_vict = "A strong psionic armor now protects you.";
            to_room = "A strong psionic armor protects $n.";
            break;

        case PSIONIC_PSI_SHIELD:
            af[0].location = APPLY_AC;
            af[0].modifier = GET_LEVEL(ch)/2 + (GET_INT(ch)+GET_PCN(ch))/2 + GET_REMORTS(ch)*2;
            af[0].duration = GET_LEVEL(ch)/3 + (GET_INT(ch)+GET_PCN(ch))/4 + GET_REMORTS(ch);
            SET_BIT_AR(af[0].bitvector, AFF_PSI_SHIELD );
            to_vict = "A telekinetic shield now surrounds you from danger.";
            to_room = "A telekinetic shield surrounds $n.";
            break;

        case PSIONIC_INDESTRUCTABLE_AURA:
            af[0].location = APPLY_AC;
            af[0].modifier = GET_LEVEL(ch) + ((GET_INT(ch) + GET_PCN(ch)) / 2) + (ch->player.num_remorts * 3);
            af[0].duration = (GET_LEVEL(ch) / 3) + ((GET_INT(ch) + GET_PCN(ch)) / 4) + ch->player.num_remorts;
			SET_BIT_AR(af[0].bitvector, AFF_INDESTRUCT_AURA );
            to_vict = "A blue glimmer indicates that you are surrounded by the strongest of shields.";
            to_room = "A blue glimmer surrounds $n.";
            break;

        case PSIONIC_SECOND_SIGHT:
            af[0].duration = GET_LEVEL(ch)/6 + (GET_PCN(ch)+GET_PCN(victim))/4;
            SET_BIT_AR(af[0].bitvector, AFF_SECOND_SIGHT );
            to_vict = "You become more perceptive in combat.";
            accum_duration = TRUE;
            break;

		case PSIONIC_INFRAVISION:
			af[0].duration = GET_LEVEL(ch)/6 + (GET_PCN(ch)/4);
			SET_BIT_AR(af[0].bitvector, AFF_INFRAVISION );
			to_vict = "Your eyes begin to glow red, and your perception of your surroundings increases.";
			break;

        case PSIONIC_BIO_REGEN:
			af[0].duration = 2;
			SET_BIT_AR(af[0].bitvector, AFF_BIO_REGEN);
			to_vict = "A blue electrical hue surrounds you.";
			to_room = "A blue electrical hue surrounds $n.";
            break;

        case PSIONIC_BLINDNESS:
            if (MOB_FLAGGED(victim, MOB_NOBLIND) || AFF_FLAGGED(victim, AFF_BLIND_IMMUNE)) {
                send_to_char(ch, "You fail.\r\n");
                return;
            }

            d_roll = rand_number(1, 100);

            num = GET_LEVEL(ch)/10 + GET_INT(ch)/10;
            if ((d_roll > 90 && num == 4) || (d_roll > 80 && num == 3) ||
                (d_roll > 70 && num == 2) || (d_roll > 60 && num == 1)) {
                send_to_char(ch, "Your psionic fails.\r\n");
                return;
            }

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            af[0].location = APPLY_HITNDAM;
            af[0].modifier = -num;
            af[0].duration = GET_LEVEL(ch)/10 + 2;
            SET_BIT_AR(af[0].bitvector, AFF_BLIND );

            to_room = "$n seems to be blinded!";
            to_vict = "You have been blinded!";
            break;

        case PSIONIC_DETECT_INVISIBILITY:
            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/4;
            SET_BIT_AR(af[0].bitvector, AFF_DETECT_INVIS );
            accum_duration = TRUE;
            to_vict = "Your eyes tingle.";
            break;

        case PSIONIC_INVISIBILITY:

            if (!victim)
                victim = ch;

            // why do you need to invis a mob? -Lump
            if (IS_NPC(victim))
                return;

            num = GET_LEVEL(ch)/10 + GET_INT(ch)/10;

            af[0].duration  = 3*num;
            SET_BIT_AR(af[0].bitvector, AFF_INVISIBLE );
            accum_duration = TRUE;
            to_vict = "You vanish.";
            to_room = "$n slowly fades out of existence.";
            break;

        case PSIONIC_POISON:

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            af[0].location = APPLY_STR;
            af[0].duration = GET_LEVEL(ch)/3 + (GET_INT(ch)+GET_PCN(ch))/2;
            af[0].modifier = -2;
            SET_BIT_AR(af[0].bitvector, AFF_POISON );

            to_vict = "You feel very sick.";             to_room = "$n gets violently ill!";
            if (ch != victim && !FIGHTING(victim))
                violence = TRUE;
            break;

        case PSIONIC_SANCTUARY:
            if (((affected_by_psionic(victim, PSIONIC_SANCTUARY)) ||
                (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) ||
                (affected_by_psionic(victim, PSIONIC_MEGA_SANCT))) && ack) {
//                send_to_char(ch, "Nothing happens.\r\n");
                return;
            }

            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/1;
            accum_duration = TRUE;
            SET_BIT_AR(af[0].bitvector, AFF_SANCT );

            to_vict = "A white aura momentarily surrounds you.";
            to_room = "$n is surrounded by a white aura.";
            break;

        case PSIONIC_SUPER_SANCT:

            if (affected_by_psionic(victim, PSIONIC_SANCTUARY)) {
                affect_from_char(victim, PSIONIC_SANCTUARY, FALSE);
            }
            if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) {
                return;
            }
            if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT)) {
                return;
            }

            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            af[0].duration   = 3 * num;
            SET_BIT_AR(af[0].bitvector, AFF_SUPER_SANCT );

            to_vict = "You start glowing with a silver glimmer.";
            to_room = "$n is surrounded by a silver aura.";

            break;

        case PSIONIC_MEGA_SANCT:

            if (affected_by_psionic(victim, PSIONIC_SANCTUARY)) {
                affect_from_char(victim, PSIONIC_SANCTUARY, FALSE);
            }
            if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) {
                affect_from_char(victim, PSIONIC_SUPER_SANCT, FALSE);
            }
            if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT)) {
                return;
            }

            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            af[0].duration = 4 * num;
            SET_BIT_AR(af[0].bitvector, AFF_MEGA_SANCT );

            to_vict = "You start glowing with a golden glimmer.";
            to_room = "$n is surrounded by a golden aura.";
            break;

        case PSIONIC_INDUCE_SLEEP:

            if (GET_LEVEL(victim) > LVL_IMMORT && ack) {
                send_to_char(ch, "No NO, they look pretty scary!\r\n");
                return;
            }

            if (GET_MOB_SPEC(victim) == shop_keeper && ack) {
                send_to_char(ch, "Your victim doesn't look amused.\r\n");
                return;
            }

            if (!IS_NPC(ch) && !IS_NPC(victim) && !PK_OK(ch, victim) && ack) {
                send_to_char(ch, "But that wouldn't be right!\n\r");
                return;
            }

            if (MOB_FLAGGED(victim, MOB_NOSLEEP) && ack) {
                send_to_char(ch, "You can't seem to put them to sleep, they're too awake!\r\n");
                return;
            }


            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "You failed.\r\n");
                return;
            }

            d_roll = GET_LEVEL(ch) + (GET_INT(ch)+GET_PCN(ch))/20 + rand_number(1,10);
			v_roll = GET_LEVEL(victim) + (GET_INT(victim)+GET_PCN(victim))/20 + rand_number(1,10);

            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            if (d_roll < v_roll) {
                send_to_char(ch, "You fail to dominate %s's mind with your will.\r\n", GET_NAME(victim));
                return;
            }
			else {
				send_to_char(ch, "You use your will to put %s to sleep.\r\n", GET_NAME(victim));
			}

            af[0].duration = 2 * num;
            SET_BIT_AR(af[0].bitvector, AFF_SLEEP );

            if (GET_POS(victim) > POS_SLEEPING) {
                act("Your mind feels dominated by another force, and are induced into a deep sleep.", FALSE, victim, 0, 0, TO_CHAR);
                act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
                GET_POS(victim) = POS_SLEEPING;
            }
            break;

        case PSIONIC_SENSE_LIFE:
            d_roll = rand_number(0, 100);
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;
            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/4;
            SET_BIT_AR(af[0].bitvector, AFF_SENSE_LIFE );
            accum_duration = TRUE;
            to_vict = "You feel your awareness improve.";
            break;

        case PSIONIC_REVEAL_LIFE:
            d_roll = rand_number(0, 100);
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;
            if (((d_roll > 60 && num == 1) || (d_roll > 70 && num == 2) ||
                (d_roll > 80 && num == 3) || (d_roll > 90 && num >= 4)) && ack) {
                send_to_char(ch, "You fail.\r\n");
                return;
            }

            to_room = "All life is revealed!";

            for (temp = world[IN_ROOM(ch)].people; temp; temp = temp->next_in_room) {

                if (AFF_FLAGGED(temp, AFF_HIDE))
                    REMOVE_BIT_AR(AFF_FLAGS(temp), AFF_HIDE);

                if (PRF_FLAGGED(temp, PRF_STEALTH))
                    REMOVE_BIT_AR(PRF_FLAGS(temp), PRF_STEALTH);
            }
            break;

        case PSIONIC_LEVITATE:
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            af[0].duration = 2 * num;
            SET_BIT_AR(af[0].bitvector, AFF_LEVITATE );
            accum_duration = TRUE;

            to_vict = "You suddenly feel lighter as your feet lift off the ground.";
            to_room = "$n starts to levitate in the air.";
            break;


        case PSIONIC_FANATICAL_FURY:
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            af[0].duration  = 2 * num;
            af[0].modifier  = 5 * num + GET_REMORTS(ch);
            af[0].location  = APPLY_HITNDAM;
            SET_BIT_AR(af[0].bitvector, AFF_FANATICAL_FURY );

            to_vict = "Your resound a ferocious warcry pooling your anger and your energy!";
            to_room = "$n sound's a ferocious warcry attempting to energize and focus' $s anger.";
            break;

        case PSIONIC_HASTE:

            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/10;
            SET_BIT_AR(af[0].bitvector, AFF_HASTE);

            to_vict = "Everything slows down around you.";
			to_room = "$n begins to move faster and twitch with excess energy.";
            break;

        case PSIONIC_VORPAL_SPEED:
            af[0].duration = GET_REMORTS(ch)/10+(GET_INT(ch)+GET_PCN(ch))/10;
            SET_BIT_AR(af[0].bitvector, AFF_VORPAL_SPEED);
            to_room = "$n starts moving damn fast!";
            to_vict = "You start moving at the speed of light!";
            break;

        case PSIONIC_LETHARGY:
            if ((psi_savingthrow(ch, victim, 0, 0) || AFF_FLAGGED(victim, AFF_LETH_IMMUNE)) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            af[0].duration  = GET_LEVEL(ch)/5 + (GET_PCN(ch)+GET_INT(ch))/4;
			SET_BIT_AR(af[0].bitvector, PSIONIC_LETHARGY);

            to_vict = "You feel much slower.";
            to_room = "$n seems to be moving much slower.";
            break;

        case PSIONIC_WEAKEN:

            if (affected_by_psionic(victim, PSIONIC_WEAKEN) && ack) {
                send_to_char(ch, "But your victim is already weak!\n\r");
                return;
            }

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;

            af[0].duration = num;
            af[0].modifier = -4;
            af[0].location = APPLY_STR;

            af[1].duration = num;
            af[1].modifier = -10;
            af[1].location = APPLY_HITROLL;

            af[2].duration = num;
            af[2].modifier = -10;
            af[2].location = APPLY_DAMROLL;

			SET_BIT_AR(af[0].bitvector, AFF_WEAKEN);
			SET_BIT_AR(af[1].bitvector, AFF_WEAKEN);
			SET_BIT_AR(af[2].bitvector, AFF_WEAKEN);

            if (ack)
                send_to_char(ch, "They seem to be a bit weaker.\r\n");

            to_vict = "$N utters some words, and you begin to feel weak.";
            if (ch != victim && !FIGHTING(victim))
                violence = TRUE;
            break;

        case PSIONIC_INVIGORATE:
            if (affected_by_psionic(victim, PSIONIC_INVIGORATE) && ack) {
                send_to_char(ch, "That's not gonna help!\n\r");
                return;
            }

            if (GET_PSI(victim) < 100) {
                send_to_char(ch, "Impossible!\n\r");
                return;
            }

            af[0].duration   = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/4;
            af[0].modifier   = 100;
            af[0].location   = APPLY_PSI2HIT;
			SET_BIT_AR(af[0].bitvector, PSIONIC_WEAKEN);
            to_vict = "You feel bigger!\n\r";
            break;

        case PSIONIC_AURA_OF_COOL:

            af[0].duration = GET_LEVEL(ch)/6+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_COOL_AURA );

            to_vict = "You feel a cool aura protecting your body from excess heat.\n\r";
            break;

        case PSIONIC_THERMAL_AURA:

            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_THERMAL_AURA );

            to_vict = "You feel a warm aura insulating your body.\n\r";
            break;

        case PSIONIC_BIO_TRANCE:
            af[0].duration = GET_LEVEL(ch)/6+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_BIO_TRANCE );
            to_vict = "Energy flows outward from your body creating rift between you and the elements.";
			to_room = "Energy flows outward from $n's body creating rift between $m and the elements.";
            break;

        case PSIONIC_PSI_SHACKLE:

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            af[0].duration   = GET_LEVEL(ch)/6 + (GET_INT(ch)+GET_PCN(ch))/8;
            // todo: af[0].bitvector = AFF_PSI_SHACKLE;

            to_vict = "You feel yourself chained in place.\n\r";
            break;

        case PSIONIC_INSPIRE:
            af[0].duration   = GET_LEVEL(ch)/3 + (GET_INT(ch)+GET_PCN(ch))/4+ GET_REMORTS(ch);
            af[0].modifier   = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20+ GET_REMORTS(ch);
            af[0].location   = APPLY_HITNDAM;
            SET_BIT_AR(af[0].bitvector, AFF_INSPIRE );
            to_vict = "You feel great inspiration.\r\n";
            break;

        case PSIONIC_COURAGE:
            af[0].duration   = GET_LEVEL(ch)/6 + (GET_INT(ch)+GET_PCN(ch))/8+ GET_REMORTS(ch);
            af[0].modifier   = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/18+ GET_REMORTS(ch);
            af[0].location   = APPLY_HITNDAM;
            SET_BIT_AR(af[0].bitvector, AFF_COURAGE );
            to_vict = "Your self confidence increases.\r\n";
            break;

        case PSIONIC_PETRIFY:

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            if (IS_HIGHLANDER(victim)) {
                act("$n breaks out of a stone shell!", TRUE, victim, 0, 0, TO_CHAR);
                act("$n doesn't seem affected at all by that nasty stone skin!", TRUE, victim, 0, 0, TO_CHAR);
                send_to_char(ch, "Heh, petrify doesn't work on the likes of your kind!");
                return;
            }

            af[0].duration = 1;
            SET_BIT_AR(af[0].bitvector, AFF_PETRIFY );
            to_vict = "You suddenly feel unable to move!";
            to_room = "$n freezes in $s tracks! $n can't move!";
            break;

        case PSIONIC_PSI_MIRROR:

            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_PSI_MIRROR );
            to_vict = "You are protected by a strong psionic field.\n\r";
            break;

        case PSIONIC_PARALYZE:

            if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            af[0].duration = 1;
            SET_BIT_AR(af[0].bitvector, AFF_PARALYZE );
            to_vict = "You suddenly feel unable to move!";
            to_room = "$n freezes in $s tracks! $n can't move!";
            break;

        case PSIONIC_BLIND_IMMUNE:
            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_BLIND_IMMUNE );
            to_vict = "You steel your gaze.";
            break;

        case PSIONIC_LETHARGY_IMMUNE:
            af[0].duration = GET_LEVEL(ch)/5+(GET_INT(ch)+GET_PCN(ch))/5;
            SET_BIT_AR(af[0].bitvector, AFF_LETH_IMMUNE);
            to_vict = "Your flesh crackles with electricity.";
            break;

        case PSIONIC_DEGRADE:
            if ((affected_by_psionic(victim, PSIONIC_DEGRADE) || affected_by_psionic(victim, PSIONIC_MEGA_DEGRADE)) && ack) {
                send_to_char(ch, "Your victim is already vulnerable.\n\r");
                return;
            }
            else {
				send_to_char(ch, "This psionic has been temporarily disabled.\r\n");
				return;
				//
                //int degradeAmount;
                //int degradeTmp;
                //float degradePercent;

                //if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                //    send_to_char(ch, "You failed.\r\n");
                //    return;
                //}

                //add random 1-4%
                //degradeTmp = GET_LEVEL(ch)/30 + (GET_INT(ch) + GET_PCN(ch))/25 + rand_number(1, 4);
                //degradePercent = (float)degradeTmp/100.0;
                //degradeAmount = MAX(-125, -1.0 * GET_AC(victim) * degradePercent);

                //af[0].duration = GET_LEVEL(ch)/10 + (GET_PCN(ch) + GET_INT(ch))/10;
                //af[0].modifier = degradeAmount;
                //af[0].location = APPLY_AC;
                //SET_BIT_AR(af[0].bitvector, PSIONIC_DEGRADE);

                //to_vict = "You have become more vulnerable to attacks.";
                //to_room = "$n appears more vulnerable to attack.";
            }
            break;
		case PSIONIC_RESURRECTION_FATIGUE:
			if (affected_by_psionic(victim, PSIONIC_RESURRECTION_FATIGUE))
				return;
			else {
				int duration = rand_number(0, (GET_LEVEL(victim) / 2));
				af[0].duration = duration;
				af[0].modifier = -1 * (GET_HITROLL(ch) / 2);
				af[0].location = APPLY_HITROLL;
				af[1].duration = duration;
				af[1].modifier = -1 * (GET_DAMROLL(ch) / 2);
				af[1].location = APPLY_DAMROLL;
				af[2].duration = duration;
				af[2].modifier = -1 * (GET_AC(ch) / 2);
				af[2].location = APPLY_AC;
				af[3].duration = duration;
				af[3].modifier = -8;
				af[3].location = APPLY_ALL_STATS;
				SET_BIT_AR(af[0].bitvector, PSIONIC_RESURRECTION_FATIGUE);
				SET_BIT_AR(af[1].bitvector, PSIONIC_RESURRECTION_FATIGUE);
				SET_BIT_AR(af[2].bitvector, PSIONIC_RESURRECTION_FATIGUE);
				SET_BIT_AR(af[3].bitvector, PSIONIC_RESURRECTION_FATIGUE);

				to_vict = "Your body doesn't feel right, you feel incredibly ill.";
				to_room = "$n appears incredibly ill and dazed.";
			}
			break;
        case PSIONIC_MEGA_DEGRADE:
            if ((affected_by_psionic(victim, PSIONIC_DEGRADE) || affected_by_psionic(victim, PSIONIC_MEGA_DEGRADE)) && ack) {
                send_to_char(ch, "But your victim is already vulnerable!\n\r");
                return;
            }
            else {
				send_to_char(ch, "This psionic has been temporarily disabled.\r\n");
				return;
                //int degradeAmount;
                //int degradeTmp;
                //float degradePercent;

                //if (psi_savingthrow(ch, victim, 0, 0) && ack) {
                //    send_to_char(ch, "Your psionic has no effect!\r\n");
                //    return;
                //}

                //degradeTmp = (GET_LEVEL(ch) + (GET_REMORTS(ch)*30))/30 + (GET_INT(ch) + GET_PCN(ch))/10;
                //degradeTmp += rand_number(1,4); //add random 1-4%
                //degradePercent = (float)degradeTmp/100.0;
                //degradeAmount = MAX(-250, -1.0 * GET_AC(victim) * degradePercent);

                //af[0].duration = GET_LEVEL(ch)/5 + (GET_PCN(ch)+GET_INT(ch))/4;
                //af[0].modifier = degradeAmount;
                //af[0].location = APPLY_AC;
                //SET_BIT_AR(af[0].bitvector, 38);

                //to_vict = "You have become MUCH more vulnerable to attack.";
                //to_room = "$n has become MUCH more vulnerable to attack.";
            }
            break;

        case PSIONIC_COMBAT_MIND:
            af[0].duration = 1+(GET_REMORTS(ch)/2);
            SET_BIT_AR(af[0].bitvector, AFF_COMBAT_MIND);
            to_vict = "With careful preparation, you harden your will to the ways of combat.";
            to_room = "$n prepares $s mind for combat!";
            break;
			
		case PSIONIC_SLOW_POISON:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_POISON;
			SET_BIT_AR(af[0].bitvector, PSIONIC_SLOW_POISON);
			to_vict = "You prepare to raise your defences against poison.";
			to_room = "$n prepares to raise their defences against poison.";
			break;

        case PSIONIC_WOLF_SENSE:
                                af[0].duration = 1+(GET_REMORTS(ch)/2);

                                af[1].duration = 1+(GET_REMORTS(ch)/2);
                                af[1].modifier = (GET_INT(ch) / 5);
                                af[1].location = APPLY_PCN;
                                SET_BIT_AR(af[0].bitvector, AFF_DODGE);                     
                                SET_BIT_AR(af[1].bitvector, AFF_ANIMAL_STATS);


            to_vict = "You feel as agile as the wolf.";
            to_room = "$n flickers into the shape of a wolf, then returns to normal form!";
            break;
        case PSIONIC_PUMA_SPEED:
                                af[0].duration = 1+(GET_REMORTS(ch)/2);
//                                af[1].modifier = (GET_LEVEL(ch) + GET_INT(ch)) * (GET_REMORTS(ch) + 1);
                                af[0].modifier = 10 + GET_LEVEL(ch) + ((GET_REMORTS(ch)+ 1) * 2);
                                af[0].location = APPLY_HITROLL;
                                af[1].duration = 1+(GET_REMORTS(ch)/2);
                                af[1].modifier = (GET_INT(ch) / 5);
                                af[1].location = APPLY_DEX;
                                SET_BIT_AR(af[0].bitvector, AFF_ANIMAL_STATS);
                                SET_BIT_AR(af[1].bitvector, AFF_ANIMAL_STATS);
            to_vict = "You feel much faster as the spirit of the puma fills your body.";
            to_room = "$n flickers into the shape of a puma, then returns to normal form!";
            break;
        case PSIONIC_BEAR_RAGE:

                                af[0].duration = 1+(GET_REMORTS(ch)/2);
                                af[0].modifier = 10 + GET_LEVEL(ch) + ((GET_REMORTS(ch)+ 1) * 2);
                                af[0].location = APPLY_DAMROLL;
                                af[1].duration = 1+(GET_REMORTS(ch)/2);
                                af[1].modifier = (GET_INT(ch) / 5);
                                af[1].location = APPLY_STR;
                                SET_BIT_AR(af[1].bitvector, AFF_ANIMAL_STATS);
                                SET_BIT_AR(af[2].bitvector, AFF_ANIMAL_STATS);
            to_vict = "You roar in rage as you gain the power of the bear.";
            to_room = "$n flickers into the shape of a bear, then returns to normal form!";
            break;
			
		case PSIONIC_NEUTRALIZE_DRUG:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_DRUG;
			SET_BIT_AR(af[0].bitvector, PSIONIC_NEUTRALIZE_DRUG);
			to_vict = "You begin to raise your defences against drugs.";
			to_room = "$n begins to raise their defences against drugs.";
			break;
			
		case PSIONIC_HEAT_SHIELD:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_FIRE;
			SET_BIT_AR(af[0].bitvector, PSIONIC_HEAT_SHIELD);
			to_vict = "A red outline surrounds you and then flickers out of existence.";
			to_room = "A red outline surrounds $n and then flickers out of existence.";
			break;

		case PSIONIC_COLD_SHIELD:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_COLD;
			SET_BIT_AR(af[0].bitvector, PSIONIC_COLD_SHIELD);
			to_vict = "A blue outline surrounds you and then flickers out of existence.";
			to_room = "A blue outline surrounds $n and then flickers out of existence.";
			break;
			
		case PSIONIC_DISSIPATE:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_ELECTRICITY;
			SET_BIT_AR(af[0].bitvector, PSIONIC_DISSIPATE);
			to_vict = "Electricity flows through you as you raise your defences against it.";
			to_room = "Electricity begins to course through $n!";
			break;	
			
		case PSIONIC_MAGNETIC_FIELD:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_EXPLOSIVE;
			SET_BIT_AR(af[0].bitvector, PSIONIC_MAGNETIC_FIELD);
			to_vict = "A sudden wave of energy flows outwards from your core.";
			to_room = "A sudden wave of energy flows outwards from $n's core.";
			break;			
		
		case PSIONIC_PSIONIC_SHIELD:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_PSIONIC;
			SET_BIT_AR(af[0].bitvector, PSIONIC_PSIONIC_SHIELD);
			to_vict = "Psionic energy begins to consume you, as your senses begin to magnify.";
			to_room = "A field of psionic energy exposes $n, as their senses begin to magnify.";
			break;		
			
		case PSIONIC_SPIRIT_WARD:
			af[0].duration = 2 + (GET_LEVEL(ch) / 10);
			af[0].modifier = 20;
			af[0].location = APPLY_SAVING_NONORM;
			SET_BIT_AR(af[0].bitvector, PSIONIC_SPIRIT_WARD);
			to_vict = "The ghosts of ethereal spirits surround you for protection.";
			to_room = "$n is suddenly surrounded by several ghosts, protecting them from harm.";
			break;			
			
        default:
            send_to_char(ch, "Psionic unimplemented, it would seem.\r\n");
            log("Unimplemented psionic called inside psi_affects: %d", psi_num);
            return;
    }

    // If this is a mob that has this affect set in its mob file, do not perform
    // the affect.  This prevents people from un-sancting mobs by sancting them
    // and waiting for it to fade, for example.
	if (IS_NPC(victim) && !affected_by_psionic(victim, psi_num) && !obj) { 
		for (i = 0; i < MAX_PSIONIC_AFFECTS; i++) { 
			for (j=0; j<NUM_AFF_FLAGS; j++) { 
				if (IS_SET_AR(af[i].bitvector, j) && AFF_FLAGGED(victim, j)) { 
					send_to_char(ch, "%s", CONFIG_NOEFFECT); 
					return;
				}
			}
		}
	}


    // If the victim is already affected by this psionic, and the psionic does not
    // have an accumulative effect, then fail the psionic
    if (affected_by_psionic(victim, psi_num) && !(accum_duration || accum_affect) && ack && !obj) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
    }

    for (i = 0; i < MAX_PSIONIC_AFFECTS; i++)
        if (af[i].bitvector[0] || af[i].bitvector[1] || af[i].bitvector[2] || af[i].bitvector[3] || (af[i].location != APPLY_NONE)) {
            // this allows us to have equipment perm effects cast perpetual psionics
            af[i].duration = (psi_level == EQ_PSI_LEVEL ? -1 : af[i].duration);

            if (GET_PSIMASTERY(ch) > 0)
                tmpPM = 1.0 + (float)GET_PSIMASTERY(ch)/100.0;

            if (af[i].duration != -1)
                af[i].duration = (int)(af[i].duration * tmpPM);

            if (af[i].modifier != 0)
                af[i].modifier = MAX(-125, MIN(125, (int)(af[i].modifier * tmpPM)));

            affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
        }

    // if you want a to_char message, you'll have to add it
    // for now, the message can go to the room

    if (to_vict != NULL && ack)
        act(to_vict, FALSE, victim, 0, ch, TO_CHAR);

    if (to_room != NULL && ack)
        act(to_room, TRUE, victim, 0, ch, TO_ROOM);

    if (violence) {
		int thaco = rand_number(1,30); 
        hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
	}
}

// This function is used to provide services to psi_groups.  This function is
// the one you should change to add new group psionics
static void perform_psi_groups(int psi_level, struct char_data *ch, struct char_data *tch, int psi_num, struct obj_data *obj)
{
    switch (psi_num) {

        case PSIONIC_GROUP_HEAL:
            psi_points(psi_level, ch, tch, PSIONIC_HEAL, obj);
            break;

        // todo: this would be a cool psionic
        //case PSIONIC_GROUP_ARMOR:
        //    psi_affects(psi_level, ch, tch, PSIONIC_ARMOR, savetype, TRUE);
        //    break;

        case PSIONIC_GROUP_SANCT:
            psi_affects(psi_level, ch, tch, PSIONIC_SANCTUARY, TRUE, obj);
            break;
    }
}

// Every psionic that affects the group should run through here. perform_psi_groups
// contains the switch statement to send us to the right psionic. Group psionics
// affect everyone grouped with the caster who is in the room, caster last. To
// add new group psionics, you shouldn't have to change anything in psi_groups.
// Just add a new case to perform_psi_groups
void psi_groups(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj)
{
    struct char_data *tch;
    struct char_data *k;
    struct follow_type *f;
    struct follow_type *f_next;

    if (ch == NULL)
        return;

    if (!AFF_FLAGGED(ch, AFF_GROUP)) {
        send_to_char(ch, "But you are not a member of a group!\r\n");
        return;
    }

    if (ch->master != NULL)
        k = ch->master;
    else
        k = ch;

    for (f = k->followers; f; f = f_next) {
        f_next = f->next;
        tch = f->follower;

        if (IN_ROOM(tch) != IN_ROOM(ch))
            continue;

        if (!AFF_FLAGGED(tch, AFF_GROUP))
            continue;

        if (ch == tch)
            continue;

        perform_psi_groups(psi_level, ch, tch, psi_num, obj);
    }

    if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
        perform_psi_groups(psi_level, ch, k, psi_num, obj);

    perform_psi_groups(psi_level, ch, ch, psi_num, obj);
}

// Mass psionics affect every creature in the room except the caster.
// No psionics of this class currently implemented
void psi_masses(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj)
{
    struct char_data *tch;
    struct char_data *tch_next;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
        tch_next = tch->next_in_room;

        if (tch == ch)
            continue;

        switch (psi_num) {
        }
    }
}

// Every psionic that affects an area (room) runs through here.  These are
// generally offensive psionics.  This calls psi_damage to do the actual damage.
// All psionics listed here must also have a case in psi_damage() in order for
// them to work. Area psionics have limited targets within the room
void psi_areas(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj)
{
    struct char_data *tch;
    struct char_data *next_tch;
    const char *to_char = NULL;
    const char *to_room = NULL;

    if (ch == NULL)
        return;

    // to add psionics just add the message here plus
    // an entry in psi_damage for the damaging part of the psionic.
    switch (psi_num) {

        //case PSIONIC_EARTHQUAKE:
        //    to_char = "You gesture and the earth begins to shake all around you!";
        //    to_room ="$n gracefully gestures and the earth begins to shake violently!";
        //    break;

        case PSIONIC_SUPER_NOVA:
            to_char = "A searing blast of light erupts from your body.";
            to_room = "A searing blast of light erupts from $n's body.";
            break;

        case PSIONIC_PYROKINETIC_ERUPTION:
            to_char = "You cause the room to burst into flames.";
            to_room = "$n engulfs the room in flames.";
    }

    if (to_char != NULL)
        act(to_char, FALSE, ch, 0, 0, TO_CHAR);

    if (to_room != NULL)
        act(to_room, FALSE, ch, 0, 0, TO_ROOM);

    for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {

        next_tch = tch->next_in_room;

        // The skips: 1: the caster
        //            2: immortals
        //            3: if no pk on this mud, skips over all players
        //            4: pets (charmed NPCs)
        //            5: group members
        //            6: spectators
        if (tch == ch)
            continue;

        if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT)
            continue;

        if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(tch))
            continue;

        // dont damage your charmies
        if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
            continue;

        // dont damage your group mates
        if (group_member_check(ch, tch))
            continue;

        // Doesn't matter if they die here so we don't check. -gg 6/24/98
        psi_damage(psi_level, ch, tch, psi_num, obj);
    }
}

//----------------------------------------------------------------------------
// Begin Psionic Summoning - Generic Routines and Local Globals
//----------------------------------------------------------------------------

// Every psionic which summons/gates/conjours a mob comes through here
// These use act(), don't put the \r\n.
static const char *psi_summon_msgs[] = {
    "\r\n",
    "$n makes a strange gesture; you feel a strong breeze!",
    "$n animates a corpse!",
    "$N appears from a cloud of thick blue smoke!",
    "$N appears from a cloud of thick green smoke!",
    "$N appears from a cloud of thick red smoke!",
    "$N disappears in a thick black cloud!"
    "As $n makes a strange gesture, you feel a strong breeze.",
    "As $n makes a strange gesture, you feel a searing heat.",
    "As $n makes a strange gesture, you feel a sudden chill.",
    "As $n makes a strange gesture, you feel the dust swirl.",
    "$n fantastically divides!",
    "$n animates a corpse!"
};

// Keep the \r\n because these use send_to_char
static const char *psi_summon_fail_msgs[] = {
    "\r\n",
    "There are no such creatures.\r\n",
    "Uh oh...\r\n",
    "Oh dear.\r\n",
    "Gosh durnit!\r\n",
    "The elements resist!\r\n",
    "You failed.\r\n",
    "There is no corpse!\r\n"
};

// Defines for psi_summons
#define MOB_CLONE            10   /* < vnum for the clone mob.  */
#define OBJ_CLONE            161  /* < vnum for clone material. */
#define MOB_ZOMBIE           11   /* < vnum for the zombie mob. */

void psi_summons(int psi_level, struct char_data *ch, struct obj_data *obj, int psi_num)
{
    struct char_data *mob = NULL;
    struct obj_data *tobj;
    struct obj_data *next_obj;
    int pfail = 0;
    int msg = 0;
    int fmsg = 0;
    int num = 1;
    int handle_corpse = FALSE, i;
    mob_vnum mob_num;

    if (ch == NULL)
        return;

    switch (psi_num) {

        default:
            return;
    }

    if (AFF_FLAGGED(ch, AFF_CHARM)) {
        send_to_char(ch, "You are too giddy to have any followers!\r\n");
        return;
    }

    if (rand_number(0, 101) < pfail) {
        send_to_char(ch, "%s", psi_summon_fail_msgs[fmsg]);
        return;
    }

    for (i = 0; i < num; i++) {

        if (!(mob = read_mobile(mob_num, VIRTUAL))) {
            send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
            return;
        }

        char_to_room(mob, IN_ROOM(ch));
        IS_CARRYING_W(mob) = 0;
        IS_CARRYING_N(mob) = 0;
        SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);

        //if (psi_num == PSIONIC_CLONE) {
        //    // Don't mess up the prototype; use new string copies
        //    mob->player.name = strdup(GET_NAME(ch));
        //    mob->player.short_descr = strdup(GET_NAME(ch));
        //}

        act(psi_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
        load_mtrigger(mob);
        add_follower(mob, ch);
    }

    if (handle_corpse) {

        for (tobj = obj->contains; tobj; tobj = next_obj) {
            next_obj = tobj->next_content;
            obj_from_obj(tobj);
            obj_to_char(tobj, mob);
        }

        extract_obj(obj);
    }
}

// Clean up the defines used for psi_summons
#undef MOB_CLONE
#undef OBJ_CLONE
#undef MOB_ZOMBIE

//----------------------------------------------------------------------------
// End Psionic Summoning - Generic Routines and Local Globals
//----------------------------------------------------------------------------

void psi_points(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj)
{
    int hit = 0;
    int move = 0;
    int psi = 0;
    float level = 0.0;
    float tmpPM = 0.0;

    if (victim == NULL)
        return;

    level = ((GET_LEVEL(ch)/10.0) + ((GET_INT(ch)+GET_PCN(ch))/10.0) + (GET_CON(victim)/5.0))/2.0;

    switch (psi_num) {

        case PSIONIC_PSYCHIC_SURGERY:
            hit = 90 * level;
            send_to_char(victim, "Your body is filled with a psychic glow.\r\n");
            act("A psychic glow surrounds $n for a short time.", TRUE, victim, 0, ch, TO_ROOM);
            break;

        case PSIONIC_HEAL:
            hit = 150;
            send_to_char(victim, "A warm feeling floods your body.\r\n");
            act("$n looks to be in better health.", TRUE, victim, 0, ch, TO_ROOM);
            break;

        case PSIONIC_GROUP_HEAL:
            hit = 150;
            if (ch != victim)
                send_to_char(ch, "You feel much better thanks to %s.\n\r", GET_NAME(ch));
            else
                send_to_char(victim, "You feel much better now.\r\n");
            act("$n looks to be in better health.", TRUE, victim, 0, ch, TO_ROOM);
            break;

        case PSIONIC_RESIST_FATIGUE:
            move = 20 * level;
            send_to_char(victim, "You feel less tired.\r\n");
            break;

        case PSIONIC_MEGAHEAL:
            hit = 180 * level;
            send_to_char(victim, "Your body is filled with a great warmth.\r\n");
            act("$n looks to be in MUCH better health.", TRUE, victim, 0, ch, TO_ROOM);
            break;

        case PSIONIC_THERAPY:
            move = 100;
            psi += (20 * level);
            break;

        default:
            break;
    }

    if (GET_PSIMASTERY(ch) > 0) {
        tmpPM = (float)1+(float)GET_PSIMASTERY(ch)/100;
        hit = hit * tmpPM;
        psi = psi * tmpPM;
    }

	if ((GET_MAX_HIT(victim) - GET_HIT(victim)) > hit && obj == NULL) {
		int exp = (rand_number(-250,250) + ((psionics_info[psi_num].min_level * 25 * (GET_LEVEL(ch) / 2))));
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), exp, ch);
	}
	else if (obj == NULL) {
		int exp = (rand_number(0,250) + ((psionics_info[psi_num].min_level * 5 * (GET_LEVEL(ch) / 10))));
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), exp, ch);
	}

    // from the way the code worked, psi therapy looked like it could get you more
    // psi points that you would normally be allowed to hold
    if (psi_num != PSIONIC_THERAPY) {
        GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
        GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hit);
        GET_PSI(victim) = MIN(GET_MAX_PSI(victim), GET_PSI(victim) + psi);
    }
    else {
        GET_MOVE(victim) = MAX(0, GET_MOVE(victim) - move);
        GET_PSI(victim) = GET_PSI(victim) + psi;
    }

    update_pos(victim);
}

void psi_unaffects(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj)
{
    int psionic = 0;
    const char *to_vict = NULL;
    const char *to_room = NULL;
    const char *to_char = NULL;
    int num;
    bool violence = FALSE;
    int d_roll = 0;

    if (victim == NULL)
        return;

    switch (psi_num) {

        case PSIONIC_HEAL:
            // fall-through
        case PSIONIC_RESTORE_SIGHT:

            // Heal also restores health, so don't give the "no effect" message if the
            // target isn't afflicted by the 'blindness' psionic
            if (!affected_by_psionic(victim, PSIONIC_BLINDNESS)) {
                if (psi_num != PSIONIC_HEAL)
                    send_to_char(ch, "%s", CONFIG_NOEFFECT);
                return;
            }

            psionic = PSIONIC_BLINDNESS;
            to_vict = "Your vision returns!";
            to_room = "There's a momentary gleam in $n's eyes.";
            break;

        case PSIONIC_REMOVE_SHACKLE:

            if (!affected_by_psionic(victim, PSIONIC_PSI_SHACKLE)) {
                if (ch != victim)
                    send_to_char(ch, "But your victim is not shackled!\n\r");
                else
                    send_to_char(ch, "But you aren't shackled!\r\n");
                return;
            }

            num = GET_LEVEL(ch)/30 + (GET_INT(ch)+ GET_PCN(ch))/20;
            d_roll = rand_number(1, 100);
            if ((d_roll > 60 && num == 1) || (d_roll > 70 && num == 2) ||
                (d_roll > 80 && num == 3) || (d_roll > 90 && num >= 4)) {
                send_to_char(ch, "Your psionic seems to be ineffective.\r\n");
                return;
            }

            if (ch != victim) {
                to_char = "Your victim is now able to flee.\n\r";
                to_vict = "You are able to move again!\r\n";
            }
            else
                to_vict = "You can move again!";

            psionic =  PSIONIC_PSI_SHACKLE;
            break;

        case PSIONIC_COUNTER_SANCT:
		
			if (!IS_NPC(ch) && !IS_NPC(victim)) {
				if (ch != victim && FIGHTING(ch) != victim) {
					send_to_char(ch, "You cannot use this psionic on that player.\r\n");
					return;
				}
			}
            if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT) ||
                affected_by_psionic(victim, PSIONIC_MEGA_SANCT)) {
               send_to_char(ch, "You need a higher counter psionic to remove super or mega sanctuary.\r\n");
               return;
            }

            if (!affected_by_psionic(victim, PSIONIC_SANCTUARY)) {
                send_to_char(ch, "But your victim is not affected by sanctuary!\r\n");
                return;
            }

            d_roll = rand_number(1, 100);
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;
            if (affected_by_psionic(victim, PSIONIC_SANCTUARY)) {
                if ((d_roll > 60 && num == 1) || (d_roll > 70 && num == 2) ||
                    (d_roll > 80 && num == 3) || (d_roll > 90 && num >= 4)) {
                    send_to_char(ch, "You failed.\r\n");
                    return;
                }

                psionic = PSIONIC_SANCTUARY;
            }

            if (psi_savingthrow(ch, victim, 0, 0)) {
                send_to_char(ch, "You failed.\r\n");
                return;
            }
            to_char = "The white aura surrounding $N fades.\r\n";
			to_vict = "The white aura around you fades.\r\n";
            if (ch != victim && !FIGHTING(victim))
                violence = TRUE;
            break;

        case PSIONIC_COUNTER_SUPER_SANCT:
		
			if (!IS_NPC(ch) && !IS_NPC(victim)) {
				if (ch != victim && FIGHTING(ch) != victim) {
					send_to_char(ch, "You cannot use this psionic on that player.\r\n");
					return;
				}
			}          
			if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT)) {
               send_to_char(ch, "You need a higher counter psionic to remove mega sanctuary.\r\n");
               return;
            }
            if ((!affected_by_psionic(victim, PSIONIC_SANCTUARY)) &&
                (!affected_by_psionic(victim, PSIONIC_SUPER_SANCT))) {
                send_to_char(ch, "But your victim is not affected by sanctuary!\r\n");
                return;
            }

            d_roll = rand_number(1, 100);
            num = GET_LEVEL(ch)/10 + (GET_INT(ch)+GET_PCN(ch))/20;
            // counter super sanct automatically removes basic sanctuary
            if (affected_by_psionic(victim, PSIONIC_SANCTUARY))
                psionic = PSIONIC_SANCTUARY;
            else if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) {
                if ((d_roll > 40 && num == 1) || (d_roll > 50 && num == 2) ||
                    (d_roll > 60 && num == 3) || (d_roll > 70 && num == 4) ||
                    (d_roll > 80 && num >= 5)) {
                    send_to_char(ch, "You failed.\r\n");
                    return;
                }

                psionic = PSIONIC_SUPER_SANCT;
            }

            if (psi_savingthrow(ch, victim, 0, 0)) {
                send_to_char(ch, "You failed.\r\n");
                return;
            }

            to_char = "The silver aura surrounding $N fades.\r\n";
			to_vict = "The silver aura around you fades.\r\n";

            if (ch != victim && !FIGHTING(victim))
                violence = TRUE;
            break;

        case PSIONIC_MEGA_COUNTER:
		
			if (!IS_NPC(ch) && !IS_NPC(victim)) {
				if (ch != victim && FIGHTING(ch) != victim) {
					send_to_char(ch, "You cannot use this psionic on that player.\r\n");
					return;
				}
			}
            // mega counter automatically removes basic sanctuary
            if (affected_by_psionic(victim, PSIONIC_SANCTUARY))
                psionic = PSIONIC_SANCTUARY;
            else if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) {

                d_roll = rand_number(1, 100);
                num = GET_LEVEL(ch)/5 + (GET_INT(ch)+GET_PCN(ch))/10;

                if ((d_roll > 60 && num == 1) || (d_roll > 70 && num == 2) ||
                    (d_roll > 80 && num == 3) || (d_roll > 90 && num >= 4)) {
                    send_to_char(ch, "You failed.\r\n");
                    return;
                }

                psionic = PSIONIC_SUPER_SANCT;
            }
            else if (affected_by_psionic(victim, PSIONIC_MEGA_SANCT)) {

                d_roll = rand_number(1, 100);
                num = GET_LEVEL(ch)/5 + (GET_INT(ch)+GET_PCN(ch))/10;

                if ((d_roll > 40 && num == 1) || (d_roll > 50 && num == 2) ||
                    (d_roll > 60 && num == 3) || (d_roll > 70 && num == 4) ||
                    (d_roll > 80 && num >= 5)) {
                    send_to_char(ch, "You failed.\r\n");
                    return;
                }

                psionic = PSIONIC_MEGA_SANCT;
            }
            else {
                send_to_char(ch, "But your victim is not affected by sanctuary!\r\n");
                return;
            }

            if (psi_savingthrow(ch, victim, 0, 0)) {
                send_to_char(ch, "You failed.\r\n");
                return;
            }

            to_char = "The golden aura surrounding $N fades.\r\n";
			to_vict = "The golden aura around you fades.\r\n";

            if (ch != victim && !FIGHTING(victim))
                violence = TRUE;
            break;

        case PSIONIC_RESILIANCE:

            if (affected_by_psionic(victim, PSIONIC_DEGRADE)) {
                to_char = "$N appears less vulnerable.\r\n";
                to_vict = "You feel less vulnerable.\r\n";
                psionic = PSIONIC_DEGRADE;
            }
            else if (affected_by_psionic(victim, PSIONIC_MEGA_DEGRADE)) {

                if (!IS_REMORT(ch)) {
                    // another check to make it harder on non remorts
                    if (rand_number(0,31) < GET_LEVEL(ch)) {
                        send_to_char(ch, "The psionic resists your attempt!\r\n");
                        return;
                    }
                }

                to_char = "$N appears much less vulnerable.\r\n";
                to_vict = "You feel much less vulnerable.\r\n";
                psionic = PSIONIC_MEGA_DEGRADE;
            }
            else {
                send_to_char(ch, "Your psionic has no effect!\r\n");
                return;
            }

            if (psi_savingthrow(ch, victim, 0, 0)) {
                send_to_char(ch, "The psionic resists your attempt!\r\n");
                return;
            }

            break;

        default:
            log("SYSERR: unknown psi_num %d passed to psi_unaffects", psi_num);
            return;
    }

    // the affected_by_psionic check has been moved into each switch case so each message
    // can be more specific
    affect_from_char(victim, psionic, TRUE);

	if (obj == NULL) {
		int exp = (rand_number(-250,250) + ((psionics_info[psi_num].min_level * 25 * (GET_LEVEL(ch) / 2))));
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), exp, ch);
	}
    
	if (to_vict != NULL)
        act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
    if (to_room != NULL)
        act(to_room, TRUE, victim, 0, ch, TO_ROOM);
    if (to_char != NULL)
        act(to_char, TRUE, ch, 0, victim, TO_CHAR);
    if (violence) {
		int thaco = rand_number(1,30);
        hit(victim, ch, TYPE_UNDEFINED, thaco, 0);
	}
}

void psi_alter_objs(int psi_level, struct char_data *ch, struct obj_data *obj, int psi_num)
{
    const char *to_char = NULL;
    const char *to_room = NULL;
    int i = 0;
	int j = 0;
	int d = 0;
	int newbonus = 0;
    int bonusnum = 0;
	int chance = 0;
	int purdmg = 0;
	int numimbue = 0;
	int freak = 0;
	int loc = 0;
	bool ok = TRUE;
	int dmg;

    if (obj == NULL)
        return;

    switch (psi_num) {

        case PSIONIC_IMBUE_ESSENCE:
            
			if (OBJ_FLAGGED(obj, ITEM_PSIONIC)) {
				send_to_char(ch, "That item has already been imbued once before.\r\n");
				ok = FALSE;
                return;
			}

            for (i = 0; i < MAX_OBJ_APPLY; i++) {
                if (GET_OBJ_APPLY_LOC(obj, i) != APPLY_NONE) {
					send_to_char(ch, "That item can not be imbued.\r\n");
					ok = FALSE;
                    return;
				}
			}
			
			if (!ok) {
				send_to_char(ch, "Something went terribly wrong.\r\n");
				return;
			}
			else {
			
				SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_PSIONIC);
				d = rand_number(0,100);

				if (d < 6) {
					send_to_char(ch, "@RThe imbued item heats up to a glowing red, and then vaporizes hitting you with shrapnel!\r\n");
					dmg = GET_LEVEL(ch) * (rand_number(50, 1000));
					obj_from_char(obj);
					extract_obj(obj);
					obj_damage_char(obj, ch, dmg);
					update_pos(ch);
					return;
				}
				if (d >= 6 && d < 70)
					numimbue = 1;
				if (d >= 70 && d < 80)
					numimbue = 2;
				if (d >= 80 && d < 90)
					numimbue = 3;
				if (d >= 90 && d < 96)
					numimbue = 4;
				if (d >= 96) {
					numimbue = 4;
					int fk = rand_number(1,100);
					if (fk > 90)
						freak = 1;
					else 
						freak = 0;
				}	

				for (j = 0; j < numimbue; j++) {
					int random = rand_number(1, 30);
					
					if (random == 1) {
						loc = APPLY_STR;
						bonusnum = rand_number(-2, 2);
					}
					else if (random == 2) {
						loc = APPLY_DEX;
						bonusnum = rand_number(-1, 1);
					}
					else if (random == 3) {
						loc = APPLY_INT;
						bonusnum = rand_number(-1, 1);
					}
					else if (random == 4) {
						loc = APPLY_PCN;
						bonusnum = rand_number(-1, 1);
					}
					else if (random == 5) {
						loc = APPLY_CON;
						bonusnum = rand_number(-1, 1);
					}
					else if (random == 6) {
						loc = APPLY_CHA;
						bonusnum = rand_number(-1, 1);
					}
					else if (random == 7) {
						loc = APPLY_AGE;
						bonusnum = rand_number(-10, 10);
					}
					else if (random == 8) {
						loc = APPLY_CHAR_WEIGHT;
						bonusnum = rand_number(-10, +10);
					}
					else if (random == 9) {
						loc = APPLY_CHAR_HEIGHT;
						bonusnum = rand_number(-10, +10);
					}
					else if (random == 10) {
						loc = APPLY_PSI;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 11) {
						loc = APPLY_HIT;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 12) {
						loc = APPLY_MOVE;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 13) {
						loc = APPLY_ARMOR;
						bonusnum = rand_number(-30, 30);
					}
					else if (random == 14) {
						loc = APPLY_HITROLL;
						bonusnum = rand_number(-15, 15);
					}
					else if (random == 15) {
						loc = APPLY_DAMROLL;
						bonusnum = rand_number(-15, 15);
					}
					else if (random == 16) {
						loc = APPLY_SAVING_PSIONIC;
						bonusnum = rand_number(-20, 20);
					}
					else if (random == 17) {
						loc = APPLY_PSI_REGEN;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 18) {
						loc = APPLY_HIT_REGEN;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 19) {
						loc = APPLY_MOVE_REGEN;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 20) {
						loc = APPLY_HITNDAM;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 21) {
						loc = APPLY_REGEN_ALL;
						bonusnum = rand_number(-50, 50);
					}
					else if (random == 22) {
						loc = APPLY_SAVING_DRUG;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 23) {
						loc = APPLY_SAVING_POISON;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 24) {
						loc = APPLY_SAVING_FIRE;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 25) {
						loc = APPLY_SAVING_COLD;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 26) {
						loc = APPLY_SAVING_ELECTRICITY;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 27) {
						loc = APPLY_SAVING_EXPLOSIVE;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 28) {
						loc = APPLY_SAVING_PSIONIC;
						bonusnum = rand_number(-4, 4);
					}
					else if (random == 29) {
						loc = APPLY_SAVING_NONORM;
						bonusnum = rand_number(-4, 4);
					}
					else {
						loc = APPLY_HPV;
						bonusnum = rand_number(-50, 50);
					}

					if (GET_OBJ_LEVEL(obj) <= 10)
						newbonus = bonusnum / 4;
					else if (GET_OBJ_LEVEL(obj) <= 20)
						newbonus = bonusnum / 3;
					else if (GET_OBJ_LEVEL(obj) <= 30)
						newbonus = bonusnum / 2;
					else 
						newbonus = bonusnum;

					if (newbonus == 0)
						newbonus = 1;

					if (freak == 1) {
						GET_OBJ_APPLY_LOC(obj, j) = loc;
						GET_OBJ_APPLY_MOD(obj, j) = newbonus * 2;
						to_char = "$p glows a brilliant white as something went wrong with your essence.";
						to_room = "$p glows a brilliant white as something went wrong with $n's essence.";
					}
					else {
						GET_OBJ_APPLY_LOC(obj, j) = loc;
						GET_OBJ_APPLY_MOD(obj, j) = newbonus;
						to_char = "$p starts to hum with your essence.";
						to_room = "$p starts to hum with $n's essence.";

					}

				}
			}
            break;

        case PSIONIC_INVISIBILITY:
            if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
                SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
                to_char = "$p vanishes.";
            }
            break;

        case PSIONIC_POISON:
            if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
                (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
                (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
                GET_OBJ_VAL(obj, 3) = 1;
                to_char = "$p steams briefly.";
            }
            break;

		case PSIONIC_PURIFY:
			if (!OBJ_FLAGGED(obj, ITEM_NODROP))
				to_char = "You don't need to purify an object that is not cursed.";
			else {
				chance = rand_number(1, 100) + GET_PCN(ch);
				if (chance > 50 || GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
					to_char = "@BA brilliant blue glow surrounds $p as you try and strip the curse off of it.@n";
					REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
				}
				else {
					purdmg = rand_number(GET_LEVEL(ch), (GET_LEVEL(ch) * GET_LEVEL(ch)));
					to_char = "@W$p flashes a brilliant white light and explodes!@n";
					send_to_char(ch, "@R[%d] Your attempt to purify damages you considerably!\r\n", purdmg);
					send_to_char(ch, "@RThat really did HURT!@n\r\n");
					to_room = "@WA brilliant white glow surrounds $p as it blows up in $n's face!\r\n";
					obj_damage_char(obj, ch, purdmg);
					update_pos(ch);
					extract_obj(obj);
				}
			}
			break;

        default:
            send_to_char(ch, "Psionic unimplemented, it would seem.\r\n");
            log("Unimplemented psionic called inside psi_alter_objs: %d", psi_num);
            return;

    }

    if (to_char == NULL)
        send_to_char(ch, "%s", CONFIG_NOEFFECT);

    else 
        act(to_char, TRUE, ch, obj, 0, TO_CHAR);
	
	if (to_room != NULL)
        act(to_room, TRUE, ch, obj, 0, TO_ROOM);
	if (obj == NULL) {
		int exp = (rand_number(-250,250) + ((psionics_info[psi_num].min_level * 10 * (GET_LEVEL(ch) / 2))));
		increment_skilluse(psi_num, 0, GET_IDNUM(ch), exp, ch);
	}


}

void psi_creations(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj)
{
    struct obj_data *tobj;
    obj_vnum z;

    if (ch == NULL)
        return;

    switch (psi_num) {
        case PSIONIC_MYSTIC_BULB:
            z = 1289;
            break;

        default:
            send_to_char(ch, "Psionic unimplemented, it would seem.\r\n");
            log("Unimplemented psionic called inside psi_creations: %d", psi_num);
            return;
    }

    if (!(tobj = read_object(z, VIRTUAL))) {
        send_to_char(ch, "I seem to have goofed.\r\n");
        log("SYSERR: psi_creations, psionic %d, obj %d: obj not found", psi_num, z);
        return;
    }

    obj_to_char(tobj, ch);
    act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
    load_otrigger(tobj);
}
