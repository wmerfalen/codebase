//////////////////////////////////////////////////////////////////////////////////
// This craft.c file ha been written from scratch by Gahan.						//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the crafting system for CyberASSAULT									//
// Anything that has to do with it will be found here or within craft.h			//
//////////////////////////////////////////////////////////////////////////////////

#define __CRAFT_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "psionics.h"
#include "house.h"
#include "ban.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "shop.h"
#include "act.h"
#include "genzon.h"
#include "class.h"
#include "genolc.h"
#include "fight.h"
#include "house.h"
#include "modify.h"
#include "quest.h"
#include "drugs.h"
#include "config.h"
#include "bionics.h"
#include "config.h"
#include "craft.h"

// Object manipulation declares
void obj_from_char(struct obj_data * object);
void obj_to_char(struct obj_data * object, struct char_data * ch);
static int find_name(char *searchname, struct char_data *ch);
static int find_worn(int wearpos, struct char_data *ch);
static int find_type(int type, struct char_data *ch);
static int find_flag(int flag, struct char_data *ch);


// This is a compilation of the structures for crafting system
const char *list_composition_types[] = {
	"nothing_composition_types",
	"trade_composition_types",
	"metal_composition_types",
	"pmetal_composition_types",
	"gem_composition_types",
	"rock_composition_types",
	"organic_composition_types",
	"other_composition_types"
};

const int maxlist_composition_types[] = {
	1,
	7,
	7,
	5,
	6,
	7,
	6,
	8
};

const char *category_composition_types[] = {
	"Nothing",
	"Trade Goods",
	"Metals",
	"Precious Metals",
	"Gems",
	"Rocks",
	"Organics",
	"Others"
};

const char *nothing_composition_types[] = {
	""
};

const char *trade_composition_types[] = {
	"Plastic",
	"Wood",
	"Cloth",
	"Leather",
	"Electronics",
	"Paper",
	"Glass",
	"kevlar",
	"Ceramic"
};

const char *metal_composition_types[] = {
	"Iron",
	"Steel",
	"Brass",
	"Osmium",
	"Aluminum",
	"Titanium",
	"Adamantium"
};

const char *pmetal_composition_types[] = {
	"Copper",
	"Bronze",
	"Silver",
	"Gold",
	"Platinum"
};
const char *gem_composition_types[] = {
	"Amber",
	"Emerald",
	"Sapphire",
	"Ruby",
	"Pearl",
	"Diamond"
};

const char *rock_composition_types[] = {
	"Coal",
	"Lead",
	"Concrete",
	"Granite",
	"Obsidian",
	"Quartz",
	"Jade"
};

const char *organic_composition_types[] = {
	"Bone",
	"Shell",
	"Teeth",
	"Flesh",
	"DNA",
	"Plant",
	"Scale"
};

