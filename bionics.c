/* ************************************************************************
*   File: bionics.c                              Part of After the Exodus *
*  Usage: bionic and body part handling routines                          *
*                                                                         *
*  All rights reserved.                                                   *
*                                                                         *
*  Copyright (C) 2002 Brian J. Taylor                                     *
*  After the Exodus is based on CircleMUD, Copyright (C) 1993             *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

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
#include "screen.h"
#include "fight.h"
#include "bionics.h"

// external functions
//int damage_object(struct obj_data *obj, int damage);
//bool skill_success(struct char_data *ch, int skill_percent, int min, int max);

const struct bionic_info_type bionic_info[] = {
{"interface", "Bio-Neural Interface", BIO_INTERFACE, {-1, -1, -1}, BIONIC_BASIC, 50000, 150, 0, "We can implant that into your brain with no problem.\n\r"},
{"chassis",	"Bionic Chassis",            BIO_CHASSIS,            {BIO_INTERFACE, -1, -1},            BIONIC_BASIC,            100000, 50, 0,            "Sure, no problem.\n\r"        },
{"spine",            "Reinforced Spine",            BIO_SPINE,            {BIO_CHASSIS, -1, -1},            BIONIC_BASIC,            300000, 150, 0,            "Sure, no problem.\n\r"        },
{"shoulder",            "Shoulder Assembly",            BIO_SHOULDER,            {BIO_SPINE, -1, -1},            BIONIC_BASIC,            15000, 15, 0,            "Some little technician replaces your shoulder with a metal thing.\n\r"        },
{"ribcage",            "Protective Rib Cage",            BIO_RIBCAGE,            {BIO_SPINE, -1, -1},            BIONIC_BASIC,            100000, 25, 0,            "Okay...no problem!.\n\r"        },
{"hip",            "Hip Actuator",            BIO_HIP,            {BIO_SPINE, -1, -1},            BIONIC_BASIC,            15000, 10, 0,            "You feel less human.\n\r"        },
{"arms",           "Bionic Arms",            BIO_ARMS,            {BIO_SHOULDER, -1, -1},            BIONIC_BASIC,            200000, 10, 0,            "Your arms are now bionic.\n\r"        },
{"neck",            "Neck Assembly",            BIO_NECK,            {BIO_SPINE, -1, -1},            BIONIC_BASIC,            10000, 5, 0,            "You now have an electronic neck.\n\r"        },
{"lung",            "Bionic Lung",            BIO_LUNG,            {BIO_RIBCAGE, -1, -1},            BIONIC_BASIC,            300000, 10, 0,            "You now have Bionic Lungs.\n\r"        },
{"legs",            "Bionic Legs",            BIO_LEGS,            {BIO_HIP, -1, -1},            BIONIC_BASIC,            300000, 10, 0,            "You now have some really bitchin legs.\n\r"        },
{"skull",            "Cybernetic Skull",            BIO_SKULL,            {BIO_NECK, -1, -1},            BIONIC_BASIC,            500000, 10, 0,            "Your skull is now Cybernetic.\n\r"        },
{"core",            "Fusion Power Core",            BIO_CORE,            {BIO_LUNG, BIO_LEGS, -1},            BIONIC_BASIC,            1000000, 50, 10,            "You now have a Fusion Reactor implanted in your chest.\n\r"        },
{"blaster",            "Forearm Blaster",            BIO_BLASTER,            {BIO_ARMS, -1, -1},            BIONIC_BASIC,            100000, 0, 10,            "You now have a Forearm Blaster implanted.\n\r"        },
{"blades",            "Vibro-Blades",            BIO_BLADES,            {BIO_ARMS, -1, -1},            BIONIC_BASIC,            30000, 0, 10,            "Okay...You now have a Vibro-Blade implanted in your forearm.\n\r"        },
{"exarms",            "Extra Arms",            BIO_EXARMS,            {BIO_ARMS, BIO_CORE, -1},            BIONIC_BASIC,            1000000, 0, 10,            "You now have a set of Extra Arms.  Enjoy them.\n\r"        },
{"eyes",            "Multi-Optic Eyes",            BIO_EYE,            {BIO_SKULL, -1, -1},            BIONIC_BASIC,            200000, 15, 0,            "You now have a set of Multi-Optic Eyes.\n\r"        },
{"headjack",            "Headjack",            BIO_JACK,            {BIO_SKULL, -1, -1},            BIONIC_BASIC,            100000, 0, 0,            "You now have a Headjack. Thank you, drive through.\n\r"        },
{"voice",            "Voice Synthesizer",            BIO_VOICE,            {BIO_SKULL, -1, -1},            BIONIC_BASIC,            20000, 5, 0,            "You now have a synthesizer built into your voice.\n\r"        },
{"armor",            "Cyber Armor",            BIO_ARMOR,            {BIO_ARMS, BIO_SKULL, BIO_CORE},            BIONIC_BASIC,            5000000, 0, 15,            "Okay...You now have strong Cyber Armor protecting you.\n\r"        },
{"jet",            "Jet Pack",            BIO_JET,            {BIO_ARMOR, -1, -1},            BIONIC_BASIC,            3000000, 0, 20,            "A technician attaches a large jet pack to your back.\n\r"        },
{"destruct",            "Self-Destruct",            BIO_DESTRUCT,            {BIO_ARMOR, -1, -1},            BIONIC_BASIC,            1000000, 0, 20,            "A technician attaches a small device to your fore-arm.\n\r"        },
{"chipjack",            "Chipjack",            BIO_CHIPJACK,            {-1, -1, -1},            BIONIC_BASIC,            1000000, 30, 10,            "A technician inserts a small slot into the base of your skull.\r\n"        },
{            "\n", "\n", -1, {-1, -1, -1}, -1, -1, -1, -1, "\n"        }  // this must always be last
};

const struct bionic_info_type bionic_info_advanced[] = {
{"interface",            "Bio-Neural Interface",            BIO_INTERFACE,            {-1, -1, -1},            BIONIC_ADVANCED,            2500000, 50, 0,           "A technician inserts a chip into your interface.\n\r"        },
{"skull",            "Cybernetic Skull",            BIO_SKULL,            {BIO_INTERFACE, -1, -1},            BIONIC_ADVANCED,            6000000, 0, 0,            "You now have an Advanced Cybernetic Skull.\n\r"        },
{"core",            "Fusion Power Core",            BIO_CORE,            {BIO_INTERFACE, -1, -1},            BIONIC_ADVANCED,            3000000, 30, 10,            "You now have a second Fusion Reactor implanted in your chest.\n\r"        },
{"jet",            "Jet Pack",            BIO_JET,            {BIO_INTERFACE, -1, -1},            BIONIC_ADVANCED,            5000000, 0, 20,            "A technician attaches a large hover device to your back.\n\r"        },
{"\n", "\n", -1, {-1, -1, -1}, -1, -1, -1, -1, "\n"}  // this must always be last
};


// ITEM_BIONIC_DEVICE
// value[0] - bionic location
// value[1] - bionic charisma loss
// value[2] - minimum level to use
// value[3] - bionic level (BASIC, ADVANCED, etc.)
// value[4] - pre-req device (1)
// value[5] - pre-req device (2)

// percentage bionic a part is
int bionic_percent[NUM_BODY_PARTS] = {
  /* reserved   */  0,
  /* head       */  7,
  /* left eye   */  3,
  /* right eye  */  3,
  /* mouth      */  3,
  /* neck       */  3,
  /* left arm   */  7,
  /* right arm  */  7,
  /* left wrist */  3,
  /* right wrist*/  3,
  /* left hand  */  3,
  /* right hand */  3,
  /* left leg   */  7,
  /* right leg  */  7,
  /* left foot  */  4,
  /* right foot */  4,
  /* chest      */  10,
  /* abdomen    */  10,
  /* back       */  5,
  /* internal1  */  2,
  /* internal1  */  2,
  /* internal1  */  2,
  /* internal1  */  2,
};

