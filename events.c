//////////////////////////////////////////////////////////////////////////////////
// This events.c file ha been written from scratch by Gahan.					//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the world event system for CyberASSAULT, invasions, etc				//
// Anything that has to do with it will be found here or within events.h		//
//////////////////////////////////////////////////////////////////////////////////

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "ban.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "act.h"
#include "genzon.h"
#include "genolc.h"
#include "config.h"
#include "craft.h"
#include "modify.h"
#include "events.h"
#include "class.h"
#include "string.h"

const char *event_types[] = {
	"No one",
	"Demons",
	"Aliens",
	"Mechs",
	"Raiders",
	"\n"
};

struct mob_vnums_type mob_vnums[] = {
	{ 0,0,0,0,0,0,0 },
	{ 0, 10801, 10813, 10825, 10837, 10849, 10861 },
	{ 0, 10804, 10816, 10828, 10840, 10852, 10864 },
	{ 0, 10807, 10819, 10831, 10843, 10855, 10867 },
	{ 0, 10810, 10822, 10834, 10846, 10858, 10870 },
	{ 0,0,0,0,0,0,0 }
};
	
struct event_sizes_type event_sizes[] = {
	{"@BNon-Existent@n", 	0, "0", 0,	0, 10890 }, // 0
	{"@GMild@n", 			1, "10", 10, 2, 10890 },		// 1
	{"@MWidespread@n", 		3, "20", 17, 3, 10890 },	// 2
	{"@yExtensive@n", 		5, "30", 25, 2, 10891 },	// 3
	{"@YImmense@n", 		10, "40", 35, 3, 10891 },	// 4
	{"@rEpidemic@n", 		15, "Remort", 40, 1, 10892 },	// 5
	{"@l@RPandemic@n",		20, "Elite", 45, 2, 10892 }		// 6
};

ACMD(do_eventinfo) {
	int i = MAX_EVENT_AREAS;

	if (!EVENT_LIVE)
		send_to_char(ch, "There are currently no world events going on.\r\n");
	if (EVENT_LIVE) {
		send_to_char(ch, "\r\n@G Current World Event Statistics:@n\r\n");
		send_to_char(ch, "@W-----------------------------------------------------------@n\r\n");
		send_to_char(ch, "@W The world is currently under attack by : @G%s@n\r\n", event_types[event_info.eventtype]);
		send_to_char(ch, "@W The size of invasion is classified as  : @G%s@n\r\n", event_sizes[event_info.eventsize].event_size_text);
		send_to_char(ch, "@W The level of the invasion underway is  : @G%s@n\r\n", event_info.eventlevel);
		send_to_char(ch, "@W There are currently @G%d@W rifts open.@n\r\n", event_info.riftsleft);
		send_to_char(ch, "@W There are currently @G%d@W attackers left to be defeated.@n\r\n", event_info.mobsleft);
		if (GET_LEVEL(ch) >= LVL_IMMORT) {
			send_to_char(ch, "@G-----[For Immortals]---------------------------------------@n\r\n");
			send_to_char(ch, "@G There have been %d shards popped in this event so far.@n\r\n", event_info.shards);
			send_to_char(ch, "@G The current chances of popping a shard is 1 in %d.@n\r\n", (60 + (20 * (event_info.shards +1))));
		}
		send_to_char(ch, "@W-----------------------------------------------------------@n\r\n");
		send_to_char(ch, "@G Areas currently being affected by invasions:@n\r\n");
		
		for (i = 0; i < MAX_EVENT_AREAS; i++) 
			if (event_info.zones[i] != NULL) {
				if (GET_LEVEL(ch) >= LVL_IMMORT)
					send_to_char(ch, "@W - %s @G(Room: %d)@n\r\n", event_info.zones[i], GET_ROOM_VNUM(event_info.rooms[i]));
				else
					send_to_char(ch, "@W - %s@n\r\n", event_info.zones[i]);
			}
		send_to_char(ch, "@W-----------------------------------------------------------@n\r\n");
		send_to_char(ch, "@G To view the leaderboard type '@Yeventleaders@G'\r\n");
	}
}

