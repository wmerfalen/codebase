/**************************************************************************
*  File: spec_procs.c                                      Part of tbaMUD *
*  Usage: Implementation of special procedures for mobiles/objects/rooms. *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* For more examples: 
 * ftp://ftp.circlemud.org/pub/CircleMUD/contrib/snippets/specials */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "constants.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "skilltree.h"
#include "fight.h"
#include "modify.h"
#include "drugs.h"
#include "bionics.h"

// locally defined functions of local (file) scope
static int compare_psionics(const void *x, const void *y);
static void npc_steal(struct char_data *ch, struct char_data *victim);
bool check_class(int prereq, int tier, struct char_data *ch);

// Special procedures for mobiles
int *psionics_sort_info = NULL;
int *skills_sort_info = NULL;

#define PSIONIC    0
#define SKILL    1

static int compare_psionics(const void *x, const void *y)
{
    int a = *(const int *)x;
    int b = *(const int *)y;

    return (strcmp(psionics_info[a].name, psionics_info[b].name));
}

static int compare_skills(const void *x, const void *y)
{
    int a = *(const int *)x;
    int b = *(const int *)y;
    return (strcmp(skills_info[a].name, skills_info[b].name));
}

void sort_psionics(void)
{
    int a;
	CREATE(psionics_sort_info, int, NUM_PSIONICS+1);
    // initialize array, avoiding reserved
    for (a = 1; a <= NUM_PSIONICS; a++)
        psionics_sort_info[a] = a;
    // Don't sort the RESERVED or \n entries.
    qsort(psionics_sort_info + 1, NUM_PSIONICS - 1, sizeof(int), compare_psionics);
}

void sort_skills(void)
{
    int a;
	CREATE(skills_sort_info, int, NUM_SKILLS+1);
    // initialize array, avoiding reserved
    for (a = 1; a <= NUM_SKILLS; a++)
        skills_sort_info[a] = a;
    // Don't sort the RESERVED or \n entries.
    qsort(skills_sort_info + 1, NUM_SKILLS - 1, sizeof(int), compare_skills);
}

char *how_good(int level)
{
    if (level == 0)
        return (" (@Rnot learned@n) ");
	if (level == 1)
		return (" (@MBasic@n)       ");
	if (level == 2)
		return (" (@YIntermediate@n)");
	if (level == 3)
		return (" (@BProficient@n)  ");
	if (level == 4)
		return (" (@WAdvanced@n)    ");
	if (level == 5)
		return (" (@GMastered@n)    ");
	if (level == 100)
		return (" (@GGodly@n)       ");
    else
        return (" (learned)");
}

const char *prac_types[] = {
    "psionic",
    "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

// actual prac_params are in class.c
// todo: combine psionics and psionics_info and psionic_costs
bool check_class(int prereq, int tier, struct char_data *ch)
{
	if (GET_TIER(ch) < tier)
		return (FALSE);
	else if (prereq == 0 && !IS_MERCENARY(ch))
		return (FALSE);
	else if (prereq == 1 && !IS_CRAZY(ch))
		return (FALSE);
	else if (prereq == 2 && !IS_STALKER(ch))
		return (FALSE);
	else if (prereq == 3 && !IS_BORG(ch))
		return (FALSE);
	else if (prereq == 4 && !IS_HIGHLANDER(ch))
		return (FALSE);
	else if (prereq == 5 && !IS_PREDATOR(ch))
		return (FALSE);
	else if (prereq == 6 && !IS_CALLER(ch))
		return (FALSE);
	else
		return (TRUE);

}

// New practice list function by gahan - 2013
void new_practice_list(struct char_data *ch, int tree) {
	size_t len = 0;
	size_t nlen;
	char buf[MAX_STRING_LENGTH];
	int i,j,k,n,class,level;
	bool showall;

	if (tree == -1) {
		class = GET_CLASS(ch);
		level = GET_LEVEL(ch);
		showall = FALSE;
	}
	else {
		class = tree;
		level = 40;
		showall = TRUE;
	}
	if (showall == FALSE)
		send_to_char(ch, "@YYou can learn the following skills and psionics:@n\r\n\r\n");
	else
		send_to_char(ch, "@YListing comprehensive skill and psionics list for %s:@n\r\n\r\n", pc_class[class].class_name);

	for (i=0;i<MAX_CLASS_SKILLTREES;i++) {
		if (showall == FALSE)
			j = pc_class[GET_CLASS(ch)].skill_tree[i];
		else
			j = pc_class[class].skill_tree[i];
		n = 0;
		for (k=0;k<MAX_SKILLSNPSIONIC;k++) {
			if (full_skill_tree[k].tree == j) {
				if (check_class(full_skill_tree[k].pcclass, full_skill_tree[k].tier, ch) || showall == TRUE) {
					if (full_skill_tree[k].skill_level <= level) {
						if (n == 0) {
							nlen = snprintf(buf + len, sizeof(buf) - len, "  %-39s Cost    Skill         Level@n\r\n", skill_tree_names[j]);
							len += nlen;
							nlen = snprintf(buf + len, sizeof(buf) - len, "@W--------------------------------------------------------------------------@n\r\n");
							len += nlen;
						}
						if (full_skill_tree[k].psi_or_skill == 0) {
							nlen = snprintf(buf + len, sizeof(buf) - len, "  %-40s %-4d %-15s %3d\r\n",full_skill_tree[k].display, full_skill_tree[k].practice_cost, how_good(GET_PSIONIC_LEVEL(ch, full_skill_tree[k].skillnum)), full_skill_tree[k].skill_level);
							if (len + nlen >= sizeof(buf) || nlen < 0)
								break;
							len += nlen;
						}
						else {
							nlen = snprintf(buf + len, sizeof(buf) - len, "  %-40s %-4d %-15s %3d\r\n",full_skill_tree[k].display, full_skill_tree[k].practice_cost, how_good(GET_SKILL_LEVEL(ch, full_skill_tree[k].skillnum)), full_skill_tree[k].skill_level);
							if (len + nlen >= sizeof(buf) || nlen < 0)
								break;
							len += nlen;
						}
						n++;
					}
				}
			}
		}
		if (n > 0) {
			nlen = snprintf(buf + len, sizeof(buf) - len, "\r\n");
			len += nlen;
		}
	}
	if (showall == FALSE) {
		nlen = snprintf(buf + len, sizeof(buf) - len, "\r\n@YYou currently have @W%d@Y practice sessions left.@n\r\n\r\n", GET_PRACTICES(ch));
		len += nlen;
	}
	page_string(ch->desc, buf, TRUE);
	return;
}

// Re-written to support new class/skill code: Gahan 2012
SPECIAL(guild)
{
    int skill_num;
    int psionic_num = 0;
    int cost = 0;
	int prereq;
	int prereqid;
	int i,j;
	bool foundpsionic = FALSE;
	bool foundskill = FALSE;

    if (IS_NPC(ch) || !CMD_IS("practice"))
        return (FALSE);

    skip_spaces(&argument);

	// Display this when they supply no argument

    if (!*argument) {
		new_practice_list(ch, -1);
        return (TRUE);
    }
	
    skill_num = find_skill_num(argument);

    if (skill_num < 1) {

        // check if selection is a psionic
        psionic_num = find_psionic_num(argument);

        if (psionic_num < 1 || psionics_info[psionic_num].min_level > LVL_IMMORT || !strcmp("!UNUSED!", psionics_info[psionic_num].name)) {
            send_to_char(ch, "Sorry, you cannot practice that.\r\n");
            return (TRUE);
        }
		
		for (i = 0; i < MAX_CLASS_SKILLTREES; i++)
			if (psionics_info[psionic_num].tree == pc_class[GET_CLASS(ch)].skill_tree[i])
				foundpsionic = TRUE;
		
		if (foundpsionic == FALSE) {
			send_to_char(ch, "Sorry, you cannot practice that.\r\n");
			return (TRUE);
		}
		if (GET_LEVEL(ch) < psionics_info[psionic_num].min_level) {
			send_to_char(ch, "That psionic is a little beyond your capabilities right now.\r\n");
			return (TRUE);
		}
        if (GET_PSIONIC_LEVEL(ch, psionic_num) > 0) {
            send_to_char(ch, "You have already studied this to your highest potential.\n\r");
            return (TRUE);
        }
        if (psionics_info[psionic_num].remort && !IS_REMORT(ch)) {
            send_to_char(ch, "That psionic is far beyond your capabilities!\r\n");
            return (TRUE);
        }

		prereq = psionics_info[psionic_num].prereq;
		prereqid = psionics_info[psionic_num].prereqid;

		if (!check_class(psionics_info[psionic_num].pcclass, psionics_info[psionic_num].tier, ch)) {
			send_to_char(ch, "Sorry, you cannot learn that psionic.\r\n");
			return (TRUE);
		}

		if (prereq > 0) {
			if (prereqid == 0) {
				if (GET_PSIONIC_LEVEL(ch, prereq) < 1) {
					send_to_char(ch, "You need to learn %s first before you can learn that.\r\n", psionics_info[prereq].name);
					return (TRUE);
				}
			}
			else {
				if (GET_SKILL_LEVEL(ch, prereq) < 1) {
					send_to_char(ch, "You need to learn %s first before you can learn that.\r\n", skills_info[prereq].name);
					return (TRUE);
				}
			}
		}

        cost = psionics_info[psionic_num].prac_cost;

        // -1 means the char is not allowed to practice this skill
        if (cost <= -1) {
            send_to_char(ch, "Sorry, you cannot practice that.");
            return (TRUE);
        }

        if (cost > GET_PRACTICES(ch)) {
           send_to_char(ch, "You don't have enough practices to learn that.\r\n");
           return (TRUE);
        }

		else {
	        GET_PRACTICES(ch) -= cost;
		    send_to_char(ch, "You spend %d practices and learn %s.\n\r", cost, psionics_info[psionic_num].name);
			SET_PSIONIC(ch, psionic_num, 1);
		}

    }
    else {

        if (GET_SKILL_LEVEL(ch, skill_num) > 0) {
            send_to_char(ch, "You have already studied this to your highest potential.\n\r");
            return (TRUE);
        }
		
		for (j = 0; j < MAX_CLASS_SKILLTREES; j++)
			if (skills_info[skill_num].tree == pc_class[GET_CLASS(ch)].skill_tree[j])
				foundskill = TRUE;
				
		if (foundskill == FALSE) {
			send_to_char(ch, "Sorry, you cannot practice that.\r\n");
			return (TRUE);
		}
        if (skill_num < 1 || skills_info[skill_num].min_level > LVL_IMMORT || !strcmp("!UNUSED!", skills_info[skill_num].name)) {
            send_to_char(ch, "Sorry, you cannot practice that.\r\n");
            return (TRUE);
        }
		if (GET_LEVEL(ch) < skills_info[skill_num].min_level) {
			send_to_char(ch, "That skill is a little beyond your capabilities right now.\r\n");
			return (TRUE);
		}
        if (skills_info[skill_num].remort && !IS_REMORT(ch)) {
            send_to_char(ch, "That skill is far beyond your capabilities!\r\n");
            return (TRUE);
        }

		prereq = skills_info[skill_num].prereq;
		prereqid = skills_info[skill_num].prereqid;

		if (prereq > 0) {
			if (prereqid == 0) {
				if (GET_PSIONIC_LEVEL(ch, prereq) < 1) {
					send_to_char(ch, "You need to learn %s first before you can learn that.\r\n", psionics_info[prereq].name);
					return (TRUE);
				}
			}
			else {
				if (GET_SKILL_LEVEL(ch, prereq) < 1) {
					send_to_char(ch, "You need to learn %s first before you can learn that.\r\n", skills_info[prereq].name);
					return (TRUE);
				}
			}
		}

		if (!check_class(skills_info[skill_num].pcclass, skills_info[psionic_num].tier, ch)) {
			send_to_char(ch, "Sorry, you cannot learn that skill.\r\n");
			return (TRUE);
		}

        cost = skills_info[skill_num].prac_cost;

        // -1 means the class is not allowed to practice this skill
        if (cost <= -1) {
            send_to_char(ch, "Sorry, you cannot practice that.\r\n");
            return (TRUE);
        }

        if (cost > GET_PRACTICES(ch)) {
           send_to_char(ch, "You don't have enough practices to learn that.\r\n");
           return (TRUE);
        }

		else {
	        GET_PRACTICES(ch) -= cost;
		    send_to_char(ch, "You spend %d practices and learn %s.\n\r", cost, skills_info[skill_num].name);
			SET_SKILL(ch, skill_num, 1);
		}
    }
    return (TRUE);
}

SPECIAL(dump)
{
    struct obj_data *k;
    int value = 0;

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return (FALSE);

    do_drop(ch, argument, cmd, SCMD_DROP);

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
        value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
        extract_obj(k);
    }

    if (value) {

        send_to_char(ch, "You are awarded for outstanding performance.\r\n");
        act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

        if (GET_LEVEL(ch) < 3)
            gain_exp(ch, value, FALSE);
        else
            GET_UNITS(ch) += value;
    }

    return (TRUE);
}