int bionic_psi_loss[NUM_BODY_PARTS] = {
  /* reserved   */  0,
  /* head       */  100,
  /* left eye   */  20,
  /* right eye  */  20,
  /* mouth      */  10,
  /* neck       */  50,
  /* left arm   */  30,
  /* right arm  */  30,
  /* left wrist */  10,
  /* right wrist*/  10,
  /* left hand  */  15,
  /* right hand */  15,
  /* left leg   */  50,
  /* right leg  */  50,
  /* left foot  */  30,
  /* right foot */  30,
  /* chest      */  100,
  /* abdomen    */  100,
  /* back       */  75,
  /* internal1  */  25,
  /* internal1  */  25,
  /* internal1  */  25,
  /* internal1  */  25,
};

int bionic_psi_regen_loss[NUM_BODY_PARTS] = {
  /* reserved   */  0,
  /* head       */  10,
  /* left eye   */  2,
  /* right eye  */  2,
  /* mouth      */  1,
  /* neck       */  5,
  /* left arm   */  3,
  /* right arm  */  3,
  /* left wrist */  1,
  /* right wrist*/  1,
  /* left hand  */  2,
  /* right hand */  2,
  /* left leg   */  5,
  /* right leg  */  5,
  /* left foot  */  3,
  /* right foot */  3,
  /* chest      */  5,
  /* abdomen    */  5,
  /* back       */  5,
  /* internal1  */  2,
  /* internal1  */  2,
  /* internal1  */  2,
  /* internal1  */  2,
};

