/*************************************************************************
*  File: rank.h                                Designed for tbaMUD 3.52  *
*  Usage: implementation of automatic player rankings                    *
*         based on a ranking system created by Slug for CircleMUD 3.0    *
*                                                                        *
*  All rights reserved.                                                  *
*                                                                        *
*  Copyright (C) 1996 Slug                                               *
*  Rewritten in 2007 by Stefan Cole (a.k.a. Jamdog)                      *
*  To see this in action, check out TrigunMUD                            *
************************************************************************ */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "limits.h"
#include "screen.h"

#include "rank.h"

/* extern vars */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;

/* internal global vars */
struct key_data *key_list = NULL;
struct key_data *key_list_tail = NULL;
struct key_data *the_key = NULL;

int maxkeylength = 5;	/* used for formatting in rankhelp */
int maxoutlength = 5;

int keys_inited = FALSE;
int KEY_PRINTING = FALSE;	/* used by rank functions sometimes */

char kbuf[80];		/* used by rank functions */
char *kbp;

const char *rank_type_names[]={
  "Player",
  "\n"
};

ACMD(do_rank)
{
    int i,j,k,t;
	int rk;
    int toprank;
    char buffer[MAX_STRING_LENGTH];
    char keybuf[MAX_STRING_LENGTH], nbuf[MAX_STRING_LENGTH];
    struct char_data *c;
    struct descriptor_data *d;
    struct key_data *tk;
    struct rank_data tt[MAX_RANKED];
    bool first;

    char *nbufp;

   if (!keys_inited) {
	init_keys();
	keys_inited = TRUE;
    log("Setting up Rank keys");
   }

   if (IS_NPC(ch))
	return;

   skip_spaces(&argument);

   half_chop(argument, keybuf, nbuf);
   nbufp = nbuf;

   if(!(*keybuf))
	 strcpy(keybuf,"help");

   the_key = search_key(keybuf);

   if (the_key == NULL) {
     send_to_char(ch, "Please specify a valid key. Type @yrank help@n for instructions.\n\r");
     return;
   }
   else if (!strcmp(the_key->keystring,"help")) {
     j = 80 / (maxkeylength+1);
     i = 0;
     buffer[0] = '\0';
     for (t = 0; t<NUM_RANK_TYPES; t++) {
       first=TRUE;
       for(tk=key_list;tk;tk=tk->next) {
          if (tk->rank_type == t) {
            if (first) {
              sprintf(kbuf,"\r\n@W%s%s rank keys%s@n\r\n", CBCYN(ch, C_NRM), rank_type_names[t], CCNRM(ch, C_NRM));
              strcat(buffer,kbuf);
              first=FALSE;
            }
            sprintf(kbuf,"@C%s%-*s @n", CCCYN(ch, C_NRM),maxkeylength,tk->keystring);
            strcat(buffer,kbuf);
            if (++i % j == 0)
              strcat(buffer,"\n\r");
         }
       }
     }
     strcat(buffer,"\n\r");
     strcat(buffer,"Usage: rank <key> [+-][max]\n\r");
     send_to_char(ch, "%s%s", buffer, CNRM);
   }
   else{
     k = 0;
     if (the_key->rank_type == RANKTYPE_PLAYER) {
       for (d = descriptor_list; d; d = d->next) {
         if (!d->connected) {
           if (d->original)
             c = d->original;
           else
             c = d->character;

           /* Imps see everyone regardless of NORANK flag */
           if ((CAN_SEE(ch, c) || (GET_LEVEL(ch)==LVL_IMPL)) && GET_LEVEL(c) < LVL_IMMORT) {
             if (k) {
               if (k < MAX_RANKED) {
                 tt[k].ch = c;
                 k++;
               } else {    /*list is full */
                 break;
               }
             } else {        /* first player in list */
               tt[k].ch = c;
               k++;
             }
           }
         }
       }
     } else {
       log("SYSERR: Unhandled rank type (%d) in do_rank", the_key->rank_type);
     }
     toprank = TRUE;
     if (!strcmp(the_key->keystring,"deaths"))
       toprank = FALSE;
     if (*nbufp == '-') {
       toprank = FALSE;
       nbufp++;
     } else if (*nbufp == '+') {
       toprank = TRUE;
       nbufp++;
     }
     rk = atoi(nbufp);
     if (rk == 0)
	    rk = 20;
     if (k < rk)
	    rk = k;
     for(i=0;i<k;i++) {
       if (the_key->rank_type == RANKTYPE_PLAYER) {
         strcpy(tt[i].key,GET_PLAYER_KEY(tt[i].ch,the_key));
       }
     }
     /* sort */
     if (toprank)
       qsort(tt, k, sizeof(struct rank_data), rank_compare_top);
     else
       qsort(tt, k, sizeof(struct rank_data), rank_compare_bot);

     /*print out */
     KEY_PRINTING = TRUE;
    send_to_char(ch, "\r\n@D-=-=-=-=-=-=-=+-+-++-++-++-++-++-++-++-++-++-++-++-+-+=-=-=-=-=-=-=-@n\r\n");
     send_to_char(ch, "@W%s%s %d %ss ranked by %s.%s\r\n", CCYEL(ch, C_NRM), toprank ? "@GTop" : "@RBottom", rk, rank_type_names[the_key->rank_type], the_key->outstring, CCNRM(ch, C_NRM));
    send_to_char(ch, "@D-=-=-=-=-=-=-=+-+-++-++-++-++-++-++-++-++-++-++-++-+-+=-=-=-=-=-=-=-@n\r\n");
     for (i = 0; i < rk; i++) {
       if (the_key->rank_type == RANKTYPE_PLAYER) {
         strcpy(kbuf,GET_PLAYER_KEY(tt[i].ch,the_key));
       }
       for(kbp=kbuf;*kbp == ' ';kbp++);  /* Skip spaces */
       if (the_key->rank_type == RANKTYPE_PLAYER) {
         send_to_char(ch, "@W#%2d:@Y %15s @G %-*s@n\n\r", i + 1, GET_NAME(tt[i].ch), count_color_chars(kbp)+18, kbp);
       }
     }
//     send_to_char(ch, "%s\n\r", CONFIG_OK);
    send_to_char(ch, "@D-=-=-=-=-=-=-=+-+-++-++-++-++-++-++-++-++-++-++-++-+-+=-=-=-=-=-=-=-@n\r\n");

   }

}

