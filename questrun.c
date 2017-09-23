#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"

SPECIAL(shop_keeper);

void send_to_quest(char *messg);
void scatter_quest(struct char_data *ch, byte type, int bits);
void freezetag_start(struct char_data *ch, int duration);
void freezetag_end(void);
void freezetag_check(void);
void arena_start(struct char_data *ch, int type);

#define Q_SCATTER_MARBLE	1 /* scatter all marbles in inventory */
#define Q_SCATTER_ALL		2 /* scatter all items in inventory */
#define Q_FREEZETAG			3 /* freezetag game, teams tag each other for 10 minutes */
#define Q_CTF				4 /* team must capture other team's flag, and return to base (uses zone 112) */
#define Q_CW				5 /* must destroy all of the other team's bases (uses zone 109) */

#define RESTRICT_ALL_MOB	2	/* Items only go to mobs */
#define RESTRICT_ALL_ROOM	1	/* Items only go to rooms */
#define RESTRICT_MORE_MOB	4	/* More items go to mobs */
#define RESTRICT_MORE_ROOM	3	/* More items go to rooms */
#define RESTRICT_THIS_ZONE	5	/* Restrict to only this zone */

/* freezetag was originally uploaded to the circlemud ftp by dustlos@hotmail.com *
 * Lump saw it, changed it, fixed it, fucked with it, removed un-needed shit and *
 * made it overall a lot better for CyberAssault.                                */
int freezetag_timer = 0;
int freezetag_red = 0; /* number of people on red who are currently frozen */
int freezetag_blue = 0; /* nubmer of people on blue who are currently frozen */

ACMD(do_questrun)
{
  char quest[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  byte type;
  int bits = 0, duration = 10;

  two_arguments(argument, quest, arg);

  if (!*quest) {
    send_to_char(ch, "Usage:\r\n"
		 "  questrun (quest) [options]\r\n\r\n");

    send_to_char(ch, "Available quests:\r\n"
                 " marbles   -  randomly scatter marbles from inventory\r\n"
                 " scatter   -  randomly scatter ALL ITEMS from inventory\r\n"
                 " @R(DISABLED)@n freezetag -  players run around tagging each other for 10 minutes\r\n"
                 " @R(DISABLED)@n ctf       -  teams capture each other's flags in team arena\r\n"
                 " @R(DISABLED)@n colorwars -  teams kill the other team's bases\r\n\r\n");
    send_to_char(ch, "Marble and Scatter quest options:\r\n"
                 " allmob    -  Items only go to mobs\r\n"
                 " allroom   -  Items only go to rooms\r\n"
                 " moremob   -  75%% mobs, 25%% rooms\r\n"
                 " moreroom  -  25%% mobs, 75%% rooms\r\n"
                 " zone      -  Items only go to this zone\r\n\r\n"
                 "Freezetag options:\r\n"
                 " (number)  -  how many minutes freezetag will last, default is 10\r\n\r\n"
                 "Make sure you have teams set for CW, CtF and freezetag. Blue and Red only\r\n"
                 "except in ColorWars, where every team color can be used.\r\n"
                 "To set teams, do: set <player> team <color>\r\n");
    return;
  }
  if (!*arg) {
	send_to_char(ch, "You need another argument, type questrun to see a list of options.\r\n");
	return;
  }
  if (!strcmp(quest, "marbles")) 
    type = Q_SCATTER_MARBLE; // randomly scatter marbles
  else if (!strcmp(quest, "scatter"))
    type = Q_SCATTER_ALL; // randomly scatter inventory
  //else if (!strcmp(quest, "freezetag"))
  //  type = Q_FREEZETAG; // freezetag!
  //else if (!strcmp(quest, "ctf"))
  //  type = Q_CTF; // capture-the-flag
  //else if (!strcmp(quest, "cw") || !str_cmp(quest, "colorwars"))
  //  type = Q_CW; // ColorWars
  else {
    send_to_char(ch, "That quest isn't defined.\r\n");
    return;
  }

  if (type == Q_SCATTER_MARBLE || type == Q_SCATTER_ALL) {
    // can only use one of these
    if (isname("allmob", arg))
      bits = RESTRICT_ALL_MOB;
    else if (isname("allroom", arg))
      bits = RESTRICT_ALL_ROOM;
    else if (isname("moremob", arg))
      bits = RESTRICT_MORE_MOB;
    else if (isname("moreroom", arg))
      bits = RESTRICT_MORE_ROOM;
    else if (isname("zone", arg)) {
      if (ZONE_FLAGGED(world[ch->in_room].zone, ZONE_REMORT_ONLY | ZONE_NO_MARBLE | ZONE_QUEST)) {
        send_to_char(ch, "No can do, zone flags!\r\n");
		return;
	  }
	  else
        bits = RESTRICT_THIS_ZONE;
    }

    scatter_quest(ch, type, bits);
  } 
  else if (type == Q_FREEZETAG) {
    if (*arg && is_number(arg))
      duration = atoi(arg);
    else
      duration = 10;

    if (duration < 5 || duration > 30) {
      send_to_char(ch, "Time must be between 5 and 30 minutes.\r\nIf you need this changed ask Gahan.\r\n");
      return;
    }
    freezetag_start(ch, duration);
  }
  else if (type == Q_CTF || type == Q_CW)
    arena_start(ch, type);
  else
    send_to_char(ch, "Something fucked up...\r\n");
}

void send_to_quest(char *messg) {
  struct descriptor_data *i;
  char buf[MAX_INPUT_LENGTH];
     
  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && PRF_FLAGGED(i->character, PRF_QUEST))
      sprintf(buf, "%s", messg);
		send_to_char(i->character, "%s", buf);              
}   

