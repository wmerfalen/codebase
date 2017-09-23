//////////////////////////////////////////////////////////////////////////////////
// This custom.c class file has been written from scratch by Gahan.				//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the customized equipment system for CyberASSAULT						//
// Anything that has to do with it will be found here or within custom.h		//
// Special thanks to Vatiken from TBAMUD for supplying the infastructure for    //
// oset, the basis for the custom equipment code								//
//////////////////////////////////////////////////////////////////////////////////

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
#include "dg_scripts.h"
#include "oasis.h"
#include "act.h"
#include "genobj.h"
#include "custom.h"
#include "fight.h"
#include "genolc.h"

static void custom_disp_val1_menu(struct descriptor_data *d);
static void custom_disp_val2_menu(struct descriptor_data *d);
static void custom_disp_val3_menu(struct descriptor_data *d);
static void custom_disp_val4_menu(struct descriptor_data *d);
static void custom_disp_val5_menu(struct descriptor_data *d);
static void custom_disp_val6_menu(struct descriptor_data *d);
static void custom_disp_gun_menu(struct descriptor_data *d);
static void custom_disp_weapon_menu(struct descriptor_data *d);
static void custom_disp_aff_menu(struct descriptor_data *d);
static void custom_disp_wpnpsionics_menu(struct descriptor_data *d);
static void custom_disp_apply_menu(struct descriptor_data *d);

bool oset_alias(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 64; 
	int i = GET_OBJ_RNUM(obj); 
	skip_spaces(&argument); 
	if (strlen(argument) > max_len) 
		return FALSE; 
	if (i != NOWHERE && obj->name && obj->name != obj_proto[i].name)
		free(obj->name);  
	obj->name = strdup(argument); 
	return TRUE; 
} 
  
  
bool oset_long_description(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 128; 
	int i = GET_OBJ_RNUM(obj); 
  
	skip_spaces(&argument); 
  
	if (strlen(argument) > max_len) 
		return FALSE; 
  
	if (i != NOWHERE && obj->description && obj->description != obj_proto[i].description)
		free(obj->description); 
  
	obj->description = strdup(argument); 
  
	return TRUE; 
} 
bool oset_short_description(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 128; 
	int i = GET_OBJ_RNUM(obj); 
  
	skip_spaces(&argument); 
  
	if (strlen(argument) > max_len) 
		return FALSE; 
  
	if (i != NOWHERE && obj->short_description && obj->short_description != obj_proto[i].short_description)
		free(obj->short_description); 
  
	obj->short_description = strdup(argument); 
  
	return TRUE; 
} 
bool oset_action_description(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 128; 
	int i = GET_OBJ_RNUM(obj); 
  
	skip_spaces(&argument); 
  
	if (strlen(argument) > max_len) 
		return FALSE; 
  
	if (i != NOWHERE && obj->action_description && obj->action_description != obj_proto[i].action_description)
		free(obj->action_description); 
  
	obj->action_description = strdup(argument); 
  
	return TRUE; 
} 