/* ********************************************************************
*  Special procedures for mobs                                       *
******************************************************************** */

SPECIAL(mayor)
{
    char actbuf[MAX_INPUT_LENGTH];

    const char open_path[] =
        "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
    const char close_path[] =
        "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path = NULL;
    static int path_index;
    static bool move = FALSE;

    if (!move) {
        if (time_info.hours == 6) {
            move = TRUE;
            path = open_path;
            path_index = 0;
        }
        else if (time_info.hours == 20) {
            move = TRUE;
            path = close_path;
            path_index = 0;
        }
    }

    if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_FIGHTING))
        return (FALSE);

    switch (path[path_index]) {

        case '0':
        case '1':
        case '2':
        case '3':
            perform_move(ch, path[path_index] - '0', 1);
            break;

        case 'W':
            GET_POS(ch) = POS_STANDING;
            act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'S':
            GET_POS(ch) = POS_SLEEPING;
            act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'a':
            act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
            act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'b':
            act("$n says 'What a view!  I must get something done about that dump!'",
            FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'c':
            act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
            FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'd':
            act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'e':
            act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'E':
            act("$n says 'I hereby declare Motown closed!'", FALSE, ch, 0, 0, TO_ROOM);
            break;

        case 'O':
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);    /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);    /* strcpy: OK */
            break;

        case 'C':
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);    /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);    /* strcpy: OK */
            break;

        case '.':
            move = FALSE;
            break;

    }

    path_index++;
    return (FALSE);
}

/* General special procedures for mobiles. */

