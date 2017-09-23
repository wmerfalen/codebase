//////////////////////////////////////////////////////////////////////////////////
// This events.h file has been written from scratch by Gahan.					//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the world event system for CyberASSAULT								//
// Anything that has to do with it will be found here or within events.c		//
//////////////////////////////////////////////////////////////////////////////////

#ifndef _EVENTS_H_
#define _EVENTS_H_

void world_event_loader(bool buy);
void update_world_event();
void world_event_boot();
void load_prize(struct char_data *ch);
int compare_leaders(const void *x, const void *y);
#define MAX_EVENT_AREAS		20
typedef struct world_event_data WOLRD_EVENT_DATA;
typedef struct world_event_data world_event_data;
struct world_event_data {

	int eventstatus;
	int eventtype;
	char *eventlevel;
	int eventsize;
	int riftsleft;
	int mobsleft;
	int min_level;
	int max_level;
	int shards;
	int shard_vnums;
	int playerrifts;
	int maxrifts;

	int rooms[MAX_EVENT_AREAS +1];
	char *zones[MAX_EVENT_AREAS +1];
};

extern world_event_data event_info;

struct prize_data {
	char *host;
	struct char_data *ch;
	struct prize_data *next;
};

typedef struct prize_data prize_data;

struct mob_vnums_type {
	int noone;
	int lvl10;
	int lvl20;
	int lvl30;
	int lvl40;
	int remort;
	int elite;
	int endoflist;
};

struct event_sizes_type {
	const char *event_size_text;
	int event_size;
	char *event_level;
	int real_level;
	int loading_shards;
	int shard_vnums;
};

#define EVENT			(event_info.eventstatus)
#define EVENT_LIVE		(event_info.eventstatus > 0)

extern const char *event_types[];

#ifndef __EVENTS_C__

ACMD(do_eventinfo);
ACMD(do_eventleaders);
ACMD(do_newevent);

#endif
#endif /* _EVENTS_H_ */