void scatter_quest(struct char_data *ch, byte type, int bits)
{
  struct obj_data *inv, *inv_next;   
  int target = 0, where_room = 0, where_mob = 0;
  struct char_data *mob;
  int j = 0;
  bool found = FALSE;
  bool scattered = FALSE;

  inv = ch->carrying;

  /* preliminary counting of mobs */
  for (mob = character_list; mob; mob = mob->next) 
    if (IS_NPC(mob))
      j++;

  while(inv != NULL) {
  
    inv_next = inv->next_content;
 
    if (type == Q_SCATTER_ALL || (type == Q_SCATTER_MARBLE && inv->obj_flags.type_flag == ITEM_MARBLE_QUEST)) 
    {
      /* decide if it goes to a room or a mob */
      found = TRUE;
      scattered = FALSE;

      if (bits == 1)
        target = 1;
      else if (bits == 2)
        target = 2;
      else if (bits == 3) {
        if (rand_number(0, 3) > 0)
          target = 1;
        else
          target = 2;
      } else if (bits == 4) {
        if (rand_number(0, 3) > 0)
          target = 2;
        else
          target = 1;
      } else
        target = rand_number(1, 2);

      if (target == 1) {
        do {	
            where_room = rand_number(1, top_of_world);

          if (bits == 5 && world[where_room].zone != world[ch->in_room].zone)
            continue;
          
          if (ROOM_FLAGGED(where_room, ROOM_DEATH) || 
            (ROOM_FLAGGED(where_room, ROOM_GODROOM)) ||
            (ROOM_FLAGGED(where_room, ROOM_PRIVATE)) ||
            (ROOM_FLAGGED(where_room, ROOM_HOUSE)) ||
            (ROOM_FLAGGED(where_room, ROOM_HOUSE_CRASH)) ||
            (ROOM_FLAGGED(where_room, ROOM_NO_TP)) ||  
            (ROOM_FLAGGED(where_room, ROOM_ATRIUM)))
              continue;

          /* our random room is part of a NO_MARBLE zone! blah, let's try again */
          if (ZONE_FLAGGED(world[where_room].zone, ZONE_REMORT_ONLY | ZONE_NO_MARBLE | ZONE_QUEST))
            continue;

          obj_from_char(inv);
          obj_to_room(inv, where_room);
          scattered = TRUE;

        } while (!scattered);

      } else if (target == 2) {
        do {
          /* load onto a mob */
          where_mob = rand_number(1, j);

          for (mob = character_list; mob; mob = mob->next) {
            if (IS_NPC(mob))
  	       where_mob--;

            /* we have our random mob */
            if (where_mob == 0)
              break;
          }

          /* something screwed up somewhere */
          if (!mob || !IS_NPC(mob))
            continue;

          if (bits == 5 &&
              world[ch->in_room].zone != world[mob->in_room].zone)
            continue;

          /* don't give marbles to shop keepers */
          if (mob_index[GET_MOB_RNUM(mob)].func == shop_keeper)
            continue;

          /* Can't see marbles that are above mort level */
          if (GET_LEVEL(mob) >= LVL_IMMORT)
            continue;

          /* our random mob is in a NO_MARBLE zone! blah, let's try again */
          if (ZONE_FLAGGED(world[mob->in_room].zone, ZONE_REMORT_ONLY | ZONE_NO_MARBLE | ZONE_QUEST))
            continue;

          obj_from_char(inv);
          obj_to_char(inv, mob);
          scattered = TRUE;

        } while (!scattered);
      }
    }
	else if (!found && type == Q_SCATTER_MARBLE) {
		send_to_char(ch, "I cannot find any marble quest items in your inventory!!\r\n");
		return;
	}
	else if (!found && type == Q_SCATTER_ALL) {
		send_to_char(ch, "I cannot find any items in your inventory!!\r\n");
		return;
	}
	else
		break;
    if (scattered) {
      if (inv_next)
        inv = inv_next;
      else
        break;
    }
  }

  if(!found) {
    if (type == Q_SCATTER_MARBLE)
      send_to_char(ch, "You don't have any marble quest items in your inventory!!\r\n");
    else
      send_to_char(ch, "You don't have any items in your inventory!!\r\n");
    return;
  } else {
    if (type == Q_SCATTER_MARBLE)
      send_to_char(ch, "Your marbles have been scattered across the mud.\r\n");
    else
      send_to_char(ch, "Your inventory has been scattered across the mud.\r\n");
  }
}