static void npc_steal(struct char_data *ch, struct char_data *victim)
{
    int units;

    if (IS_NPC(victim))
        return;

    if (GET_LEVEL(victim) >= LVL_IMMORT)
        return;

    if (!CAN_SEE(ch, victim))
        return;

    if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
        act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
        act("$n tries to steal units from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    }
    else {
        // Steal some resource units
        units = (GET_UNITS(victim) * rand_number(1, 10)) / 100;
        if (units > 0) {
            GET_UNITS(ch) += units;
            GET_UNITS(victim) -= units;
        }
    }
}

/* Quite lethal to low-level characters. */
SPECIAL(snake)
{
  if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_psionic(ch, FIGHTING(ch), 0, PSIONIC_POISON, GET_LEVEL(ch), CAST_PSIONIC, TRUE);
  return (TRUE);
}

SPECIAL(stalker)
{
    struct char_data *cons;

    if (cmd || GET_POS(ch) != POS_STANDING)
        return (FALSE);

    for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
        if (!IS_NPC(cons) && GET_LEVEL(cons) < LVL_IMMORT && !rand_number(0, 4)) {
            npc_steal(ch, cons);
            return (TRUE);
        }

    return (FALSE);
}

SPECIAL(mercenary)
{
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return (FALSE);

    // pseudo-randomly choose someone in the room who is fighting me
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    // if I didn't pick any of those, then just slam the guy I'm fighting
    if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
        vict = FIGHTING(ch);

    // Hm...didn't pick anyone...I'll wait a round.
    if (vict == NULL)
        return (TRUE);

    if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
        cast_psionic(ch, vict, NULL, PSIONIC_INDUCE_SLEEP);

    if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
        cast_psionic(ch, vict, NULL, PSIONIC_BLINDNESS);

    // removed a few other psionics - we could add more here
    if (rand_number(0, 4))
        return (TRUE);

    return (TRUE);
}

SPECIAL(guard_west)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if (cmd == 4) {
        act("The Guard steps in front of $n, blocking $s way west.\r\n", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The Guard blocks you from going west.\r\n");
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(guard_east)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if (cmd == 2) {
        act("The Guard steps in front of $n, blocking $s way east.\r\n", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The Guard blocks you from going east.\r\n");
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(guard_south)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if (cmd == 3) {
        act("The Guard steps in front of $n, blocking $s way south.\r\n", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The Guard blocks you from going south..\r\n");
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(guard_north)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if (cmd == 1) {
        act("The Guard steps in front of $n, blocking $s way north.\r\n", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The Guard blocks you from going north..\r\n");
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(guardian)
{
    if (cmd > 6 || cmd < 1)
        return FALSE;

    if ((IN_ROOM(ch) == real_room(13067)) && (cmd == 1)) {
        act("An Ivory Guardian steps in front of $n, and blocks $s path.\n\r", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "An Ivory Guardian steps in front of you, and blocks your path.\n\r");
        return (TRUE);
    }

    return (FALSE);
}

// this is just a hack of the special(bouncer) -jack
SPECIAL(imm_museum_guard)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if ((IN_ROOM(ch) == real_room(10000)) && (cmd == 1) && !IS_REMORT(ch) && (GET_LEVEL(ch) < 31)) {
        act("The Guardian says, 'This hallowed ground is accessible only by those who have been reborn.'\n\r", TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
    }

    return (FALSE);
};

SPECIAL(bouncer)
{
    if (cmd > 6 || cmd < 1)
        return (FALSE);

    if ((IN_ROOM(ch)  == real_room(3024)) && (cmd == 3) && (GET_AGE(ch) <= 20) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        act("The bouncer says, 'We don't let children in the bar. Come back when you're 21!'\n\r", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The bouncer says, 'We don't let children in the bar. Come back when you're 21!'\n\r");
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(dummy)
{
   struct char_data *vict = NULL;

   if (!vict) return (FALSE);

    if (CMD_IS("grope")) {
        vict = world[IN_ROOM(ch)].people;
        cast_psionic(ch, vict, NULL, PSIONIC_SANCTUARY);
        act("The practice dummy sancts $n.\n\r", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The practice dummy says, 'Sanctuary halves damage.'\n\r");
        return (TRUE);
    }

    if (CMD_IS("stroke")) {
        vict = world[IN_ROOM(ch)].people;
        cast_psionic(ch, vict, NULL, PSIONIC_HEAL);
        act("The practice dummy heals $n.\n\r", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char(ch, "The practice dummy says, 'Have a heal on me!'\n\r");
        return (TRUE);
    }

    return (FALSE);
 }

// todo: should we re-implement this?
SPECIAL(justice)
{
//    struct char_data *vict;

//    if (cmd)
//        return (FALSE);

//    vict = world[IN_ROOM(ch)].people;
//    if (!IS_NPC(ch) && ch->player_specials->saved.criminal != 0) {
//        act("$n says, 'Justice will be served.'", FALSE, ch, 0, 0, TO_ROOM);
//        hit(ch, vict, TYPE_UNDEFINED);
//        return (TRUE);
//    }

    return (FALSE);
}

// i loved puff - why was he removed?
SPECIAL(puff)
{
    if (cmd)
        return (FALSE);

    switch (rand_number(0, 60)) {
        case 0:
            do_say(ch, "My god!  It's full of stars!", 0, 0);
            return (TRUE);
        case 1:
            do_say(ch, "How'd all those fish get up here?", 0, 0);
            return (TRUE);
        case 2:
            do_say(ch, "I'm a very female dragon.", 0, 0);
            return (TRUE);
        case 3:
            do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
            return (TRUE);
        default:
            return (FALSE);
    }
}

SPECIAL(fido)
{
    struct obj_data *i;
    struct obj_data *temp;
    struct obj_data *next_obj;

    if (cmd || !AWAKE(ch))
        return (FALSE);

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!IS_CORPSE(i))
            continue;

        act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);

        for (temp = i->contains; temp; temp = next_obj) {
            next_obj = temp->next_content;
            obj_from_obj(temp);
            obj_to_room(temp, IN_ROOM(ch));
        }

        extract_obj(i);
        return (TRUE);
    }
    return (FALSE);
}

SPECIAL(janitor)
{
    struct obj_data *i;

    if (cmd || !AWAKE(ch))
        return (FALSE);

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
            continue;

        if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
            continue;

        act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(cityguard)
{
    struct char_data *tch;
    struct char_data *evil;
    struct char_data *spittle;
    int max_evil;
    int min_cha;

    if (cmd || !AWAKE(ch) || FIGHTING(ch))
        return (FALSE);

    max_evil = 1000;
    min_cha = 6;
    spittle = evil = NULL;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {

        if (!CAN_SEE(ch, tch))
            continue;

        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
            act("$n screams 'The murderer! I found you!'", FALSE, ch, 0, 0, TO_ROOM);
			int thaco = rand_number(15,30);
            hit(ch, tch, TYPE_UNDEFINED, thaco, 0);
            return (TRUE);
        }

        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
            act("$n screams 'Oh a wise guy eh? I'll show you! Gimme back my money!'", FALSE, ch, 0, 0, TO_ROOM);
			int thaco = rand_number(15,30);
            hit(ch, tch, TYPE_UNDEFINED, thaco, 0);
            return (TRUE);
        }

        if (GET_CHA(tch) < min_cha) {
            spittle = tch;
            min_cha = GET_CHA(tch);
        }
    }

    // Reward the socially inept
    if (spittle && !rand_number(0, 9)) {

        static int spit_social;

        if (!spit_social)
            spit_social = find_command("spit");

        if (spit_social > 0) {
            char spitbuf[MAX_NAME_LENGTH + 1];
            strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));    /* strncpy: OK */
            spitbuf[sizeof(spitbuf) - 1] = '\0';
            do_action(ch, spitbuf, spit_social, 0);
            return (TRUE);
        }
    }

    return (FALSE);
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 1000)
SPECIAL(pet_shops)
{
    char buf[MAX_STRING_LENGTH];
    char pet_name[256];
    room_rnum pet_room;
    struct char_data *pet;
    struct follow_type *k;
    int j = 0;

    //  pet_room = IN_ROOM(ch) + 1;
    //
    // I hard coded this until we re-arrange rooms.
    // I know, I suck.  Thats what the other comments seem to say.  -- Tails 6/13/08

    if (IN_ROOM(ch) == real_room(14028)) pet_room = real_room(14147);
    else if (IN_ROOM(ch) == real_room(21004)) pet_room = real_room(21098);
    else if (IN_ROOM(ch) == real_room(14210)) pet_room = real_room(14299);
    else
        pet_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list")) {

        send_to_char(ch, "Available pets are:\r\n");
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
            // No, you can't have the Implementor as a pet if he's in there.
            if (!IS_NPC(pet))
                continue;

            send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
        }

        return (TRUE);
    }
    else if (CMD_IS("buy")) {

        two_arguments(argument, buf, pet_name);

        if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
            send_to_char(ch, "There is no such pet!\r\n");
            return (TRUE);
        }

        if (GET_UNITS(ch) < PET_PRICE(pet)) {
            send_to_char(ch, "You don't have enough units!\r\n");
            return (TRUE);
        }

		if (IS_NPC(ch)) {
			send_to_char(ch, "I see what you're trying to do here...\r\n");
			return (TRUE);
		}

        // limit number of followers to 3
        // how many charmed followers?
        for (k = ch->followers; k; k = k->next)
            if (AFF_FLAGGED(k->follower, AFF_CHARM))
                j += 1;

        if (j >= MAX(1, (GET_INT(ch)/8))) {
            send_to_char(ch, "You don't seem to be able to control that many leashes!\r\n");
            return (TRUE);
        }

        if (GET_LEVEL(pet) > GET_LEVEL(ch)) {
            send_to_char(ch, "That pet is too big for you!\r\n");
            return (TRUE);
        }

        GET_UNITS(ch) -= PET_PRICE(pet);
        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

        if (*pet_name) {

            snprintf(buf, sizeof(buf), "%s %s", pet->player.name, pet_name);
            // free(pet->player.name); don't free the prototype!
            pet->player.name = strdup(buf);

            snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
            pet->player.description, pet_name);
            // free(pet->player.description); don't free the prototype!
            pet->player.description = strdup(buf);
        }

        char_to_room(pet, IN_ROOM(ch));
        add_follower(pet, ch);
		if (!AFF_FLAGGED(ch, AFF_GROUP))
			perform_group(ch, ch);
		perform_group(ch, pet);

        // Be certain that pets can't get/carry/use/wield/wear items
        IS_CARRYING_W(pet) = 1000;
        IS_CARRYING_N(pet) = 100;

        send_to_char(ch, "May you enjoy your pet.\r\n");
        act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

        return (TRUE);
    }

    // All commands except list and buy
    return (FALSE);
}

/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(bank)
{
    int amount;

    if (CMD_IS("balance")) {
        if (GET_BANK_UNITS(ch) > 0)
            send_to_char(ch, "Your current balance is %d units.\r\n", GET_BANK_UNITS(ch));
        else
            send_to_char(ch, "You currently have no money deposited.\r\n");
        return (TRUE);
    }
    else if (CMD_IS("deposit")) {

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to deposit?\r\n");
            return (TRUE);
        }

        if (GET_UNITS(ch) < amount) {
            send_to_char(ch, "You don't have that many units!\r\n");
            return (TRUE);
        }

        GET_UNITS(ch) -= amount;
		if ((GET_BANK_UNITS(ch) + amount) > MAX_BANK) {
			send_to_char(ch, "You have deposited more than the max bank amount, the bank can only gaurantee you %d units.\r\n", MAX_BANK);
			GET_BANK_UNITS(ch) = MAX_BANK;
		}
		else
			GET_BANK_UNITS(ch) += amount;
        send_to_char(ch, "You deposit %d resource units.\r\n", amount);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    }
    else if (CMD_IS("withdraw")) {

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to withdraw?\r\n");
            return (TRUE);
        }

        if (GET_BANK_UNITS(ch) < amount) {
            send_to_char(ch, "You don't have that many units deposited!\r\n");
            return (TRUE);
        }

        GET_UNITS(ch) += amount;
        GET_BANK_UNITS(ch) -= amount;
        send_to_char(ch, "You withdraw %d resource units.\r\n", amount);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return (TRUE);
    }
    else
        return (FALSE);
}

SPECIAL(healer)
{
	char arg[MAX_INPUT_LENGTH];
	int cost2 = 0;
	int cost1 = 0;
	int i;
	
	for (i = 1; i < NUM_BODY_PARTS; i++)
		if (!(GET_BODYPART_CONDITION(ch, i, NORMAL)))
			cost1 += GET_LEVEL(ch) * 1000;
	if (affected_by_psionic(ch, PSIONIC_POISON))
		cost2 += GET_LEVEL(ch) * 500;
	if (IS_ON_DRUGS(ch))
		cost2 += GET_LEVEL(ch) * 500;
	if (GET_OVERDOSE(ch) > 0)
		cost2 += GET_LEVEL(ch) * 100;
	if (GET_ADDICTION(ch) > 0)
		cost2 += GET_LEVEL(ch) * 100;

	if (CMD_IS("list")) {
		send_to_char(ch, "@Y\r\nServices that this medical facility provides:\r\n");
		send_to_char(ch, "@Y-----------------------------------------------------\r\n");
		send_to_char(ch, "@W Body part restoration   (heal)    Cost: %d\r\n", cost1);
		send_to_char(ch, "@W Poison treatments       (detox)   Cost: %d\r\n", cost2);
		send_to_char(ch, "@Y-----------------------------------------------------\r\n");
		send_to_char(ch, "@W Type buy <service> to make a purchase.@n\r\n");

		return TRUE;
	}
	if (CMD_IS("buy")) {
		one_argument(argument, arg);
		if (!*arg) {
			send_to_char(ch, "What medical services would you like to purchase?\r\n");
			return TRUE;
		}
		else {
			if (!strcmp(arg, "heal")) {
				if (GET_UNITS(ch) < cost1) {
					send_to_char(ch, "You can't afford to pay for such medical treatment!\r\n");
					return TRUE;
				}
				if (cost1 == 0) {
					send_to_char(ch, "You don't appear to need any medical services.\r\n");
					return TRUE;
				}
				else {
					GET_UNITS(ch) -= cost1;
					send_to_char(ch, "You spend %d units.\r\n", cost1);
					send_to_char(ch, "The doctor scans you as your wounds begin to mend before your eyes!\r\n");
					for (i = 1; i < NUM_BODY_PARTS; i++) {
						GET_NO_HITS(ch, i) = 0;
					    // clear all previous conditions
						BODYPART_CONDITION(ch, i) = 0;
					    // if bionic, set to BIONIC_NORMAL, else set to NORMAL
						if (BIONIC_DEVICE(ch, i))
							SET_BODYPART_CONDITION(ch, i, BIONIC_NORMAL);
						else
							SET_BODYPART_CONDITION(ch, i, NORMAL);
					}
					return TRUE;
				}
			}
			else if (!strcmp(arg, "detox")) {
				if (GET_UNITS(ch) < cost2) {
					send_to_char(ch, "You can't afford to pay for such medical treatment!\r\n");
					return TRUE;
				}
				if (cost2 == 0) {
					send_to_char(ch, "You don't appear to need any medical services.\r\n");
					return TRUE;
				}
				else {
					GET_UNITS(ch) -= cost2;
					send_to_char(ch, "You spend %d units.\r\n", cost2);
					send_to_char(ch, "The doctor injects you with an antidote, you feel it coursing through your veins.\r\n");
					call_psionic(ch, ch, 0, PSIONIC_DETOX, 40, CAST_PSIONIC, TRUE);
					return TRUE;
				}
			}
			else {
				send_to_char(ch, "The shopkeeper doesn't sell that service.\r\n");
				return TRUE;
			}
		}
	}
	return FALSE;
}
	

	
SPECIAL(scratch_ticket)
{
    struct obj_data *ticket;
    char arg[MAX_INPUT_LENGTH];

    if (!CMD_IS("scratch"))
        return (FALSE);

    if (*argument) {

        one_argument(argument, arg);

        if (!(ticket = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
            send_to_char(ch, "Ticket not found.\r\n");
            return (TRUE);
        }

        if (GET_OBJ_TYPE(ticket) != ITEM_SCRATCH) {
            send_to_char(ch, "You can't scratch that!\r\n");
            return (TRUE);
        }

        if (IS_SCRATCHED(ticket)) {
            send_to_char(ch, "But this ticket has already been scratched!\r\n");
            return (TRUE);
        }

        // ok, go ahead and scratch
        IS_SCRATCHED(ticket) = TRUE;
        send_to_char(ch, "You scratch the ticket...\r\n");
        return (TRUE);

    }
    else {
        send_to_char(ch, "Usage: Scratch <ticket>\r\n");
        return (TRUE);
    }

    return (FALSE);
}


#define RU 0
#define QP 1

// DTs have been eliminated
SPECIAL(mortuary)
{
    return (FALSE);
}

SPECIAL(bionics_shop)
{

    char field[MAX_STRING_LENGTH];
    int len;
    int index;

    int unitcost = 0;
    int psicost = 0;
    int min_level;
    int device;

    int i = 0;
    int j = 0;
    int k = 0;
    struct affected_type *temp;
    struct affected_type temp_affects[MAX_AFFECT];
    struct obj_data *char_eq[NUM_WEARS];

  //  these ones aren't sold -- perhaps we should have them sold?
  //      {"footjets",     BIO_FOOTJET,         0,           0, 99},
  //      {"matrix",       BIO_MATRIX,         0,           0, 99},

    if (CMD_IS("list")) {
        send_to_char(ch,
            "\n\rThe following bioinics are available for purchase:\n\r"
            "\n\r"
            "Name                        Keyword to purchase   Level  Unit Cost  Psi Cost\n\r"
            "-------------------------   --------------------  -----  ---------  --------\n\r");


        // iterate through the list
        for (index = 0; bionic_info[index].device != -1; index++) {
            send_to_char(ch, "%25s  %20s  %5d  %9d  %8d\r\n",
                bionic_info[index].long_name, bionic_info[index].short_name, bionic_info[index].min_level,
                bionic_info[index].unit_cost, bionic_info[index].psi_cost);
        }

        send_to_char(ch,
            "\n\r"
            "To purchase a bionic implant, you would type buy <keyword>.\r\n"
            "Be aware of the prerequisites and the cost, both in terms of the units and the\n\r"
            "psi points. For more information, type 'help bionics'.\n\r");

        return (TRUE);

    }

    if (CMD_IS("buy")) {

        if (IS_NPC(ch)) {
            send_to_char(ch, "Don't be silly.. you're an NPC.\n\r");
            return (TRUE);
        }

        argument = one_argument(argument, field);

        if (!*field) {
            send_to_char(ch, "Buy what?\n\r");
            return (TRUE);
        }

        len = strlen(field);
        for (index = 0; *(bionic_info[index].short_name) != '\n'; index++)
            if (!strncmp(field, bionic_info[index].short_name, len))
                break;

        if (*(bionic_info[index].short_name) == '\n') {
            send_to_char(ch, "Sorry, that is not avaliable.  Type list for a list of bionics that we deal in here.\r\n");
            return (TRUE);
        }

        unitcost = bionic_info[index].unit_cost;
        psicost = bionic_info[index].psi_cost;
        min_level = bionic_info[index].min_level;
        device = bionic_info[index].device;


        // do they already have the bionic?
        if (HAS_BIONIC(ch, device)) {
            send_to_char(ch, "You already have %s %s.\r\n", STANA(bionic_info[index].long_name), bionic_info[index].long_name);
            return (TRUE);
        }

        // are they the right level?
        if (GET_LEVEL(ch) < min_level) {
            send_to_char(ch, "You need to be level %d to get that.\r\n", min_level);
            return (TRUE);
        }

        // can they afford the unit cost?
        if (GET_UNITS(ch) < unitcost) {
            send_to_char(ch, "That will cost %d units ... which I see you cannot afford.\r\n", unitcost);
            return (TRUE);
        }

        // check for prerequisites
        for (i = 0; i < 3; i++) {
            if (bionic_info[index].prereq[i] >= 0) {
                if (!HAS_BIONIC(ch, bionic_info[index].prereq[i])) {
                    send_to_char(ch, "You do not have the correct prerequisite implant for the %s.\r\n", bionic_info[index].long_name);
                    send_to_char(ch, "You must have %s %s installed first.\r\n",
                        STANA(bionic_names[bionic_info[index].prereq[i]][1]), bionic_names[bionic_info[index].prereq[i]][1]);
                    return (TRUE);
                }
            }
        }

        // remove bionic affects so we can build them back up
        bionics_unaffect(ch);

        // store and then remove psionic effects
        for (temp = ch->affected, j = 0; j < MAX_AFFECT; j++) {
            if (temp) {
                temp_affects[j] = *temp;
                temp_affects[j].next = 0;
                temp = temp->next;
            }
            else {
                temp_affects[j].type = 0;    /* Zero signifies not used */
                temp_affects[j].duration = 0;
                temp_affects[j].modifier = 0;
                temp_affects[j].location = 0;
                temp_affects[j].next = 0;
            }
        }

        if (ch->affected)
            while (ch->affected)
                affect_remove(ch, ch->affected);

        // remove drug affects -- shouldnt we store these?
        if (IS_ON_DRUGS(ch))
            while (ch->drugs_used)
                drug_remove(ch, ch->drugs_used);

        // remove equipment affects  -- shouldnt we store the worn eq?
        for (k = 0; k < NUM_WEARS; k++) {
            if (GET_EQ(ch, k))
                char_eq[k] = unequip_char(ch, k, FALSE);
            else
                char_eq[k] = NULL;
        }

        // can they afford the psi cost?
        if (GET_MAX_PSI(ch) < psicost)
            send_to_char(ch, "Your body cannot sustain the bionic implant.  You don't have enough psi points.\r\n");
        else {
            // pay the cost
            GET_UNITS(ch) -= unitcost;
            GET_MAX_PSI_PTS(ch) -= psicost;

            // generate the success message
            send_to_char(ch, "%s", bionic_info[index].success);

            // update the bionic level
            BIONIC_LEVEL(ch, device) = bionic_info[index].bio_level;
        }
        // Add back in all bionics
        bionics_affect(ch);

        // Add back all psionic effects
        for (j = 0; j < MAX_AFFECT; j++)
            if (temp_affects[j].type)
                affect_to_char(ch, &temp_affects[j]);

        // Add back the equipment affects
        for (i = 0; i < NUM_WEARS; i++)
            if (char_eq[i])
                equip_char(ch, char_eq[i], i, FALSE);

        // add back in drug effects?

        save_char(ch);

        return (TRUE);
    }

   return (FALSE);
}

SPECIAL(bionics_upgrade)
{
    char field[MAX_STRING_LENGTH];
    int len;
    int index;

    int unitcost = 0;
    int psicost = 0;
    int min_level;
    int device;

    int i = 0;
    int j = 0;
    int k = 0;
    struct affected_type *temp;
    struct affected_type temp_affects[MAX_AFFECT];
    struct obj_data *char_eq[NUM_WEARS];

    if (CMD_IS("list")) {
        send_to_char(ch,
            "\n\rYou can upgrade the following bionics here:\n\r"
            "\n\r"
            "Name                        Keyword to purchase   Level  Unit Cost  Psi Cost\n\r"
            "-------------------------   --------------------  -----  ---------  --------\n\r");

        // iterate through the list
        for (index = 0; bionic_info_advanced[index].device != -1; index++) {
            send_to_char(ch, "%25s  %20s  %5d  %9d  %8d\r\n",
                bionic_info_advanced[index].long_name, bionic_info_advanced[index].short_name, bionic_info_advanced[index].min_level,
                bionic_info_advanced[index].unit_cost, bionic_info_advanced[index].psi_cost);
        }

        send_to_char(ch,
            "\n\r"
            "To upgrade a bionic implant, you would type upgrade <keyword>.\r\n"
            "Be aware of the prerequisites and the cost, both in terms of the units and the\n\r"
            "psi points. For more information, type 'help upgrade'.\n\r");

        return (TRUE);

    }

    if (CMD_IS("upgrade")) {

        if (IS_NPC(ch)) {
            send_to_char(ch, "Don't be silly.. you're an NPC.\n\r");
            return (TRUE);
        }

        if (GET_LEVEL(ch) < 15) {
            send_to_char(ch, "You need to be level 15 to upgrade bionics.\r\n");
            return (TRUE);
        }

        argument = one_argument(argument, field);

        if (!*field) {
            send_to_char(ch, "Upgrade what?\n\r");
            return (TRUE);
        }

        len = strlen(field);
        for (index = 0; *(bionic_info_advanced[index].short_name) != '\n'; index++)
            if (!strncmp(field, bionic_info_advanced[index].short_name, len))
                break;

        if (*(bionic_info_advanced[index].short_name) == '\n') {
            send_to_char(ch, "Sorry, that is not avaliable.  Type list for a list of bionics that we deal in here.\r\n");
            return (TRUE);
        }

        if (*(bionic_info_advanced[index].short_name) == '\n') {
            send_to_char(ch, "Sorry, that is not avaliable.  Type list for a list of bionics that we deal in here.\r\n");
            return (TRUE);
        }

        unitcost = bionic_info_advanced[index].unit_cost;
        psicost = bionic_info_advanced[index].psi_cost;
        min_level = bionic_info_advanced[index].min_level;
        device = bionic_info_advanced[index].device;

        // do they already have the bionic?
        if (!HAS_BIONIC(ch, device)) {
            send_to_char(ch, "You need a basic version first.\r\n");
            return (TRUE);
        }

        if (BIONIC_LEVEL(ch, device) > BIONIC_BASIC) {
            send_to_char(ch, "You've already got an upgrade there.\r\n");
            return (TRUE);
        }

        // can they afford the unit cost?
        if (GET_UNITS(ch) < unitcost) {
            send_to_char(ch, "That will cost %d units ... which I see you cannot afford.\r\n", unitcost);
            return (TRUE);
        }

        // check for prerequisites
        for (i = 0; i < 3; i++) {
            if (bionic_info_advanced[index].prereq[i] >= 0) {
                if (!HAS_BIONIC(ch, bionic_info_advanced[index].prereq[i]) && BIONIC_LEVEL(ch, bionic_info_advanced[index].prereq[i]) < BIONIC_ADVANCED) {
                    send_to_char(ch, "You do not have the correct prerequisite implant for the %s.\r\n", bionic_info_advanced[index].long_name);
                    send_to_char(ch, "You must have %s %s installed first.\r\n",
                        STANA(bionic_names[bionic_info_advanced[index].prereq[i]][1]), bionic_names[bionic_info_advanced[index].prereq[i]][1]);
                    return (TRUE);
                }
            }
        }

        // remove bionic affects so we can build them back up
        bionics_unaffect(ch);

        // store and then remove psionic effects
        for (temp = ch->affected, j = 0; j < MAX_AFFECT; j++) {
            if (temp) {
                temp_affects[j] = *temp;
                temp_affects[j].next = 0;
                temp = temp->next;
            }
            else {
                temp_affects[j].type = 0;    /* Zero signifies not used */
                temp_affects[j].duration = 0;
                temp_affects[j].modifier = 0;
                temp_affects[j].location = 0;
                temp_affects[j].next = 0;
            }
        }

        if (ch->affected)
            while (ch->affected)
                affect_remove(ch, ch->affected);

        // remove drug affects -- shouldnt we store these?
        if (IS_ON_DRUGS(ch))
            while (ch->drugs_used)
                drug_remove(ch, ch->drugs_used);

        // remove equipment affects  -- shouldnt we store the worn eq?
        for (k = 0; k < NUM_WEARS; k++) {
            if (GET_EQ(ch, k))
                char_eq[k] = unequip_char(ch, k, FALSE);
            else
                char_eq[k] = NULL;
        }

        // can they afford the psi cost?
        if (GET_MAX_PSI(ch) < psicost)
            send_to_char(ch, "Your body cannot sustain the bionic implant.  You don't have enough psi points.\r\n");
        else {

            // pay the cost
            GET_UNITS(ch) -= unitcost;
            GET_MAX_PSI_PTS(ch) -= psicost;

            // generate the success message
            send_to_char(ch, "%s", bionic_info_advanced[index].success);

            // update the bionic level
            BIONIC_LEVEL(ch, device) = bionic_info_advanced[index].bio_level;
        }

        // Add back in all bionics
        bionics_affect(ch);

        // Add back all psionic effects
        for (j = 0; j < MAX_AFFECT; j++)
            if (temp_affects[j].type)
                affect_to_char(ch, &temp_affects[j]);

        // Add back the equipment affects
        for (i = 0; i < NUM_WEARS; i++)
            if (char_eq[i])
                equip_char(ch, char_eq[i], i, FALSE);

        // add back in drug effects?

        save_char(ch);

        return (TRUE);
    }

    return (FALSE);
}

int exchange_vnums[] =
{
  1290,  /* hilts */
  21513, /* medals */
  21504 /* paper */
};

SPECIAL(quest_store)
{
    struct obj_data *obj;
    struct obj_data *exchange_objs[10];
    char buf[MAX_STRING_LENGTH];
    int qp = 0;
    int i;
    int obj_vnum = 0;
    int obj_cost;
    int r_num;
    int value;
    char buf2[MAX_INPUT_LENGTH];
    int loc;
    int bonus;

    if (CMD_IS("list")) {
        send_to_char(ch, "@CThe shopkeeper says, 'I can do much more than you know! Check out the hologram for more information!'@n\n\r\r\n");
    }

    if (CMD_IS("exchange")) {

        argument = one_argument(argument, buf);

        if (!*buf) {
            send_to_char(ch, "The shopkeeper says, 'Whadda you wanna exchange mate?'\r\n");
            return (TRUE);
        }

        if (!str_cmp(buf, "tokens") || !str_cmp(buf, "rainbow")) {

            int num_tokens = 0;
            obj = ch->carrying;

            // todo: i don't like the hard-coding, deal with it later
            while (obj && (num_tokens < 6)) {
                if (GET_OBJ_VNUM(obj) >= 1 && GET_OBJ_VNUM(obj) <= 10)
                    exchange_objs[num_tokens++] = obj;
                obj = obj->next_content;
            }

            if (num_tokens == 6) {

                for (i = 0; i < 6; i++) {
                    obj_from_char(exchange_objs[i]);
                    extract_obj(exchange_objs[i]);
                }

                GET_QUESTPOINTS(ch) += 1;
                send_to_char(ch, "The shopkeeper takes the tokens and gives you a questpoint.\r\n");
                send_to_char(ch, "The shopkeeper says, 'Nice doing business with you.'\r\n");
                log("QP: QuestStore -(1)-> %s (TOKENS)", GET_NAME(ch));
            }
            else
                send_to_char(ch, "The shopkeeper says, 'You need 6 rainbow tokens before you can get a questpoint!'\r\n");

            return (TRUE);
        }
        else if (str_cmp(buf, "hilts") == 0) {

            int num_hilts = 0;
            obj = ch->carrying;

            while (obj && (num_hilts < 10)) {
                if (GET_OBJ_VNUM(obj) == exchange_vnums[0])
                    exchange_objs[num_hilts++] = obj;

                obj = obj->next_content;
            }

            if (num_hilts == 10) {

                int i;

                for (i = 0; i < 10; i++) {
                    obj_from_char(exchange_objs[i]);
                    extract_obj(exchange_objs[i]);
                }

                obj = read_object(exchange_vnums[1], VIRTUAL);
                obj_to_char(obj, ch);
                send_to_char(ch, "The shopkeeper takes some hilts and gives you a medal of honor.\r\n");
                send_to_char(ch, "The shopkeeper says, 'Nice doing business with you.'\r\n");
            }
            else
                send_to_char(ch, "The shopkeeper says, 'Hilts?!? You've not got enough hilts to get a medal. Go Away!'\r\n");

            return (TRUE);
        }
        else if (str_cmp(buf, "medals") == 0) {

            int num_medals = 0;
            obj = ch->carrying;

            while (obj && (num_medals < 5)) {
                if (GET_OBJ_VNUM(obj) == exchange_vnums[1])
                    exchange_objs[num_medals++] = obj;
                    obj = obj->next_content;
            }

            if (num_medals == 5) {

                int i;
                for (i = 0; i < 5; i++) {
                    obj_from_char(exchange_objs[i]);
                    extract_obj(exchange_objs[i]);
                }

                 obj = read_object(22771, VIRTUAL);

                // BEGIN Restring the ticket!
                obj->short_description = strdup("a restring ticket");
                obj->description = strdup("A muddy crimson ticket lays here, on the ground.");
                obj->name = strdup("crimson ticket restring");
                obj->action_description = strdup("Bring this to a Staff member.");
                // END  Restring the ticket!

                obj_to_char(obj, ch);
                send_to_char(ch, "The shopkeeper takes some medals and gives you a restring ticket.\r\n");
                send_to_char(ch, "The shopkeeper says, 'Nice doing business with you.'\r\n");
            }
            else
                send_to_char(ch, "The shopkeeper says, 'Medals?!? You've not got enough medals to get a restring ticket. Go Away!'\r\n");

            return (TRUE);
        }
        else if (!str_cmp(buf, "sabers") || !str_cmp(buf, "saber")) {

            bool found = FALSE;
            obj = ch->carrying;
            struct obj_data *next_obj = NULL;

            while (obj) {

                next_obj = obj->next_content;

                // only items: cell, mech, lens, diss, gem
                if (GET_OBJ_VNUM(obj) > 1290 && GET_OBJ_VNUM(obj) < 1296) {

                    found = TRUE;
                    GET_QUESTPOINTS(ch) += GET_OBJ_COST(obj);
                    send_to_char(ch, "The shopkeeper takes %s and gives you %d questpoints.\r\n",
                        obj->short_description, GET_OBJ_COST(obj));

                    log("QP: QuestStore -(%d)-> %s (SABERS) %s", GET_OBJ_COST(obj),
                        GET_NAME(ch), obj->short_description);

                    obj_from_char(obj);
                    extract_obj(obj);
                }

                obj = next_obj;
            }

            if (found)
                send_to_char(ch, "The shopkeeper says, \'Nice doing business with you.\'\r\n");
            else
                send_to_char(ch, "The shopkeeper says, \'You don\'t have any saber parts!\'\r\n");
        }
        else
            send_to_char(ch, "The shopkeeper says, \'I don\'t like that trash you want to exchange. Go Away!!\'\r\n");

        return (TRUE);
    }

    if (CMD_IS("tednugent")) {

        argument = one_argument(argument, buf);

        if (!*buf) {
            send_to_char(ch, "The shopkeeper says, \'Whadda you wanna buy?\'\n\r");
            return (TRUE);
        }

        if (IS_NPC(ch)) {
            send_to_char(ch, "The shopkeeper says, \'You can\'t be serious, fella.\'\n\r");
            return (TRUE);
        }

        qp = ch->player_specials->saved.questpoints;

        // todo: remove the hard coding
        // for (j = 30000; j < 30182; j++) {
        obj_vnum = atoi(buf);
        if (obj_vnum < 30000 || obj_vnum > 30181) {
            send_to_char(ch, "The shopkeeper says, \'What exactly was it you wanted to buy?\'\n\r");
            return (TRUE);
        }

        r_num = real_object(obj_vnum);
        obj = read_object(r_num, REAL);
        obj_cost = GET_OBJ_COST(obj);

        if (obj_cost > qp && GET_LEVEL(ch) < LVL_GRGOD) {
            send_to_char(ch, "The shopkeeper says, \'Sorry friend, you can\'t afford an item that fine.\'\n\r");
            extract_obj(obj);
            return (TRUE);
        }
        else {
            obj_to_char(obj, ch);
            send_to_char(ch, "The shopkeeper gives you a brown wrapped package tied with string.\n\r");
            send_to_char(ch, "The shopkeeper says, \'Thanks for the business. Enjoy your new equipment.\'\n\r");

            if (GET_LEVEL(ch) < LVL_GRGOD) {
                ch->player_specials->saved.questpoints -= obj_cost;
                log("QP: %s -(%d)-> QuestStore (%s)", GET_NAME(ch), obj_cost, obj->short_description);
                return (TRUE);
            }
        }
   }


    if (CMD_IS("sell")) {
        argument = one_argument(argument, buf);

        if (!*buf) {
            send_to_char(ch, "The shopkeeper says, \'Yeah... but what do you want to sell?\'\n\r");
            return (TRUE);
        }

        if (IS_NPC(ch)) {
            send_to_char(ch, "The shopkeeper says, \'You can't be serious, fella.\'\n\r");
            return (TRUE);
        }

        obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying);

        if (!obj) {
            send_to_char(ch, "The shopkeeper says, \'You don't even have that item.\'\n\r");
            return (TRUE);
        }

        if (!OBJ_FLAGGED(obj, ITEM_QUEST_ITEM) || IS_BOUND(obj)) {
            send_to_char(ch, "The shopkeeper says, \'I only buy the finest items... that\'s rubbish.\'\n\r");
            return (TRUE);
        }

        value = GET_OBJ_COST(obj);

        if ((value == 0)) {
            send_to_char(ch, "The shopkeeper says, \'not this time bucko! It has no value!\'\n\r");
            return (TRUE);
        }

        if (value > 1)
            value = (value*3)/4;
        else
            value = 0;

        obj_from_char(obj);
        ch->player_specials->saved.questpoints += value;
        send_to_char(ch, "The shopkeeper takes your item and gives you some questpoints.\n\r");
        send_to_char(ch, "The shopkeeper says, 'Thank you for your patronage. Come again.'\n\r");
        log("QP: QuestStore -(%d)-> %s (%s)", value, GET_NAME(ch), obj->short_description);
        extract_obj(obj);
        return (TRUE);
    }

    if (CMD_IS("imbue")) { /* imbue - by Lump 04/07 suggested by Scias */

        argument = two_arguments(argument, buf, buf2);


        if (IS_NPC(ch)) {
            send_to_char(ch, "The shopkeeper says, \'You can't be serious, fella.\'\n\r");
            return (TRUE);
        }

        // todo: move this into a struct for efficiency
        if (!*buf || !*buf) {
            send_to_char(ch, "The shopkeeper says, \'Yeah... but what do you want to imbue? Try: imbue <item> <affect>\'\n\r");
            send_to_char(ch, "The shopkeeper says, \'Here is what I can do:\'\n\r");
            send_to_char(ch,
                "armor    (APPLY_ARMOR+5)      cost:  5qp\r\n"
                "armor+   (APPLY_ARMOR+10)     cost: 15qp\r\n"
                "hnd      (APPLY_HITNDAM+5)    cost: 10qp\r\n"
                "hnd+     (APPLY_HITNDAM+10)   cost: 15qp\r\n"
                "hpv      (APPLY_HPV+20)       cost: 10qp\r\n"
                "hpv+     (APPLY_HPV+30)       cost: 15qp\r\n"
                "hitregen (APPLY_HIT_REGEN+10) cost: 10qp\r\n"
                "psiregen (APPLY_PSI_REGEN+10) cost: 10qp\r\n"
                "moveregen(APPLY_MOVE_REGEN+10)cost: 10qp\r\n"
                "str      (APPLY_STR+1)        cost: 10qp\r\n"
                "int      (APPLY_INT+1)        cost: 10qp\r\n"
                "dex      (APPLY_DEX+1)        cost: 10qp\r\n"
                "pcn      (APPLY_PCN+1)        cost: 10qp\r\n"
                "con      (APPLY_CON+1)        cost: 10qp\r\n"
                "regenall (APPLY_REGEN_ALL+10) cost: 20qp\r\n"
                "allstats (APPLY_ALL_STATS+1)  cost: 50qp\n\r");
            return (TRUE);
        }

        obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying);

        if (!obj) {
            send_to_char(ch, "The shopkeeper says, \'You don't even have that item.\'\n\r");
            return (TRUE);
        }

        if (OBJ_FLAGGED(obj, ITEM_PSIONIC)) {
            send_to_char(ch, "The shopkeeper says, \'I've seen this item in the past! I can only do this once\'\n\r");
            return (TRUE);
        }

        if (!OBJ_FLAGGED(obj, ITEM_QUEST_ITEM)) {
            send_to_char(ch, "The shopkeeper says, \'I can only imbue the finest items... that's rubbish.\'\n\r");
            return (TRUE);
        }

        if (!str_cmp(buf2, "hnd")) {
            loc = APPLY_HITNDAM;
            bonus = 5;
            value = 5;
        }
        else if (!str_cmp(buf2, "hnd+")) {
            loc = APPLY_HITNDAM;
            bonus = 10;
            value = 15;
        }
        else if (!str_cmp(buf2, "hpv")) {
            loc = APPLY_HPV;
            bonus = 20;
            value = 10;
        }
        else if (!str_cmp(buf2, "hpv+")) {
            loc = APPLY_HPV;
            bonus = 30;
            value = 15;
        }
        else if (!str_cmp(buf2, "armor")) {
            loc = APPLY_ARMOR;
            bonus = 5;
            value = 10;
        }
        else if (!str_cmp(buf2, "armor+")) {
            loc = APPLY_ARMOR;
            bonus = 10;
            value = 15;
        }
        else if (!str_cmp(buf2, "hitregen")) {
            loc = APPLY_HIT_REGEN;
            bonus = 10;
            value = 10;
        }
        else if (!str_cmp(buf2, "psiregen")) {
            loc = APPLY_PSI_REGEN;
            bonus = 10;
            value = 10;
        }
        else if (!str_cmp(buf2, "moveregen")) {
            loc = APPLY_MOVE_REGEN;
            bonus = 10;
            value = 10;
        }
        else if (!str_cmp(buf2, "regenall")) {
            loc = APPLY_REGEN_ALL;
            bonus = 10;
            value = 20;
        }
        else if (!str_cmp(buf2, "str")) {
            loc = APPLY_STR;
            bonus = 1;
            value = 10;
        }
        else if (!str_cmp(buf2, "int")) {
            loc = APPLY_INT;
            bonus = 1;
            value = 10;
        }
        else if (!str_cmp(buf2, "dex")) {
            loc = APPLY_DEX;
            bonus = 1;
            value = 10;
        }
        else if (!str_cmp(buf2, "pcn")) {
            loc = APPLY_PCN;
            bonus = 1;
            value = 10;
        }
        else if (!str_cmp(buf2, "con")) {
            loc = APPLY_CON;
            bonus = 1;
            value = 10;
        }
        else if (!str_cmp(buf2, "allstats")) {
            loc = APPLY_ALL_STATS;
            bonus = 1;
            value = 50;
        }
        else {
            send_to_char(ch, "The shopkeeper says, \'I don't know how to imbue that ability! Ask someone else...\'\n\r");
            return (TRUE);
        }
        if ((ch->player_specials->saved.questpoints) >= value) {
        ch->player_specials->saved.questpoints -= value;
        } else {
            send_to_char(ch, "You are broke\n\r");
            return (TRUE);
}        
        for (i = 0; i < MAX_OBJ_APPLY; i++)
            if (GET_OBJ_APPLY_LOC(obj, i) == APPLY_NONE)
                break;

        if (i == MAX_OBJ_APPLY) {
            send_to_char(ch, "The shopkeeper says, \'This weapon is FAR too powerful already\'!\n\r");
            return (TRUE);
        }

        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_PSIONIC);

        GET_OBJ_APPLY_LOC(obj, i) = loc;
        GET_OBJ_APPLY_MOD(obj, i) = bonus;

        send_to_char(ch, "The shopkeeper takes your questpoints and imbues his essence on your item.\n\r");
        send_to_char(ch, "The shopkeeper says, \'Thank you for your patronage. Come again.\'\n\r");
        log("QP: %s -(%d)-> Queststore (imbue) %s", GET_NAME(ch), value, obj->short_description);
        return (TRUE);
    }

    return (FALSE);
}