// indicates contribution to health for each body part
int body_part_health[NUM_BODY_PARTS] = {
  /* reserved   */  0,
  /* head       */  10,
  /* left eye   */  2,
  /* right eye  */  2,
  /* mouth      */  2,
  /* neck       */  5,
  /* left arm   */  3,
  /* right arm  */  3,
  /* left wrist */  2,
  /* right wrist*/  2,
  /* left hand  */  2,
  /* right hand */  2,
  /* left leg   */  5,
  /* right leg  */  5,
  /* left foot  */  3,
  /* right foot */  3,
  /* chest      */  10,
  /* abdomen    */  10,
  /* back       */  8,
  /* internal1  */  3,
  /* internal1  */  3,
  /* internal1  */  3,
  /* internal1  */  3,
};

void body_part_affects(struct char_data *ch)
{
    int i = 0;

    // for each body part - check for damage
    for (i = 1; i < NUM_BODY_PARTS; i++) {

        // apply loss of health
        if (GET_BODYPART_CONDITION(ch, i, BROKEN))
            BODY_PART_HIT_LOSS(ch) += (int) (0.5 * body_part_health[i] * GET_MAX_HIT_PTS(ch));
        else if (GET_BODYPART_CONDITION(ch, i, RIPPED))
            BODY_PART_HIT_LOSS(ch) += (int) (body_part_health[i] * GET_MAX_HIT_PTS(ch));

    }
}

// do we ever need this function?
void body_part_unaffects(struct char_data *ch)
{
    BODY_PART_HIT_LOSS(ch) = 0;
}

void bionics_apply_affect(struct char_data *ch, struct obj_data *device, bool on, bool ack)
{

    //int j = 0;

    if (!ch || !device)
        return;

    // apply mod to charisma
    if (on)
        BIONICS_CHA_LOSS(ch) += BIONIC_CHARISMA_LOSS(device);
    else
        BIONICS_CHA_LOSS(ch) -= BIONIC_CHARISMA_LOSS(device);

    //  go through and apply bionic location/mods
	if (on) {
		//for (j = 0; j < MAX_OBJ_APPLY; j++) {
		//	affect_modify_ar(ch, GET_OBJ_APPLY_LOC(device, j), GET_OBJ_APPLY_MOD(device, j), GET_OBJ_AFFECT(device), TRUE);
		//	send_to_char(ch, "Adding 1\r\n");
		//}
		// now apply the affects
		//for (j = 0; j < NUM_AFF_FLAGS; j++)
		//	if (OBJAFF_FLAGGED(device, j))
		//		obj_affect_to_char(ch, j, ack, device);
	}
	else {
		//for (j = 0; j < MAX_OBJ_APPLY; j++) {
		//	affect_modify_ar(ch, GET_OBJ_APPLY_LOC(device, j), GET_OBJ_APPLY_MOD(device, j), GET_OBJ_AFFECT(device), FALSE);
		//	send_to_char(ch, "Removing 1\r\n");
		//}

		// now apply the affects
		//for (j = 0; j < NUM_AFF_FLAGS; j++)
		//	if (OBJAFF_FLAGGED(device, j))
		//		obj_affect_from_char(ch, j, ack);
	}

	affect_total(ch);
}

