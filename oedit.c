/**************************************************************************
*  File: oedit.c                                           Part of tbaMUD *
*  Usage: Oasis OLC - Objects.                                            *
*                                                                         *
* By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "psionics.h"
#include "db.h"
#include "boards.h"
#include "constants.h"
#include "shop.h"
#include "genolc.h"
#include "genobj.h"
#include "genzon.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_olc.h"
#include "fight.h"
#include "modify.h"
#include "drugs.h"
#include "craft.h"

/* local functions */
static void oedit_setup_new(struct descriptor_data *d);
static void oedit_disp_container_flags_menu(struct descriptor_data *d);
static void oedit_disp_extradesc_menu(struct descriptor_data *d);
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
static void oedit_liquid_type(struct descriptor_data *d);
static void oedit_disp_apply_menu(struct descriptor_data *d);
static void oedit_disp_weapon_menu(struct descriptor_data *d);
static void oedit_disp_psionics_menu(struct descriptor_data *d);
static void oedit_disp_val1_menu(struct descriptor_data *d);
static void oedit_disp_val2_menu(struct descriptor_data *d);
static void oedit_disp_val3_menu(struct descriptor_data *d);
static void oedit_disp_val4_menu(struct descriptor_data *d);
static void oedit_disp_val5_menu(struct descriptor_data *d);
static void oedit_disp_val6_menu(struct descriptor_data *d);
static void oedit_disp_type_menu(struct descriptor_data *d);
static void oedit_disp_extra_menu(struct descriptor_data *d);
static void oedit_disp_wear_menu(struct descriptor_data *d);
static void oedit_disp_body_part_menu(struct descriptor_data *d);
static void oedit_disp_menu(struct descriptor_data *d);
static void oedit_disp_aff_menu(struct descriptor_data *d);
static void oedit_disp_composition_menu(struct descriptor_data *d);
static void oedit_disp_subcomposition_menu(struct descriptor_data *d);
static void oedit_disp_ammodam_menu(struct descriptor_data *d);
static void oedit_disp_wpndam_menu(struct descriptor_data *d);
static void oedit_save_to_disk(int zone_num);


/* handy macro */
#define S_PRODUCT(s, i) ((s)->producing[(i)])

/* Utility and exported functions */
ACMD(do_oasis_oedit)
{
  int number = NOWHERE, save = 0, real_num;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /* No building as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments. */
  buf3 = two_arguments(argument, buf1, buf2);

  /* If there aren't any arguments they can't modify anything. */
  if (!*buf1) {
    send_to_char(ch, "Specify an object VNUM to edit.\r\n");
    return;
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }

    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0) {
      zone_rnum zlok;

      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }

    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }

  /* If a numeric argument was given, get it. */
  if (number == NOWHERE)
    number = atoi(buf1);

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_OEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That object is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }

  /* Point d to the builder's descriptor (for easier typing later). */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE,
      "SYSERR: do_oasis: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Find the zone. */
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* Everyone but IMPLs can only edit zones they have been assigned. */
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_cannot_edit(ch, zone_table[OLC_ZNUM(d)].number);
    /* Free the OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  /* If we need to save, save the objects. */
  if (save) {
    send_to_char(ch, "Saving all objects in zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves object info for zone %d.", GET_NAME(ch),
      zone_table[OLC_ZNUM(d)].number);

    /* Save the objects in this zone. */
    save_objects(OLC_ZNUM(d));

    /* Free the descriptor's OLC structure. */
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;

  /* If a new object, setup new, otherwise setup the existing object. */
  if ((real_num = real_object(number)) != NOTHING)
    oedit_setup_existing(d, real_num);
  else
    oedit_setup_new(d);

  oedit_disp_menu(d);
  STATE(d) = CON_OEDIT;

  /* Send the OLC message to the players in the same room as the builder. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /* Log the OLC message. */
  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}

static void oedit_setup_new(struct descriptor_data *d)
{
	int i = 0;

  CREATE(OLC_OBJ(d), struct obj_data, 1);

  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = strdup("unfinished object");
  OLC_OBJ(d)->description = strdup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = strdup("an unfinished object");
  SET_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), ITEM_WEAR_TAKE);
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
    for (i = 0; i < 6; i++)
        GET_OBJ_VAL(OLC_OBJ(d), i) = 0;
  SCRIPT(OLC_OBJ(d)) = NULL;
  OLC_OBJ(d)->proto_script = OLC_SCRIPT(d) = NULL;
}

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{
  struct obj_data *obj;

  /* Allocate object in memory. */
  CREATE(obj, struct obj_data, 1);
  copy_object(obj, &obj_proto[real_num]);

  /* Attach new object to player's descriptor. */
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  OLC_ITEM_TYPE(d) = OBJ_TRIGGER;
  dg_olc_script_copy(d);
  /* The edited obj must not have a script. It will be assigned to the updated
   * obj later, after editing. */
  SCRIPT(obj) = NULL;
  OLC_OBJ(d)->proto_script = NULL;
}

void oedit_save_internally(struct descriptor_data *d)
{
  int i;
  obj_rnum robj_num;
  struct descriptor_data *dsc;
  struct obj_data *obj;

  i = (real_object(OLC_NUM(d)) == NOTHING);

  if ((robj_num = add_object(OLC_OBJ(d), OLC_NUM(d))) == NOTHING) {
    log("oedit_save_internally: add_object failed.");
    return;
  }

  /* Update triggers and free old proto list  */
  if (obj_proto[robj_num].proto_script &&
      obj_proto[robj_num].proto_script != OLC_SCRIPT(d))
    free_proto_script(&obj_proto[robj_num], OBJ_TRIGGER);
  /* this will handle new instances of the object: */
  obj_proto[robj_num].proto_script = OLC_SCRIPT(d);

  /* this takes care of the objects currently in-game */
  for (obj = object_list; obj; obj = obj->next) {
    if (obj->item_number != robj_num)
      continue;
    /* remove any old scripts */
    if (SCRIPT(obj))
      extract_script(obj, OBJ_TRIGGER);

    free_proto_script(obj, OBJ_TRIGGER);
    copy_proto_script(&obj_proto[robj_num], obj, OBJ_TRIGGER);
    assign_triggers(obj, OBJ_TRIGGER);
  }
  /* end trigger update */

  if (!i)	/* If it's not a new object, don't renumber. */
    return;

  /* Renumber produce in shops being edited. */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_SEDIT)
      for (i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != NOTHING; i++)
	if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
	  S_PRODUCT(OLC_SHOP(dsc), i)++;


  /* Update other people in zedit too. From: C.Raehl 4/27/99 */
  for (dsc = descriptor_list; dsc; dsc = dsc->next)
    if (STATE(dsc) == CON_ZEDIT)
      for (i = 0; OLC_ZONE(dsc)->cmd[i].command != 'S'; i++)
        switch (OLC_ZONE(dsc)->cmd[i].command) {
          case 'P':
            OLC_ZONE(dsc)->cmd[i].arg3 += (OLC_ZONE(dsc)->cmd[i].arg3 >= robj_num);
            /* Fall through. */
          case 'E':
          case 'G':
          case 'O':
            OLC_ZONE(dsc)->cmd[i].arg1 += (OLC_ZONE(dsc)->cmd[i].arg1 >= robj_num);
            break;
          case 'R':
            OLC_ZONE(dsc)->cmd[i].arg2 += (OLC_ZONE(dsc)->cmd[i].arg2 >= robj_num);
            break;
          default:
          break;
        }
}

static void oedit_save_to_disk(int zone_num)
{
  save_objects(zone_num);
}