#define LOTTO_TICKET_VNUM 1215
#define SCRATCH_TICKET_VNUM 1216
#define SCRATCH_VERSION 1

// todo: break this into support functions
SPECIAL(lotto)
{
    // should never get here, but just in case
    return (FALSE);
}

SPECIAL(aggr_15)
{
    return (FALSE);
}

// this function seemed like it was to take into account mob damage  -- but it never did
SPECIAL(aggr_20)
{
    bool foundHitter = FALSE;
    bool foundHealer = FALSE;
    struct char_data *vict = NULL;
    struct char_data *hitter = NULL;
    struct char_data *healer = NULL;

    // find a player to attack
    if (GET_POS(ch) != POS_FIGHTING) {

        // first look for a target that can fight
        // must be level 20+, no mega sanc or super sanc, AC less than 150, Damroll more than 70
        // but also look for a target that can heal
        // must be level 20+, no mega or super sanc, AC less than 120, Psi more than 300, crazy is prefered
        for (vict = world[IN_ROOM(ch)].people; vict && !foundHitter; vict = vict->next_in_room) {

            if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
                continue;

            if (GET_LEVEL(vict) < 20)
                continue;

            // aggro mobs don't attack their own team!
            if (GET_TEAM(ch) > 0 && GET_TEAM(ch) == GET_TEAM(vict))
                continue;

            if (affected_by_psionic(vict, PSIONIC_MEGA_SANCT))
                continue;

            // did we find a hitter?
            if (affected_by_psionic(vict, PSIONIC_SUPER_SANCT) && GET_AC(vict) < 150 && GET_DAMROLL(vict) > 70) {
                foundHitter = TRUE;
                hitter = vict;
            }

            // did we find a healer?
            if (!foundHealer && GET_AC(vict) < 120 && GET_PSI(vict) > 300)
                if ((GET_HIT(vict) < 0) ||
                    !affected_by_psionic(vict, PSIONIC_SUPER_SANCT) ||
                    GET_CLASS(vict) == CLASS_CRAZY) {
                    foundHealer = TRUE;
                    healer = vict;
                    }
        }
    }

    // go for hitter first
    if (foundHitter) {
		int thaco = rand_number(1,30);
        hit(ch, hitter, TYPE_UNDEFINED, thaco, 0);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
        return (TRUE);
    }
    else if (foundHealer) {
		int thaco = rand_number(1,30);
        hit(ch, healer, TYPE_UNDEFINED, thaco, 0);
        GET_WAIT_STATE(ch) = PULSE_VIOLENCE;
        return (TRUE);
    }

    return (FALSE);
}