ACMD(do_oset) 
{ 
	char arg[MAX_INPUT_LENGTH]; 
	char arg2[MAX_INPUT_LENGTH]; 
	struct obj_data *obj; 
	bool success = TRUE; 
  
	argument = one_argument(argument, arg); 
  
	if (!*arg)	
		send_to_char(ch, "oset what?\r\n"); 
	else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)) && 
		!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) 
		send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg); 
	else { 
		argument = one_argument(argument, arg2); 
  
		if (!*arg2) 
			send_to_char(ch, "What?\r\n"); 
		else { 
			if (is_abbrev(arg2, "alias") && (success = oset_alias(obj, argument))) 
				send_to_char(ch, "Object alias set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "longdesc") && (success = oset_long_description(obj, argument))) 
				send_to_char(ch, "Object long description set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "shortdesc") && (success = oset_short_description(obj, argument))) 
				send_to_char(ch, "Object short description set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "actiondesc") && (success = oset_action_description(obj, argument))) 
				send_to_char(ch, "Object action description set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "apply") && (success = oset_apply(obj, argument))) 
				send_to_char(ch, "Remort level set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "remortlevel") && (success = oset_remortlevel(obj, argument))) 
				send_to_char(ch, "Object apply set to %s.\r\n", argument); 
			else if (is_abbrev(arg2, "itemlevel") && (success = oset_itemlevel(obj, argument))) 
				send_to_char(ch, "Item Level set to %s.\r\n", argument); 
			else { 
				if (!success) 
					send_to_char(ch, "%s was unsuccessful.\r\n", arg2); 
				else 
					send_to_char(ch, "Unknown argument: %s.\r\n", arg2); 
				return; 
			} 
		} 
	} 
}
// Display main menu
static void custom_disp_menu(struct descriptor_data *d, struct obj_data *obj)
{
    char buf1[MAX_INPUT_LENGTH];
	char buf2[MAX_INPUT_LENGTH];
	get_char_colors(d->character);
    clear_screen(d);

    // Build buffers for first part of menu. */
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf1, sizeof(buf1));
    sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

    // Build first half of menu. */
    write_to_output(d,
		"\r\n@GCyberASSAULT @DCustom Gear Creation\r\n"
		"@D----------------------------------\r\n\r\n"
        "%sA%s) Keywords   : %s%s\r\n"
        "%sB%s) Short Desc : %s%s\r\n"
        "%sC%s) Long Desc  :\r\n%s%s\r\n\r\n"
        "%sD%s) Type        : %s%s\r\n"
        "%sE%s) Extra flags : %s%s\r\n",

        grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
        grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
        grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
        grn, nrm, cyn, buf1,
        grn, nrm, cyn, buf2
        );

    sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, EF_ARRAY_MAX, buf1);
    sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, buf2);

    write_to_output(d,
        "%sF%s) Wear flags   : %s%s\r\n"
        "%sG%s) Values       : %s%d %d %d %d %d %d\r\n"
        "%sH%s) Applies menu\r\n"
        "%sI%s) Min Level    : %s%d\r\n"
        "%sJ%s) Perm Affects : %s%s\r\n"
        "%sK%s) Weapon Psionic\r\n\r\n"
		"@D----------------------------------\r\n\r\n"
		"%sP%s) Purchase Item\r\n"
        "%sQ%s) Quit and abandon Item\r\n\r\n"
        "Enter choice : ",

        grn, nrm, cyn, buf1,
        grn, nrm, cyn, GET_OBJ_VAL(obj, 0),
        GET_OBJ_VAL(obj, 1),
        GET_OBJ_VAL(obj, 2),
        GET_OBJ_VAL(obj, 3),
        GET_OBJ_VAL(obj, 4),
        GET_OBJ_VAL(obj, 5),
        grn, nrm,
        grn, nrm, cyn, GET_OBJ_LEVEL(obj),
        grn, nrm, cyn, buf2,
        grn, nrm,
		grn, nrm,
        grn, nrm
        );

    CUSTOM_MODE(d) = CUSTOM_MAIN_MENU;
}

static void build_new_custom(struct char_data *ch)
{
	char buf[MAX_INPUT_LENGTH];
	struct descriptor_data *d;
	struct obj_data *obj;
	d = ch->desc;
	CREATE(d->custom, struct custom_editor_data, 1);
    STATE(d) = CON_CUSTOM;
	act("$n starts building an object.", TRUE, d->character, 0, 0, TO_ROOM);
	obj = create_obj();
	obj->item_number = NOTHING;
	IN_ROOM(obj) = NOWHERE;
	obj->name = strdup("test object");
	snprintf(buf, sizeof(buf), "A test object is lying here.");
	obj->description = strdup(buf);
	snprintf(buf, sizeof(buf), "A test object");
	obj->short_description = strdup(buf);
	SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
	CUSTOM_OBJ(d) = obj;
	CUSTOM_MODE(d) = CUSTOM_MAIN_MENU;
	custom_disp_menu(d, CUSTOM_OBJ(d));
}
static void edit_existing_custom(struct obj_data *obj, struct char_data *ch, char *argument)
{
    send_to_char(ch, "This part of the custom code has not yet been implemented, sorry.\r\n");
	return;
}

bool restring_name(struct char_data *ch, struct obj_data *obj, char *argument)
{
	skip_spaces(&argument);
	static int max_len = MAX_INPUT_LENGTH; 
	int i = GET_OBJ_RNUM(obj); 
    if (strlen(argument) > max_len)
		return FALSE;
	if (strlen(argument) < 4) {
		send_to_char(ch, "String must be atleast 4 characters long.\r\n");
		return FALSE;
	}
	if (i != NOWHERE && obj->name && obj->name != obj_proto[i].name)
	  free(obj->name); 
    
	obj->name = strdup(argument); 
  	return TRUE; 
}
bool restring_short(struct char_data *ch, struct obj_data *obj, char *argument)
{
	skip_spaces(&argument);
	static int max_len = 64; 
	int i = GET_OBJ_RNUM(obj); 
    if (strlen(argument) > max_len)
		return FALSE;
	if (strlen(argument) < 4) {
		send_to_char(ch, "String must be atleast 4 characters long.\r\n");
		return FALSE;
	}
	if (i != NOWHERE && obj->short_description && obj->short_description != obj_proto[i].short_description)
	  free(obj->short_description); 
    
	obj->short_description = strdup(argument); 
  	return TRUE; 
}

bool oset_apply(struct obj_data *obj, char * argument)
{ 
  int i = 0, apply = -1, location = -1, mod = 0, empty = -1, value;
  char arg[MAX_INPUT_LENGTH];
  
  argument = one_argument(argument, arg);
  
  skip_spaces(&argument);
  
  if ((value = atoi(argument)) == 0)
    return FALSE;
  
  while (*apply_types[i] != '\n') {
    if (is_abbrev(apply_types[i], arg)) {
      apply = i;
      break;
    }
    i++;
  }
  
  if (apply == -1)
    return FALSE;
    
  for (i = 0; i < MAX_OBJ_APPLY; i++) {
    if (GET_OBJ_APPLY_LOC(obj, i) == apply) {
      location = i;
//        GET_OBJ_APPLY_LOC(obj, i) = loc;
        mod = GET_OBJ_APPLY_MOD(obj, location);
//      mod = obj->affected[i].modifier;
      break;
    } else if (GET_OBJ_APPLY_LOC(obj, i) == APPLY_NONE && empty == -1) {
      empty = i;
    }
  }
  
  /* No slot already using APPLY_XXX, so use an empty one... if available */
  if (location == -1)
    location = empty;  
  
  /* There is no slot already using our APPLY_XXX type, and no empty slots either */
  if (location == -1)
    return FALSE;
        GET_OBJ_APPLY_MOD(obj, location) = mod + value;
//  obj->affected[location].modifier = mod + value;
  
  /* Our modifier is set at 0, so lets just clear the apply location so that it may
   * be reused at a later point */
  if (GET_OBJ_APPLY_MOD(obj, location) != 0)
    GET_OBJ_APPLY_LOC(obj, location) = apply;
  else
    GET_OBJ_APPLY_LOC(obj, location) = APPLY_NONE;
  
  return TRUE;
}

bool restring_long(struct char_data *ch, struct obj_data *obj, char *argument)
{
	skip_spaces(&argument);
	static int max_len = 128; 
	int i = GET_OBJ_RNUM(obj); 
    if (strlen(argument) > max_len)
		return FALSE;
	if (strlen(argument) < 4) {
		send_to_char(ch, "String must be atleast 4 characters long.\r\n");
		return FALSE;
	}
	if (i != NOWHERE && obj->description && obj->description != obj_proto[i].description)
	  free(obj->description); 
    
	obj->description = strdup(argument); 
  	return TRUE; 
}
static void custom_disp_type_menu(struct descriptor_data *d)
{
    get_char_colors(d->character);
    clear_screen(d);
	write_to_output(d,  "@G1@W) Light@n\r\n"
		"@G2@W) Melee Weapon@n\r\n"
		"@G3@W) Ranged Weapon@n\r\n"
		"@G4@W) Weapon Upgrade@n\r\n"
		"@G5@W) Armor@n\r\n"
		"@G6@W) Expansion@n\r\n");
    write_to_output(d, "\r\nEnter object type : ");
    CUSTOM_MODE(d) = CUSTOM_TYPE;
}
static void custom_disp_extra_menu(struct descriptor_data *d)
{
    char bits[MAX_STRING_LENGTH];

    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d, "@G1@W) None@n\r\n"
		"@G2@W) Quest Item@n\r\n");

    sprintbitarray(GET_OBJ_EXTRA(CUSTOM_OBJ(d)), extra_bits, EF_ARRAY_MAX, bits);
    write_to_output(d, "\r\nObject flags: %s%s%s\r\nEnter object extra flag (0 to quit) : ", cyn, bits, nrm);
    CUSTOM_MODE(d) = CUSTOM_EXTRAS;
}
static void custom_disp_wear_menu(struct descriptor_data *d)
{
    char bits[MAX_STRING_LENGTH];
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
            wear_bits[counter], !(++columns % 2) ? "\r\n" : "");

    sprintbitarray(GET_OBJ_WEAR(CUSTOM_OBJ(d)), wear_bits, TW_ARRAY_MAX, bits);
    write_to_output(d, "\r\nWear flags: %s%s%s\r\nEnter wear flag, 0 to quit : ", cyn, bits, nrm);
    CUSTOM_MODE(d) = CUSTOM_WEAR;
}
static void custom_disp_prompt_apply_menu(struct descriptor_data *d)
{
    char apply_buf[MAX_STRING_LENGTH];
    int counter;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < MAX_OBJ_APPLY; counter++) {
        if (GET_OBJ_APPLY_MOD(CUSTOM_OBJ(d), counter)) {
            sprinttype(GET_OBJ_APPLY_LOC(CUSTOM_OBJ(d), counter), apply_types, apply_buf, sizeof(apply_buf));
			int mod = GET_OBJ_APPLY_MOD(CUSTOM_OBJ(d), counter);
            
			if(!strcmp(apply_buf,"SKILLSET"))
				write_to_output(d, " %s%d%s) Grant Skill: %s\r\n", grn, counter + 1, nrm, skills_info[mod].name);
			else if (!strcmp(apply_buf,"PSI_MASTERY"))
				write_to_output(d, " %s%d%s) Grant Psi: %s\r\n", grn, counter + 1, nrm, psionics_info[mod].name);
			else
				write_to_output(d, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm, mod, apply_buf);
        }
        else
            write_to_output(d, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
    }

    write_to_output(d, "\r\nEnter affection to modify (0 to quit) : ");
    OLC_MODE(d) = CUSTOM_PROMPT_APPLY;
}
void custom_cleanup(struct descriptor_data *d)
{
  if (d->custom == NULL)
    return;
  if (d->character) {
    REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
    act("$n stops building an object.", TRUE, d->character, NULL, NULL, TO_ROOM);
    STATE(d) = CON_PLAYING;
  }
  free(d->custom);
  d->custom = NULL;
}
static void custom_disp_val1_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_1;
    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_LIGHT:
            custom_disp_val3_menu(d);
            break;
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            custom_disp_val2_menu(d);
            break;
        case ITEM_ARMOR:
            write_to_output(d, "Apply to AC : ");
            break;
        case ITEM_GUN:
            custom_disp_gun_menu(d);
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Regen all Mod : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_val2_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_2;

    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            write_to_output(d, "Number of damage dice : ");
            break;
        case ITEM_GUN:
            write_to_output(d, "Number of damage dice (loaded) : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of AC to increase/decrease : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_val3_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_3;

    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_LIGHT:
            write_to_output(d, "Number of hours (0 = burnt, -1 is infinite) : ");
            break;
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            write_to_output(d, "Size of damage dice : ");
            break;
        case ITEM_GUN:
            write_to_output(d, "Size of damage dice (loaded) : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of HnD to increase/decrease : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_val4_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_4;
    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            custom_disp_weapon_menu(d);
            break;
        case ITEM_GUN:
            write_to_output(d, "Shots per round : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of Hit Power to increase/decrease : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_val5_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_5;

    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_GUN:
            write_to_output(d, "Range : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of Psi Power to increase/decrease : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_val6_menu(struct descriptor_data *d)
{
    CUSTOM_MODE(d) = CUSTOM_VALUE_6;

    switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
        case ITEM_GUN:
            write_to_output(d, "Size of damage dice (unloaded) : ");
            break;
        default:
            custom_disp_menu(d, CUSTOM_OBJ(d));
            break;
    }
}
static void custom_disp_gun_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < NUM_GUN_TYPES; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm, gun_types[counter],
            !(++columns % 2) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter type : ");
}
static void custom_disp_weapon_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < NUM_ATTACK_TYPES; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
            attack_hit_text[counter].singular, !(++columns % 2) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter weapon type : ");
}
static void custom_disp_aff_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;
    char bits[MAX_STRING_LENGTH];

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter < NUM_AFF_FLAGS; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel,
            affected_bits[counter], nrm, !(++columns % 3) ? "\r\n" : "");

    sprintbitarray(GET_OBJ_AFFECT(OLC_OBJ(d)), affected_bits, AF_ARRAY_MAX, bits);
    write_to_output(d, "\r\nPerm Affects: %s%s%s\r\nEnter affect type (0 is no affect and exits) : ", cyn, bits, nrm);

    CUSTOM_MODE(d) = CUSTOM_AFF;
}
static void custom_disp_wpnpsionics_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter < NUM_WPNPSIONICS; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            wpn_psionics[counter], !(++columns % 3) ? "\r\n" : "");

    write_to_output(d, "\r\n%sEnter psionic choice (0 for none) : ", nrm);
    CUSTOM_MODE(d) = CUSTOM_WPNPSIONIC;
}
static void custom_disp_apply_menu(struct descriptor_data *d)
{
	int counter;
	int columns = 0;
	get_char_colors(d->character);
	clear_screen(d);
	for (counter = 0; counter < NUM_APPLIES; counter++)
	  write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel, apply_types[counter],nrm, !(++columns % 3) ? "\r\n" : "");

	write_to_output(d, "\r\nEnter apply type (0 is no apply) : ");
	CUSTOM_MODE(d) = CUSTOM_APPLY;
}
void custom_parse(struct descriptor_data *d, char *arg)
{
	int number;
	int min_val;
	int max_val;
	switch (CUSTOM_MODE(d)) {
		case CUSTOM_MAIN_MENU:
			switch (*arg) {
			case 'A':
			case 'a':
				write_to_output(d, "Enter keywords : ");
                CUSTOM_MODE(d) = CUSTOM_KEYWORD;
                break;
			case 'B':
			case 'b':
				write_to_output(d, "Enter short desc : ");
                CUSTOM_MODE(d) = CUSTOM_SHORTDESC;
                break;
			case 'C':
			case 'c':
				write_to_output(d, "Enter long desc :\r\n ");
                CUSTOM_MODE(d) = CUSTOM_LONGDESC;
                break;
			case 'D':
			case 'd':
                custom_disp_type_menu(d);
                break;
			case 'E':
			case 'e':
                custom_disp_extra_menu(d);
                break;
			case 'F':
			case 'f':
                custom_disp_wear_menu(d);
                break;
			case 'G':
			case 'g':
                GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
                GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
                GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
                GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
                GET_OBJ_VAL(OLC_OBJ(d), 4) = 0;
                GET_OBJ_VAL(OLC_OBJ(d), 5) = 0;
                custom_disp_val1_menu(d);
                break;
			case 'H':
			case 'h':
                custom_disp_prompt_apply_menu(d);
                break;
			case 'I':
			case 'i':
                write_to_output(d, "Enter new minimum level: ");
                CUSTOM_MODE(d) = CUSTOM_LEVEL;
                break;
			case 'J':
			case 'j':
                custom_disp_aff_menu(d);
                break;
			case 'K':
			case 'k':
                custom_disp_wpnpsionics_menu(d);
                break;
			case 'P':
			case 'p':
				// This is where they will actually buy out the item.
			case 'Q':
			case 'q':
				// if they can afford to keep the custom, have it create the object from the CUSTOM_OBJ(d) then free the CUSTOM structure
				// if they cant afford to keep the custom, or just plain quit out, just extract the object.
				extract_obj(CUSTOM_OBJ(d));
				custom_cleanup(d);
				break;
			default:
				custom_disp_menu(d, CUSTOM_OBJ(d));
				break;
			}
			//break;
        case CUSTOM_KEYWORD:
            if (!genolc_checkstring(d, arg))
                break;
            if (CUSTOM_OBJ(d)->name)
                free(CUSTOM_OBJ(d)->name);
            CUSTOM_OBJ(d)->name = str_udup(arg);
            break;
        case CUSTOM_SHORTDESC:
            if (!genolc_checkstring(d, arg))
                break;

            if (CUSTOM_OBJ(d)->short_description)
                free(CUSTOM_OBJ(d)->short_description);

            CUSTOM_OBJ(d)->short_description = str_udup(arg);
            break;
        case CUSTOM_LONGDESC:
            if (!genolc_checkstring(d, arg))
                break;

            if (CUSTOM_OBJ(d)->description)
                free(CUSTOM_OBJ(d)->description);

            CUSTOM_OBJ(d)->description = str_udup(arg);
            break;
        case CUSTOM_LEVEL:
            GET_OBJ_LEVEL(CUSTOM_OBJ(d)) = LIMIT(atoi(arg), 0, (LVL_IMMORT -1));
            break;
        case CUSTOM_VALUE_1:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                    GET_OBJ_VAL(CUSTOM_OBJ(d), 0) = MIN(MAX(atoi(arg), 0), 1000);
                    break;
                case ITEM_GUN:
                    if (number < 0 || number > NUM_GUN_TYPES)
                        custom_disp_gun_menu(d);
                    else
                        GET_OBJ_VAL(CUSTOM_OBJ(d), 0) = number;
                    break;
                case ITEM_EXPANSION:
                    if (number < 0 || number > 250) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 250);
                        return;
                    }
                    else
                        GET_OBJ_VAL(CUSTOM_OBJ(d), 0) = number;
                    break;
                default:
                    if (number < 0 || number > 100000000) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 10000);
                        return;
                    }
                    else
                        GET_OBJ_VAL(CUSTOM_OBJ(d), 0) = number;
            }
            custom_disp_val2_menu(d);
            return;
        case CUSTOM_VALUE_2:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                  GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, MAX_WEAPON_NDICE);
                  custom_disp_val3_menu(d);
                  break;
                case ITEM_GUN:
                    if (number < 0 || number > 1000) {
                        write_to_output(d, "Try again (%d to %d) : ", 1, 1000);
                        return;
                    }
                    else
                        GET_OBJ_VAL(CUSTOM_OBJ(d), 1) =  number;
                    break;
                case ITEM_EXPANSION:
                    if (number < 0 || number > 250) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 250);
                        return;
                    }
                    else
                        GET_OBJ_VAL(CUSTOM_OBJ(d), 1) =  number;
                    break;
                default:
                    if (number < 0 || number > 10000) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 10000);
                        return;
                    }
                    else
                       GET_OBJ_VAL(CUSTOM_OBJ(d), 1) =  number;
                    break;
            }

            custom_disp_val3_menu(d);
            return;
        case CUSTOM_VALUE_3:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                    min_val = 1;
                    max_val = MAX_WEAPON_SDICE;
                    break;
                case ITEM_GUN:
                    min_val = 0;
                    max_val = 1000;
                    break;
                case ITEM_EXPANSION:
                    min_val = 0;
                    max_val = 250;
                    break;
                default:
                    min_val = -65000;
                    max_val = 65000;
                    break;
            }
            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(CUSTOM_OBJ(d), 2) = number;
                custom_disp_val4_menu(d);
            }
            return;
        case CUSTOM_VALUE_4:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                    min_val = 0;
                    max_val = NUM_ATTACK_TYPES - 1;
                    break;
                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;
                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;
                default:
                    min_val = -65000;
                    max_val = 65000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(CUSTOM_OBJ(d), 3) = number;
                custom_disp_val5_menu(d);
            }
            return;
        case CUSTOM_VALUE_5:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {
                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;
                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;
                default:
                    min_val = 0;
                    max_val = 10000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(CUSTOM_OBJ(d), 4) = number;
                custom_disp_val6_menu(d);
            }
            return;
        case CUSTOM_VALUE_6:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(CUSTOM_OBJ(d))) {

                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;
                case ITEM_EXPANSION:
                    min_val = 0;
                    max_val = 250;
                    break;
                default:
                    min_val = 0;
                    max_val = 10000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(CUSTOM_OBJ(d), 5) = number;
                custom_disp_menu(d, CUSTOM_OBJ(d));
            }
            return;
        case CUSTOM_WEAR:
            number = atoi(arg);
            if ((number < 0) || (number > NUM_ITEM_WEARS)) {
                write_to_output(d, "That's not a valid choice!\r\n");
                custom_disp_wear_menu(d);
                return;
            }
            else if (number == 0)    /* Quit. */
                break;
            else {
                TOGGLE_BIT_AR(GET_OBJ_WEAR(CUSTOM_OBJ(d)), (number - 1));
                custom_disp_wear_menu(d);
                return;
            }
        case CUSTOM_PROMPT_APPLY:
            if ((number = atoi(arg)) == 0)
                break;
            else if (number < 0 || number > MAX_OBJ_APPLY) {
                custom_disp_prompt_apply_menu(d);
                return;
            }
            custom_disp_apply_menu(d);
            return;
        case CUSTOM_APPLY:
            if ((number = atoi(arg)) == 0) {
                GET_OBJ_APPLY_LOC(CUSTOM_OBJ(d), CUSTOM_VAL(d)) = 0;
                GET_OBJ_APPLY_MOD(CUSTOM_OBJ(d), CUSTOM_VAL(d)) = 0;
                custom_disp_prompt_apply_menu(d);
            }
            else if (number < 0 || number >= NUM_APPLIES)
                custom_disp_apply_menu(d);
            else {
                GET_OBJ_APPLY_LOC(CUSTOM_OBJ(d), CUSTOM_VAL(d)) = number;

                if (number == APPLY_SKILL) {
                    int i = 0;
                    strcpy(help, "Skill being one of the following:\r\n");

                    for (i = 1; i <= NUM_SKILLS; i++) {
                        if (*skills_info[i].name == '!')
                            continue;

                        sprintf(help + strlen(help), "%2d) %-18s", i, skills_info[i].name);

                        if (i % 4 == 3) {
                            strcat(help, "\r\n");
                            write_to_output(d, "%s", help);
                            *help = '\0';
                        }
                    }

                    if (*help)
                        write_to_output(d, "%s", help);

                    write_to_output(d, "\r\nWhat skill should this imbue? : ");
                    CUSTOM_MODE(d) = CUSTOM_APPLYMOD;
                }
                else {
                    write_to_output(d, "Modifier : ");
                    CUSTOM_MODE(d) = CUSTOM_APPLYMOD;
                }
            }
            return;

        case CUSTOM_APPLYMOD:
            number = atoi(arg);
            if (GET_OBJ_APPLY_LOC(CUSTOM_OBJ(d), CUSTOM_VAL(d)) == APPLY_SKILL) {

                if (number == 0)    /* Quit. */
                    break;

                if (number < 1 || number > TOP_SKILL_DEFINE || !strcmp("!UNUSED!", skills_info[number].name)) {
                    write_to_output(d, "Unrecognized skill.\r\n");
                    return;
                }
            }

            GET_OBJ_APPLY_MOD(CUSTOM_OBJ(d), CUSTOM_VAL(d)) = number;
            custom_disp_prompt_apply_menu(d);
            return;
        case OEDIT_WPNPSIONIC:
            if ((number = atoi(arg)) == 0) {
                CUSTOM_OBJ(d)->obj_wpnpsi.which_psionic = 0;
                CUSTOM_OBJ(d)->obj_wpnpsi.skill_level = 0;
                break;
            }
            else if (number < 0 || number > NUM_WPNPSIONICS) {
                write_to_output(d, "Invalid psionic, try again: ");
                return;
            }
            CUSTOM_OBJ(d)->obj_wpnpsi.which_psionic = number;
            write_to_output(d, "Skill Level: ");
            CUSTOM_MODE(d) = CUSTOM_WPNSKILL;
            return;

        case CUSTOM_WPNSKILL:
            number = atoi(arg);
            if (number < 0 || number > LVL_IMPL + 65) {
                write_to_output(d, "Invalid level, try again: ");
                return;
            }
            CUSTOM_OBJ(d)->obj_wpnpsi.skill_level = number;
            break;

        case CUSTOM_AFF:
            number = atoi(arg);
            if (number < 0 || number >= NUM_AFF_FLAGS) {
                write_to_output(d, "Invalid selection.  Enter affect type (0 is no affect and exits) :");
                return;
            }
			else if (number == APPLY_SAVING_NORMAL) {
				write_to_output(d, "Sorry, you cant use that.\r\n");
				return;
			}
            else if (number == 0)
                // go back to the main menu
                break;
            else if (OBJAFF_FLAGGED(CUSTOM_OBJ(d), number)) {
                REMOVE_BIT_AR(GET_OBJ_AFFECT(CUSTOM_OBJ(d)), number);
                custom_disp_aff_menu(d);
                return;
            }
            else {

                int count = 0;
                int a = 0;
                // do we have too many affects on this object?
                for (a = 0; a < NUM_AFF_FLAGS; a++)
                    if (OBJAFF_FLAGGED(CUSTOM_OBJ(d), a))
                        count++;

                if (count > MAX_OBJ_AFFECT) {
                    write_to_output(d, "Too many object affects.  Max is: %d.  Remove an affect before you add one.", MAX_OBJ_AFFECT);
                    write_to_output(d, "Enter affect type (0 is no affect and exits) :");
                    return;
                }

                SET_BIT_AR(GET_OBJ_AFFECT(CUSTOM_OBJ(d)), number);
                custom_disp_aff_menu(d);
                return;
            }
            break;
        default:
			extract_obj(CUSTOM_OBJ(d));
			custom_cleanup(d);
            mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: CUSTOM EDITOR: Reached default case in custom_parse()!");
            write_to_output(d, "Oops...\r\n");
            break;
	}
}
ACMD(do_custom)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct obj_data *ticket;
  bool success = FALSE;
  bool hasticket = FALSE;
  const char usage[] = "Usage: \r\n" 
	"custom new                    - Create new custom item (Min: 25 QP) @R- CURRENTLY DISABLED@x\r\n" 
	"custom <item> edit            - Modify an existing custom item @R- CURRENTLY DISABLED@x\r\n"
	"custom <item> name <keywords> - Change an item's keywords (Requires restring ticket)\r\n"
	"custom <item> short <string>  - Change an item's worn description (Requires restring ticket)\r\n"
	"custom <item> long <string>   - Change an item's ground description  (Reqruires restring ticket)\r\n";
  
  argument = one_argument(argument, arg1); 
  if (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_CUSTOM))) {
	send_to_char(ch, "You must find a custom fabricator for that.\r\n");
	return;
  }
  if (IS_NPC(ch)) {
	send_to_char(ch, "ROFL, no.\r\n");
	return;
  }
  if (!*arg1) {
	send_to_char(ch, usage);
	return;
  } 
  if (!strcmp(arg1, "new")) {
	if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
	  return;
	if (GET_QUESTPOINTS(ch) < 25) {
	  send_to_char(ch, "You need 25 quest points banked in order to begin the process.\r\n");
	  return;
	}
	send_to_char(ch, "This command has been temporarily disabled.  Please check back later.\r\n");
	build_new_custom(ch);
	return;
  }
  else {
	if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
	  send_to_char(ch, "You don't seem to have that objct.\r\n");
      return;
    } 
	else {
	    argument = one_argument(argument, arg2);
		if (!*arg2) {
			send_to_char(ch, usage);
			return;
		}
	    if (!strcmp(arg2, "edit")) {
			if (GET_QUESTPOINTS(ch) < 25) {
				send_to_char(ch, "You need 25 quest points banked in order to begin the process.\r\n");
				return;
			}
		    send_to_char(ch, "This command has been temporarily disabled.  Please check back later.\r\n");
		    //edit_existing_custom(obj, ch, arg2);
		    return;
		}
		else {
			for (ticket = ch->carrying; ticket; ticket = ticket->next_content)
				if (GET_OBJ_VNUM(ticket) == 22771) {
					hasticket = TRUE;
					break;
				}
			if (hasticket == FALSE) {
				send_to_char(ch, "You don't have any restring tickets.\r\n");
				return;
			}
			if (is_abbrev(arg2, "name") && hasticket == TRUE)
				success = restring_name(ch, obj, argument);
			else if (is_abbrev(arg2, "short") && hasticket == TRUE)
				success = restring_short(ch, obj, argument);
			else if (!strcmp(arg2, "long") && hasticket == TRUE)
				success = restring_long(ch, obj, argument);
			if (success == TRUE && hasticket == TRUE && ticket != NULL) {
				send_to_char(ch, "Object successfully customized.\r\n");
				extract_obj(ticket);
				return;
			}
			else {
				send_to_char(ch, usage);
				return;
			}
	  }
    }
  }
}