ACMD(do_request)
{
   /* This function by Psycho, 2/13/98.  Part of AutoQuest routines */

  struct obj_data *obj;
  struct char_data *mob;
  int whichquest = 0, rand = 0, whichroom = 0, i = 0, j = 0, whichmob = 0;

send_to_char(ch, "Autoquest is temporarily disabled.\n\r");
return;
 
   if((ch->in_room) != (real_room(3067)))
      send_to_char(ch, "Quests must be requested at the Questmaster.\n\r");
   else
      {
//      if(ch->player_specials->saved.time_to_next_quest > 0)
       if (1)
         {
         send_to_char(ch, "Sorry, you cannot quest right now.\n\r");
         send_to_char(ch, "Try again later.\n\r");
         }
      else if((GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) <= LVL_GRGOD))
         {
         send_to_char(ch, "You're an Immortal! Don't be silly.\n\r");
         }
      else if(GET_LEVEL(ch) < 15)
         {
         send_to_char(ch, "Sorry, you must be level 15 or over to participate in quests.\n\r");
         } 
      else
         {
         whichquest = rand_number(1, 10);
         switch(whichquest)
            { 
            case 1:
              send_to_char(ch, "Find the ball whose color is that of sailor's delight.\n\r");
              break;
            case 2:
              send_to_char(ch, "The twisted shape of this artifact is the color of the ocean.\n\r");
              break;
            case 3:
              send_to_char(ch, "To find the transparent rod is your quest.\n\r");
              break;
            case 4:
              send_to_char(ch, "Filled with a glowing element is a mineral mass.\n\r");
              break;
           case 5:
              send_to_char(ch, "Precious and rare, the pebble's value is beyond compare.\n\r");
              break;
           case 6:
              send_to_char(ch, "Vulcan forged the fragment you seek.\n\r");
              break;
           case 7:
              send_to_char(ch, "The sphere pulses with luminescence of power.\n\r");
              break;
           case 8:
              send_to_char(ch, "The blood of the cowards covers the triangle.\n\r");
              break;
           case 9:
              send_to_char(ch, "Seek the rod colored as the morning sky.\n\r");
              break;
           case 10:
              send_to_char(ch, "The heliotrope needle can be recovered only by the bravest.\n\r");
              break;
           default:
              send_to_char(ch, "Undefined quest.\n\r");
//              ch->player_specials->saved.current_quest = 0;
              break;
           }

//          ch->player_specials->saved.current_quest = whichquest;
//          ch->player_specials->saved.time_to_next_quest = 150; 
          obj = read_object(whichquest, REAL);

          rand = rand_number(1, 2);
          if(rand == 1)            /* Load item in a room */
             {
             whichroom = rand_number(1, top_of_world + 1);
             while(ROOM_FLAGGED(whichroom, ROOM_DEATH) ||
                (ROOM_FLAGGED(whichroom, ROOM_GODROOM)) ||
                (ROOM_FLAGGED(whichroom, ROOM_PRIVATE)) ||
                (ROOM_FLAGGED(whichroom, ROOM_HOUSE)) ||
                (ROOM_FLAGGED(whichroom, ROOM_HOUSE_CRASH)) ||
                (ROOM_FLAGGED(whichroom, ROOM_ATRIUM)))
                whichroom = rand_number(1, top_of_world + 1);
             obj_to_room(obj, whichroom);
             }

          if(rand == 2)            /* Load item on a mob */
             {
             for(mob = character_list; mob; mob = mob->next) if(IS_NPC(mob)) i++;
             j = i;
             whichmob = rand_number(1, j);
             for (mob = character_list; mob; mob = mob->next)
                {
                if(IS_NPC(mob)) whichmob--;
                if(whichmob == 0)
                   {
                   if(IS_NPC(mob))
                      {
                      obj_to_char(obj, mob);
                      break;
                      }
                   else
                      {
                      log("REALLY BAD SHIT: Request just gave the part to a player.\n\r");
                      }
                   }
                }
             }
         }
      }
}