const char *other_composition_types[] = {
	"Energy",
	"Fuel",
	"Acid",
	"Drug",
	"Rubber",
	"Powder",
	"Plutonium",
	"Clay",
	"Nothing"
};
ACMD(do_craft)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	two_arguments(argument, arg1, arg2);

	if (!*arg1) {
		send_to_char(ch, "\r\n@GCrafting usage:@n\r\n");
		send_to_char(ch, "@W---------------------------------------------------------------------------------------------@n\r\n");
		send_to_char(ch, "@Ycraft professions@W             - List all crafting professions.\r\n");
		send_to_char(ch, "@Ycraft <profession name>@W       - List all crafting skills within a certain profession.\r\n");
		send_to_char(ch, "@Ycraft learn <profession name>@W - Choose a crafting profession.\r\n");
		send_to_char(ch, "@Ycraft learn <crafting skill>@W  - Learn a crafting skill.\r\n");
		send_to_char(ch, "@Ycraft list@W                    - Show all information about your current crafting professions.\r\n");
		send_to_char(ch, "@W---------------------------------------------------------------------------------------------@n\r\n");
		return;
	}

	if (!strcmp(arg1, "professions")) {
		send_to_char(ch, "\r\n@GList of available professions:@n\r\n\r\n");
		send_to_char(ch, "@Y Main Engineering Professions@n\r\n");
		send_to_char(ch, "@W  Structural@n\r\n");
		send_to_char(ch, "@W  Mechanical@n\r\n");
		send_to_char(ch, "@W  Weapons@n\r\n");
		send_to_char(ch, "@W  Biomedical@n\r\n\r\n");
		send_to_char(ch, "@Y Sub Professions@n\r\n");
		send_to_char(ch, "@W  Mining@n\r\n");
		send_to_char(ch, "@W  Harvesting@n\r\n");
		send_to_char(ch, "@W  Salvaging@n\r\n");
		send_to_char(ch, "@W  Dissecting@n\r\n\r\n");
		send_to_char(ch, "For more information about professions, type help <profession name>@n\r\n");
		return;
	}

	else if (!strcmp(arg1, "learn")) {
		// what would you like to learn?
		return;
	}

	else if (!strcmp(arg1, "list")) {
		// list your current skills and info about crafting
		return;
	}

	else {
		send_to_char(ch, "Type '@Ycraft@n' to learn more about how crafting works.\r\n");
		return;
	}

}
// The salvageitem command allows players to break down special types of equipment
// they find useless into smaller pieces of useful equipment, used in createing
// other goods.
ACMD(do_salvageitem)
{
	char arg[MAX_INPUT_LENGTH];
	struct obj_data *obj;
	one_argument(argument, arg);
        struct obj_data *loaded_object;
        int craftroll = 0;

	if (!*arg) {
		send_to_char(ch, "What item would you like to salvage?\r\n");
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) {
        send_to_char(ch, "You can't seem to find anything with that name to salvage.\r\n");
        return;
    }

    if (IS_COMP_NOTHING(obj)) {
        send_to_char(ch, "You can't salvage that!\n\r");
        return;
    }

       send_to_char(ch, "You begin to salvage the item.\r\n");

      if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Plastic") == 0) {
       send_to_char(ch, "Item subcomposition is PLASTIC!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-50,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PLASTIC_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PLASTIC_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLASTIC_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PLASTIC_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLASTIC_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PLASTIC_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PLASTIC_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLASTIC_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLASTIC_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLASTIC_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Wood") == 0) {
       send_to_char(ch, "Item subcomposition is WOOD!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_WOOD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_WOOD_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_WOOD_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_WOOD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_WOOD_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_WOOD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_WOOD_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_WOOD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_WOOD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_WOOD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Cloth") == 0) {
       send_to_char(ch, "Item subcomposition is CLOTH!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_CLOTH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_CLOTH_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLOTH_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_CLOTH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLOTH_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_CLOTH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_CLOTH_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLOTH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLOTH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLOTH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Leather") == 0) {
       send_to_char(ch, "Item subcomposition is LEATHER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_LEATHER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_LEATHER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEATHER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_LEATHER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEATHER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_LEATHER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_LEATHER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEATHER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEATHER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEATHER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Electronics") == 0) {
       send_to_char(ch, "Item subcomposition is ELECTRONICS!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ELECTRONICS_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ELECTRONICS_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_ELECTRONICS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ELECTRONICS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ELECTRONICS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ELECTRONICS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Paper") == 0) {
       send_to_char(ch, "Item subcomposition is PAPER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PAPER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PAPER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PAPER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PAPER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PAPER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PAPER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PAPER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PAPER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PAPER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PAPER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Glass") == 0) {
       send_to_char(ch, "Item subcomposition is GLASS!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_GLASS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_GLASS_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GLASS_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_GLASS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GLASS_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_GLASS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_GLASS_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GLASS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GLASS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GLASS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Kevlar") == 0) {
       send_to_char(ch, "Item subcomposition is KEVLAR!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_KEVLAR_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_KEVLAR_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_KEVLAR_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_KEVLAR_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_KEVLAR_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_KEVLAR_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_KEVLAR_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_KEVLAR_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_KEVLAR_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_KEVLAR_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Ceramic") == 0) {
       send_to_char(ch, "Item subcomposition is CERAMIC!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_CERAMIC_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_CERAMIC_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CERAMIC_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_CERAMIC_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CERAMIC_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_CERAMIC_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_CERAMIC_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CERAMIC_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CERAMIC_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CERAMIC_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
       else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Nothing") == 0) {
			loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Iron") == 0) {
       send_to_char(ch, "Item subcomposition is IRON!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_IRON_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_IRON_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_IRON_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_IRON_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_IRON_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_IRON_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_IRON_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_IRON_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_IRON_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_IRON_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Steel") == 0) {
       send_to_char(ch, "Item subcomposition is STEEL!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_STEEL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_STEEL_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_STEEL_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_STEEL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_STEEL_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_STEEL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_STEEL_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_STEEL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_STEEL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_STEEL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Brass") == 0) {
       send_to_char(ch, "Item subcomposition is BRASS!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_BRASS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_BRASS_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRASS_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_BRASS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRASS_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_BRASS_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_BRASS_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRASS_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRASS_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRASS_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Osmium") == 0) {
       send_to_char(ch, "Item subcomposition is OSMIUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_OSMIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_OSMIUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OSMIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_OSMIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OSMIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_OSMIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_OSMIUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OSMIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OSMIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OSMIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Aluminum") == 0) {
       send_to_char(ch, "Item subcomposition is ALUMINUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ALUMINUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ALUMINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_ALUMINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ALUMINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ALUMINUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ALUMINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Titanium") == 0) {
       send_to_char(ch, "Item subcomposition is TITANIUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_TITANIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_TITANIUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TITANIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_TITANIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TITANIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_TITANIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_TITANIUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TITANIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TITANIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TITANIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Adamantium") == 0) {
       send_to_char(ch, "Item subcomposition is ADAMANTIUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ADAMANTIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Copper") == 0) {
       send_to_char(ch, "Item subcomposition is COPPER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_COPPER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_COPPER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COPPER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_COPPER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COPPER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_COPPER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_COPPER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COPPER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COPPER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COPPER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Bronze") == 0) {
       send_to_char(ch, "Item subcomposition is BRONZE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_BRONZE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_BRONZE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRONZE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_BRONZE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRONZE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_BRONZE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_BRONZE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRONZE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BRONZE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BRONZE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Silver") == 0) {
       send_to_char(ch, "Item subcomposition is SILVER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_SILVER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_SILVER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SILVER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_SILVER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SILVER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_SILVER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_SILVER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SILVER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SILVER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SILVER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Gold") == 0) {
       send_to_char(ch, "Item subcomposition is GOLD!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_GOLD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_GOLD_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GOLD_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_GOLD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GOLD_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_GOLD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_GOLD_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GOLD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GOLD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GOLD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Platinum") == 0) {
       send_to_char(ch, "Item subcomposition is PLATINUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PLATINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PLATINUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLATINUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PLATINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLATINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PLATINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PLATINUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLATINUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLATINUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLATINUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Amber") == 0) {
       send_to_char(ch, "Item subcomposition is AMBER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_AMBER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_AMBER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_AMBER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_AMBER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_AMBER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_AMBER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_AMBER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_AMBER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_AMBER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_AMBER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Emerald") == 0) {
       send_to_char(ch, "Item subcomposition is EMERALD!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_EMERALD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_EMERALD_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_EMERALD_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_EMERALD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_EMERALD_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_EMERALD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_EMERALD_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_EMERALD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_EMERALD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_EMERALD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Sapphire") == 0) {
       send_to_char(ch, "Item subcomposition is SAPPHIRE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SAPPHIRE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SAPPHIRE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_SAPPHIRE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SAPPHIRE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SAPPHIRE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SAPPHIRE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
       
       else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Ruby") == 0) {
       send_to_char(ch, "Item subcomposition is ruby!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_RUBY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_RUBY_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBY_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_RUBY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBY_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_RUBY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_RUBY_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Pearl") == 0) {
       send_to_char(ch, "Item subcomposition is PEARL!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PEARL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PEARL_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PEARL_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PEARL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PEARL_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PEARL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PEARL_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PEARL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PEARL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PEARL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Diamond") == 0) {
       send_to_char(ch, "Item subcomposition is DIAMOND!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_DIAMOND_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_DIAMOND_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DIAMOND_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_DIAMOND_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DIAMOND_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_DIAMOND_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_DIAMOND_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DIAMOND_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DIAMOND_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DIAMOND_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Coal") == 0) {
       send_to_char(ch, "Item subcomposition is COAL!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_COAL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_COAL_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COAL_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_COAL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COAL_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_COAL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_COAL_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COAL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_COAL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_COAL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Lead") == 0) {
       send_to_char(ch, "Item subcomposition is LEAD!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_LEAD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_LEAD_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEAD_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_LEAD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEAD_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_LEAD_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_LEAD_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEAD_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_LEAD_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_LEAD_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Concrete") == 0) {
       send_to_char(ch, "Item subcomposition is CONCRETE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_CONCRETE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_CONCRETE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CONCRETE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_CONCRETE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CONCRETE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_CONCRETE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_CONCRETE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CONCRETE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CONCRETE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CONCRETE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Granite") == 0) {
       send_to_char(ch, "Item subcomposition is GRANITE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_GRANITE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_GRANITE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GRANITE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_GRANITE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GRANITE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_GRANITE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_GRANITE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GRANITE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_GRANITE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_GRANITE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Obsidian") == 0) {
       send_to_char(ch, "Item subcomposition is OBSIDIAN!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OBSIDIAN_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OBSIDIAN_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_OBSIDIAN_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OBSIDIAN_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_OBSIDIAN_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_OBSIDIAN_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Quartz") == 0) {
       send_to_char(ch, "Item subcomposition is QUARTZ!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_QUARTZ_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_QUARTZ_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_QUARTZ_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_QUARTZ_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_QUARTZ_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_QUARTZ_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_QUARTZ_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_QUARTZ_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_QUARTZ_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_QUARTZ_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Jade") == 0) {
       send_to_char(ch, "Item subcomposition is JADE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_JADE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_JADE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_JADE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_JADE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_JADE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_JADE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_JADE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_JADE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_JADE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_JADE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Bone") == 0) {
       send_to_char(ch, "Item subcomposition is BONE!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_BONE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_BONE_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BONE_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_BONE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BONE_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_BONE_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_BONE_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BONE_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_BONE_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_BONE_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Shell") == 0) {
       send_to_char(ch, "Item subcomposition is SHELL!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_SHELL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_SHELL_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SHELL_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_SHELL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SHELL_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_SHELL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_SHELL_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SHELL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_SHELL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_SHELL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Teeth") == 0) {
       send_to_char(ch, "Item subcomposition is TEETH!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_TEETH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_TEETH_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TEETH_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_TEETH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TEETH_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_TEETH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_TEETH_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TEETH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_TEETH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_TEETH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Flesh") == 0) {
       send_to_char(ch, "Item subcomposition is FLESH!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_FLESH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_FLESH_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FLESH_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_FLESH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FLESH_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_FLESH_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_FLESH_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FLESH_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FLESH_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FLESH_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "DNA") == 0) {
       send_to_char(ch, "Item subcomposition is DNA!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_DNA_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_DNA_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DNA_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_DNA_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DNA_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_DNA_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_DNA_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DNA_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DNA_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DNA_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Plant") == 0) {
       send_to_char(ch, "Item subcomposition is PLANT!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PLANT_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PLANT_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLANT_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PLANT_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLANT_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PLANT_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PLANT_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLANT_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLANT_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLANT_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Energy") == 0) {
       send_to_char(ch, "Item subcomposition is ENERGY!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_ENERGY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_ENERGY_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ENERGY_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_ENERGY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ENERGY_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_ENERGY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_ENERGY_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ENERGY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ENERGY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ENERGY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Fuel") == 0) {
       send_to_char(ch, "Item subcomposition is FUEL!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_FUEL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_FUEL_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FUEL_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_FUEL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FUEL_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_FUEL_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_FUEL_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FUEL_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_FUEL_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_FUEL_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Acid") == 0) {
       send_to_char(ch, "Item subcomposition is ACID!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_ACID_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_ACID_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ACID_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_ACID_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ACID_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_ACID_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_ACID_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ACID_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_ACID_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_ACID_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Drug") == 0) {
       send_to_char(ch, "Item subcomposition is DRUG!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_DRUG_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_DRUG_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DRUG_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_DRUG_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DRUG_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_DRUG_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_DRUG_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DRUG_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_DRUG_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_DRUG_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Rubber") == 0) {
       send_to_char(ch, "Item subcomposition is RUBBER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_RUBBER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_RUBBER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBBER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_RUBBER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBBER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_RUBBER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_RUBBER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBBER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_RUBBER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_RUBBER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Powder") == 0) {
       send_to_char(ch, "Item subcomposition is POWDER!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_POWDER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_POWDER_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_POWDER_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_POWDER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_POWDER_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_POWDER_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_POWDER_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_POWDER_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_POWDER_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_POWDER_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Plutonium") == 0) {
       send_to_char(ch, "Item subcomposition is PLUTONIUM!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLUTONIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLUTONIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_PLUTONIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLUTONIUM_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_PLUTONIUM_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_PLUTONIUM_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
else if (strcmp(GET_ITEM_SUBCOMPOSITION(obj), "Clay") == 0) {
       send_to_char(ch, "Item subcomposition is CLAY!\r\n");
       craftroll =  (GET_INT(ch) + GET_DEX(ch) + GET_PCN(ch) + rand_number(-30,30)) ;
       send_to_char(ch, "@G"); 
       switch (craftroll) {
       case  0:
            loaded_object = read_object(real_object(CRAFT_NOTHING), REAL);
            obj_to_char(loaded_object, ch);
            break;
       case 1 ... 30:
            loaded_object = read_object(real_object(CRAFT_CLAY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 31 ... 60:
            loaded_object = read_object(real_object(CRAFT_CLAY_1), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLAY_2), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 61 ... 80:
            loaded_object = read_object(real_object(CRAFT_CLAY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLAY_3), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 81 ... 90:
		    loaded_object = read_object(real_object(CRAFT_CLAY_1), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_2), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_4), REAL);
            obj_to_char(loaded_object, ch);
            break;
        case 91 ... 120:
            loaded_object = read_object(real_object(CRAFT_CLAY_5), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_4), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLAY_3), REAL);
            obj_to_char(loaded_object, ch);
            loaded_object = read_object(real_object(CRAFT_CLAY_2), REAL);
            obj_to_char(loaded_object, ch);
			loaded_object = read_object(real_object(CRAFT_CLAY_1), REAL);
            obj_to_char(loaded_object, ch);
            break;
         
        }
            send_to_char(ch, "You successfully salvage some %s.\r\n", GET_ITEM_SUBCOMPOSITION(obj)); 
            extract_obj(obj);
       }
	else {
	send_to_char(ch, "I cannot find that item type.\r\n");
	return;
        }
		            act("$n salvages a useless item.", FALSE, ch, 0, 0, TO_ROOM);
}
// The dissect command allows players to extract DNA, de-scale, skin leathers,
// remove bone and various other things used for crafting.
ACMD(do_dissect)
{
	char arg[MAX_INPUT_LENGTH];
	struct obj_data *obj;

	one_argument(argument, arg);

	if (!*arg) {
		send_to_char(ch, "What would you like to dissect?\r\n");
		return;
	}

	if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) {
		send_to_char(ch, "You can't seem to find anything with that name to dissect.\r\n");
		return;
	}

	if (OBJ_FLAGGED((obj), ITEM_CORPSE_UNDEAD)) {
		send_to_char(ch, "You dissect the undead corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_HUMANOID)) {
		send_to_char(ch, "You dissect the humanoid corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_MECHANICAL)) {
		send_to_char(ch, "You dissect the mechanical corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_HIGHLANDER)) {
		send_to_char(ch, "You dissect the highlander corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_ANIMAL)) {
		send_to_char(ch, "You dissect the animal corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_CANFEL)) {
		send_to_char(ch, "You dissect the canine/feline corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_INSECT)) {
		send_to_char(ch, "You dissect the insect corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_REPTILE)) {
		send_to_char(ch, "You dissect the reptile corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_AQUATIC)) {
		send_to_char(ch, "You dissect the aquatic corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_ALIEN)) {
		send_to_char(ch, "You dissect the alien corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_MUTANT)) {
		send_to_char(ch, "You dissect the mutant corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_PLANT)) {
		send_to_char(ch, "You dissect the plant corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_ETHEREAL)) {
		send_to_char(ch, "You dissect the ethereal corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_SHELL)) {
		send_to_char(ch, "You dissect the shell corpse.\r\n");
		return;
	}
	else if (OBJ_FLAGGED((obj), ITEM_CORPSE_BIRD)) {
		send_to_char(ch, "You dissect the bird corpse.\r\n");
		return;
	}
	else {
		send_to_char(ch, "That item cannot be dissected.\r\n");
		return;
	}
}

///////////////////////////////////////////
// Cyber Assault Storage System by Gahan //
// Will move to another file after this  //
// Has been tested properly.			 //
///////////////////////////////////////////

int in_storage_room(struct char_data *ch)
{    
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_WAREHOUSE))
        return (TRUE);
    else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
        return (TRUE);
    else
        return (FALSE);
}

struct storage_record *storage_create(void)
{
	struct storage_record *storage;

	storage = (struct storage_record*)malloc(sizeof(struct storage_record));
	
	// Is storage->max_items even used? - Igmo
	storage->max_items = 50;
	storage->contents = NULL;

	return (storage);
}
bool storage_check(struct char_data *ch)
{
	FILE *fl;
	char filename[MAX_STRING_LENGTH];
	
	if (GET_STORAGESPACE(ch) > 0)
		snprintf(filename, sizeof(filename), LIB_STORAGE"%s.stor", GET_NAME(ch));
	else
		return (FALSE);
		
	if (!(fl = fopen(filename, "r")))
		return (FALSE);

	fclose(fl);
	return (TRUE);
}

bool storage_open(struct char_data *ch)
{
    FILE *fl;
    char filename[MAX_STRING_LENGTH];
    obj_save_data *loaded;
    obj_save_data *current;

	// Handle null storage pointers. If ch->storage isn't initialized, we go ahead and
	// create a new instance using storage_create(). Hopefully this won't cause problems
	// with the max_items stuff, but I'm pretty sure it's not used. - Igmo
	if(!ch->storage)
		ch->storage = storage_create();

	snprintf(filename, sizeof(filename), LIB_STORAGE"%s.stor", GET_NAME(ch));

    // find file
    if (!(fl = fopen(filename, "r")))
        return (FALSE);

	loaded = objsave_parse_objects(fl);

    // put objects into the storage
    // don't use obj_to_storage as the rent file is already in order
	for (current = loaded; current != NULL; current = current->next) {
		current->obj->next_content = ch->storage->contents;
		ch->storage->contents = current->obj;
		IN_ROOM(current->obj) = NOWHERE;  
		current->obj->carried_by = NULL;
	}

	// now it's safe to free the obj_save_data list - all members of it
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		free(current);
	}

   fclose(fl);
   return (TRUE);

}

// close the storage file and save it if we need to
bool storage_close(struct char_data *ch)
{
    struct obj_data *obj;
    struct obj_data *temp;

    if (!ch->storage) {
        send_to_char(ch, "[ERROR:003] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    // save the storage file
    if (!storage_crash_save(ch)) {
        send_to_char(ch, "[ERROR:004] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    // remove these objects from the world
    obj = ch->storage->contents;
    while (obj) {
        temp = obj->next_content;
        obj_from_storage(obj, ch);
        extract_obj(obj);
        obj = temp;
    }
	GET_STORAGESTATE(ch) = 0;
	if (ch->storage != NULL)
		ch->storage->contents = NULL;
	ch->storage = NULL;		
	save_char(ch);
    return (TRUE);
}

void storage_store_obj(struct obj_data *obj, struct char_data *ch)
{
    if (!obj)
        return;

    // recurse into all objects contained in this object
    storage_store_obj(obj->contains, ch);

    // now iterate through any and all objects contained with this object
    storage_store_obj(obj->next_content, ch);

    // remove object from a container
    if (obj->in_obj)
        obj_from_obj(obj);

    // store the object in the bay
    obj_to_storage(obj, ch);
    send_to_char(ch, "You put %s into your storage bay.\n\r", obj->short_description);
}

bool obj_to_storage(struct obj_data *object, struct char_data *ch)
{
    struct obj_data *cur = NULL;
    struct obj_data *prev = NULL;
    bool found = FALSE;

    if (!ch->storage) {
        send_to_char(ch, "[ERROR:005] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    if (!object) {
        send_to_char(ch, "[ERROR:006] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    // add similar items to head of list of similar items
    for (cur = ch->storage->contents; !found && cur; prev = cur, cur = cur->next_content) {
        if (GET_OBJ_VNUM(cur) == GET_OBJ_VNUM(object)) {
            // found a similar item, place the object here
            if (!prev) {
                object->next_content = ch->storage->contents;  // list of similar items at front of contents
                ch->storage->contents = object;
            }
            else {
                prev->next_content = object;
                object->next_content = cur;
            }
            found = TRUE;
        }
    }

    // unique object, just put it at front of storage
    if (!found) {
        object->next_content = ch->storage->contents;
        ch->storage->contents = object;
    }

    // clear the object fields
    object->carried_by = NULL;
    IN_ROOM(object) = NOWHERE;
    object->worn_on = NOWHERE;

    return (TRUE);
}

bool obj_from_storage(struct obj_data *obj, struct char_data *ch)
{
    struct obj_data *prev = NULL;
    struct obj_data *cur = NULL;

    if (!ch->storage) {
        send_to_char(ch, "[ERROR:007] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    if (!obj) {
        send_to_char(ch, "[ERROR:008] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    for (cur = ch->storage->contents; cur; prev = cur, cur = cur->next_content) {
        if (cur == obj)
            break;
    }

    if (!cur)
        return (FALSE);

    if (!prev)
        //we removed first element, so prev->next is not valid
        ch->storage->contents = cur->next_content;
    else {
        prev->next_content = cur->next_content;
        cur->next_content = NULL;  // is this necessary?
    }

    return (TRUE);
}

void storage_count_contents(struct obj_data *obj, int *count)
{

    if (obj) {

        (*count)++;

        // check for guns with ammo (and assume the ammo itself is not a container)
        if (obj->obj_weapon.ammo_loaded)
            (*count)++;

        storage_count_contents(obj->contains, count);
        storage_count_contents(obj->next_content, count);
    }
}

int num_items_storage(struct char_data *ch)
{
    struct obj_data *obj;
    int num = 0;

    if (!ch->storage) {
        send_to_char(ch, "[ERROR:009] Something wrong with your storage file. Report this code to Gahan."); 
        return (0);
    }

    for (obj = ch->storage->contents; obj; obj = obj->next_content)
        num++;

    return (num);
}

bool storage_save(struct obj_data *obj, FILE *fp)
{
    int result;

    if (obj) {

        storage_save(obj->next_content, fp);
        result = objsave_save_obj_record(obj, fp, 0);  // 0: this object is not worn

        if (!result)
            return (FALSE);
    }

    return (TRUE);
}

bool storage_crash_save(struct char_data *ch)
{
    FILE *fl;
    char filename[MAX_STRING_LENGTH];

    snprintf(filename, sizeof(filename), LIB_STORAGE"%s.stor", GET_NAME(ch));

    if (!(fl = fopen(filename, "w"))) {
        send_to_char(ch, "[ERROR:010] Something wrong with your storage file. Report this code to Gahan."); 
        return (FALSE);
    }

    storage_save(ch->storage->contents, fl);

    fclose(fl);
    return (TRUE);
}

void storage_load(struct char_data *ch)
{
	// Just initialize ch->storage using create storage.
	ch->storage = storage_create();

	ch->storage->max_items = GET_STORAGESPACE(ch);

	storage_crash_load(ch);

	if (ch->storage) {
		storage_crash_save(ch);
		storage_close(ch);
	}


}

void storage_crash_load(struct char_data *ch)
{
	storage_open(ch);
	return;	
}

void delete_storage(struct char_data *ch)
{
	FILE *fl;
	char filename[MAX_STRING_LENGTH];

	snprintf(filename, sizeof(filename), LIB_STORAGE"%s.stor", GET_NAME(ch));

	if (!(fl = fopen(filename, "r"))) {
		return;
	}

	fclose(fl);

	if (remove(filename)) {
		return;
	}
}

void stock(struct char_data *ch, char *name)
{
    struct obj_data *obj = NULL;
    int num_items;

	if (ch->storage)
		storage_close(ch);	

	if (storage_open(ch)) {
		GET_STORAGESTATE(ch) = 1;

		num_items = num_items_storage(ch);

	    if (num_items >= GET_STORAGESPACE(ch)) {
		    send_to_char(ch, "The storage bay is full!  Time to upgrade or clean up.\r\n");
			if (!storage_close(ch))
				send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
			return;
	    }

			obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying);

	        if (!obj) {
		        send_to_char(ch, "You don't see that item in your inventory.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
	        }

		    // count the number of items this will add (mainly for containers and/or loaded guns)
			num_items++;  // add the item itself
	        storage_count_contents(obj->contains, &num_items); // add all items this might contain
		    if (obj->obj_weapon.ammo_loaded) num_items++;  // add for ammo, if loaded

	        if (num_items >= GET_STORAGESPACE(ch) + 2) {
		        if (IS_GUN(obj) && IS_LOADED(obj))
			        send_to_char(ch, "The storage bay can't hold a loaded weapon.  Unload it first.\r\n");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan.");
				return;
			}

	        if (OBJ_FLAGGED(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 ||
		        GET_OBJ_RNUM(obj) == NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY) {
						
	            send_to_char(ch, "You can't store that item.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}

	        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->contains) {

	            // if the item is a container, remove all of its contents and try to store them all
		        // containers but no corpses! (but corpses should be no rent anyway and caught above)
			    if (!GET_OBJ_VAL(obj, 3)) {

	                // empty the container and store everything
		            storage_store_obj(obj->contains, ch);

	                // now store the container
		            obj_from_char(obj);
			        obj_to_storage(obj, ch);
				    send_to_char(ch, "You put %s in your storage bay.\n\r", obj->short_description);
					if (!storage_close(ch))
						send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
	                return;
		        }
			    else {
				    send_to_char(ch, "You can't store corpses.\n\r");
					if (!storage_close(ch))
						send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
					return;
	           }
		    }
			else if (IS_GUN(obj) && IS_LOADED(obj)) {

	            struct obj_data *ammo;

		        // remove the ammo and try to store it
			    obj_from_obj(obj->obj_weapon.ammo_loaded);
				ammo = obj->obj_weapon.ammo_loaded;
	            obj->obj_weapon.ammo_loaded = NULL;
		        obj_to_storage(ammo, ch);
			    send_to_char(ch, "You put %s in your storage bay.\n\r", ammo->short_description);

	            // now store the weapon
		        obj_from_char(obj);
			    obj_to_storage(obj, ch);
				send_to_char(ch, "You put %s in your storage bay.\n\r", obj->short_description);
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}

			else {
				obj_from_char(obj);
				obj_to_storage(obj, ch);
				send_to_char(ch, "You put %s in your storage bay.\n\r", obj->short_description);
				save_char(ch);
				storage_crash_save(ch);
				
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:011] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}
		}
	else {
		send_to_char(ch, "You don't own a storage bay.\r\n");
		return;
	}
}

void unstock(struct char_data *ch, char *name)
{

    struct obj_data *obj;
	char *split = NULL;
	char *tokenize = malloc(strlen(name)+1);
	int count = 0;
	
	if (ch->storage)
		storage_close(ch);

	if (storage_open(ch) > 0) {
		GET_STORAGESTATE(ch) = 1;

		if (!(obj = ch->storage->contents)) {
			send_to_char(ch, "Your storage bay is empty.\n\r");
			
			if (!storage_close(ch))
				send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
			return;
		}

		strcpy(tokenize, name);
		split = strtok(tokenize,".");

		if(strlen(name) != strlen(split)){
		    // If strtok doesn't return a string the same length as the name, we have a
		    // dot separator.
		    if(is_number(split)){
		        // Check if the first part of the string is, in fact, number.
		        // NOTE: I re-used the function from the ACMD(do_get) to check this --
		        // if this doesn't work, we'll have to manually check it. No biggie.
			    count = atoi(split);
			    name = strtok(NULL,".");

			    // Set count to the number gleaned from the tokenization, and
				// reset the name pointer to the second part.

		    }
		}


		for (obj = ch->storage->contents; obj; obj = obj->next_content) {
		    if (isname(name, obj->name)){
		        if (CAN_SEE_OBJ(ch, obj) && count <= 1)
		            break;
		        count--;
		    }
		}

	    if (!obj) {
			send_to_char(ch, "You don't see that in your storage bay.\n\r");
			if (!storage_close(ch))
				send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
			return;
		}

	    // check weight, carrying, and if object is ITEM_TAKE
		if (can_take_obj(ch, obj)) {
					
			if (!obj_from_storage(obj, ch)) {
				send_to_char(ch, "[ERROR:013] Something wrong with your storage file. Report this code to Gahan."); 
				mudlog(NRM, LVL_GOD, TRUE, "Error retrieving %s by %s.", obj->short_description, GET_NAME(ch));
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}

			send_to_char(ch, "You get %s from your storage bay.\n\r", obj->short_description);
			obj_to_char(obj, ch);
			save_char(ch);
			storage_crash_save(ch);

			if (!storage_close(ch))
				send_to_char(ch, "[ERROR:014] Something wrong with your storage file. Report this code to Gahan."); 
			return;	
		}
		
	}
	else {
		send_to_char(ch, "You don't own a storage bay.\r\n");
		return;
	}

	if (!storage_close(ch))
		send_to_char(ch, "[ERROR:014] Something wrong with your storage file. Report this code to Gahan."); 
}

static int find_worn(int wearpos, struct char_data *ch)
{
	struct obj_data *obj;
	int found = 0;
	for (obj = ch->storage->contents; obj; obj = obj->next_content) {
		if (CAN_WEAR(obj, wearpos))
			send_to_char(ch, "%3d) %s\r\n", ++found, obj->short_description);
	}
	return (found);
}

static int find_name(char *searchname, struct char_data *ch)
{
    struct obj_data *obj;
    int found = 0;
	for (obj = ch->storage->contents; obj; obj = obj->next_content) {
        if (isname(searchname, obj->name))
            send_to_char(ch, "%3d) %s\r\n", ++found, obj->short_description);
	}
    return (found);
}

static int find_type(int type, struct char_data *ch)
{
    struct obj_data *obj;
    int found = 0;
	for (obj = ch->storage->contents; obj; obj = obj->next_content) {
		if (GET_OBJ_TYPE(obj) == type) {
			send_to_char(ch, "%3d) %s\r\n", ++found, obj->short_description);
		}
	}
	send_to_char(ch, "Found %d\r\n", found);
    return (found);
}

static int find_flag(int flag, struct char_data *ch)
{
    struct obj_data *obj;
    int found = 0;
	for (obj = ch->storage->contents; obj; obj = obj->next_content) {
		if (OBJ_FLAGGED(obj, flag)) {
			send_to_char(ch, "%3d) %s\r\n", ++found, obj->short_description);
		}
	}
	send_to_char(ch, "Found %d\r\n", found);
    return (found);
}


void baylist(struct char_data *ch, char *arg, char *arg2)
{
    struct obj_data *obj;
	int i;

	if (ch->storage)
		storage_close(ch);

    if (arg == NULL && arg2 == NULL) {
		if (storage_open(ch) > 0) {
			GET_STORAGESTATE(ch) = 1;

			if (!(obj = ch->storage->contents)) {
				send_to_char(ch, "Your storage bay is empty.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}
			else {

			    send_to_char(ch, "Storage bay contains:\r\n\r\n");
				list_obj_to_char(obj, ch, 1, TRUE);
				send_to_char(ch, "\r\n%d item(s) of %d MAX.\r\n", num_items_storage(ch), GET_STORAGESPACE(ch));

				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:015] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}
		}
		else {
			send_to_char(ch, "You don't own a storage bay.\r\n");
			return;
		}
    }
	if (arg != NULL && arg2 == NULL) {
		send_to_char(ch, "Syntax:'\r\n");
		send_to_char(ch, "Storage list name <object>\r\n");
		send_to_char(ch, "Storage list worn <location>\r\n");
		send_to_char(ch, "Storage list type <object type> (IE: container, weapon)\r\n");
		send_to_char(ch, "Storage list flag <object flag> (IE: hum, quest_item, remort_only)\r\n");
		return;
	}

    if (is_abbrev(arg, "worn")) {
		if (storage_open(ch) > 0) {
			GET_STORAGESTATE(ch) = 1;

			if (!(obj = ch->storage->contents)) {
				send_to_char(ch, "Your storage bay is empty.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}
			    
			for (i = 1; i < NUM_ITEM_WEARS; i++) {
				if (is_abbrev(arg2, wear_bits[i])) {
					if (!find_worn(i, ch)) {
						send_to_char(ch, "No objects by that name.\r\n");
						if (!storage_close(ch))
							send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
						return;
					}
				}
			}
		}
	}

    else if (is_abbrev(arg, "type")) {
		if (storage_open(ch) > 0) {
			GET_STORAGESTATE(ch) = 1;

			if (!(obj = ch->storage->contents)) {
				send_to_char(ch, "Your storage bay is empty.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
				return;
			}
			    
			for (i = 1; i < NUM_ITEM_TYPES + 1; i++) {
				if (is_abbrev(arg2, item_types[i])) {
					if (!find_type(i, ch)) {
						send_to_char(ch, "No objects by that name.\r\n");
						if (!storage_close(ch))
							send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan."); 
					}
					if (ch->storage)
						storage_close(ch);
					return;
				}
			}
			storage_close(ch);
			send_to_char(ch, "Please use a valid type.\r\n");
			return;
		}
	}

	else if (is_abbrev(arg, "flag")) {
        if (storage_open(ch) > 0) {
            GET_STORAGESTATE(ch) = 1;

            if (!(obj = ch->storage->contents)) {
                send_to_char(ch, "Your storage bay is empty.\n\r");
                if (!storage_close(ch))
                    send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan.");
                return;
            }
              
            for (i = 1; i < NUM_ITEM_FLAGS + 1; i++) {
                if (is_abbrev(arg2, extra_bits[i])) {
                    if (!find_flag(i, ch)) {
                        send_to_char(ch, "No objects by that name.\r\n");
                        if (!storage_close(ch))
                            send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan.");
                    }
					if (ch->storage)
						storage_close(ch);
                    return;
                }
		
            }
		storage_close(ch);
		send_to_char(ch, "Please use a valid flag.\r\n");
		return;
        }
    }

    else if (is_abbrev(arg, "name")) {
		if (storage_open(ch) > 0) {
			GET_STORAGESTATE(ch) = 1;

			if (!(obj = ch->storage->contents)) {
				send_to_char(ch, "Your storage bay is empty.\n\r");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan.");
				return;
			}

			if (!find_name(arg2, ch)) {
				send_to_char(ch, "No objects by that name.\r\n");
				if (!storage_close(ch))
					send_to_char(ch, "[ERROR:016] Something wrong with your storage file. Report this code to Gahan.");
				return;

			}
		}
	}
	else {
		send_to_char(ch, "Syntax:'\r\n");
		send_to_char(ch, "Storage list name <object>\r\n");
		send_to_char(ch, "Storage list worn <location>\r\n");
		send_to_char(ch, "Storage list type <object type> (IE: container, weapon)\r\n");
		send_to_char(ch, "Storage list flag <object flag> (IE: hum, quest_item, remort_only)\r\n");
	}

	if (ch->storage)
		storage_close(ch);
}


ACMD(do_storage)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	one_argument(two_arguments(argument, arg1, arg2), arg3);

	if (!(in_storage_room(ch))) {
		send_to_char(ch, "You must be in a warehouse to access your storage bay.\r\n");
		return;
	}

	if (IS_NPC(ch)) {
		send_to_char(ch, "ROFL, no.\r\n");
		return;
	}

	if (!*arg1) {
		send_to_char(ch, "How to use your storage bay:\r\n\r\n");
		send_to_char(ch, "Storage buy                 - Purchase a storage bay (5,000,000RU for 50 slots)\r\n");
		send_to_char(ch, "Storage upgrade             - Upgrade your bay capacity\r\n");
		send_to_char(ch, "Storage list                - List what is in your storage bay\r\n");
		send_to_char(ch, "Storage put <item>          - Deposits <item> into your storage bay\r\n");
		send_to_char(ch, "Storage get <item>          - Retrieves <item> from your storage bay\r\n");
		return;
	}

	else {

		if (!strcmp(arg1, "buy")) {
			
			if (storage_check(ch)) {
				send_to_char(ch, "You have already purchased a storage bay.  Maybe you'd like to upgrade?\r\n");
				return;
			}

			if (GET_UNITS(ch) < 5000000) {
				send_to_char(ch, "Storage bays cost 5,000,000 units.\r\n");
				return;
			}
			else {
				ch->storage = storage_create();
				GET_STORAGESPACE(ch) = 50;

				GET_UNITS(ch) -= 5000000;
				storage_crash_save(ch);
				save_char(ch);
				send_to_char(ch, "You spend 5,000,000 Resource Units and buy a storage bay.\r\n");
				mudlog(NRM, LVL_GRGOD, TRUE, "%s has purchased a storage bay.", GET_NAME(ch));
			}
		}
		else if (!strcmp(arg1, "upgrade")) {
			
			if (!storage_check(ch)) {
				send_to_char(ch, "You need to own a storage bay before you can upgrade it!\r\n");
				return;
			}
			if (GET_STORAGESPACE(ch) <= 500) {

				if (GET_UNITS(ch) < 10000000) {
					send_to_char(ch, "Upgrades cost 10000000 units.\r\n");
					return;
				}
				else {
					GET_STORAGESPACE(ch) += 50;
					GET_UNITS(ch) -= 10000000;

					save_char(ch);
					send_to_char(ch, "You spend 10,000,000 Resource Units and upgrade your storage bay to %d slots.\r\n", GET_STORAGESPACE(ch));
				}
			}
			else if (GET_STORAGESPACE(ch) <= 1000 && GET_STORAGESPACE(ch) > 500) {
				if (GET_UNITS(ch) < 15000000) {
					send_to_char(ch, "Upgrades after 500 units are 15,000,000 units each.\r\n");
					return;
				}
				else {
					GET_STORAGESPACE(ch) += 50;
					GET_UNITS(ch) -= 15000000;

					save_char(ch);
					send_to_char(ch, "You spend 15,000,000 Resource Units and upgrade your storage bay to %d slots.\r\n", GET_STORAGESPACE(ch));
				}
			}
			else if (GET_STORAGESPACE(ch) < 2000 && GET_STORAGESPACE(ch) > 1000) {
				if (GET_UNITS(ch) < 20000000) {
					send_to_char(ch, "Upgrades after 1000 units are 20,000,000 units each.\r\n");
					return;
				}
				else {
					GET_STORAGESPACE(ch) += 50;
					GET_UNITS(ch) -= 20000000;

					save_char(ch);
					send_to_char(ch, "You spend 20,000,000 Resource Units and upgrade your storage bay to %d slots.\r\n", GET_STORAGESPACE(ch));
				}
			}
			else {
				send_to_char(ch, "You can't purchase more than 2000 units, the warehouse is not big enough for that.\r\n");
				return;
			}
		}
	

		else if (!strcmp(arg1, "delete")) {

			if (!storage_check(ch)) {
				send_to_char(ch, "You need to own a storage bay before you can delete it!\r\n");
				return;
			}			
			if (!*arg2) {
				send_to_char(ch, "Are you sure?? Please type 'storage delete -force' to confirm.\r\n");
				return;
			}
			else if (!strcmp(arg2, "-force")) {
				delete_storage(ch);
				send_to_char(ch, "Storage file has been deleted.\r\n");
				return;
			}
		}
		else if (!strcmp(arg1, "list")) {

			if (!storage_check(ch)) {
				send_to_char(ch, "You need to own a storage bay before you can list its contents!\r\n");
				return;
			}
			else if (!*arg2) {
				baylist(ch, NULL, NULL);
				return;
			}
			else if (!*arg3) {
				baylist(ch, arg2, NULL);
				return;
			}
			else {
				baylist(ch, arg2, arg3);
				return;
			}
		}
		else if (!strcmp(arg1, "put")) {

			if (!storage_check(ch)) {
				send_to_char(ch, "You need to own a storage bay before you can put stuff in it!\r\n");
				return;
			}

			if (!*arg2) {
				send_to_char(ch, "What would you like to put into your storage bay?\r\n");
				return;
			}
			else {
				stock(ch, arg2);
				return;
			}
		}
		else if (!strcmp(arg1, "get")) {

			if (!storage_check(ch)) {
				send_to_char(ch, "You need to own a storage bay before you get stuff from it!\r\n");
				return;
			}

			if (!*arg2) {
				send_to_char(ch, "What would you like to get from your storage bay?\r\n");
				return;
			}
			else {
				unstock(ch, arg2);
				return;
			}
		}
		else {
			send_to_char(ch, "Improper use of storage command, type 'storage' for a list of storage commands.\r\n");
			return;
		}
	}
}


