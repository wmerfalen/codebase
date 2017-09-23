/**************************************************************************
*  File: museum.c                                    Part of CyberASSAULT *
*  Usage: Special functions that handle the Imm museum                    *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "fight.h"
#include "act.h"

#define MUSEUM_ZONE 100

#define C_MOB_SPEC(zone,mob) (mob_index[real_mobile(((zone)*100)+(mob))].func)
#define R_MOB(zone, mob) (real_mobile(((zone)*100)+(mob)))
#define R_OBJ(zone, obj) (real_object(((zone)*100)+(obj)))
#define R_ROOM(zone, num) (real_room(((zone)*100)+(num)))

#define MUSEUM_ITEM(item) (MUSEUM_ZONE*100+(item))

void update_pos(struct char_data * victim);
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void raw_kill(struct char_data *ch, struct char_data *killer, bool gibblets);
SPECIAL(apok_statue);
SPECIAL(gabriel_statue);
SPECIAL(zar_statue);
SPECIAL(tick_statue);
SPECIAL(imm_statue);
SPECIAL(imp_statue);


// Routine assign_statues
// Used to assign function pointers to all mobiles in the Museum
// Add a call to it from spec_assign.c!

void assign_statues(void)
{
    ASSIGNMOB(10000, apok_statue);       // Apok
    ASSIGNMOB(10001, imm_statue);        // Estrella
    ASSIGNMOB(10002, imm_statue);        // Techno
    ASSIGNMOB(10003, imm_statue);        // Krondor
    ASSIGNMOB(10004, imm_statue);        // Ash
    ASSIGNMOB(10005, imp_statue);        // Uatu
    ASSIGNMOB(10006, imm_statue);        // Shaft
    ASSIGNMOB(10007, imm_statue);        // Jinx
    ASSIGNMOB(10008, imp_statue);        // Morden
    ASSIGNMOB(10009, gabriel_statue);    // Gabriel
    ASSIGNMOB(10010, imm_statue);        // Sweeto
    ASSIGNMOB(10011, imm_statue);        // Strat
    ASSIGNMOB(10012, imm_statue);        // Dexter
    ASSIGNMOB(10013, imp_statue);        // Tails
    ASSIGNMOB(10014, zar_statue);        // Zar
    ASSIGNMOB(10015, imm_statue);        // Felix
    ASSIGNMOB(10016, tick_statue);       // Tick
    ASSIGNMOB(10017, imm_statue);        // Devon
}

void mob_mega_sanct(struct char_data *ch)
{
    if (GET_POS(ch) < POS_FOCUSING) return;

    if (!affected_by_psionic(ch, PSIONIC_MEGA_SANCT)) {
        act("$n says, 'Bah! Where's my golden glow?'", FALSE, ch, 0, 0, TO_ROOM);
        act("$n is surrounded by a golden aura.", FALSE, ch, 0, 0, TO_ROOM);
        call_psionic(ch, ch, NULL, PSIONIC_MEGA_SANCT, 6, CAST_PSIONIC, TRUE);
        return;
    }

    return;
}

void mob_cheater_heal(struct char_data *ch)
{
    int i = 0;

    if (GET_POS(ch) < POS_FOCUSING) return;

    act("$n grins and says, 'The best part about being immortal is that I can cheat!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n utters the words 'Pzar Ultimus!'", FALSE, ch, 0, 0, TO_ROOM);

    for (i = 0; i < 3; i++) {
        cast_psionic(ch, ch, NULL, PSIONIC_MEGAHEAL);
        act("$n heals $mself.", FALSE, ch, 0, 0, TO_ROOM);
    }

    return;
}

void mob_psionic_up(struct char_data *ch)
{

    if (GET_POS(ch) < POS_FOCUSING) return;

    if (!affected_by_psionic(ch, PSIONIC_FANATICAL_FURY) && !affected_by_psionic(ch, PSIONIC_INDESTRUCTABLE_AURA)) {
        act("$n says, 'What is this foolishness? Teaming up on me?'", FALSE, ch, 0, 0, TO_ROOM);
        cast_psionic(ch, ch, NULL, PSIONIC_INDESTRUCTABLE_AURA);
        act("$n is surrounded by an indestructible aura.", FALSE, ch, 0, 0, TO_ROOM);
        cast_psionic(ch, ch, NULL, PSIONIC_FANATICAL_FURY);
        act("$n's fury becomes fanatical!", FALSE, ch, 0, 0, TO_ROOM);
    }

    return;
}

void mob_counter_sanct(struct char_data *ch)
{
    struct char_data *victim;

    victim = FIGHTING(ch);

    if (GET_POS(ch) != POS_FIGHTING) return;

    if (!victim) return;

    if (affected_by_psionic(victim, PSIONIC_SANCTUARY)) {
        act("$n says, 'Let's see how well YOU fight without sanctuary!'", FALSE, ch, 0, 0, TO_ROOM);
        act("$n points at $N", FALSE, ch, 0, victim, TO_NOTVICT);
        send_to_char(victim, "Your sanctuary has been countered!\n\r");
        cast_psionic(ch, victim, NULL, PSIONIC_COUNTER_SANCT);
        return;
    }

    return;
}

void mob_counter_super_sanct(struct char_data *ch)
{
    struct char_data *victim;

    victim = FIGHTING(ch);

    if (GET_POS(ch) != POS_FIGHTING) return;

    if (!victim) return;

    if (affected_by_psionic(victim, PSIONIC_SUPER_SANCT)) {
        act("$n says, 'Bah! You don't need that puny supersanctuary!'", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(victim, "Your sanctuary has been countered!\n\r");
        cast_psionic(ch, victim, NULL, PSIONIC_COUNTER_SUPER_SANCT);
        return;
    }

    return;
}


void mob_weaken(struct char_data *ch)
{
    struct char_data *victim;

    victim = FIGHTING(ch);

    if (GET_POS(ch) != POS_FIGHTING) return;

    if (!victim) return;

    if (!affected_by_psionic(victim, PSIONIC_WEAKEN)) {
        act("$n says, 'You're too strong! You need to be WEAK!'", FALSE, ch, 0, 0, TO_ROOM);
        cast_psionic(ch, victim, NULL, PSIONIC_WEAKEN);
        return;
    }

    return;
}

void mob_psychic_leech(struct char_data *ch)
{
    struct char_data *victim;

    if (GET_POS(ch) != POS_FIGHTING) return;

    act("$n says, 'Bah... too many psi points makes the fight go badly.'", FALSE, ch, 0, 0, TO_ROOM);
    for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room) {
        if (!IS_NPC(victim))
            GET_PSI(victim) *= .85;
        send_to_char(victim, "You feel your psi points being drained!\n\r");
        return;
    }
    return;
}

void mob_mirror_image(struct char_data *ch)
{
    if (GET_POS(ch) < POS_FIGHTING) return;

    if (!affected_by_psionic(ch, PSIONIC_DUPLICATES)) {
        act("$n says, 'I hate getting hit.'", FALSE, ch, 0, 0, TO_ROOM);
        cast_psionic(ch, ch, NULL, PSIONIC_DUPLICATES);
        return;
    }

    return;
}

void pzar_maximus(struct char_data *ch)
{
    struct char_data *victim;

    if (GET_POS(ch) < POS_FOCUSING) return;

    victim = FIGHTING(ch);
    if (!victim) return;
    else {
        act("$n says, 'I'm sorry... Did I hurt you?", FALSE, ch, 0, 0, TO_ROOM);
        act("$n utters the words 'pzar maximus'", FALSE, ch, 0, 0, TO_ROOM);
        GET_HIT(victim) = GET_MAX_HIT(victim);
        update_pos(victim);
        char_from_room(victim);
        char_to_room(victim, real_room(3001));
        do_look(victim, " ", 15, 0);
        return;
    }
    return;
}

void zar_panther_pounce(struct char_data *ch)
{
    struct char_data *victim;

    if (GET_POS(ch) < POS_FOCUSING) return;

    victim = FIGHTING(ch);

    if (!victim) return;
    else {
        act("$n growls like a panther.", FALSE, ch, 0, 0, TO_ROOM);
        act("You pounce on $N!", TRUE, ch, 0, victim, TO_CHAR);
        act("$n pounces on $N!", TRUE, ch, 0, victim, TO_NOTVICT);
        act("$n pounces on you!", TRUE, ch, 0, victim, TO_VICT);
        send_to_char(victim, "Ouch! That really did HURT!\n\r");
        damage(ch, victim, 310, TYPE_UNDEFINED, DMG_NORMAL);
    }
    return;
}

void gabriel_dim_mak(struct char_data *ch)
{
    struct char_data *victim;
    int n = rand_number(1, 2);

    if (GET_POS(ch) < POS_FOCUSING) return;

    victim = FIGHTING(ch);

    if (!victim) return;
    else {
        act("$n says, 'Let me show you a little move I invented.'", FALSE, ch, 0, 0, TO_ROOM);
        act("$n performs the dim mak maneuver.", FALSE, ch, 0, 0, TO_ROOM);
        if (n == 2)
            raw_kill(victim, ch, FALSE);
    }
    return;
}

SPECIAL(zar_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) != POS_FIGHTING) return (FALSE);

    switch(rand_number(1, 10)) {

        case 1:
            mob_mega_sanct(ch);
            break;

        case 2:
            mob_cheater_heal(ch);
            break;

        case 3:
            mob_psionic_up(ch);
            break;

        case 4:
            mob_counter_sanct(ch);
            break;

        case 5:
            mob_counter_super_sanct(ch);
            break;

        case 6:
            mob_weaken(ch);
            break;

        case 7:
            mob_psychic_leech(ch);
            break;

        case 8:
            mob_mirror_image(ch);
            break;

        case 9:
            zar_panther_pounce(ch);
            break;

        case 10:
            pzar_maximus(ch);
            break;

        default:
            break;
    }

    return (FALSE);
}

SPECIAL(gabriel_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) != POS_FIGHTING) return (FALSE);

    switch(rand_number(1, 10)) {

        case 1:
            mob_mega_sanct(ch);
            break;

        case 2:
            mob_cheater_heal(ch);
            break;

        case 3:
            mob_psionic_up(ch);
            break;

        case 4:
            mob_counter_sanct(ch);
            break;

        case 5:
            mob_counter_super_sanct(ch);
            break;

        case 6:
            mob_weaken(ch);
            break;

        case 7:
            mob_psychic_leech(ch);
            break;

        case 8:
            mob_mirror_image(ch);
            break;

        case 9:
            gabriel_dim_mak(ch);
            break;

        default:
            break;
    }

    return (FALSE);
}

SPECIAL(apok_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) != POS_FIGHTING) {

        switch(rand_number(1, 4)) {

            case 1:
                act("$n says, 'Where's my pillow?'", FALSE, ch, 0, 0, TO_ROOM);
                break;

            case 2:
                act("$n yawns.", FALSE, ch, 0, 0, TO_ROOM);
                break;

            default:
                break;
        }

        return (FALSE);
    }

    switch(rand_number(1, 10)) {

            case 1:
                mob_mega_sanct(ch);
                break;

            case 2:
                mob_cheater_heal(ch);
                break;

            case 3:
                mob_psionic_up(ch);
                break;

            case 4:
                mob_counter_sanct(ch);
                break;

            case 5:
                mob_counter_super_sanct(ch);
                break;

            case 6:
                mob_weaken(ch);
                break;

            case 7:
                mob_psychic_leech(ch);
                break;

            case 8:
                mob_mirror_image(ch);
                break;

            default:
                break;

        }

    return (FALSE);
}

SPECIAL(imm_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) == POS_FIGHTING) {

        switch(rand_number(1, 20)) {

            case 1:
                mob_mega_sanct(ch);
                break;

            case 2:
                mob_cheater_heal(ch);
                break;

            case 3:
                mob_psionic_up(ch);
                break;

            case 4:
                mob_counter_sanct(ch);
                break;

            case 5:
                mob_counter_super_sanct(ch);
                break;

            case 6:
                mob_weaken(ch);
                break;

            case 7:
                mob_psychic_leech(ch);
                break;

            case 8:
                mob_mirror_image(ch);
                break;

            default:
                act("$n grins evilly.", FALSE, ch, 0, 0, TO_ROOM);
                break;
        }

    }

    return (TRUE);
}

SPECIAL(imp_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) == POS_FIGHTING) {

        switch(rand_number(1, 10)) {

            case 1:
                mob_mega_sanct(ch);
                break;

            case 2:
                mob_cheater_heal(ch);
                break;

            case 3:
                mob_psionic_up(ch);
                break;

            case 4:
                mob_counter_sanct(ch);
                break;

            case 5:
                mob_counter_super_sanct(ch);
                break;

            case 6:
                mob_weaken(ch);
                break;

            case 7:
                mob_psychic_leech(ch);
                break;

            case 8:
                mob_mirror_image(ch);
                break;

            default:
                act("$n grins evilly.", FALSE, ch, 0, 0, TO_ROOM);
                break;
        }
    }

    return (TRUE);
}

SPECIAL(tick_statue)
{
    if (cmd) return (FALSE);

    if (GET_POS(ch) != POS_FIGHTING) {

        switch(rand_number(1, 4)) {

            case 1:
                act("$n says, 'Spoon!'", FALSE, ch, 0, 0, TO_ROOM);
                break;

            case 2:
                act("$n says, 'Where's Arthur?'.", FALSE, ch, 0, 0, TO_ROOM);
                break;

            default:
                break;
        }

        return (FALSE);
    }

    if (GET_POS(ch) == POS_FIGHTING) {

        switch(rand_number(1, 10)) {

            case 1:
                mob_mega_sanct(ch);
                break;

            case 2:
                mob_cheater_heal(ch);
                break;

            case 3:
                mob_psionic_up(ch);
                break;

            case 4:
                mob_counter_sanct(ch);
                break;

            case 5:
                mob_counter_super_sanct(ch);
                break;

            case 6:
                mob_weaken(ch);
                break;

            case 7:
                mob_psychic_leech(ch);
                break;

            case 8:
                mob_mirror_image(ch);
                break;

            default:
                break;
        }
    }

    return (FALSE);
}