ACMD(do_complete)
{
   /* This function by Psycho, 2/13/98. Part of AutoQuest */

   struct obj_data *obj;
   int whichquest = 0, questitem = 0;
   bool found = FALSE;

   obj = ch->carrying;
   whichquest = 0; //ch->player_specials->saved.current_quest;

   if(whichquest == 0)
      {
      send_to_char(ch, "You currently are not on a quest.\n\r");
      }

   else
      {
      while((obj != NULL) && (found != TRUE))
         {
         if(obj->obj_flags.type_flag == ITEM_AUTOQUEST)
            {
            questitem = obj->obj_flags.value[0];
            if(questitem == whichquest) found = TRUE; 
               else obj = obj->next_content;
            }
         else obj = obj->next_content;
         }
     
      if(found)
         {
         extract_obj(obj);
         send_to_char(ch, "Congratulations, your quest is complete.\n\r");
         send_to_char(ch, "You have been rewarded handsomely.\n\r");
         GET_QUESTPOINTS(ch) += 1;
//         ch->player_specials->saved.current_quest = 0;
         }
      else
         {
         send_to_char(ch, "You have not yet completed your quest.\n\r");
         }
      }
}


ACMD(do_repeat)
{
  int whichquest;

  if((ch->in_room) != (real_room(3067)))
     send_to_char(ch, "To have your quest repeated, visit the Quest Master.\n\r");
  else
     {
     whichquest = 0; //ch->player_specials->saved.current_quest;
     switch(whichquest)
         { 
         case 0:
            send_to_char(ch, "You are not on a quest.\n\r");
            break;
         case 1:
            send_to_char(ch, "Find the ball whose color is that of sailor's delight.\n\r");
            break;
         case 2:
            send_to_char(ch, "The twisted shape of this artifact is the color of the ocean.\n\r");
            break;
         case 3:
            send_to_char(ch, "To find the transparent rod is your quest.\n\r");
            break;
         case 4:
            send_to_char(ch, "Filled with a glowing element is a mineral mass.\n\r");
            break;
         case 5:
            send_to_char(ch, "Precious and rare, the pebble's value is beyond compare.\n\r");
            break;
         case 6:
            send_to_char(ch, "Vulcan forged the fragment you seek.\n\r");
            break;
         case 7:
            send_to_char(ch, "The sphere pulses with luminescence of power.\n\r");
            break;
         case 8:
            send_to_char(ch, "The blood of the cowards covers the triangle.\n\r");
            break;
         case 9:
            send_to_char(ch, "Seek the rod colored as the morning sky.\n\r");
            break;
         case 10:
            send_to_char(ch, "The heliotrope needle can be recovered only by the bravest.\n\r");
            break;
         default:
            send_to_char(ch, "Undefined quest.\n\r");
           }
    }

}