void new_bionics_affect(struct char_data *ch, bool ack)
{
    int i = 0;
    struct obj_data *device = NULL;

    // for each body part - check for bionic
    for (i = 1; i < NUM_BODY_PARTS; i++) {

        // is this bionic?
        if ((device = BIONIC_DEVICE(ch, i))) {

            // apply bionic percent
            BIONIC_PERCENT(ch) += bionic_percent[i];

            // apply loss of psi
            BIONICS_PSI_LOSS(ch) += bionic_psi_loss[i];

            // apply loss of psi regen
            BIONIC_PSI_REGEN_LOSS(ch) += bionic_psi_regen_loss[i];

            // apply affects and location/mods
            bionics_apply_affect(ch, device, TRUE, ack);

        }

    }

}

void new_bionics_unaffect(struct char_data *ch, bool ack)
{
   // int j = 0;
    int i = 0;
    struct obj_data *device = NULL;

    // for each body part - check for bionic
    for (i = 1; i < NUM_BODY_PARTS; i++) {

        // is this bionic?
        if ((device = BIONIC_DEVICE(ch, i))) {

            // remove bionic percent
            BIONIC_PERCENT(ch) -= bionic_percent[BIONIC_LOCATION(device)];

            // remove loss of mana
            BIONICS_PSI_LOSS(ch) -= bionic_psi_loss[BIONIC_LOCATION(device)];

            // remove loss of mana regen
            BIONIC_PSI_REGEN_LOSS(ch) -= bionic_psi_regen_loss[BIONIC_LOCATION(device)];

            // apply affects and location/mods
            bionics_apply_affect(ch, device, FALSE, ack);

        }

    }

}

bool implant_bionic(struct char_data *ch, int location, struct obj_data *device)
{
	int j;
    // check location
    if (location <= 0 || location >= NUM_BODY_PARTS)
        return (FALSE);

    // does this device fit at the location
    if (BIONIC_LOCATION(device) != location)
        return (FALSE);

    // remove bionics affects
    //new_bionics_unaffect(ch, TRUE);

    // install the device
    BIONIC_DEVICE(ch, location) = device;
    device->implanted = ch;

    // heal the location
    BODYPART_CONDITION(ch, location) = 0;
    SET_BODYPART_CONDITION(ch, location, BIONIC_NORMAL);

	BIONIC_PERCENT(ch) += bionic_percent[location];
	BIONICS_PSI_LOSS(ch) += bionic_psi_loss[location];
	BIONIC_PSI_REGEN_LOSS(ch) += bionic_psi_regen_loss[location];
	BIONICS_CHA_LOSS(ch) += BIONIC_CHARISMA_LOSS(device);

	for (j = 0; j < MAX_OBJ_APPLY; j++)
		affect_modify_ar(ch, GET_OBJ_APPLY_LOC(BIONIC_DEVICE(ch, location), j), GET_OBJ_APPLY_MOD(BIONIC_DEVICE(ch, location), j), GET_OBJ_AFFECT(BIONIC_DEVICE(ch, location)), TRUE);
	
	for (j = 0; j < NUM_AFF_FLAGS; j++)
		if (OBJAFF_FLAGGED(BIONIC_DEVICE(ch, location), j))
			obj_affect_to_char(ch, j, TRUE, BIONIC_DEVICE(ch, location));

    // add bionics affect
    //new_bionics_affect(ch, TRUE);

    return (TRUE);
}

