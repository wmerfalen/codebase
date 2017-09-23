//////////////////////////////////////////////////////////////////////////////////
// This craft.h file ha been written from scratch by Gahan.						//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the crafting system for CyberASSAULT									//
// Anything that has to do with it will be found here or within craft.c			//
//////////////////////////////////////////////////////////////////////////////////

#ifndef _CRAFT_H_
#define _CRAFT_H_

extern const char *list_composition_types[];
extern const int maxlist_composition_types[];
extern const char *category_composition_types[];
extern const char *nothing_composition_types[];
extern const char *trade_composition_types[];
extern const char *metal_composition_types[];
extern const char *pmetal_composition_types[];
extern const char *gem_composition_types[];
extern const char *rock_composition_types[];
extern const char *organic_composition_types[];
extern const char *other_composition_types[];

int in_storage_room(struct char_data *ch);
bool storage_check(struct char_data *ch);
bool storage_open(struct char_data *ch);
bool storage_close(struct char_data *ch);
void storage_store_obj(struct obj_data *obj, struct char_data *ch);
bool obj_to_storage(struct obj_data *obj, struct char_data *ch);
bool obj_from_storage(struct obj_data *obj, struct char_data *ch);
void storage_count_contents(struct obj_data *obj, int *count);
int num_items_storage(struct char_data *ch);
bool storage_save(struct obj_data *obj, FILE *fp);
bool storage_crash_save(struct char_data *ch);
void show_storage(struct char_data *ch);
void do_storage_browse(struct char_data *ch);
void storage_crash_load(struct char_data *ch);
void storage_load(struct char_data *ch);
void delete_storage(struct char_data *ch);
void baylist(struct char_data *ch, char *arg, char *arg2);
void stock(struct char_data *ch, char *name);
void unstock(struct char_data *ch, char *name);

#define MAX_NUM_ITEMS		500
#define NUM_COMPOSITION		8
#define IS_COMP_NOTHING(obj)		(GET_OBJ_COMPOSITION(obj) == 0)
// unused
struct storage_record_element {
   int max_items;
   char owner[MAX_NAME_LENGTH+1];
};

struct storage_record {
   int max_items;
   char owner[MAX_NAME_LENGTH+1];
   byte open;
   struct obj_data *contents;
   struct storage_record *next_storage;
};

#define COMP_NOTHING		0

#define MAX_NOTHING			1

#define COMP_PLASTIC		0
#define COMP_WOOD			1
#define COMP_CLOTH			2
#define COMP_LEATHER		3
#define COMP_ELECTRONICS	4
#define COMP_PAPER			5
#define COMP_GLASS			6
#define COMP_KEVLAR			7
#define COMP_CERAMIC		8

#define MAX_TRADEGOODS		9

#define COMP_IRON			0
#define COMP_STEEL			1
#define COMP_BRASS			2
#define COMP_OSMIUM			3
#define COMP_ALUMINUM		4
#define COMP_TITANIUM		5
#define COMP_ADAMANTIUM		6

#define MAX_METALS			7

#define COMP_COPPER			0
#define COMP_BRONZE			1
#define COMP_SILVER			2
#define COMP_GOLD			3
#define COMP_PLATINUM		4

#define MAX_PRECIOUSMETALS	5

#define COMP_AMBER			0
#define COMP_EMERALD		1
#define COMP_SAPPHIRE		2
#define COMP_RUBY			3
#define COMP_PEARL			4
#define COMP_DIAMOND		5

#define MAX_GEMS			6

#define COMP_COAL			0
#define COMP_LEAD			1
#define COMP_CONCRETE		2
#define COMP_GRANITE		3
#define COMP_OBSIDIAN		4
#define COMP_QUARTZ			5
#define COMP_JADE			6

#define MAX_ROCKS			7

#define COMP_BONE			0
#define COMP_SHELL			1
#define COMP_TEETH			2
#define COMP_FLESH			3
#define COMP_DNA			4
#define COMP_PLANT			5
#define COMP_ORGANIC		6

#define MAX_ORGANICS		7

#define COMP_ENERGY			0
#define COMP_FUEL			1
#define COMP_ACID			3
#define COMP_DRUG			4
#define COMP_RUBBER			5
#define COMP_POWDER			6
#define COMP_PLUTONIUM		7
#define COMP_CLAY			8

#define MAX_OTHERS			9

#ifndef __CRAFT_C__

ACMD(do_craft);
ACMD(do_salvageitem);
ACMD(do_dissect);
//ACMD(do_stock);
//ACMD(do_unstock);
//ACMD(do_baylist);
ACMD(do_storage);

extern struct storage_record *storage;
#endif
#endif /* _CRAFT_H_ */