/* freezetag stuff */

void freezetag_end()
{
  struct descriptor_data *d;
  int red = 0, blue = 0;
  char buf[MAX_INPUT_LENGTH];

  /* let's count how many people are still on each team */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PLAYING && d->character) {
       if (GET_TEAM(d->character) == TEAM_RED) red++;
       else if (GET_TEAM(d->character) == TEAM_BLUE) blue++;
    }
  }

  sprintf(buf, "The freeze tag match has ended!  Frozen: %d/%d red, %d/%d blue\r\n",
	freezetag_red, red, freezetag_blue, blue);
  send_to_quest(buf);

  if (blue - freezetag_blue == red - freezetag_red)
    sprintf(buf, "There has been a tie in the freeze tag match!\r\n");
  else if (blue - freezetag_blue > red - freezetag_red)
    sprintf(buf, "The Blue Team has won the match!\r\n");
  else
    sprintf(buf, "The Red Team has won the match!\r\n");
  send_to_quest(buf);

  freezetag_red = 0;
  freezetag_blue = 0;
  freezetag_timer = 0;

  for (d = descriptor_list; d; d = d->next)
    if (d->character && PRF_FLAGGED(d->character, PRF_FROZEN))
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_FROZEN);
}

ACMD(do_clearteam) {
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_PLAYING && d->character)
      if (GET_TEAM(d->character)) {
        GET_TEAM(d->character) = TEAM_NONE;
      }
}

void freezetag_check()
{
  struct descriptor_data *d;
  int red = 0, blue = 0;

  /* let's count how many people are still on each team */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PLAYING && d->character) {
       if (GET_TEAM(d->character) == TEAM_RED) red++;
       else if (GET_TEAM(d->character) == TEAM_BLUE) blue++;
    }
  }

  if (freezetag_timer > 0 && (red - freezetag_red == 0 || blue - freezetag_blue == 0))
     freezetag_end();
}

void freezetag_start(struct char_data *ch, int duration)
{
  struct descriptor_data *d;
  int red = 0, blue = 0;
  char buf[MAX_INPUT_LENGTH];

  if (freezetag_timer > 0) { 
     send_to_char(ch, "A freeze tag match has already started!\r\n");
     return;
  }
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PLAYING) {
       if (GET_TEAM(d->character) == TEAM_RED) red++;
       else if (GET_TEAM(d->character) == TEAM_BLUE) blue++;
    }
  }
  if (!blue || !red) {
     send_to_char(ch, "Do the teams even exist?\r\n");
     return;
  }

  freezetag_red = 0;
  freezetag_blue = 0;
  freezetag_timer = duration;
  sprintf(buf, "A freeze tag match has started!  %d blue vs. %d red, %d minutes\r\n",
          blue, red, freezetag_timer);
  send_to_quest(buf);

  mudlog(NRM, LVL_GOD, FALSE, "%s has started a Freezetag game (%d blue/%d red/%d minutes).",
	GET_NAME(ch), blue, red, freezetag_timer);
  

}