/* Menu functions */
/* For container flags. */
static void oedit_disp_container_flags_menu(struct descriptor_data *d)
{
  char bits[MAX_STRING_LENGTH];
  get_char_colors(d->character);
  clear_screen(d);

  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, bits, sizeof(bits));
  write_to_output(d,
	  "%s1%s) CLOSEABLE\r\n"
	  "%s2%s) PICKPROOF\r\n"
	  "%s3%s) CLOSED\r\n"
	  "%s4%s) LOCKED\r\n"
	  "Container flags: %s%s%s\r\n"
	  "Enter flag, 0 to quit : ",
	  grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, bits, nrm);
}

/* For extra descriptions. */
static void oedit_disp_extradesc_menu(struct descriptor_data *d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);

  get_char_colors(d->character);
  clear_screen(d);
  write_to_output(d,
	  "Extra desc menu\r\n"
	  "%s1%s) Keywords: %s%s\r\n"
	  "%s2%s) Description:\r\n%s%s\r\n"
	  "%s3%s) Goto next description: %s\r\n"
	  "%s0%s) Quit\r\n"
	  "Enter choice : ",

     	  grn, nrm, yel, (extra_desc->keyword && *extra_desc->keyword) ? extra_desc->keyword : "<NONE>",
	  grn, nrm, yel, (extra_desc->description && *extra_desc->description) ? extra_desc->description : "<NONE>",
	  grn, nrm, !extra_desc->next ? "Not set." : "Set.", grn, nrm);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/* Ask for *which* apply to edit. */
static void oedit_disp_prompt_apply_menu(struct descriptor_data *d)
{
    char apply_buf[MAX_STRING_LENGTH];
    int counter;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < MAX_OBJ_APPLY; counter++) {
        if (GET_OBJ_APPLY_MOD(OLC_OBJ(d), counter)) {
            sprinttype(GET_OBJ_APPLY_LOC(OLC_OBJ(d), counter), apply_types, apply_buf, sizeof(apply_buf));
			int mod = GET_OBJ_APPLY_MOD(OLC_OBJ(d), counter);
            
			if(!strcmp(apply_buf,"SKILLSET"))
			{
				write_to_output(d, " %s%d%s) Grant Skill: %s\r\n", grn, counter + 1, nrm, skills_info[mod].name);
			}
			else if (!strcmp(apply_buf,"PSI_MASTERY"))
			{
				write_to_output(d, " %s%d%s) Grant Psi: %s\r\n", grn, counter + 1, nrm, psionics_info[mod].name);
			}
			else
			{
				write_to_output(d, " %s%d%s) %+d to %s\r\n", grn, counter + 1, nrm, mod, apply_buf);
			}
        }
        else
            write_to_output(d, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
    }

    write_to_output(d, "\r\nEnter affection to modify (0 to quit) : ");
    OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/* Ask for liquid type. */
static void oedit_liquid_type(struct descriptor_data *d)
{
  get_char_colors(d->character);
  clear_screen(d);
  column_list(d->character, 0, drinks, NUM_LIQ_TYPES, TRUE);
  write_to_output(d, "\r\n%sEnter drink type : ", nrm);
  OLC_MODE(d) = OEDIT_VALUE_3;
}
static void oedit_disp_subcomposition_menu(struct descriptor_data *d)
{
	int i;
	int columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	if ((OLC_OBJ(d))->obj_flags.composition < 0) {
		for (i = 0; i < MAX_NOTHING; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			nothing_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	if ((OLC_OBJ(d))->obj_flags.composition == 0) {
		for (i = 0; i < MAX_NOTHING; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			nothing_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 1) {
		for (i = 0; i < MAX_TRADEGOODS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			trade_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 2) {
		for (i = 0; i < MAX_METALS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			metal_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 3) {
		for (i = 0; i < MAX_PRECIOUSMETALS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			pmetal_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 4) {
		for (i = 0; i < MAX_GEMS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			gem_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 5) {
		for (i = 0; i < MAX_ROCKS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			rock_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 6) {
		for (i = 0; i < MAX_ORGANICS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			organic_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition == 7) {
		for (i = 0; i < MAX_OTHERS; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			other_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else if ((OLC_OBJ(d))->obj_flags.composition > 7) {
		for (i = 0; i < MAX_NOTHING; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			nothing_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}
	}
	else 
		for (i = 0; i < MAX_NOTHING; i++) {
			write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
			nothing_composition_types[i], !(++columns % 1) ? "\r\n" : "");
		}

	write_to_output(d, "\r\n%sEnter SubComposition type : ", nrm);
	OLC_MODE(d) = OEDIT_SUBCOMPOSITION;
}

static void oedit_disp_composition_menu(struct descriptor_data *d)
{
	int i;
	int columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (i = 0; i < NUM_COMPOSITION; i++) {
		write_to_output(d, "%s%2d%s %s %s", grn, i, nrm,
		category_composition_types[i], !(++columns % 1) ? "\r\n" : "");
	}

	write_to_output(d, "\r\n%sEnter Composition type : ", nrm);
	OLC_MODE(d) = OEDIT_COMPOSITION;
}
/* The actual apply to set. */
static void oedit_disp_apply_menu(struct descriptor_data *d)
{
	int counter;
	int columns = 0;
  get_char_colors(d->character);
  clear_screen(d);
  //column_list(d->character, 0, apply_types, NUM_APPLIES, TRUE);
  for (counter = 0; counter < NUM_APPLIES; counter++)
	  write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel, apply_types[counter],nrm, !(++columns % 3) ? "\r\n" : "");

  write_to_output(d, "\r\nEnter apply type (0 is no apply) : ");
  OLC_MODE(d) = OEDIT_APPLY;
}
static void oedit_disp_aff_menu(struct descriptor_data *d)
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

    OLC_MODE(d) = OEDIT_AFF;
}

/* Ammo damage types - Gahan 2013 */
static void oedit_disp_ammodam_menu(struct descriptor_data *d)
{
	int counter;
	int columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < MAX_DMG; counter++)
		write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel,
			damagetype_bits[counter], nrm, !(++columns % 3) ? "\r\n" : "");

	write_to_output(d, "\r\nEnter the damage type for this ammo (0 is normal damage) : ");
	OLC_MODE(d) = OEDIT_AMMODAM;
}

/* Weapon damage types - Gahan 2013 */
static void oedit_disp_wpndam_menu(struct descriptor_data *d)
{
	int counter;
	int columns = 0;

	get_char_colors(d->character);
	clear_screen(d);

	for (counter = 0; counter < MAX_DMG; counter++)
		write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel,
			damagetype_bits[counter], nrm, !(++columns % 3) ? "\r\n" : "");

	write_to_output(d, "\r\nEnter the damage type for this weapon (0 is normal damage) : ");
}

// Drug types
static void oedit_disp_drugtypes_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter <= MAX_DRUGS; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel,
            drug_names[counter], nrm, !(++columns % 3) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter drug type (0 is no effect) : ");
    OLC_MODE(d) = OEDIT_DRUGTYPE;
}

// Bombs and Explosives.
static void oedit_disp_explosive_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter <= NUM_EXPLOSIVE_TYPES; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s%s", grn, counter, nrm, yel, explosive_types[counter], nrm,
            !(++columns % 2) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter type : ");
}

static void oedit_disp_subexplosive_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    if (GET_EXPLOSIVE_TYPE(OLC_OBJ(d)) == EXPLOSIVE_GRENADE) {

        for (counter = 1; counter <= NUM_GRENADE_TYPES; counter++)
            write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm, subexplosive_types[counter],
                !(++columns % 2) ? "\r\n" : "");

        write_to_output(d, "\r\nEnter type : ");
    }
    else
        oedit_disp_val3_menu(d);
}

// Gun/Ammo type
static void oedit_disp_gun_menu(struct descriptor_data *d)
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

// Weapon type
static void oedit_disp_weapon_menu(struct descriptor_data *d)
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

// Psionic type
static void oedit_disp_psionics_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter <= NUM_PSIONICS; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            psionics_info[counter].name, !(++columns % 3) ? "\r\n" : "");

    write_to_output(d, "\r\n%sEnter psionic choice (-1 for none) : ", nrm);
}