bool oset_weaponpsionic(struct obj_data *obj, char * argument)
{ 
  int i = 0, apply = -1, location = -1, mod = 0, empty = -1, value;
  char arg[MAX_INPUT_LENGTH];
  
  argument = one_argument(argument, arg);
  
  skip_spaces(&argument);
  
  if ((value = atoi(argument)) == 0)
    return FALSE;
  
  while (*wpn_psionics[i] != '\n') {
    if (is_abbrev(wpn_psionics[i], arg)) {
      apply = i;
      break;
    }
    i++;
  }
  
  if (apply == -1)
    return FALSE;
	
  for (i = 0; i < NUM_WPNPSIONICS; i++) {
    if (GET_OBJ_WEAPONPSIONIC == apply) {
      location = i;
        mod = GET_OBJ_WEAPONPSIONICSKILL;
      break;
    } else if (GET_OBJ_WEAPONPSIONIC == 0 && empty == -1) {
      empty = i;
    }
  }
  
  /* No slot already using APPLY_XXX, so use an empty one... if available */
  if (location == -1)
    location = empty;  
  
  /* There is no slot already using our APPLY_XXX type, and no empty slots either */
  if (location == -1)
    return FALSE;
        GET_OBJ_WEAPONPSIONICSKILL  = mod + value;
  
  /* Our modifier is set at 0, so lets just clear the apply location so that it may
   * be reused at a later point */
  if (GET_OBJ_WEAPONPSIONICSKILL  != 0)
    GET_OBJ_WEAPONPSIONIC  = apply;
  else
    GET_OBJ_WEAPONPSIONIC  = 0;
  
  return TRUE;
}






bool oset_itemlevel(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 2; 

	skip_spaces(&argument); 
  
	if (strlen(argument) > max_len) 
		return FALSE; 
  
	GET_OBJ_LEVEL(obj) = atoi(argument); 
  
	return TRUE; 
} 
bool oset_remortlevel(struct obj_data *obj, char * argument) 
{ 
	static int max_len = 2; 

	skip_spaces(&argument); 
  
	if (strlen(argument) > max_len) 
		return FALSE; 
  
	GET_OBJ_REMORT(obj) = atoi(argument); 
         SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_REMORT_ONLY);
	return TRUE; 
} 