ACMD(do_newevent) {
	char arg[MAX_INPUT_LENGTH];
	bool buy = TRUE;
	
	one_argument(argument, arg);
	
	if (EVENT == 1) {
		send_to_char(ch, "There is already a world event going on!\r\n");
		return;
	}
	if (!*arg) {
		send_to_char(ch, "@RWARNING: This costs 5 quest points.  To spawn an event, type 'eventbuy -force'\r\n@n");
		return;
	}
	if (!strcmp(arg, "-force")) {
		if (GET_QUESTPOINTS(ch) < 5 && GET_LEVEL(ch) < LVL_IMPL) {
			send_to_char(ch, "You do not have the required 5 questpoints to taunt another dimension into a battle.");
			return;
		}		
		else {
			if (GET_LEVEL(ch) < LVL_IMPL)
				GET_QUESTPOINTS(ch) -= 5;
			world_event_loader(buy);
		}
	}
	else {
		send_to_char(ch, "@RWARNING: This costs 5 quest points.  To spawn an event, you MUST type 'eventbuy -force'\r\n@n");
		return;	
	}
}

void world_event_boot() {
	struct char_data *tch;
	struct char_data *next_tch;
	int i, j;
	
	event_info.eventstatus = 0;
	event_info.eventtype = 0;
	event_info.eventlevel = 0;
	event_info.eventsize = 0;
	event_info.riftsleft = 0;
	event_info.mobsleft = 0;
	event_info.shards = 0;
	event_info.shard_vnums = 0;
	event_info.playerrifts = 0;
	event_info.maxrifts = 0;

	for (i = 0; i < MAX_EVENT_AREAS; i++)
		event_info.zones[i] = NULL;
	for (j = 0; i < MAX_EVENT_AREAS; j++)
		event_info.rooms[j] = 0;
	for (tch = character_list; tch; tch = next_tch) {
		next_tch = tch->next;
		
		if (IS_MOB(tch))
			continue;
		
		tch->char_specials.totalkills = 0; 
		tch->char_specials.totalrifts = 0;
		
	}
}

void update_world_event() {
	struct char_data *ch;
	struct char_data *next_ch;
	struct obj_data *obj;
	struct obj_data *next_obj;
	struct char_data *voice;
	char buf[MAX_INPUT_LENGTH];
	int to_room = 0;
	int i;
	int j = 0;
	int pracs = 0;
	int xp = 0;
	int qps = 0;
	int number;
	
	// Reset data
	event_info.mobsleft = 0;
	event_info.riftsleft = 0;
	// Recalculate mobs
	for (ch = character_list; ch; ch = next_ch) {
		next_ch = ch->next;

		if (!IS_MOB(ch))
			continue;
	  
		if (IS_MOB(ch))
			if ((GET_MOB_VNUM(ch) > 10800) && (GET_MOB_VNUM(ch) < 10900))
				event_info.mobsleft += 1;
	}
    // Recalculate objects
    for (obj = object_list; obj; obj = next_obj) {
        next_obj = obj->next;
		
		if (GET_OBJ_VNUM(obj) == 10899)
			event_info.riftsleft += 1;
	}

	for (i = 0; i < MAX_EVENT_AREAS; i++) {
		to_room = event_info.rooms[i];
		j = 0;
		for (ch = world[to_room].people; ch;ch = ch->next_in_room)
			if ((GET_MOB_VNUM(ch) > 10800) && (GET_MOB_VNUM(ch) < 10900))
				j += 1;
		for (obj = world[to_room].contents; obj;obj = obj->next_content)
			if (GET_OBJ_VNUM(obj) == 10899)
				j += 1;
		if (j == 0)
			event_info.zones[i] = NULL;
	}

	if ((event_info.mobsleft == 0) && (event_info.riftsleft == 0)) {
		voice = read_mobile(real_mobile(8000), REAL);
		char_to_room(voice, real_room(1));
		GET_LEVEL(voice) = 45;
		if (event_info.playerrifts >= 1)
			number = (100 * event_info.playerrifts) / (event_info.maxrifts);
		else
			number = 0;
		if (number > 50) {
			sprintf(buf, "All of the invading forces have been destroyed! The players are victorious!");
			do_happyhour(voice, "default", 0, 0);
			do_happyhour(voice, "time 48", 0, 0);

			for (ch = character_list; ch; ch = next_ch) {
				next_ch = ch->next;
				if (IS_MOB(ch))
					continue;
				if (!strcmp(event_info.eventlevel, "10")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 2);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					gain_exp(ch, xp, FALSE);
					do_happyhour(voice, "exp 50", 0, 0);
				}
				if (!strcmp(event_info.eventlevel, "20")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 3);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					gain_exp(ch, xp, FALSE);
					do_happyhour(voice, "exp 50", 0, 0);
				}
				if (!strcmp(event_info.eventlevel, "30")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 4);
					pracs = rand_number(1,3);
					gain_exp(ch, xp, FALSE);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					send_to_char(ch, "@YYou gain %d practices!\r\n@n", pracs);
					GET_PRACTICES(ch) += pracs;
					do_happyhour(voice, "exp 50", 0, 0);
				}
				if (!strcmp(event_info.eventlevel, "40")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 5);
					pracs = rand_number(1,6);
					gain_exp(ch, xp, FALSE);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					send_to_char(ch, "@YYou gain %d practices!\r\n@n", pracs);
					GET_PRACTICES(ch) += pracs;
					do_happyhour(voice, "exp 75", 0, 0);
				}
				if (!strcmp(event_info.eventlevel, "Remort")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 6);
					pracs = ((GET_LEVEL(ch) / 6) + rand_number(1, 2));
					qps = rand_number(1,3); 
					gain_exp(ch, xp, FALSE);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					send_to_char(ch, "@YYou gain %d practices!\r\n@n", pracs);
					send_to_char(ch, "@YYou gain %d questpoints!!\r\n@n", qps);
					GET_PRACTICES(ch) += pracs;
					GET_QUESTPOINTS(ch) += qps;
					do_happyhour(voice, "exp 75", 0, 0);
				}
				if (!strcmp(event_info.eventlevel, "Elite")) {
					xp = (exp_cap[GET_LEVEL(ch)].exp_cap * 8);
					pracs = ((GET_LEVEL(ch) / 4) + rand_number(1, 4));
					qps = rand_number(2,4);
					gain_exp(ch, xp, FALSE);
					send_to_char(ch, "@YYou gain %d experience!@n\r\n", xp);
					send_to_char(ch, "@YYou gain %d practices!\r\n@n", pracs);
					send_to_char(ch, "@YYou gain %d questpoints!!\r\n@n", qps);
					GET_PRACTICES(ch) += pracs;
					GET_QUESTPOINTS(ch) += qps;
					do_happyhour(voice, "exp 100", 0, 0);
				}
			}
			
		}
		if (number <= 50) {
			sprintf(buf, "All of the invading forces have been returned to their home dimension.  The players have lost the battle.\r\n");
		}
		do_event(voice, buf, 0, 0);
		extract_char(voice);
		world_event_boot();
	}
}