bool implant_bionictwo(struct char_data *ch, int location, struct obj_data *device)
{
    // check location
    if (location <= 0 || location >= NUM_BODY_PARTS)
        return (FALSE);

    // does this device fit at the location
    if (BIONIC_LOCATION(device) != location)
        return (FALSE);

	BIONIC_PERCENT(ch) += bionic_percent[location];
    
	// install the device
    BIONIC_DEVICE(ch, location) = device;
    device->implanted = ch;

    // heal the location
    BODYPART_CONDITION(ch, location) = 0;
    SET_BODYPART_CONDITION(ch, location, BIONIC_NORMAL);

    // add bionics affect
    //new_bionics_affect(ch, TRUE);

    return (TRUE);
}

struct obj_data *remove_bionic(struct char_data *ch, int location)
{
	int j;
	//int taeller;
    struct obj_data *device;
    // does this person have a bionic device at the location?
    if (!BIONIC_DEVICE(ch, location))
        return (NULL);

    // remove bionics affects
    //new_bionics_unaffect(ch, FALSE);

	device = BIONIC_DEVICE(ch, location);

    if (device) {
		BIONIC_PERCENT(ch) -= bionic_percent[location];
		BIONICS_PSI_LOSS(ch) -= bionic_psi_loss[location];
		BIONIC_PSI_REGEN_LOSS(ch) -= bionic_psi_regen_loss[location];
		BIONICS_CHA_LOSS(ch) -= BIONIC_CHARISMA_LOSS(device);
	}
	for (j = 0; j < MAX_OBJ_APPLY; j++)
		affect_modify_ar(ch, GET_OBJ_APPLY_LOC(BIONIC_DEVICE(ch, location), j), GET_OBJ_APPLY_MOD(BIONIC_DEVICE(ch, location), j), GET_OBJ_AFFECT(BIONIC_DEVICE(ch, location)), FALSE);
	
	for (j = 0; j < NUM_AFF_FLAGS; j++)
		if (OBJAFF_FLAGGED(BIONIC_DEVICE(ch, location), j)) {
			obj_affect_from_char(ch, j, FALSE);
			if (IS_SET_AR(AFF_FLAGS(ch), j))
				REMOVE_BIT_AR(AFF_FLAGS(ch), j);
		}

    // remove the bionic device
    device->implanted = NULL;

    BIONIC_DEVICE(ch, location) = NULL;
	BODYPART_CONDITION(ch, location) = 0;
    SET_BODYPART_CONDITION(ch, location, NORMAL);
	affect_total(ch);
	//new_bionics_affect(ch, TRUE);
    return (device);
}

