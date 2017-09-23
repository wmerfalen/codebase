/*************************************************************************
*  File: rank.h                                Designed for tbaMUD 3.52  *
*  Usage: implementation of automatic player rankings                    *
*         function prototypes and defines for rank.c                     *
*         based on a ranking system created by Slug for CircleMUD 3.0    *
*                                                                        *
*  All rights reserved.                                                  *
*                                                                        *
*  Copyright (C) 1996 Slug                                               *
*  Rewritten in 2007 by Stefan Cole (a.k.a. Jamdog)                      *
*  To see this in action, check out TrigunMUD                            *
************************************************************************ */

#define MAX_RANKED 180
#define GET_PLAYER_KEY(cc,tk) ((*((tk)->plrfunction))(cc))

/* Rank types */
#define RANKTYPE_PLAYER 0

#define NUM_RANK_TYPES  1

typedef char *ranktype;

#define RANK_PLAYER(name)  ranktype name(struct char_data *ch)

struct key_data {
	char *keystring;
	char *outstring;
	ranktype (*plrfunction)(struct char_data *ch);
	int    rank_type;
	struct key_data *next;
};

struct rank_data {
	struct char_data *ch;
	char key[80];
};

ACMD(do_rank);

RANK_PLAYER(rank_bionic);
RANK_PLAYER(rank_clanbucks);
RANK_PLAYER(rank_hp);
RANK_PLAYER(rank_hpregen);
RANK_PLAYER(rank_psi);
RANK_PLAYER(rank_psiregen);
RANK_PLAYER(rank_moves);
RANK_PLAYER(rank_mvregen);
RANK_PLAYER(rank_curhp);
RANK_PLAYER(rank_power);
RANK_PLAYER(rank_str);
RANK_PLAYER(rank_int);
RANK_PLAYER(rank_pcn);
RANK_PLAYER(rank_dex);
RANK_PLAYER(rank_con);
RANK_PLAYER(rank_cha);
RANK_PLAYER(rank_luck);
RANK_PLAYER(rank_qp);
RANK_PLAYER(rank_fame);
RANK_PLAYER(rank_fitness);
RANK_PLAYER(rank_hitroll);
RANK_PLAYER(rank_damroll);
RANK_PLAYER(rank_armor);
RANK_PLAYER(rank_xp);
RANK_PLAYER(rank_height);
RANK_PLAYER(rank_weight);
RANK_PLAYER(rank_encumb);
RANK_PLAYER(rank_fatness);
RANK_PLAYER(rank_units);
RANK_PLAYER(rank_bank);
RANK_PLAYER(rank_age);
RANK_PLAYER(rank_deaths);
RANK_PLAYER(rank_dts);
RANK_PLAYER(rank_kd);
RANK_PLAYER(rank_played);
RANK_PLAYER(rank_birth);
RANK_PLAYER(rank_level);
RANK_PLAYER(rank_questsc);
RANK_PLAYER(rank_rifts);
RANK_PLAYER(rank_eventmob);


int rank_compare_top(const void *n1, const void *n2);
int rank_compare_bot(const void *n1, const void *n2);
int char_compare(const void *n1, const void *n2);

void eat_spaces(char **source);

void init_keys(void);

void add_player_key(char *key,char *out,ranktype (*f)(struct char_data *ch));
struct key_data *search_key(char *key);