ACMD(do_tag)
{
  struct char_data *vict = NULL;
  int chance;
  char buf[MAX_INPUT_LENGTH];
  
  if (IS_NPC(ch))
    return;

  if (!freezetag_timer) {
     send_to_char(ch, "There isn't a freeze tag match running!\r\n");
     return;
  }

  if (GET_LEVEL(ch) < LVL_IMMORT && PRF_FLAGGED(ch, PRF_FROZEN)) {
    send_to_char(ch, "You've been tagged!  Can't do much until your own team tags you.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "You need to tag someone!\r\n");
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf, NULL))) {
    send_to_char(ch, "They don't seem to be around here.\r\n");
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
  }
  if (ch == vict) {
    send_to_char(ch, "Ha ha ha ha...\r\n");
    return;
  }
  if (IS_NPC(vict)) {
    send_to_char(ch, "You can only tag other players!\r\n");
    return;
  }
  if (!GET_TEAM(ch)) {
    send_to_char(ch, "You aren't even playing freeze tag!\r\n");
    return;
  }
  if (!GET_TEAM(vict)) {
    send_to_char(ch, "They aren't even playing freeze tag!\r\n");
    return;
  }


  if (GET_TEAM(vict) == GET_TEAM(ch) && PRF_FLAGGED(vict, PRF_FROZEN)) {
     send_to_char(ch, "You tag your fellow team mate and restore them!\r\n");
     send_to_char(vict, "You've been thawed!\r\n");
     REMOVE_BIT_AR(PRF_FLAGS(vict), PRF_FROZEN);
     if (GET_TEAM(vict) == TEAM_BLUE) freezetag_blue--;
     else if (GET_TEAM(vict) == TEAM_RED) freezetag_red--;
     sprintf(buf, "%s has thawed %s!\r\n", GET_NAME(ch), GET_NAME(vict));
     send_to_quest(buf);
  } else if (GET_TEAM(vict) != GET_TEAM(ch) && !PRF_FLAGGED(vict, PRF_FROZEN)) {
    /*  Chances of missing a tag:
     * 1/4  -  Equal dex/speed
     * 1/5  -  Faster tagging Slower
     * 1/3  -  Slower tagging Faster
     *  approximate values only, milage may vary
     */
    chance = (GET_DEX(ch) + (30 - GET_DEX(vict))) / 10;  

    if (!rand_number(0, chance)) {
      send_to_char(ch, "You miss the tag!\r\n");
      act("$n leaps at you for the tag, but you dodge out of their way!", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to tag $N, but misses completely!", FALSE, ch, 0, vict, TO_NOTVICT);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return;
    }

     send_to_char(ch, "You tag your enemy, they are frozen!\r\n");
     send_to_char(vict, "You've been tagged!\r\n");
     SET_BIT_AR(PRF_FLAGS(vict), PRF_FROZEN);
     if (GET_TEAM(vict) == TEAM_BLUE) freezetag_blue++;
     else if (GET_TEAM(vict) == TEAM_RED) freezetag_red++;
     sprintf(buf, "%s has frozen %s!\r\n", GET_NAME(ch), GET_NAME(vict));
     send_to_quest(buf);
  } else {
     send_to_char(ch, "That won't accomplish anything at all!\r\n");
     return;
  }

  WAIT_STATE(ch, PULSE_VIOLENCE*2);
  freezetag_check();
}