//void repair_bionic(struct char_data *ch, struct char_data *patient, struct obj_data *device)
//{
//  int skill_percent = GET_SKILL_PERCENT(ch, SKILL_BIO_MECHANICS);
//  int points;
//  int energy = 5;
//  int location = NOWHERE;
//  int dam_percent;
//
//  /* does the char have the skill? */
//  if (skill_percent <= 0) {
//    send_to_char(ch, "Huh?!?\r\n");
//    return;
//  }
//
//  /* is the bionic device even damaged? */
//  if (GET_OBJ_DAM(device) == GET_OBJ_MAX_DAM(device)) {
//    send_to_char(ch, "That device is in excellent condition.\r\n");
//    return;
//  }
//
//  /* remove energy */
//  GET_ENERGY(ch) -= energy;
//
//  points = (skill_percent/20) * 3 + 5 + random_number(1, 10);
//
//  if (patient)
//    location = BIONIC_LOCATION(device);
//
//  /* check for failure */
//  if (!skill_success(ch, skill_percent, 1, 101)) {
//
//    /* failure - check for possible "problems" */
//    if (random_number(1, 5) == 5) {
//
//      /* do more damage */
//      points = random_number(1, 10);
//
//      GET_OBJ_DAM(device) += points;
//
//      /* generate messages */
//      if (patient) {
//        if (ch == patient)
//          act("Hmm, that didn't appear to work!", TRUE, ch, 0, 0, TO_CHAR);
//        else {
//          act("A piece of your $p breaks off in $N's hands.", TRUE, ch, device, patient, TO_VICT);
//          act("You break off a component from $n's $p.", TRUE, ch, device, patient, TO_CHAR);
//        }
//
//        /* did this critically damage the device? */
//        dam_percent = (GET_OBJ_DAM(device) * 100)/GET_OBJ_MAX_DAM(device);
//
//        if (dam_percent < 50)
//          SET_BODYPART_CONDITION(patient, location, BIONIC_DAMAGED);
//        else if (dam_percent < 75)
//          SET_BODYPART_CONDITION(patient, location, BIONIC_BROKEN);
//        else if (dam_percent < 90)
//          SET_BODYPART_CONDITION(patient, location, BIONIC_BUSTED);
//        else {
//          SET_BODYPART_CONDITION(patient, location, BIONIC_MISSING);
//          BIONIC_DEVICE(patient, location) = NULL;
//          extract_obj(device);
//        }
//      }
//      else {
//        /* there was no patient */
//        act("You seem to make $p worse.", TRUE, ch, device, 0, TO_CHAR);
//      }
//    }
//    else
//      send_to_char(ch, "You fail.\r\n");
//
//    return;
//  }
//
//  /* success */
//  GET_OBJ_DAM(device) = MAX(GET_OBJ_MAX_DAM(device), GET_OBJ_DAM(device) - points);
//
//  /* generate messages */
//  if (patient) {
//    if (ch == patient)
//      act("You have repaired your $p.", TRUE, ch, device, 0, TO_CHAR);
//    else {
//      act("$N repairs your $p.", TRUE, ch, device, patient, TO_VICT);
//      act("You repair $n's $p.", TRUE, ch, device, patient, TO_CHAR);
//    }
//
//    /* did this remove a critical damage to the device? */
//    dam_percent = (GET_OBJ_DAM(device) * 100)/GET_OBJ_MAX_DAM(device);
//
//    if (dam_percent == 0)
//      SET_BODYPART_CONDITION(patient, location, BIONIC_NORMAL);
//    if (dam_percent < 50)
//      SET_BODYPART_CONDITION(patient, location, BIONIC_DAMAGED);
//    else if (dam_percent < 75)
//      SET_BODYPART_CONDITION(patient, location, BIONIC_BROKEN);
//    else {
//      /* do nothing */
//    }
//  }
//  else {
//    /* the character is simply fixing a bionic device */
//    act("You have repaired $p.", TRUE, ch, device, 0, TO_CHAR);
//  }
//}


void damage_body_part(struct char_data *ch, int location, int hit)
{

    // increase the number of hits
    GET_NO_HITS(ch, location) += hit;

    // does this change the part status?

    // does the part need to be removed?
}

// there is no check to require that the player has a knife or blade */
void salvagebionic(struct char_data *ch, struct obj_data *obj)
{
    bool found = FALSE;
    struct bionic_data *curr = NULL;
    struct bionic_data *prev = NULL;

    // does the char have the skill?
    //if (skill_percent <= 0) {
    //    send_to_char(ch, "Huh?!?\r\n");
    //    return;
    //}

    // is there a bionic device in the corpse which installs at location? */
	if (obj->bionics) {
        prev = obj->bionics;
        for (curr = obj->bionics; curr; prev = curr, curr = curr->next) {
            if (BIONIC_LOCATION(curr->device)) {
                found = TRUE;
                break;
            }
        }
	}
    
    // was there a match?
    if (!found) {
        send_to_char(ch, "Unable to find anything.\r\n");
        return;
    }
	else {
		if (prev != curr)
			prev->next = curr->next;
		else
			if (prev->next == NULL)
				obj->bionics = NULL;
			else
				obj->bionics = curr->next;

		curr->device->in_obj = NULL;

		if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch) || ((IS_CARRYING_TOT(ch) + GET_OBJ_WEIGHT(curr->device)) > CAN_CARRY_W(ch)) ||
			(!(CAN_WEAR(curr->device, ITEM_WEAR_TAKE))))
			obj_to_room(curr->device, IN_ROOM(ch));
		else
			obj_to_char(curr->device, ch);

		curr->device = NULL;
		free(curr);
	    // generate messages
		act("$n salvages a bionic device from $p.", TRUE, ch, obj, 0, TO_ROOM);
		act("You salvage a bionic device from $p.", TRUE, ch, obj, 0, TO_CHAR);
		extract_obj(obj);
	}