void scale_world_event() {
	struct char_data *ch;
	struct char_data *next_ch;
	struct obj_data *obj;
	struct obj_data *next_obj;
	struct char_data *voice;
	char buf[MAX_INPUT_LENGTH];
	//int i = 0;
	int j = 0;

	// if yes, determine wether or not it should be strengthened, or weakened
	//for (ch = character_list; ch; ch = next_ch) {
	//	next_ch = ch->next;
	//	if (!IS_NPC(ch))
	//		i++;
	//}

	for (ch = character_list; ch; ch = next_ch) {
		next_ch = ch->next;

		if (!IS_MOB(ch))
			continue;
		if (IS_MOB(ch)) {
			if ((GET_MOB_VNUM(ch) > 10800) && (GET_MOB_VNUM(ch) < 10900)) {
				extract_char(ch);
				j++;
			}
		}
		if (j >= 8)
			break;
	}

    for (obj = object_list; obj; obj = next_obj) {
        next_obj = obj->next;	

		if (GET_OBJ_VNUM(obj) == 10899) {
			extract_obj(obj);
			break;
		}
	}
	voice = read_mobile(real_mobile(8000), REAL);
	char_to_room(voice, real_room(1));
	sprintf(buf, "The attacking forces have seized control of one of their locations!");
	do_event(voice, buf, 0, 0);
	extract_char(voice);
}