SPECIAL(bionics_doctor)
{
    struct obj_data *part;
    struct obj_data *device;
	struct char_data *temp;
	int mode;
    int part_room;
    int where, i, j;
    bool found = FALSE;
    char name[MAX_INPUT_LENGTH];
	int dupes = 0;
	int reqs = 0;
    part_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list")) {
		send_to_char(ch, "\r\n     Cost    Device                                   Type                 Location          Level");
		send_to_char(ch, "\r\n--------------------------------------------------------------------------------------------------\r\n\r\n");
        for (part = world[part_room].contents; part; part = part->next_content) {
            if (GET_OBJ_TYPE(part) == ITEM_BIONIC_DEVICE) {
                send_to_char(ch, "%9d    %-40s %-20s %-20s %2d\r\n", GET_OBJ_COST(part), part->short_description, biotypes[GET_OBJ_VAL(part, 2)], body_parts[GET_OBJ_VAL(part, 0)], GET_OBJ_LEVEL(part) );
                found = TRUE;
            }
        }
        if (!found)
            send_to_char(ch, "None.\r\n");
        return (TRUE);

    }
	else if (CMD_IS("shopscan")) {
		argument = one_argument(argument, name);
		skip_spaces(&argument);

		if (!(part = get_obj_in_list_vis(ch, name, NULL, world[part_room].contents))) {
			send_to_char(ch, "I'm afraid I don't have that bionic part.  Sorry.\r\n");
			return (TRUE);
		}
		else if (GET_SKILL_LEVEL(ch, SKILL_GADGETRY) == 0) {
			send_to_char(ch, "You attempt to use the shopkeepers identifier but fail!\r\n");
			return (TRUE);
		}
		else {
			if (part) {
			    send_to_char(ch, "You pick up the shopkeepers identifier and scan the item.\r\n");
			    call_psionic(ch, NULL, part, PSIONIC_IDENTIFY, 30, 0, TRUE);
				return (TRUE);
			}
			else {
				send_to_char(ch, "Somehow we weren't able to find the bionic you were looking for.\r\n");
				return (TRUE);
			}
		}
	}
	else if (CMD_IS("buy")) {

        argument = one_argument(argument, name);
        skip_spaces(&argument);

        // is this bionic device for sale?
        if (!(part = get_obj_in_list_vis(ch, name, NULL, world[part_room].contents))) {
            send_to_char(ch, "I'm afraid I don't have that bionic part.  Sorry.\r\n");
            return (TRUE);
        }
        // can the player afford the bionic device?
        if (GET_UNITS(ch) < GET_OBJ_COST(part)) {
            send_to_char(ch, "I'm sorry but you don't have enough money for that device.\r\n");
            return (TRUE);
        }
        // where does the player want this bionic device installed?
        if ((where = search_block(argument, body_parts, FALSE)) == -1) {
            send_to_char(ch, "Where did you want me to install that bionic device?\r\n");
            return (TRUE);
        }
        // does the player already have a bionic device in the device location?
        if (BIONIC_DEVICE(ch, where) != 0) {
            send_to_char(ch, "Oh my, you already have a bionic device installed there.  You should have that removed first.\r\n");
            return (TRUE);
        }
        // does this bionic device install in this location?

        if (BIONIC_LOCATION(part) != where) {
            send_to_char(ch, "But that bionic device isn't designed for the %s.\r\n", body_parts[where]);
            return (TRUE);
		}
		// level requirements?
		if (GET_OBJ_LEVEL(part) > GET_LEVEL(ch)) {
			send_to_char(ch, "You do not meet the level requirements for that bionic.\r\n");
			return (TRUE);
		}
		// class requirements?
		if (!check_restrictions(ch, part))
			return (TRUE);
        // does the player have the pre-reqs?
		int valarray[] = {
			// put the prereqs into an int array
			GET_OBJ_VAL(part, 3),
			GET_OBJ_VAL(part, 4),
			GET_OBJ_VAL(part, 5)
		};
		// does the bionic to be installed have a prerequisite location that is installed?
		if(GET_OBJ_VAL(part,1) > 0 && BIONIC_DEVICE(ch, GET_OBJ_VAL(part, 1)))
		   // if yes, does the location where the prerequisite bionic is installed match the type prerequisite of the bionic?
		   if (GET_OBJ_VAL(BIONIC_DEVICE(ch, GET_OBJ_VAL(part,1)), 2) != GET_OBJ_VAL(part,3))
			// no?? then make the prereq negative so it auto-fails
			valarray[0] *= -1; 

        for (i = 0; i < NUM_BODY_PARTS; i++) {
			if (BIONIC_DEVICE(ch, i)) {
				for (j = 0; j < 3; j++)
					if (GET_OBJ_VAL(BIONIC_DEVICE(ch,i), 2) == valarray[j])
						valarray[j] = 0;
			}
		}
		if (valarray[0] < 0)
			valarray[0] *= -1;

		if (valarray[0] > 0 || valarray[1] > 0 || valarray[2] > 0) {
			send_to_char(ch, "You do you not meet the requirements of that bionic device, ");
			if (valarray[0] != 0) {
				if (GET_OBJ_VAL(part, 1) > 0)
					send_to_char(ch, "%s device installed on %s is required.\r\n", biotypes[valarray[0]], body_parts[GET_OBJ_VAL(part, 1)]);
				else
					send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[0]]);
			}
			else if (valarray[1] != 0)
				send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[1]]);
			else if (valarray[2] != 0)
				send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[2]]);
			return (TRUE);
		}

        // everything appears fine, perform the operation
        // create a new device - so more than one can be sold
        device = read_object(GET_OBJ_VNUM(part), VIRTUAL);

        // perform the operation
        implant_bionic(ch, where, device);

        // generate messages
        send_to_char(ch, "You are now bio-mechanically enhanced.\r\n");
        GET_UNITS(ch) -= GET_OBJ_COST(device);
		save_char(ch);
		Crash_crashsave(ch);
        bionics_save(ch);
        return (TRUE);

    }
    else if (CMD_IS("install")) {

        argument = one_argument(argument, name);
        skip_spaces(&argument);



        // is this bionic device for sale?
		mode = generic_find(name, FIND_OBJ_INV, ch, &temp, &part);
        if (mode == 0) {
            send_to_char(ch, "You have to have the appropriate bionic in your inventory to install, or you could 'buy' one from me.\r\n");
            return (TRUE);
        }

        // verify this is a bionic device
        if (GET_OBJ_TYPE(part) != ITEM_BIONIC_DEVICE) {
            send_to_char(ch, "That item is not a bionic device!\r\n");
            return (TRUE);
        }
        // can the player afford the bionic device?
        if (GET_UNITS(ch) < (GET_OBJ_COST(part)/2)) {
            send_to_char(ch, "I'm sorry but you don't have enough money for me to install that device.\r\n");
            return (TRUE);
        }
        // where does the player want this bionic device installed?
        if ((where = search_block(argument, body_parts, FALSE)) == -1) {
            send_to_char(ch, "Where did you want me to install that bionic device?\r\n");
            return (TRUE);
        }
        // does the player already have a bionic device in the device location?
        if (BIONIC_DEVICE(ch, where) != 0) {
            send_to_char(ch, "Oh my, you already have a bionic device installed there.  You should have that removed first.\r\n");
            return (TRUE);
        }
        // does this bionic device install in this location?
        if (BIONIC_LOCATION(part) != where) {
            send_to_char(ch, "But that bionic device isn't designed for the %s.\r\n", body_parts[where]);
            return (TRUE);
		}
		// level requirements?
		if (GET_OBJ_LEVEL(part) > GET_LEVEL(ch)) {
			send_to_char(ch, "You do not meet the level requirements for that bionic.\r\n");
			return (TRUE);
		}
		// class requirements?
		if (!check_restrictions(ch, part))
			return (TRUE);
        // does the player have the pre-reqs?
		int valarray[] = {
			// put the prereqs into an int array
			GET_OBJ_VAL(part, 3),
			GET_OBJ_VAL(part, 4),
			GET_OBJ_VAL(part, 5)
		};
		// does the bionic to be installed have a prerequisite location that is installed?
		if(GET_OBJ_VAL(part,1) > 0 && BIONIC_DEVICE(ch, GET_OBJ_VAL(part, 1)))
		   // if yes, does the location where the prerequisite bionic is installed match the type prerequisite of the bionic?
		   if (GET_OBJ_VAL(BIONIC_DEVICE(ch, GET_OBJ_VAL(part,1)), 2) != GET_OBJ_VAL(part,3))
			// no?? then make the prereq negative so it auto-fails
			valarray[0] *= -1; 

        for (i = 0; i < NUM_BODY_PARTS; i++) {
			if (BIONIC_DEVICE(ch, i)) {
				for (j = 0; j < 3; j++)
					if (GET_OBJ_VAL(BIONIC_DEVICE(ch,i), 2) == valarray[j])
						valarray[j] = 0;
			}
		}
		if (valarray[0] < 0)
			valarray[0] *= -1;

		if (valarray[0] > 0 || valarray[1] > 0 || valarray[2] > 0) {
			send_to_char(ch, "You do you not meet the requirements of that bionic device, ");
			if (valarray[0] != 0) {
				if (GET_OBJ_VAL(part, 1) > 0)
					send_to_char(ch, "%s device installed on %s is required.\r\n", biotypes[valarray[0]], body_parts[GET_OBJ_VAL(part, 1)]);
				else
					send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[0]]);
			}
			else if (valarray[1] != 0)
				send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[1]]);
			else if (valarray[2] != 0)
				send_to_char(ch, "%s devices are required.\r\n", biotypes[valarray[2]]);
			return (TRUE);
		}

        // perform the operation
        send_to_char(ch, "You are now bio-mechanically enhanced.\r\n");
        GET_UNITS(ch) -= (GET_OBJ_COST(part) / 2);
        implant_bionic(ch, where, part);
		obj_from_char(part);
		save_char(ch);
		Crash_crashsave(ch);
        bionics_save(ch);
        return (TRUE);

    }
    else if (CMD_IS("remove")) {

        skip_spaces(&argument);

        // is the player a human?

        // where does the player want the bionic device removed?
        if ((where = search_block(argument, body_parts, FALSE)) == -1) {
            send_to_char(ch, "From where did you want me to remove a bionic device?\r\n");
            return (TRUE);
        }

        // does the player have a bionic device in the device location?
        if (BIONIC_DEVICE(ch, where) == 0) {
            send_to_char(ch, "But you have no bionic device installed there.\r\n");
            return (TRUE);
        }

        // we charge for device removal
        if (GET_UNITS(ch) < GET_OBJ_COST(BIONIC_DEVICE(ch, where))/2) {
            send_to_char(ch, "I'm sorry but you don't have enough money to remove that device.\r\n");
            return (TRUE);
        }
		part = (BIONIC_DEVICE(ch, where));
		// check to see if this device is dependant on any other devices that are installed
		bool canremove = FALSE;
		int biotype = GET_OBJ_VAL(part, 2); // Get the base type of the bionic

		if(biotype == 0)
			canremove = TRUE;
		else {
			for (i = 0; i < NUM_BODY_PARTS;i++){
				if (BIONIC_DEVICE(ch, i)) {
					if (GET_OBJ_VAL(BIONIC_DEVICE(ch,i), 2) == biotype)
						dupes += 1; // Found bionic of the same type.
					for (j = 3; j <= 5; j++)
						if (GET_OBJ_VAL(BIONIC_DEVICE(ch,i), j) == biotype)
							reqs += 1; // Found bionic that requires this bionic type.
				}
			}

			if(dupes > 1 || reqs == 0) // If we have another of this type, or nothing requires it, it's ok to remove.
				canremove = TRUE;
		}

		if(!canremove){
			send_to_char(ch,"You have other bionics that rely on this part -- you must remove them first or buy another bionic of this type before removing it.\r\n");
			return(TRUE);
		}
		// everything appears fine, remove the bionic
        device = remove_bionic(ch, where);
		if (GET_EQ(ch, WEAR_EXPANSION) && !AFF_FLAGGED(ch, AFF_CHIPJACK))
			obj_to_char(unequip_char(ch, WEAR_EXPANSION, TRUE), ch);

        GET_UNITS(ch) -= GET_OBJ_COST(device)/2;

        // for now, give the device back to the player
        obj_to_char(device, ch);
        send_to_char(ch, "You'll need to do something about the missing part.\r\n");
		//            SET_BODYPART_CONDITION(ch, where, NORMAL);
		save_char(ch);
		Crash_crashsave(ch);
		bionics_save(ch);
		affect_total(ch);
		return (TRUE);

    }
    // all commands except list, buy, and remove
    return(FALSE);
}
SPECIAL(quest_store2)
{
    struct obj_data *obj;
    char buf[MAX_STRING_LENGTH];
    int value;

    if (CMD_IS("sell")) {
        argument = one_argument(argument, buf);

        if (!*buf) {
            send_to_char(ch, "The shopkeeper says, \'Yeah... but what do you want to sell?\'\n\r");
            return (TRUE);
        }

        if (IS_NPC(ch)) {
            send_to_char(ch, "The shopkeeper says, \'You can't be serious, fella.\'\n\r");
            return (TRUE);
        }

        obj = get_obj_in_list_vis(ch, buf, NULL, ch->carrying);

        if (!obj) {
            send_to_char(ch, "The shopkeeper says, \'You don't even have that item.\'\n\r");
            return (TRUE);
        }

        if (!OBJ_FLAGGED(obj, ITEM_QUEST_ITEM)) {
            send_to_char(ch, "The shopkeeper says, \'I only buy the finest items... that\'s rubbish.\'\n\r");
            return (TRUE);
        }

        value = GET_OBJ_COST(obj);

        if ((value == 0)) {
            send_to_char(ch, "The shopkeeper says, \'not this time bucko! It has no value!\'\n\r");
            return (TRUE);
        }

        if (value > 1)
            value = (value*3)/4;
        else
            value = 0;

        obj_from_char(obj);
        ch->player_specials->saved.questpoints += value;
        send_to_char(ch, "The shopkeeper takes your item and gives you some questpoints.\n\r");
        send_to_char(ch, "The shopkeeper says, 'Thank you for your patronage. Come again.'\n\r");
        log("QP: QuestStore2 -(%d)-> %s (%s)", value, GET_NAME(ch), obj->short_description);
            extract_obj(obj);
        return (TRUE);
    }
        return (FALSE);

}