// Weapon Psionics
static void oedit_disp_wpnpsionics_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter < NUM_WPNPSIONICS; counter++)
        write_to_output(d, "%s%2d%s) %s%-20.20s %s", grn, counter, nrm, yel,
            wpn_psionics[counter], !(++columns % 3) ? "\r\n" : "");

    write_to_output(d, "\r\n%sEnter psionic choice (0 for none) : ", nrm);
    OLC_MODE(d) = OEDIT_WPNPSIONIC;
}

// Object value #1
static void oedit_disp_val1_menu(struct descriptor_data *d)
{
	int i;
	int columns = 0;
    OLC_MODE(d) = OEDIT_VALUE_1;

log(".. object item type: %d", GET_OBJ_TYPE(OLC_OBJ(d)));

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {

        case ITEM_LIGHT:
            // values 1 and 2 are unused.. jump to 3
            oedit_disp_val3_menu(d);
            break;
        case ITEM_KEY:
            // values 1 to 4 are unused.. jump to 5
            oedit_disp_val5_menu(d);
            break;
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_POTION:
            write_to_output(d, "Psionic level (1 to 100): ");
            break;
        case ITEM_TRAP:
            oedit_disp_psionics_menu(d);
            break;
        case ITEM_WEAPON:
			oedit_disp_wpndam_menu(d);
			break;
        case ITEM_WEAPONUPG:
            // This doesn't seem to be used if I remember right
            oedit_disp_val2_menu(d);
            break;
        case ITEM_ARMOR:
            write_to_output(d, "Apply to AC : ");
            break;
        case ITEM_CONTAINER:
        case ITEM_TABLE:
            write_to_output(d, "Max weight to contain (-1 for unlimited) : ");
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            write_to_output(d, "Max drink units (-1 for unlimited) : ");
            break;
        case ITEM_FOOD:
            write_to_output(d, "Hours to fill stomach : ");
            break;
        case ITEM_MONEY:
            write_to_output(d, "Number of units : ");
            break;
        case ITEM_FURNITURE:
            write_to_output(d, "Number of people it can hold : ");
            break;
        case ITEM_GUN:
        case ITEM_AMMO:
            oedit_disp_gun_menu(d);
            break;
        case ITEM_EXPLOSIVE:
            oedit_disp_explosive_menu(d);
            break;
        case ITEM_RECALL:
            write_to_output(d, "Number of charges (0 = dead, -1 is infinite) : ");
            break;
        case ITEM_MEDKIT:
            write_to_output(d, "Amount of hit points to restore : ");
            break;
        case ITEM_DRUG:
            oedit_disp_drugtypes_menu(d);
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Regen all Mod : ");
            break;
        case ITEM_BIONIC_DEVICE:
            oedit_disp_body_part_menu(d);
            break;
        case ITEM_AUTOQUEST:
            write_to_output(d, "Token Number : ");
            break;
		case ITEM_MEDICAL:
			write_to_output(d, "Lasts how many deaths? : ");
			break;
		case ITEM_VEHICLE:
			for (i = 0; i < MAX_VEHICLES;i++)
				write_to_output(d, "%s%2d%s) %-20.20s %s", grn, i, nrm, vehicle_names[i], !(++columns % 3) ? "\r\n" : "");
			write_to_output(d, "\r\nWhich vehicle? : ");
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object value #2
static void oedit_disp_val2_menu(struct descriptor_data *d)
{
	int i;
	int columns = 0;
    OLC_MODE(d) = OEDIT_VALUE_2;

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
            oedit_disp_psionics_menu(d);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            write_to_output(d, "Max number of charges : ");
            break;
        case ITEM_TRAP:
            write_to_output(d, "Hitpoints : ");
            break;
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            write_to_output(d, "Number of damage dice : ");
            break;
        case ITEM_FOOD:
            // Values 2 and 3 are unused, jump to 4...Odd
            oedit_disp_val4_menu(d);
            break;
        case ITEM_CONTAINER:
            // These are flags, needs a bit of special handling
            oedit_disp_container_flags_menu(d);
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            write_to_output(d, "Initial drink units : ");
            break;
        case ITEM_GUN:
            write_to_output(d, "Number of damage dice (loaded) : ");
            break;
        case ITEM_AMMO:
            write_to_output(d, "Ammo remaining : ");
            break;
        case ITEM_EXPLOSIVE:
            oedit_disp_subexplosive_menu(d);
            break;
        case ITEM_MEDKIT:
            write_to_output(d, "Amount of psi points to restore : ");
            break;
        case ITEM_DRUG:
            write_to_output(d, "How long should it last? (in tics) : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of AC to increase/decrease : ");
            break;
        case ITEM_BIONIC_DEVICE:
			for (i = 1; i < NUM_BODY_PARTS;i++)
				write_to_output(d, "%s%2d%s) %-20.20s %s", grn, i, nrm, body_parts[i], !(++columns % 3) ? "\r\n" : "");
			write_to_output(d, "\r\nEnter wear Location Prerequisite ( 0 for none ) : ");
            break;
		case ITEM_VEHICLE:
			for (i = 0; i < MAX_VEHICLE_TYPES;i++)
				write_to_output(d, "%s%2d%s) %-20.20s %s", grn, i, nrm, vehicle_types[i], !(++columns % 3) ? "\r\n" : "");
			write_to_output(d, "\r\nEnter Vehicle Type : ");
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object value #3
static void oedit_disp_val3_menu(struct descriptor_data *d)
{
	int i;
    OLC_MODE(d) = OEDIT_VALUE_3;

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_LIGHT:
            write_to_output(d, "Number of hours (0 = burnt, -1 is infinite) : ");
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
            oedit_disp_psionics_menu(d);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            write_to_output(d, "Number of charges remaining : ");
            break;
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            write_to_output(d, "Size of damage dice : ");
            break;
        case ITEM_CONTAINER:
            write_to_output(d, "Vnum of key to open container (-1 for no key) : ");
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
            oedit_liquid_type(d);
            break;
        case ITEM_GUN:
            write_to_output(d, "Size of damage dice (loaded) : ");
            break;
        case ITEM_EXPLOSIVE:
            write_to_output(d, "Number of damage dice : ");
            break;
        case ITEM_MEDKIT:
            write_to_output(d, "Amount of move points to restore : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of HnD to increase/decrease : ");
            break;
        case ITEM_BIONIC_DEVICE:
			write_to_output(d, "\r\nEnter Type of Bionic: \r\n");
			for (i = 0; i < MAX_BIO_TYPES;i++)
				write_to_output(d, "%d) %s\r\n", i, biotypes[i]);
            break;
		case ITEM_VEHICLE:
			write_to_output(d, "Maximum Hitpoints : ");
			break;
        case ITEM_AMMO:
            oedit_disp_ammodam_menu(d);
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object value #4
static void oedit_disp_val4_menu(struct descriptor_data *d)
{
	int i;
    OLC_MODE(d) = OEDIT_VALUE_4;

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_WAND:
        case ITEM_STAFF:
            oedit_disp_psionics_menu(d);
            break;
        case ITEM_WEAPON:
        case ITEM_WEAPONUPG:
            oedit_disp_weapon_menu(d);
            break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
        case ITEM_FOOD:
            write_to_output(d, "Poisoned (0 = not poison) : ");
            break;
        case ITEM_GUN:
            write_to_output(d, "Shots per round : ");
            break;
        case ITEM_EXPLOSIVE:
            write_to_output(d, "Size of damage dice : ");
            break;
        case ITEM_MEDKIT:
            write_to_output(d, "Amount of charges available : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of Hit Power to increase/decrease : ");
            break;
        case ITEM_BIONIC_DEVICE:
			write_to_output(d, "\r\nEnter prerequisite #1: \r\n");
			for (i = 0; i < MAX_BIO_TYPES;i++)
				write_to_output(d, "%d) %s\r\n", i, biotypes[i]);
            break;
		case ITEM_VEHICLE:
			write_to_output(d, "Maximum Fuel : \r\n");
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object value #5
static void oedit_disp_val5_menu(struct descriptor_data *d)
{
	int i;
    OLC_MODE(d) = OEDIT_VALUE_5;

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_WAND:
        case ITEM_STAFF:
            oedit_disp_psionics_menu(d);
            break;
        case ITEM_GUN:
            write_to_output(d, "Range : ");
            break;
//timer
        case ITEM_EXPLOSIVE:
            write_to_output(d, "Detonation timer : ");
            break;
        case ITEM_EXPANSION:
            write_to_output(d, "Percentage of Psi Power to increase/decrease : ");
            break;
        case ITEM_KEY:
            write_to_output(d, "Decay timer : ");
            break;
        case ITEM_BIONIC_DEVICE:
			write_to_output(d, "\r\nEnter prerequisite #2: \r\n");
			for (i = 0; i < MAX_BIO_TYPES;i++)
				write_to_output(d, "%d) %s\r\n", i, biotypes[i]);
            break;
		case ITEM_VEHICLE:
			write_to_output(d, "Armor Class :");
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object value #6
static void oedit_disp_val6_menu(struct descriptor_data *d)
{
	int i;
    OLC_MODE(d) = OEDIT_VALUE_6;

    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
        case ITEM_GUN:
            write_to_output(d, "Size of damage dice (unloaded) : ");
            break;
        //  case ITEM_EXPANSION:
        //    write_to_output(d, "Unused: ");
        //    break;
        case ITEM_BIONIC_DEVICE:
			write_to_output(d, "\r\nEnter prerequisite #3: \r\n");
			for (i = 0; i < MAX_BIO_TYPES;i++)
				write_to_output(d, "%d) %s\r\n", i, biotypes[i]);
            break;
		case ITEM_VEHICLE:
			write_to_output(d, "How many GPS locations? (MAX: %d) : ", MAX_GPS_LOCATIONS);
			break;
        default:
            oedit_disp_menu(d);
            break;
    }
}

// Object type
static void oedit_disp_type_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 1; counter <= NUM_ITEM_TYPES -1; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm,
            item_types[counter], !(++columns % 2) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter object type : ");
    OLC_MODE(d) = OEDIT_TYPE;
}

// Object extra flags
static void oedit_disp_extra_menu(struct descriptor_data *d)
{
    char bits[MAX_STRING_LENGTH];
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < NUM_ITEM_FLAGS; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
            extra_bits[counter], !(++columns % 2) ? "\r\n" : "");

    sprintbitarray(GET_OBJ_EXTRA(OLC_OBJ(d)), extra_bits, EF_ARRAY_MAX, bits);
    write_to_output(d, "\r\nObject flags: %s%s%s\r\nEnter object extra flag (0 to quit) : ", cyn, bits, nrm);
    OLC_MODE(d) = OEDIT_EXTRAS;
}

// Object wear flags
static void oedit_disp_wear_menu(struct descriptor_data *d)
{
    char bits[MAX_STRING_LENGTH];
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    for (counter = 0; counter < NUM_ITEM_WEARS; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter + 1, nrm,
            wear_bits[counter], !(++columns % 2) ? "\r\n" : "");

    sprintbitarray(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, TW_ARRAY_MAX, bits);
    write_to_output(d, "\r\nWear flags: %s%s%s\r\nEnter wear flag, 0 to quit : ", cyn, bits, nrm);
    OLC_MODE(d) = OEDIT_WEAR;
}

// Body Part (Bionics) selection
void oedit_disp_body_part_menu(struct descriptor_data *d)
{
    int counter;
    int columns = 0;

    get_char_colors(d->character);
    clear_screen(d);

    write_to_output(d, "\r\n");

    for (counter = 1; counter < NUM_BODY_PARTS; counter++)
        write_to_output(d, "%s%2d%s) %-20.20s %s", grn, counter, nrm, body_parts[counter], !(++columns % 2) ? "\r\n" : "");

    write_to_output(d, "\r\nEnter location for bionic device : ");
}

// Display main menu
static void oedit_disp_menu(struct descriptor_data *d)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
	char buf3[MAX_STRING_LENGTH];
	char buf4[MAX_STRING_LENGTH];
    struct obj_data *obj;

    obj = OLC_OBJ(d);
    get_char_colors(d->character);
    clear_screen(d);

    // Build buffers for first part of menu. */
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf1, sizeof(buf1));
    sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf2);

    // Build first half of menu. */
    write_to_output(d,
        "-- Item number : [%s%d%s]\r\n"
        "%s1%s) Keywords : %s%s\r\n"
        "%s2%s) S-Desc   : %s%s\r\n"
        "%s3%s) L-Desc   :-\r\n%s%s\r\n"
        "%s4%s) A-Desc   :-\r\n%s%s"
        "%s5%s) Type        : %s%s\r\n"
        "%s6%s) Extra flags : %s%s\r\n",

        cyn, OLC_NUM(d), nrm,
        grn, nrm, yel, (obj->name && *obj->name) ? obj->name : "undefined",
        grn, nrm, yel, (obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
        grn, nrm, yel, (obj->description && *obj->description) ? obj->description : "undefined",
        grn, nrm, yel, (obj->action_description && *obj->action_description) ? obj->action_description : "Not Set.\r\n",
        grn, nrm, cyn, buf1,
        grn, nrm, cyn, buf2
        );

    // Send first half then build second half of menu
    sprintbitarray(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, EF_ARRAY_MAX, buf1);
    sprintbitarray(GET_OBJ_AFFECT(obj), affected_bits, AF_ARRAY_MAX, buf2);
	sprinttype(GET_OBJ_COMPOSITION(obj), category_composition_types, buf3, sizeof(buf3));
	if (GET_OBJ_COMPOSITION(obj) < 0) {
		GET_OBJ_COMPOSITION(obj) = 0;
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), nothing_composition_types, buf4, sizeof(buf4));
	}
	else if (GET_OBJ_COMPOSITION(obj) == 0)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), nothing_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 1)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), trade_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 2)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), metal_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 3)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), pmetal_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 4)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), gem_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 5)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), rock_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 6)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), organic_composition_types, buf4, sizeof(buf4));
	else if (GET_OBJ_COMPOSITION(obj) == 7)
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), other_composition_types, buf4, sizeof(buf4));
	else {
		GET_OBJ_COMPOSITION(obj) = 0;
		sprinttype(GET_OBJ_SUBCOMPOSITION(obj), nothing_composition_types, buf4, sizeof(buf4));
	}

    write_to_output(d,
        "%s7%s) Wear flags   : %s%s\r\n"
        "%s8%s) Weight       : %s%d\r\n"
        "%s9%s) Cost         : %s%d\r\n"
        "%sA%s) Composition  : %s%s\r\n"
		"%sB%s) Sub Composition : %s%s\r\n"
        "%sC%s) Timer        : %s%d\r\n"
        "%sD%s) Values       : %s%d %d %d %d %d %d\r\n"
        "%sE%s) Applies menu\r\n"
        "%sF%s) Extra descriptions menu %s%s%s \r\n"
        "%sM%s) Min Level    : %s%d\r\n"
        "%sP%s) Perm Affects : %s%s\r\n"
        "%sW%s) Weapon Psionic\r\n"
		"%sR%s) Remort Restriction: %s%d\r\n"
        "%sS%s) Script       : %s%s\r\n"
        "%sV%s) Copy object\r\n"
        "%sX%s) Delete object\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",

        grn, nrm, cyn, buf1,
        grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
        grn, nrm, cyn, GET_OBJ_COST(obj),
        grn, nrm, cyn, buf3,
		grn, nrm, cyn, buf4,
        grn, nrm, cyn, GET_OBJ_TIMER(obj),
        grn, nrm, cyn, GET_OBJ_VAL(obj, 0),
        GET_OBJ_VAL(obj, 1),
        GET_OBJ_VAL(obj, 2),
        GET_OBJ_VAL(obj, 3),
        GET_OBJ_VAL(obj, 4),
        GET_OBJ_VAL(obj, 5),
        grn, nrm,
        grn, nrm, cyn, GET_OBJ_EXTRA(obj) ? "Set." : "Not Set.", grn,
        grn, nrm, cyn, GET_OBJ_LEVEL(obj),
        grn, nrm, cyn, buf2,
        grn, nrm,
        grn, nrm, cyn, GET_OBJ_REMORT(OLC_OBJ(d)),
        grn, nrm, cyn, OLC_SCRIPT(d) ? "Set." : "Not Set.",
        grn, nrm,
        grn, nrm,
        grn, nrm
        );

    OLC_MODE(d) = OEDIT_MAIN_MENU;
}