void world_event_loader(bool buy) {
	int to_room;
	char buf1[MAX_INPUT_LENGTH];
	char buf2[MAX_INPUT_LENGTH];
	int randchance;

	// Determine wether or not a world event is already taking place
	if (EVENT == 1) {
		scale_world_event();
		return;
	}

	if (EVENT == 0) {
		int i;
		struct char_data *voice;
		struct char_data *mob=NULL;
		
		if (buy == TRUE)
			randchance = 100;
		else
			randchance = rand_number(1,100);
		
		if (randchance > 50) {
			EVENT = 1;
			// What kind of event are we going to deploy?
			event_info.eventtype = rand_number(1,4);
			// What size should it be?
			event_info.eventsize = rand_number(1,6);
			// What level should it be geared for?
			int randlevel = rand_number(1,6);
			event_info.eventlevel = event_sizes[randlevel].event_level;
			int eventlevel = event_sizes[randlevel].real_level;
			event_info.shards = 0;
			event_info.shard_vnums = event_sizes[randlevel].shard_vnums;
			// Announce the world event
			voice = read_mobile(real_mobile(8000), REAL);
			char_to_room(voice, real_room(1));
			sprintf(buf1, "Sudden waves of time and reality flicker in your sights as time seemly slows down to a halt for a brief moment.");
			sprintf(buf2, "The universe is under seige by %s! Seek them out, and destroy the rifts spawning them!", event_types[event_info.eventtype]);
			send_to_all("@D%s@n\r\n", buf1);
			do_event(voice, buf2, 0, 0);
			extract_char(voice);
			event_info.maxrifts = event_sizes[event_info.eventsize].event_size;
			// Lets calculate what rooms this event is going to be loaded into	
			for (i = 0; i < event_sizes[event_info.eventsize].event_size; i++) {
				if (strcmp(event_info.eventlevel, "Remort") || strcmp(event_info.eventlevel, "Elite")) {
					do {
						to_room = rand_number(0, top_of_world);
						event_info.zones[i] = strdup(zone_table[world[to_room].zone].name);
						event_info.rooms[i] = to_room;
						event_info.min_level = zone_table[world[to_room].zone].min_level;
						event_info.max_level = zone_table[world[to_room].zone].max_level;
					} while ((ROOM_FLAGGED(to_room, ROOM_DEATH) ||
							ROOM_FLAGGED(to_room, ROOM_PEACEFUL) ||
							ZONE_FLAGGED(world[to_room].zone, ZONE_NO_EVENT) ||
							ZONE_FLAGGED(world[to_room].zone, ZONE_REMORT_ONLY) ||
							!ZONE_FLAGGED(world[to_room].zone, ZONE_GRID)) || 
							((event_info.min_level > eventlevel) || 
							(event_info.max_level < eventlevel)));
				}
				else {
					do {
						to_room = rand_number(0, top_of_world);
						event_info.zones[i] = strdup(zone_table[world[to_room].zone].name);
						event_info.rooms[i] = to_room;
						event_info.min_level = zone_table[world[to_room].zone].min_level;
						event_info.max_level = zone_table[world[to_room].zone].max_level;
					} while ((ROOM_FLAGGED(to_room, ROOM_DEATH) ||
							ROOM_FLAGGED(to_room, ROOM_PEACEFUL) ||
							ZONE_FLAGGED(world[to_room].zone, ZONE_NO_EVENT) ||
							!ZONE_FLAGGED(world[to_room].zone, ZONE_GRID) ||
							!ZONE_FLAGGED(world[to_room].zone, ZONE_REMORT_ONLY)));
				}
				if (randlevel == 1)
					mob = read_mobile(mob_vnums[event_info.eventtype].lvl10, VIRTUAL);
				if (randlevel == 2)
					mob = read_mobile(mob_vnums[event_info.eventtype].lvl20, VIRTUAL);
				if (randlevel == 3)
					mob = read_mobile(mob_vnums[event_info.eventtype].lvl30, VIRTUAL);
				if (randlevel == 4)
					mob = read_mobile(mob_vnums[event_info.eventtype].lvl40, VIRTUAL);
				if (randlevel == 5)
					mob = read_mobile(mob_vnums[event_info.eventtype].remort, VIRTUAL);
				if (randlevel == 6)
					mob = read_mobile(mob_vnums[event_info.eventtype].elite, VIRTUAL);
				char_to_room(mob, to_room);
				load_mtrigger(mob);
			}
		}
	}
}