ACMD(do_freezetag)
{
  struct descriptor_data *d;
  int red = 0, blue = 0;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!freezetag_timer) {
     send_to_char(ch, "There isn't a freeze tag match running!\r\n");
     return;
  }
  if (GET_LEVEL(ch) < LVL_IMMORT && !GET_TEAM(ch)) {
     send_to_char(ch, "You aren't even playing freeze tag!\r\n");
     return;
  }

  if (!*argument || GET_LEVEL(ch) < LVL_GOD) {

    send_to_char(ch, "#------ Freeze  Tag ------#\r\n");

    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING) {
         if (GET_TEAM(d->character) == TEAM_BLUE) {
           send_to_char(ch, "| %-20s  %s |\r\n", GET_NAME(d->character),
                            (PRF_FLAGGED(d->character, PRF_FROZEN) ? "*" : " "));
           blue++;
         }
      }
    }

    send_to_char(ch, "| Blue Tagged : [ %2d/%-2d ] |\r\n",
                freezetag_blue, blue);

    send_to_char(ch, "|                         |\r\n");

    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING) {
         if (GET_TEAM(d->character) == TEAM_RED) {
           send_to_char(ch, "| %-20s  %s |\r\n", GET_NAME(d->character),
                            (PRF_FLAGGED(d->character, PRF_FROZEN) ? "*" : " "));
           red++;
         }
      }
    }

    send_to_char(ch, "| Red Tagged  : [ %2d/%-2d ] |\r\n",
                freezetag_red, red);

    send_to_char(ch, "|  %2d minutes remaining   |\r\n"
                 "#-------------------------#\r\n", freezetag_timer);
    return;
  }

  if (!str_cmp("stop", argument) && GET_LEVEL(ch) >= LVL_GOD) {
    mudlog(NRM, LVL_GOD, FALSE, "%s has terminated the Freezetag game.", GET_NAME(ch));
 
    send_to_char(ch, "You terminate the existing game!\r\n");
    freezetag_end();
  } else
    send_to_char(ch, "Usage:  freezetag [stop]\r\n");

}