void init_keys(void) {

/* put the add_keys in the order you want them displayed in help */

 //Alphabetized list!!!!


 add_player_key("age",        "age",                        rank_age);
 add_player_key("armor",      "armor",                      rank_armor);
 add_player_key("bank",       "bank balance",               rank_bank);
 add_player_key("bionic",     "bionic percent",             rank_bionic);
 add_player_key("birth",      "time since creation",        rank_birth);
 add_player_key("cha",        "charisma",                   rank_cha);
 add_player_key("con",        "constitution",               rank_con);
 add_player_key("curhp",      "hit points",                 rank_curhp);
 add_player_key("damroll",    "damage bonus",               rank_damroll);
 add_player_key("deaths",     "number of deaths",           rank_deaths);
 add_player_key("dex",        "dexterity",                  rank_dex);
 add_player_key("dts",        "death traps hit",            rank_dts);
 add_player_key("encumbrance","percent encumbered",         rank_encumb);
 add_player_key("eventmobs",  "event mobs killed",			rank_eventmob);
 add_player_key("fame",       "fame",                       rank_fame);
 add_player_key("fatness",    "body mass index",            rank_fatness);
 add_player_key("fitness",    "sum of stats",               rank_fitness);
 add_player_key("height",     "height",                     rank_height);
 add_player_key("hitroll",    "hit bonus",                  rank_hitroll);
 add_player_key("hp",         "hit points",                 rank_hp);
 add_player_key("hpregen",    "hitpoint regen",             rank_hpregen);
 add_player_key("int",        "intelligence",               rank_int);
 add_player_key("level",      "level",                      rank_level);
 add_player_key("units",      "money carried",              rank_units);
 add_player_key("moves",      "move points",                rank_moves);
 add_player_key("mvregen",    "move regen",                 rank_mvregen);
 add_player_key("pcn",        "perception",                 rank_pcn);
 add_player_key("played",     "time played",                rank_played);
 add_player_key("power",      "the sum of hp, psi, moves",  rank_power);
 add_player_key("psi",        "psi  points",                rank_psi);
 add_player_key("psiregen",   "psi regen",                  rank_psiregen);
 add_player_key("qp",         "qp",                         rank_qp);
 add_player_key("questsdone", "most quests completed",      rank_questsc);
 add_player_key("rifts",	  "most rifts closed",			rank_rifts);
 add_player_key("str",        "strength",                   rank_str);
 add_player_key("weight",     "weight",                     rank_weight);
 add_player_key("xp",         "total experience points",    rank_xp);

 /* 'help' should always be added last */
 add_player_key("help","help",NULL);

} /* end init_keys */