// main loop (of sorts).. basically interpreter throws all input to here
void oedit_parse(struct descriptor_data *d, char *arg)
{
    int number;
    int max_val;
    int min_val;
    char *oldtext = NULL;

    switch (OLC_MODE(d)) {

        case OEDIT_CONFIRM_SAVESTRING:
            switch (*arg) {
                case 'y':
                case 'Y':
                    oedit_save_internally(d);
                    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
                        "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));

                    if (CONFIG_OLC_SAVE) {
                        oedit_save_to_disk(real_zone_by_thing(OLC_NUM(d)));
                        write_to_output(d, "Object saved to disk.\r\n");
                    }
                    else
                        write_to_output(d, "Object saved to memory.\r\n");

                    cleanup_olc(d, CLEANUP_ALL);
                    return;

                case 'n':
                case 'N':
                    // If not saving, we must free the script_proto list
                    OLC_OBJ(d)->proto_script = OLC_SCRIPT(d);
                    free_proto_script(OLC_OBJ(d), OBJ_TRIGGER);
                    cleanup_olc(d, CLEANUP_ALL);
                    return;

                case 'a': /* abort quit */
                case 'A':
                    oedit_disp_menu(d);
                    return;

                default:
                    write_to_output(d, "Invalid choice!\r\n");
                    write_to_output(d, "Do you wish to save your changes? : \r\n");
                    return;
            }
            break;

        case OEDIT_MAIN_MENU:
            // Throw us out to whichever edit mode based on user input
            switch (*arg) {
                case 'q':
                case 'Q':
                    if (OLC_VAL(d)) {    /* Something has been modified. */
                        write_to_output(d, "Do you wish to save your changes? : ");
                        OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
                    }
                    else
                        cleanup_olc(d, CLEANUP_ALL);
                    return;

                case '1':
                    write_to_output(d, "Enter keywords : ");
                    OLC_MODE(d) = OEDIT_KEYWORD;
                    break;

                case '2':
                    write_to_output(d, "Enter short desc : ");
                    OLC_MODE(d) = OEDIT_SHORTDESC;
                    break;

                case '3':
                    write_to_output(d, "Enter long desc :-\r\n| ");
                    OLC_MODE(d) = OEDIT_LONGDESC;
                    break;

                case '4':
                    OLC_MODE(d) = OEDIT_ACTDESC;
                    send_editor_help(d);
                    write_to_output(d, "Enter action description:\r\n\r\n");

                    if (OLC_OBJ(d)->action_description) {
                        write_to_output(d, "%s", OLC_OBJ(d)->action_description);
                        oldtext = strdup(OLC_OBJ(d)->action_description);
                    }

                    string_write(d, &OLC_OBJ(d)->action_description, MAX_MESSAGE_LENGTH, 0, oldtext);
                    OLC_VAL(d) = 1;
                    break;

                case '5':
                    oedit_disp_type_menu(d);
                    break;

                case '6':
                    oedit_disp_extra_menu(d);
                    break;

                case '7':
                    oedit_disp_wear_menu(d);
                    break;

                case '8':
                    write_to_output(d, "Enter weight : ");
                    OLC_MODE(d) = OEDIT_WEIGHT;
                    break;

                case '9':
                    write_to_output(d, "Enter cost : ");
                    OLC_MODE(d) = OEDIT_COST;
                    break;

                case 'a':
                case 'A':
					oedit_disp_composition_menu(d);
                    OLC_MODE(d) = OEDIT_COMPOSITION;
                    break;
				
				case 'b':
				case 'B':
					oedit_disp_subcomposition_menu(d);
					OLC_MODE(d) = OEDIT_SUBCOMPOSITION;
					break;
                case 'c':
                case 'C':
                    write_to_output(d, "Enter timer : ");
                    OLC_MODE(d) = OEDIT_TIMER;
                    break;

                case 'd':
                case 'D':
                    // Clear any old values
                    GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
                    GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
                    GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
                    GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
                    GET_OBJ_VAL(OLC_OBJ(d), 4) = 0;
                    GET_OBJ_VAL(OLC_OBJ(d), 5) = 0;
                    OLC_VAL(d) = 1;
                    oedit_disp_val1_menu(d);
                    break;

                case 'e':
                case 'E':
                    oedit_disp_prompt_apply_menu(d);
                    break;

                case 'f':
                case 'F':
                    // If extra descriptions don't exist
                    if (OLC_OBJ(d)->ex_description == NULL) {
                        CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
                        OLC_OBJ(d)->ex_description->next = NULL;
                    }
                    OLC_DESC(d) = OLC_OBJ(d)->ex_description;
                    oedit_disp_extradesc_menu(d);
                    break;

                case 'm':
                case 'M':
                    write_to_output(d, "Enter new minimum level: ");
                    OLC_MODE(d) = OEDIT_LEVEL;
                    break;

                case 'p':
                case 'P':
                    oedit_disp_aff_menu(d);
                    break;

                case 's':
                case 'S':
                    OLC_SCRIPT_EDIT_MODE(d) = SCRIPT_MAIN_MENU;
                    dg_script_menu(d);
                    return;

                case 'w':
                case 'W':
                    oedit_disp_wpnpsionics_menu(d);
                    break;

                case 'r':
                case 'R':
                    write_to_output(d, "Enter Remort Restriction : ");
                    OLC_MODE(d) = OEDIT_REMORT;
                    break;

                case 'v':
                case 'V':
                    write_to_output(d, "Copy what object? ");
                    OLC_MODE(d) = OEDIT_COPY;
                    break;

                case 'x':
                case 'X':
                    write_to_output(d, "Are you sure you want to delete this object? ");
                    OLC_MODE(d) = OEDIT_DELETE;
                    break;

                default:
                    oedit_disp_menu(d);
                    break;
            }
            return;

        case OLC_SCRIPT_EDIT:
            if (dg_script_edit_parse(d, arg)) return;
                break;

        case OEDIT_KEYWORD:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_OBJ(d)->name)
                free(OLC_OBJ(d)->name);

            OLC_OBJ(d)->name = str_udup(arg);
            break;

        case OEDIT_SHORTDESC:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_OBJ(d)->short_description)
                free(OLC_OBJ(d)->short_description);

            OLC_OBJ(d)->short_description = str_udup(arg);
            break;

        case OEDIT_LONGDESC:
            if (!genolc_checkstring(d, arg))
                break;

            if (OLC_OBJ(d)->description)
                free(OLC_OBJ(d)->description);

            OLC_OBJ(d)->description = str_udup(arg);
            break;

        case OEDIT_TYPE:
            number = atoi(arg);
            if ((number < 1) || (number > NUM_ITEM_TYPES)) {
                write_to_output(d, "Invalid choice, try again : ");
                return;
            }
            else
                GET_OBJ_TYPE(OLC_OBJ(d)) = number;

            // what's the boundschecking worth if we don't do this ? -- Welcor
            GET_OBJ_VAL(OLC_OBJ(d), 0) = GET_OBJ_VAL(OLC_OBJ(d), 1) =
            GET_OBJ_VAL(OLC_OBJ(d), 2) = GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
            break;

        case OEDIT_EXTRAS:
            number = atoi(arg);
            if ((number < 0) || (number > NUM_ITEM_FLAGS)) {
                oedit_disp_extra_menu(d);
                return;
            }
            else if (number == 0)
                break;
            else {
                TOGGLE_BIT_AR(GET_OBJ_EXTRA(OLC_OBJ(d)), (number - 1));
                oedit_disp_extra_menu(d);
                return;
            }

        case OEDIT_WEAR:
            number = atoi(arg);
            if ((number < 0) || (number > NUM_ITEM_WEARS)) {
                write_to_output(d, "That's not a valid choice!\r\n");
                oedit_disp_wear_menu(d);
                return;
            }
            else if (number == 0)    /* Quit. */
                break;
            else {
                TOGGLE_BIT_AR(GET_OBJ_WEAR(OLC_OBJ(d)), (number - 1));
                oedit_disp_wear_menu(d);
                return;
            }
		
		case OEDIT_COMPOSITION:
			number = atoi(arg);

			if ((number < 0) || (number > NUM_COMPOSITION -1)) {
				write_to_output(d, "That's not a valid choice!\r\n");
				oedit_disp_composition_menu(d);
				return;
			}
			else {
				OLC_OBJ(d)->obj_flags.composition = number;
				OLC_OBJ(d)->obj_flags.subcomposition = 0;
				break;
			}

		case OEDIT_SUBCOMPOSITION:
			number = atoi(arg);
			int subcategory;

			if (OLC_OBJ(d)->obj_flags.composition == 0)
				subcategory = MAX_NOTHING;
			else if (OLC_OBJ(d)->obj_flags.composition == 1)
				subcategory = MAX_TRADEGOODS;
			else if (OLC_OBJ(d)->obj_flags.composition == 2)
				subcategory = MAX_METALS;
			else if (OLC_OBJ(d)->obj_flags.composition == 3)
				subcategory = MAX_PRECIOUSMETALS;
			else if (OLC_OBJ(d)->obj_flags.composition == 4)
				subcategory = MAX_GEMS;
			else if (OLC_OBJ(d)->obj_flags.composition == 5)
				subcategory = MAX_ROCKS;
			else if (OLC_OBJ(d)->obj_flags.composition == 6)
				subcategory = MAX_ORGANICS;
			else if (OLC_OBJ(d)->obj_flags.composition == 7)
				subcategory = MAX_OTHERS;
			else {
				OLC_OBJ(d)->obj_flags.composition = 0;
				subcategory = MAX_NOTHING;
			}

			if ((number < 0) || (number > subcategory -1)) {
				write_to_output(d, "That's not a valid choice!\r\n");
				oedit_disp_subcomposition_menu(d);
				return;
			}
			else {
				OLC_OBJ(d)->obj_flags.subcomposition = number;
				break;
			}

        case OEDIT_WEIGHT:
            GET_OBJ_WEIGHT(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_WEIGHT);
            break;

        case OEDIT_COST:
            GET_OBJ_COST(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_COST);
            break;

        case OEDIT_COSTPERDAY:
            GET_OBJ_RENT(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_RENT);
            break;

        case OEDIT_TIMER:
            GET_OBJ_TIMER(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, MAX_OBJ_TIMER);
            break;

        case OEDIT_LEVEL:
            GET_OBJ_LEVEL(OLC_OBJ(d)) = LIMIT(atoi(arg), 0, LVL_IMPL);
            break;

        case OEDIT_VALUE_1:
            number = atoi(arg);

            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
                case ITEM_FURNITURE:
                    if (number < 0 || number > MAX_PEOPLE)
                        oedit_disp_val1_menu(d);
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_WEAPON:
					if (number < 0 || number >= (MAX_DMG))
						oedit_disp_wpndam_menu(d);
					else
						GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
					break;

                case ITEM_WEAPONUPG:
                    GET_OBJ_VAL(OLC_OBJ(d), 0) = MIN(MAX(atoi(arg), -1000), 1000);
                    break;

                case ITEM_CONTAINER:
                case ITEM_TABLE:
                    GET_OBJ_VAL(OLC_OBJ(d), 0) = LIMIT(atoi(arg), -1, MAX_CONTAINER_SIZE);
                    break;

                case ITEM_TRAP:
                    if (number < 0 || number >= NUM_PSIONICS)
                        oedit_disp_val1_menu(d);
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_GUN:
                case ITEM_AMMO:
                    if (number < 0 || number > NUM_GUN_TYPES)
                        oedit_disp_gun_menu(d);
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_EXPLOSIVE:
                    if (number < 0 || number > NUM_EXPLOSIVE_TYPES)
                        oedit_disp_val1_menu(d);
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_EXPANSION:
                    if (number < -100 || number > 250) {
                        write_to_output(d, "Try again (%d to %d) : ", -100, 250);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_SCROLL:
                case ITEM_WAND:
                case ITEM_STAFF:
                case ITEM_POTION:
                    if (number < 1 || number > 100) {
                        write_to_output(d, "Try again (%d to %d) : ", 1, 100);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    break;

                case ITEM_BIONIC_DEVICE:
                    if (number > 0 && number < NUM_BODY_PARTS)
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
                    else {
                        write_to_output(d, "Try again : ");
                        return;
                    }
                    break;
				case ITEM_MEDICAL:
					if (number < 0 || number > 5) {
						write_to_output(d, "Try again (1 to 5) : ");
						return;
					}
					else GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
					break;
				case ITEM_VEHICLE:
					if (number < 0 || number > MAX_VEHICLES -1) {
						write_to_output(d, "Try again (1 to %d) : ", MAX_VEHICLES -1);
						return;
					}
					else GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
					break;
                default:
                    if (number < 0 || number > 100000000) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 10000);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
            }

            // proceed to menu 2
            oedit_disp_val2_menu(d);
            return;

        case OEDIT_VALUE_2:

            // Here, I do need to check for out of range values
            number = atoi(arg);

            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (number == 0 || number == -1)
                        GET_OBJ_VAL(OLC_OBJ(d), 1) = -1;
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, NUM_PSIONICS);

                    oedit_disp_val3_menu(d);
                    break;

                case ITEM_CONTAINER:
                    // Needs some special handling since we are dealing with flag values here
                    if (number < 0 || number > 4)
                        oedit_disp_container_flags_menu(d);
                    else if (number != 0) {
                        TOGGLE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), 1 << (number - 1));
                        OLC_VAL(d) = 1;
                        oedit_disp_val2_menu(d);
                    }
                    else
                        oedit_disp_val3_menu(d);
                    break;

                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                  GET_OBJ_VAL(OLC_OBJ(d), 1) = LIMIT(number, 1, MAX_WEAPON_NDICE);
                  oedit_disp_val3_menu(d);
                  break;

                case ITEM_GUN:
                    if (number < 0 || number > 1000) {
                        write_to_output(d, "Try again (%d to %d) : ", 1, 1000);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 1) =  number;
                    break;

                case ITEM_AMMO:
                    if (number < 0 || number > 1000) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 1000);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 1) =  number;
                    break;

                case ITEM_BIONIC_DEVICE:
                    if (number < 0 || number >= NUM_BODY_PARTS) {
						write_to_output(d, "Try again (%d to %d) : ", 0, NUM_BODY_PARTS);
						return;
					}
					else
						GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
                    break;

                case ITEM_EXPLOSIVE:
                    if (number < 0 || number > NUM_GRENADE_TYPES) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, NUM_GRENADE_TYPES);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 1) =  number;
                    break;

                case ITEM_EXPANSION:
                    if (number < -100 || number > 250) {
                        write_to_output(d, "Try again (%d to %d) : ", -100, 250);
                        return;
                    }
                    else
                        GET_OBJ_VAL(OLC_OBJ(d), 1) =  number;
                    break;
				case ITEM_VEHICLE:
					if (number < 0 || number > MAX_VEHICLE_TYPES -1) {
						write_to_output(d, "Try again (1 to %d) : ", MAX_VEHICLE_TYPES -1);
						return;
					}
					else GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
					break;
                default:
                    if (number < 0 || number > 10000) {
                        write_to_output(d, "Try again (%d to %d) : ", 0, 10000);
                        return;
                    }
                    else
                       GET_OBJ_VAL(OLC_OBJ(d), 1) =  number;
                    break;
            }

            oedit_disp_val3_menu(d);
            return;

        case OEDIT_VALUE_3:
            number = atoi(arg);
            // Quick'n'easy error checking
            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (number == 0 || number == -1) {
                        GET_OBJ_VAL(OLC_OBJ(d), 2) = -1;
                        oedit_disp_val4_menu(d);
                        return;
                    }

                    min_val = 1;
                    max_val = NUM_PSIONICS;
                    break;

                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                    min_val = 1;
                    max_val = MAX_WEAPON_SDICE;
                    break;

                case ITEM_WAND:
                case ITEM_STAFF:
                    min_val = 0;
                    max_val = 100;
                    break;

                case ITEM_DRINKCON:
                case ITEM_FOUNTAIN:
                    min_val = 0;
                    max_val = NUM_LIQ_TYPES - 1;
                    break;

                case ITEM_GUN:
                    min_val = 0;
                    max_val = 1000;
                    break;

                case ITEM_AMMO:
					min_val = 0;
					max_val = (MAX_DMG -1);
					break;

                case ITEM_EXPLOSIVE:
                    min_val = 1;
                    max_val = 5000;
                    break;

                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;
                case ITEM_BIONIC_DEVICE:
                        min_val = 0;
                        max_val = (MAX_BIO_TYPES -1);
                    break;
				case ITEM_VEHICLE:
					min_val = 1;
					max_val = 65000;
                default:
                    min_val = -65000;
                    max_val = 65000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(OLC_OBJ(d), 2) = number;
                oedit_disp_val4_menu(d);
            }
            return;

        case OEDIT_VALUE_4:
            number = atoi(arg);
            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (number == 0 || number == -1) {
                        GET_OBJ_VAL(OLC_OBJ(d), 3) = -1;
                        oedit_disp_val5_menu(d);
                        return;
                    }
                    min_val = 1;
                    max_val = NUM_PSIONICS;
                    break;

                case ITEM_WAND:
                case ITEM_STAFF:
                    min_val = 1;
                    max_val = NUM_PSIONICS;
                    break;

                case ITEM_WEAPON:
                case ITEM_WEAPONUPG:
                    min_val = 0;
                    max_val = NUM_ATTACK_TYPES - 1;
                    break;

                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;

                case ITEM_EXPLOSIVE:
                    min_val = 1;
                    max_val = 5000;
                    break;

                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;
                case ITEM_BIONIC_DEVICE:
                        min_val = 0;
                        max_val = (MAX_BIO_TYPES -1);
                    break;
				case ITEM_VEHICLE:
					min_val = 1;
					max_val = 30000;
                default:
                    min_val = -65000;
                    max_val = 65000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(OLC_OBJ(d), 3) = number;
                oedit_disp_val5_menu(d);
            }
            return;

        case OEDIT_VALUE_5:
            OLC_VAL(d) = 1;
            number = atoi(arg);
            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {

                case ITEM_SCROLL:
                case ITEM_POTION:
                    if (number == 0 || number == -1) {
                        GET_OBJ_VAL(OLC_OBJ(d), 4) = -1;
                        oedit_disp_menu(d);
                        return;
                    }
                    min_val = 1;
                    max_val = NUM_PSIONICS;
                    break;

                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;

                case ITEM_EXPLOSIVE:
                    min_val = 0;
                    max_val = 60;
                    break;

                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;

                case ITEM_KEY:
                    min_val = 0;
                    max_val = 65099;
                    break;

                case ITEM_BIONIC_DEVICE:
                        min_val = 0;
                        max_val = (MAX_BIO_TYPES -1);
                    break;

				case ITEM_VEHICLE:
					min_val = 1;
					max_val = 10000;

                default:
                    min_val = 0;
                    max_val = 10000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(OLC_OBJ(d), 4) = number;
                oedit_disp_val6_menu(d);
            }
            return;

        case OEDIT_VALUE_6:
            OLC_VAL(d) = 1;
            number = atoi(arg);
            switch (GET_OBJ_TYPE(OLC_OBJ(d))) {

                case ITEM_GUN:
                    min_val = 1;
                    max_val = 1000;
                    break;

                case ITEM_EXPANSION:
                    min_val = -100;
                    max_val = 250;
                    break;

                case ITEM_BIONIC_DEVICE:
                        min_val = 0;
                        max_val = (MAX_BIO_TYPES -1);
                    break;

				case ITEM_VEHICLE:
					min_val = 1;
					max_val = MAX_GPS_LOCATIONS;

                default:
                    min_val = 0;
                    max_val = 10000;
                    break;
            }

            if (number < min_val || number > max_val)
                write_to_output(d, "Try again (%d to %d) : ", min_val, max_val);
            else {
                GET_OBJ_VAL(OLC_OBJ(d), 5) = number;
                oedit_disp_menu(d);
            }
            return;

        case OEDIT_PROMPT_APPLY:
            if ((number = atoi(arg)) == 0)
                break;
            else if (number < 0 || number > MAX_OBJ_APPLY) {
                oedit_disp_prompt_apply_menu(d);
                return;
            }

            OLC_VAL(d) = number - 1;
            oedit_disp_apply_menu(d);
            return;

        case OEDIT_APPLY:
            if ((number = atoi(arg)) == 0) {
                GET_OBJ_APPLY_LOC(OLC_OBJ(d), OLC_VAL(d)) = 0;
                GET_OBJ_APPLY_MOD(OLC_OBJ(d), OLC_VAL(d)) = 0;
                oedit_disp_prompt_apply_menu(d);
            }
            else if (number < 0 || number >= NUM_APPLIES)
                oedit_disp_apply_menu(d);
            else {

                GET_OBJ_APPLY_LOC(OLC_OBJ(d), OLC_VAL(d)) = number;

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
                    OLC_MODE(d) = OEDIT_APPLYMOD;
                }
                else {
                    write_to_output(d, "Modifier : ");
                    OLC_MODE(d) = OEDIT_APPLYMOD;
                }
            }
            return;

        case OEDIT_APPLYMOD:
            number = atoi(arg);
            if (GET_OBJ_APPLY_LOC(OLC_OBJ(d), OLC_VAL(d)) == APPLY_SKILL) {

                if (number == 0)    /* Quit. */
                    break;

                if (number < 1 || number > TOP_SKILL_DEFINE || !strcmp("!UNUSED!", skills_info[number].name)) {
                    write_to_output(d, "Unrecognized skill.\r\n");
                    return;
                }
            }

            GET_OBJ_APPLY_MOD(OLC_OBJ(d), OLC_VAL(d)) = number;
            oedit_disp_prompt_apply_menu(d);
            return;

        case OEDIT_EXTRADESC_KEY:
            if (genolc_checkstring(d, arg)) {
                if (OLC_DESC(d)->keyword)
                    free(OLC_DESC(d)->keyword);
                OLC_DESC(d)->keyword = str_udup(arg);
            }
            oedit_disp_extradesc_menu(d);
            return;

        case OEDIT_EXTRADESC_MENU:
            switch ((number = atoi(arg))) {

                case 0:
                    if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
                        struct extra_descr_data *temp;

                        if (OLC_DESC(d)->keyword)
                            free(OLC_DESC(d)->keyword);
                        if (OLC_DESC(d)->description)
                            free(OLC_DESC(d)->description);

                        // Clean up pointers
                        REMOVE_FROM_LIST(OLC_DESC(d), OLC_OBJ(d)->ex_description, next);
                        free(OLC_DESC(d));
                        OLC_DESC(d) = NULL;
                    }
                    break;

                case 1:
                    OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
                    write_to_output(d, "Enter keywords, separated by spaces :-\r\n| ");
                    return;

                case 2:
                    OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
                    send_editor_help(d);
                    write_to_output(d, "Enter the extra description:\r\n\r\n");
                    if (OLC_DESC(d)->description) {
                        write_to_output(d, "%s", OLC_DESC(d)->description);
                        oldtext = strdup(OLC_DESC(d)->description);
                    }
                    string_write(d, &OLC_DESC(d)->description, MAX_MESSAGE_LENGTH, 0, oldtext);
                    OLC_VAL(d) = 1;
                    return;

                case 3:
                    // Only go to the next description if this one is finished
                    if (OLC_DESC(d)->keyword && OLC_DESC(d)->description) {
                        struct extra_descr_data *new_extra;

                        if (OLC_DESC(d)->next)
                            OLC_DESC(d) = OLC_DESC(d)->next;
                        else {
                            // Make new extra description and attach at end
                            CREATE(new_extra, struct extra_descr_data, 1);
                            OLC_DESC(d)->next = new_extra;
                            OLC_DESC(d) = OLC_DESC(d)->next;
                        }
                    }
                    // No break - drop into default case

                default:
                    oedit_disp_extradesc_menu(d);
                    return;
            }
            break;

        case OEDIT_WPNPSIONIC:
            if ((number = atoi(arg)) == 0) {
                OLC_OBJ(d)->obj_wpnpsi.which_psionic = 0;
                OLC_OBJ(d)->obj_wpnpsi.skill_level = 0;
                break;
            }
            else if (number < 0 || number > NUM_WPNPSIONICS) {
                write_to_output(d, "Invalid psionic, try again: ");
                return;
            }
            OLC_VAL(d) = TRUE;
            OLC_OBJ(d)->obj_wpnpsi.which_psionic = number;
            write_to_output(d, "Skill Level: ");
            OLC_MODE(d) = OEDIT_WPNPSIONIC_SKILL;
            return;

        case OEDIT_WPNPSIONIC_SKILL:
            number = atoi(arg);
            if (number < 0 || number > LVL_IMPL + 65) {
                write_to_output(d, "Invalid level, try again: ");
                return;
            }
            OLC_OBJ(d)->obj_wpnpsi.skill_level = number;
            break;

        case OEDIT_AFF:
            number = atoi(arg);
            if (number < 0 || number >= NUM_AFF_FLAGS) {
                write_to_output(d, "Invalid selection.  Enter affect type (0 is no affect and exits) :");
                return;
            }
			else if (number == APPLY_SAVING_NORMAL) {
				write_to_output(d, "Don't do that, please.\r\n");
				return;
			}
            else if (number == 0)
                // go back to the main menu
                break;
            else if (OBJAFF_FLAGGED(OLC_OBJ(d), number)) {
                REMOVE_BIT_AR(GET_OBJ_AFFECT(OLC_OBJ(d)), number);
                oedit_disp_aff_menu(d);
                return;
            }
            else {

                int count = 0;
                int a = 0;
                // do we have too many affects on this object?
                for (a = 0; a < NUM_AFF_FLAGS; a++)
                    if (OBJAFF_FLAGGED(OLC_OBJ(d), a))
                        count++;

                if (count > MAX_OBJ_AFFECT) {
                    write_to_output(d, "Too many object affects.  Max is: %d.  Remove an affect before you add one.", MAX_OBJ_AFFECT);
                    write_to_output(d, "Enter affect type (0 is no affect and exits) :");
                    return;
                }

                SET_BIT_AR(GET_OBJ_AFFECT(OLC_OBJ(d)), number);
                oedit_disp_aff_menu(d);
                return;
            }
            break;

        case OEDIT_REMORT:
            GET_OBJ_REMORT(OLC_OBJ(d)) = atoi(arg);
            break;
		
		case OEDIT_AMMODAM:
			GET_OBJ_VAL(OLC_OBJ(d), 2) = atoi(arg);
			break;

		case OEDIT_WPNDAM:
			GET_OBJ_VAL(OLC_OBJ(d), 0) = atoi(arg);
			break;

        case OEDIT_DRUGTYPE:
            if ((number = atoi(arg)) == 0) {
                OLC_VAL(d) = 1;
                GET_DAFFECT(OLC_OBJ(d)) = 0;
                oedit_disp_val2_menu(d);
                return;
            }
            else if (number < 0 || number > MAX_DRUGS) {
                write_to_output(d, "Invalid drug type, try again: ");
                return;
            }
            OLC_VAL(d) = 1;
            GET_DAFFECT(OLC_OBJ(d)) = number;
            oedit_disp_val2_menu(d);
            return;

        case OEDIT_COPY:
            if ((number = real_object(atoi(arg))) != NOTHING)
                oedit_setup_existing(d, number);
            else
                write_to_output(d, "That object does not exist.\r\n");
            break;

        case OEDIT_DELETE:
            if (*arg == 'y' || *arg == 'Y') {

                if (delete_object(GET_OBJ_RNUM(OLC_OBJ(d))) != NOTHING)
                    write_to_output(d, "Object deleted.\r\n");
                else
                    write_to_output(d, "Couldn't delete the object!\r\n");

                cleanup_olc(d, CLEANUP_ALL);
            }
            else if (*arg == 'n' || *arg == 'N') {
                oedit_disp_menu(d);
                OLC_MODE(d) = OEDIT_MAIN_MENU;
            }
            else
                write_to_output(d, "Please answer 'Y' or 'N': ");
            return;

        default:
            mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: Reached default case in oedit_parse()!");
            write_to_output(d, "Oops...\r\n");
            break;
    }

    // If we get here, we have changed something
    OLC_VAL(d) = 1;
    oedit_disp_menu(d);
}

void oedit_string_cleanup(struct descriptor_data *d, int terminator)
{
    switch (OLC_MODE(d)) {

        case OEDIT_ACTDESC:
            oedit_disp_menu(d);
            break;

        case OEDIT_EXTRADESC_DESCRIPTION:
            oedit_disp_extradesc_menu(d);
            break;
    }
}