/* most of CTF is handled by DG scripts in zone 112 */
/* CW doesn't have any scripts...atleast not yet */
void arena_start(struct char_data *ch, int type)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  struct char_data *guard, *next_m;
  struct obj_data *flag, *book, *next_o;
  int red = 0, blue = 0, green = 0, yellow = 0;
  room_rnum redroom = NOWHERE, blueroom = NOWHERE, greenroom = NOWHERE, yellowroom = NOWHERE;

  switch (type) {
    case Q_CTF:
      redroom = real_room(11230);
      blueroom = real_room(11269);
      break;
    case Q_CW:
      redroom = real_room(10966);
      blueroom = real_room(10967);
      greenroom = real_room(10968);
      yellowroom = real_room(10969);
      break;
  }

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PLAYING && GET_TEAM(d->character) > TEAM_NONE) {
       /* we assume there are teams, so let's just trans them to the start rooms */
       if (GET_TEAM(d->character) == TEAM_RED && redroom != NOWHERE) {
         char_from_room(d->character);
         char_to_room(d->character, redroom);
         red++;
       } else if (GET_TEAM(d->character) == TEAM_BLUE && blueroom != NOWHERE) {
         char_from_room(d->character);
         char_to_room(d->character, blueroom);
         blue++;
       } else if (GET_TEAM(d->character) == TEAM_GREEN && greenroom != NOWHERE) {
         char_from_room(d->character);
         char_to_room(d->character, greenroom);
         green++;
       } else if (GET_TEAM(d->character) == TEAM_YELLOW && yellowroom != NOWHERE) {
         char_from_room(d->character);
         char_to_room(d->character, yellowroom);
         yellow++;
       }
       /* give them the CTF rule book (what about CW?)*/
       if (type == Q_CTF) {
         book = read_object(11202, VIRTUAL);
         obj_to_char(book, d->character);
       }
       look_at_room(d->character, 0);
    }
  }

  /* CTF: we need to purge all existing guards and flags then load up new ones, then reset zone */
  if (type == Q_CTF) {
    /* go through the mob and obj lists, purge all guardians and flags in the game */
    for (guard = character_list; guard; guard = next_m) {
      next_m = guard->next;
      /* also gonna purge all team wandering mobs, and reset the zone later */
      if (IS_NPC(guard) && GET_MOB_VNUM(guard) >= 11200 && GET_MOB_VNUM(guard) <= 11299)
        extract_char(guard);
    }
    for (flag = object_list; flag; flag = next_o) {
      next_o = flag->next;
      if (GET_OBJ_VNUM(flag) == 11200 || GET_OBJ_VNUM(flag) == 11201)
        extract_obj(flag);
    }

    /* load red guardian and flag */
    guard = read_mobile(11200, VIRTUAL);
    char_to_room(guard, real_room(11260));
    GET_TEAM(guard) = TEAM_RED;
    flag = read_object(11200, VIRTUAL);
    equip_char(guard, flag, WEAR_HOLD, TRUE);

    /* load blue guardian and flag */
    guard = read_mobile(11200, VIRTUAL);
    char_to_room(guard, real_room(11239));
    GET_TEAM(guard) = TEAM_BLUE;
    flag = read_object(11201, VIRTUAL);
    equip_char(guard, flag, WEAR_HOLD, TRUE);

    sprintf(buf, "Capture-the-Flag game has started!  %d blue vs. %d red\r\n", blue, red);
    send_to_quest(buf);

    mudlog(NRM, LVL_GOD, FALSE, "%s has started a Capture the Flag game (%d blue/%d red).",
  	GET_NAME(ch), blue, red);

    send_to_char(ch, "\r\nFlag-guardians have been loaded, now the players should\r\n"
                 "spellup their team's guard, and start hunting for the other\r\n"
                 "team's flag.  To win the game, one must GIVE the other team's\r\n"
                 "flag to their own flag guardian (who must be alive!)\r\n\r\n");

    send_to_char(ch, "Note:  When a player dies, depending on how you want the game\r\n"
                 "to go, you can teleport them back into their team's regen room.\r\n"
                 "Red team goes to room 11230;  Blue team goes to room 11269.\r\n");
  } /* end CTF */

  /* let's reset the zone now (we assume red is used in all games, because it's the best! :P) */
  if (redroom != NOWHERE)
    reset_zone(world[redroom].zone, TRUE);

  /* CW: unlock the doors...that's about it */
  if (type == Q_CW) {
    if (red > 0) {
      REMOVE_BIT(W_EXIT(real_room(10901), WEST)->exit_info, EX_LOCKED | EX_CLOSED);
      REMOVE_BIT(W_EXIT(real_room(10902), EAST)->exit_info, EX_LOCKED | EX_CLOSED);
    }
    if (blue > 0) {
      REMOVE_BIT(W_EXIT(real_room(10901), EAST)->exit_info, EX_LOCKED | EX_CLOSED);
      REMOVE_BIT(W_EXIT(real_room(10918), WEST)->exit_info, EX_LOCKED | EX_CLOSED);
    }
    if (green > 0) {
      REMOVE_BIT(W_EXIT(real_room(10901), NORTH)->exit_info, EX_LOCKED | EX_CLOSED);
      REMOVE_BIT(W_EXIT(real_room(10934), SOUTH)->exit_info, EX_LOCKED | EX_CLOSED);
    }
    if (yellow > 0) {
      REMOVE_BIT(W_EXIT(real_room(10901), SOUTH)->exit_info, EX_LOCKED | EX_CLOSED);
      REMOVE_BIT(W_EXIT(real_room(10950), NORTH)->exit_info, EX_LOCKED | EX_CLOSED);
    }

    sprintf(buf, "ColorWars has started!  %d blue  %d red  %d green  %d yellow\r\n",
	blue, red, green, yellow);
    send_to_quest(buf);

    mudlog(NRM, LVL_GOD, FALSE, "%s has started a ColorWars game (%d blue/%d red/%d green/%d yellow).",
  	GET_NAME(ch), blue, red, green, yellow);

    send_to_char(ch, "\r\nNote:  When a player dies, depending on how you want the game\r\n"
                 "to go, you can teleport them back into their team's regen room.\r\n"
                 " Red team goes to room 10966\r\n Blue team goes to room 10967\r\n"
                 " Green team goes to room 10968\r\n Yellow team goes to room 10969\r\n"
                 "Or just stick them in room 10900 (above the Center of the arena)\r\n");
  } /* end CW */

}