struct key_data *search_key(char *key) {

  struct key_data *tk;

  if (strlen(key))
    for(tk=key_list;tk;tk=tk->next)
      if (!strncmp(tk->keystring,key,strlen(key)))
	    return tk;

  return NULL;

} /* end search_key */

void add_player_key(char *key,char *out,ranktype (*f)(struct char_data *ch)) {

  struct key_data *tmp_key;

  CREATE(tmp_key,struct key_data,1);
  tmp_key->keystring = strdup(key);
  tmp_key->outstring = strdup(out);
  tmp_key->plrfunction = f;
  tmp_key->rank_type = RANKTYPE_PLAYER;
  tmp_key->next = NULL;

  maxkeylength = MAX(maxkeylength,strlen(key));
  maxoutlength = MAX(maxoutlength,strlen(out));

/* append to list */
  if (!key_list)
    key_list = key_list_tail = tmp_key;
  else {
    key_list_tail->next = tmp_key;
    key_list_tail = tmp_key;
  }

} /* end add_key */

void eat_spaces(char **source) {
  while (**source == ' ')
	  (*source)++;
} /* end eat_spaces */


int rank_compare_top(const void *n1, const void *n2)
{
  char k1[80], k2[80];

  strcpy(k1,(((struct rank_data *)n1)->key));
  strcpy(k2,(((struct rank_data *)n2)->key));

  return(strcmp(k2,k1));
}
int char_compare(const void *n1, const void *n2)
{
    return ((*((char *)n1)) - (*((char *)n2)));
}

int rank_compare_bot(const void *n1, const void *n2)
{
  char k1[80], k2[80];

  strcpy(k1,(((struct rank_data *)n1)->key));
  strcpy(k2,(((struct rank_data *)n2)->key));

  return(strcmp(k1,k2));
}


/* RANK FUNCTIONS */

RANK_PLAYER(rank_hp) {
  sprintf(kbuf,"%27d",GET_MAX_HIT(ch));
  return(kbuf);
} /* end rank_hp */
RANK_PLAYER(rank_hpregen) {
  sprintf(kbuf,"%27d",hit_gain(ch)/10);
  return(kbuf);
} /* end rank_hp */

RANK_PLAYER(rank_psi) {
  sprintf(kbuf,"%27d",GET_MAX_PSI(ch));
  return(kbuf);
} /* end rank_psi */
RANK_PLAYER(rank_psiregen) {
  sprintf(kbuf,"%27d",psi_gain(ch)/10);
  return(kbuf);
} /* end rank_psi */

RANK_PLAYER(rank_moves) {
  sprintf(kbuf,"%27d",GET_MAX_MOVE(ch));
  return(kbuf);
} /* end rank_move */
RANK_PLAYER(rank_mvregen) {
  sprintf(kbuf,"%27d",move_gain(ch)/10);
  return(kbuf);
} /* end rank_move */

RANK_PLAYER(rank_curhp) {
  sprintf(kbuf,"%27d",GET_HIT(ch));
  return(kbuf);
} /* end rank_hp */

RANK_PLAYER(rank_power) {
  sprintf(kbuf,"%27d",(GET_MAX_MOVE(ch) + GET_MAX_PSI(ch) + GET_MAX_HIT(ch)));
  return (kbuf);
} /*end rank_power */

RANK_PLAYER(rank_str) {
  sprintf(kbuf,"%20d",GET_STR(ch));
  return(kbuf);
} /* end rank_str */

RANK_PLAYER(rank_int) {
  sprintf(kbuf,"%20d",GET_INT(ch));
  return(kbuf);
} /* end rank_int */

RANK_PLAYER(rank_pcn) {
  sprintf(kbuf,"%20d",GET_PCN(ch));
  return(kbuf);
} /* end rank_pcn */

RANK_PLAYER(rank_qp) {
  sprintf(kbuf,"%20d",GET_QUESTPOINTS(ch));
  return(kbuf);
} /* end rank_qp */