//    // check for failure
//    if (!skill_success(ch, skill_percent, 1, 101)) {
//
//        // failure! - damage bionic device and leave it in obj
//        // the lower the skill percent, the more damage can be done
//        damage_object(curr->device, ((random_number(40, 80) + 100 - skill_percent)/200 * GET_OBJ_MAX_DAM(curr->device)));
//
//        // remove the bionic device if we need to
//        if (GET_OBJ_DAM(curr->device) > GET_OBJ_MAX_DAM(curr->device)) {
//
//            if (prev != curr)
//                prev->next = curr->next;
//            else
//                if (prev->next == NULL)
//                    corpse->bionics = NULL;
//                else
//                    corpse->bionics = curr->next;
//
//            extract_obj(curr->device);
//            free(curr);
//        }
//
//        // generate messages
//        act("$n butchers $p as $e tries to extract a bionic device.", TRUE, ch, corpse, 0, TO_ROOM);
//        act("You butcher $p as you try to extract the bionic device.", TRUE, ch, corpse, 0, TO_CHAR);
//
//    }
}

int bionics_delete(char *name)
{
    char filename[MAX_STRING_LENGTH];
    FILE *fp;

    if (!get_filename(filename, sizeof(filename), BIONICS_FILE, name))
        return (FALSE);

    if (!(fp = fopen(filename, "r"))) {

        // if it fails but NOT because of no file
        if (errno != ENOENT)
            log("SYSERR: deleting bionics file %s (1):%s", filename, strerror(errno));
        return (FALSE);

    }

    fclose(fp);

    // if it fails, NOT because of no file
    if (remove(filename) < 0 && errno != ENOENT)
        log("SYSERR: deleting bionics file %s - %s", filename, strerror(errno));

  return (TRUE);

}

// save bionics works much like saving equipment
int bionics_save(struct char_data *ch)
{
    int i = 0;
    int result = 1;
    struct obj_data *device = NULL;
    FILE *fp = NULL;
    char filename[MAX_STRING_LENGTH];

    // does the player have any bionics?
    //if (BIONIC_PERCENT(ch) <= 0) {
    //    bionics_delete(GET_NAME(ch));
    //    return (TRUE);
    //}

    // get bionics file name */
    if (!get_filename(filename, sizeof(filename), BIONICS_FILE, GET_NAME(ch)))
        return (FALSE);

    // open up bionics file
    if (!(fp = fopen(filename, "w")))
        return (FALSE);

    // cycle through player bionics
    for (i = 1; i < NUM_BODY_PARTS; i++) {

        // is there a bionic device?
        if ((device = BIONIC_DEVICE(ch, i))) {

            // store bionic device information
            // -- assumes device contains nothing
            result = objsave_save_obj_record(device, fp, 0);

            if (!result) {
                log("ERROR: bionics_save - unable to save %s's bionics file on device %s.", GET_NAME(ch), device->short_description);
                break;
            }

        }

    }

    // close bionics file
    fprintf(fp, "$~\n");
    fclose(fp);
    return (TRUE);
}