void load_prize(struct char_data *ch) {
    struct obj_data *item;
	struct obj_data *shard;
	struct char_data *winners;
	struct prize_data *head = NULL;
	struct prize_data *curr = NULL;
	struct prize_data *cycle;
	struct prize_data *temp;
	bool awarded = FALSE;
	int die_roll = rand_number(0, (60 + (20 * (event_info.shards +1))));

	// add death to tally...
	ch->char_specials.totalkills += 1;
	GET_EVENTMOBS(ch) += 1;
    // last entry must always have index die_size
    const struct prize_info {
        char *message;
        int item_number;
    }
    prize_info[] = {
        {"a huge box of 9MM ammo", 1513},		// 0
		{"a huge box of linked shells", 1514},		// 1
		{"a blocky laser battery", 1518},		// 2
		{"a huge box of fragmentation rounds", 1515},		// 3
		{"a huge can of compressed fuel", 1516},				// 4
		{"a huge box of heat seeking rockets", 1517},			// 5
		{"an assassination order", 4598},			// 6
		{"a manual for imbuing", 14061},			// 7
		{"a manual of healing art", 14062},		// 8
		{"a Rosy Sphere", 1},      				// 9
		{"an Aqua Helix", 2},   					// 10
		{"a Clear Spindle", 3},  					// 11
		{"a Neon Rock", 4},      					// 12
		{"a Golden Stone", 5},   					// 13
		{"an Obsidian Shard", 6},					// 14
		{"a Glowing Globe", 7},  					// 15
		{"a Crimson Pyramid", 8}, 				// 16
		{"a Turquoise Cylinder", 9}, 				// 17
		{"a Violet Stylus", 10},					// 18
                {"a buff drink", 8044},					// 19
                {"a restring ticket", 22771},				// 20
                {"a Ridged Hilt with Handguard", 1290},	// 21
                {"a piece of bio armor", 21100},			// 22
                {"a Vial of GOD", 1006},					// 23
                {"a Vial of Menace", 1009},				// 24
                {"a Vial of Voodoo", 1020},				// 25
                {"a Vial of KO", 1008},					// 26
                {"a practice egg", 3057},					// 27
    };

    if (die_roll >= 0 && die_roll <= 27) {
		send_to_char(ch, "@YYou have received %s for your service!@n\r\n", prize_info[die_roll].message);
        item = read_object(real_object(prize_info[die_roll].item_number), REAL);
        obj_to_char(item, ch);
	}
	
	if (die_roll == (60 + (20 * (event_info.shards +1)))) {
		// Perfect roll, check to see if there are any shard loads left in this event
		for (winners = world[IN_ROOM(ch)].people; winners;winners = winners->next_in_room) {
			if (!IS_NPC(winners)) {
				awarded = FALSE;
				if ((temp = malloc(sizeof(prize_data))) == NULL) { abort(); }
				temp->host = GET_HOST(winners);
				temp->ch = winners;
				temp->next = NULL;
				if (!head) {
					head = temp;
					curr = temp;
				}
				else {
					cycle = head;
					while (cycle) {
						if (!strcmp(cycle->host, temp->host))
							awarded = TRUE;
						cycle = cycle->next;
					}
					if (!awarded) {
						curr->next = temp;
						curr = temp;
					}
				}
			}
		}		
		temp = head;
		while (temp) {
			shard = read_object(real_object(event_info.shard_vnums), REAL);
			obj_to_char(shard, temp->ch);
			send_to_char(temp->ch, "@M-----------------------------\r\n@n");
			send_to_char(temp->ch, "@M----- @YYOU FOUND A SHARD!@N ----\r\n@n");
			send_to_char(temp->ch, "@M-----	 @YCONGRATULATIONS!@N  ----\r\n@n");
			send_to_char(temp->ch, "@M-----------------------------\r\n@n");
			send_to_all("@GYou hear a war cry blast across the universe as %s receives a shard for their service!@n\r\n", GET_NAME(temp->ch));
			event_info.shards += 1;
			temp = temp->next;
		}
		while (head) {
			temp = head;
			head = head->next;
			free(temp);
		}
	}
}

ACMD(do_eventleaders)
{
	struct char_data *tch;
	struct char_data *next_tch;
	int i = 0;
	
	if (EVENT == 0) {
		send_to_char(ch, "There is no world event currently in progress.\r\n");
		return;
	}
	send_to_char(ch, "\r\n@G Current World Event Participants:@n\r\n");
	send_to_char(ch, "@W-----------------------------------------------------------@n\r\n");
	send_to_char(ch, "@W Player name                     Invaders Killed     Rifts@n\r\n\r\n");
	for (tch = character_list; tch; tch = next_tch) {
		next_tch = tch->next;
		
		if (IS_MOB(tch))
			continue;
		if (tch->char_specials.totalkills > 0 || tch->char_specials.totalrifts > 0) {
			send_to_char(ch, " %-32s   %5d           %5d\r\n", GET_NAME(tch), tch->char_specials.totalkills, tch->char_specials.totalrifts);
			i++;
		}
	}


	send_to_char(ch, "\r\n@y %d@g players displayed.\r\n", i);
}