RANK_PLAYER(rank_dex) {
  sprintf(kbuf,"%20d",GET_DEX(ch));
  return(kbuf);
}
RANK_PLAYER(rank_bionic) {
  sprintf(kbuf,"%20d", BIONIC_PERCENT(ch));
  return(kbuf);
} /* end rank_fame */
RANK_PLAYER(rank_fame) {
  sprintf(kbuf,"%20d",GET_FAME(ch));
  return(kbuf);
} /* end rank_fame */

RANK_PLAYER(rank_con) {
  sprintf(kbuf,"%20d",GET_CON(ch));
  return(kbuf);
} /* end rank_con */

RANK_PLAYER(rank_cha) {
  sprintf(kbuf,"%20d",GET_CHA(ch));
  return(kbuf);
} /* end rank_cha */

RANK_PLAYER(rank_fitness) {
  sprintf(kbuf,"%20d",GET_STR(ch) + GET_INT(ch) + GET_PCN(ch) + GET_DEX(ch) + GET_CON(ch) + GET_CHA(ch));
  return(kbuf);
} /* end rank_fitness */

RANK_PLAYER(rank_hitroll) {
  sprintf(kbuf,"%20d",GET_HITROLL(ch));
  return(kbuf);
} /* end rank_hitroll */

RANK_PLAYER(rank_damroll) {
  sprintf(kbuf,"%20d",GET_DAMROLL(ch));
  return(kbuf);
} /* end rank_damroll */

RANK_PLAYER(rank_armor) {
  sprintf(kbuf,"%20d",GET_AC(ch));
  return(kbuf);
} /* end rank_armor */

RANK_PLAYER(rank_xp) {
  sprintf(kbuf,"%27d",GET_EXP(ch));
  return(kbuf);
} /* end rank_xp */

RANK_PLAYER(rank_height) {
  sprintf(kbuf,"%20d",GET_HEIGHT(ch));
  return(kbuf);
} /* end rank_height */

RANK_PLAYER(rank_weight) {
sprintf(kbuf,"%20d",GET_WEIGHT(ch));
return(kbuf);
} 
RANK_PLAYER(rank_encumb) {
sprintf(kbuf,"%20d",ENCUMBRANCE(ch));
return(kbuf);
} /* end rank_weight */

RANK_PLAYER(rank_fatness) {
  float bmi;
  bmi = ((float)GET_WEIGHT(ch)*10000.0)/
        (2.2*(float)GET_HEIGHT(ch)*(float)GET_HEIGHT(ch));
  sprintf(kbuf,"%20.2f", bmi);
  return(kbuf);
} /* end rank_fatness */


RANK_PLAYER(rank_units) {
  sprintf(kbuf,"%27d",GET_UNITS(ch));
  return(kbuf);
} /* end rank_units */

RANK_PLAYER(rank_bank) {
  sprintf(kbuf,"%27d",GET_BANK_UNITS(ch));
  return(kbuf);
} /* end rank_bank */

RANK_PLAYER(rank_age) {
  sprintf(kbuf,"%3d",GET_AGE(ch));
  return(kbuf);
} /* end rank_age */


RANK_PLAYER(rank_deaths) {
  sprintf(kbuf,"%20d",GET_NUM_DEATHS(ch));
  return(kbuf);
} /* end rank_deaths */
RANK_PLAYER(rank_dts) {
  sprintf(kbuf,"%20d",GET_NUM_DTS(ch));
  return(kbuf);
} /* end rank_deaths */

RANK_PLAYER(rank_questsc) {
  sprintf(kbuf,"%20d",GET_NUM_QUESTS(ch));
  return(kbuf);
} /* end rank_squits */

RANK_PLAYER(rank_played) {
  struct time_info_data playing_time;
  playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
  sprintf(kbuf, "%3d days, %2d hours.", playing_time.day, playing_time.hours);
  return(kbuf);
} /* end rank_played */

RANK_PLAYER(rank_birth) {
  struct time_info_data *playing_time;
  playing_time = age(ch);
  sprintf(kbuf, "%3d days, %2d hours.", playing_time->day, playing_time->hours);
  return(kbuf);
} /* end rank_birth */

RANK_PLAYER(rank_level) {
  sprintf(kbuf,"%20.2d",GET_LEVEL(ch));
  return (kbuf);
} /* end rank_level */

RANK_PLAYER(rank_rifts) {
  sprintf(kbuf,"%20d",GET_RIFTS(ch));
  return (kbuf);
} /* end rank_rifts */

RANK_PLAYER(rank_eventmob) {
  sprintf(kbuf,"%20ld",GET_EVENTMOBS(ch));
  return (kbuf);
}