void bionics_restore(struct char_data *ch)
{
    char filename[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    FILE *fp = NULL;
    int location = -1;
    int num_bionics = 0;
    int num_objects = 0;
    bool result;
    obj_save_data *loaded;
    obj_save_data *current;


    // get bionics file name
    if (!get_filename(filename, sizeof(filename), BIONICS_FILE, GET_NAME(ch)))
        return;

    // open up bionics file
    if (!(fp = fopen(filename, "r"))) {

        if (errno != ENOENT) { /* if it fails, NOT because of no file */
            sprintf(buf, "SYSERR: READING BIONICS FILE %s (5)", filename);
            perror(buf);
            send_to_char(ch, "\r\n********************* NOTICE *********************\r\n"
                "There was a problem loading your bionics from disk.\r\n"
                "Contact a God for assistance.\r\n");
        }

        mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game with no bionics.", GET_NAME(ch));
        return;
    }

    loaded = objsave_parse_objects(fp);
    for (current = loaded; current != NULL; current=current->next) {

        num_objects++;

        // verify this is a bionic device
        if (GET_OBJ_TYPE(current->obj) != ITEM_BIONIC_DEVICE) {
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "WARNING: object (%s) saved in bionics file is not a bionic device (%s).  Placed in %s's inventory.", current->obj->short_description, item_types[(int) GET_OBJ_TYPE(current->obj)], GET_NAME(ch));
            send_to_char(ch, "Found a non-bionic object: %s.  Device placed in inventory.", current->obj->short_description);
            obj_to_char(current->obj, ch);
            continue;
        }

        // where does this bionic install?
        location = BIONIC_LOCATION(current->obj);
        if (location <= 0 || location >= NUM_BODY_PARTS) {
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "WARNING: device (%s) saved in bionics file has an invalid install location (%d).  Placed in %s's inventory.", current->obj->short_description, location, GET_NAME(ch));
            send_to_char(ch, "Unable to install device: %s.  Device placed in inventory.", current->obj->short_description);
            obj_to_char(current->obj, ch);
            continue;
        }

        // does the player have the appropriate body part condition
        if (!GET_BODYPART_CONDITION(ch, location, BIONIC_NORMAL) && !GET_BODYPART_CONDITION(ch, location, BIONIC_DAMAGED) &&
            !GET_BODYPART_CONDITION(ch, location, BIONIC_BROKEN) && !GET_BODYPART_CONDITION(ch, location, BIONIC_BUSTED)) {

            char buf[MAX_STRING_LENGTH];
            sprintbit(BODYPART_CONDITION(ch, location), part_condition, buf, sizeof(buf));

            // no.  drop the bionic into their inventory
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "WARNING: conflicting body part (%d) status (%s) for device (%s).  Placed in %s's inventory.", location, buf, current->obj->short_description, GET_NAME(ch));
            send_to_char(ch, "Unable to install device: %s.  Body part condition is incompatible.  Device placed in inventory.", current->obj->short_description);
            obj_to_char(current->obj, ch);
            continue;
        }

        // install the bionic
        if (!(result = implant_bionictwo(ch, location, current->obj))) {
            mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "WARNING: device (%s) saved in bionics file failed to implant.  Placed in %s's inventory.", current->obj->short_description, GET_NAME(ch));
            send_to_char(ch, "Unable to install device: %s.  Device placed in inventory.", current->obj->short_description);
            obj_to_char(current->obj, ch);
            continue;
        }

        num_bionics++;

    }

    // now it's safe to free the obj_save_data list - all members of it
    // have been put in the correct lists by handle_obj() */
    while (loaded != NULL) {
        current = loaded;
        loaded = loaded->next;
        free(current);
    }

    if (num_bionics != num_objects)
        mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "ERROR: bionics_restore - %s's bionics file had mismatches!  Expected: %d, Actual: %d", GET_NAME(ch), num_objects, num_bionics);
    else
        mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s (level %d) has %d %s.", GET_NAME(ch), GET_LEVEL(ch), num_bionics, num_bionics > 1 ? "bionics" : "bionic");

    // close bionics file
    fclose(fp);
}

