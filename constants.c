/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */

/** Current tbaMUD version.
 * @todo defined with _TBAMUD so we don't have multiple constants to change.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "CyberASSAULT 2.1 based off TBAmud (Revision 295)";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *autoexits[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
    "DARK",
    "DEATH",
    "NO_MOB",
    "INDOORS",
    "PK_OK",
    "SOUNDPROOF",
    "NO_TRACK",
    "NO_PSIONICS",
    "UNDERWATER",
    "PRIVATE",
    "GODROOM",
    "*",  /* used for breadth-first-search */
    "HOUSE",
    "HCRSH",
    "ATRIUM",
    "SUPER_REGEN",
    "MINUS_REGEN",
    "VACUUM",
    "RADIATION",
    "INTENSE_HEAT",
    "INTENSE_COLD",
    "PSI_DRAINING",
    "MOVE_DRAINING",
    "HOLY_GROUND",
    "PEACEFUL",
    "TUNNEL",
    "NO_TP",
    "!SUM_OR_PORTAL",
    "!WHERE",
    "WORLDMAP",
    "OLC",
    "LOCKER",
    "NO_RECALL",
    "NO_SUICIDE",
	"WAREHOUSE",
	"HOSPITAL",
	"MERC_ONLY",
	"CRAZY_ONLY",
	"STALKER_ONLY",
	"BORG_ONLY",
	"CALLER_ONLY",
	"HIGHLANDER_ONLY",
	"PREDATOR_ONLY",
	"CUSTOM",
	"OCCUPIED",
	"JEWELER",
	"GREEN_SHOP",
	"BLUE_SHOP",
	"BLACK_SHOP",
    "\n"
};

/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
    "!TELEPORT",
    "!MARBLE",
    "!RECALL",
    "!SUM_OR_PORT",
    "REMORT_ONLY",
    "QUEST",
    "CLOSED",
    "!IMMORT",
    "GRID",
    "NOBUILD",
	"WORLDMAP",
	"!EVENT",
    "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
    "DOOR",
    "CLOSED",
    "LOCKED",
    "PICKPROOF",
    "HIDDEN",
    "BLOWPROOF",
    "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
    "Inside",
    "City",
    "Field",
    "Forest",
    "Hills",
    "Mountains",
    "Water (Swim)",
    "Water (No Swim)",
    "Underwater",
    "In Flight",
    "(Pause)",
    "Desert",
    "Death @RX@n",
    "Quest @Y!@n",
    "Shop @G$@n",
    "Underground @DU@n",
	"Vehicle Only",
    "\n"
};
/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
    "Dead",
    "Mortally wounded",
    "Incapacitated",
    "Stunned",
    "Sleeping",
    "Focusing",
    "Meditating",
    "Resting",
    "Sitting",
    "Fighting",
    "Standing",
    "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "PKOK",
  "REP",
  "MULTI",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
    "SPEC",
    "SENTINEL",
    "SCAVENGER",
    "ISNPC",
    "AWARE",
    "AGGRO",
    "STAY-ZONE",
    "WIMPY",
    "AGGRO_15",
    "AGGRO_25",
    "AGGRO_REMORT",
    "MEMORY",
    "HELPER",
    "THIEF",
    "MOB_HASTE_2",
    "MOB_HASTE_3",
    "MOB_HASTE_4",
    "PACIFIST",
    "HUNTER",
    "JUSTICE",
    "MOB_SANCT",
    "MOB_SUPER_SANCT",
    "MOB_MEGA_SANCT",
    "CHARM",
    "NOSUMMON",
    "NOSLEEP",
    "NOBASH",
    "NOBLIND",
    "DEAD",
	"STAY-WATER",
	"QUESTMASTER",
	"NO_SNIPE",
	"NO_DRAG",
	"MERCCORE",
	"MERCCALLING",
	"MERCFOCUS",
	"CRAZYCORE",
	"CRAZYCALLING",
	"CRAZYFOCUS",
	"BORGCORE",
	"BORGCALLING",
	"BORGFOCUS",
	"CALLERCORE",
	"CALLERCALLING",
	"CALLERFOCUS",
	"PREDATORCORE",
	"PREDATORCALLING",
	"PREDATORFOCUS",
	"HIGHLANDERCORE",
	"HIGHLANDERCALLING",
	"HIGHLANDERFOCUS",
	"STALKERCORE",
	"STALKERCALLING",
	"STALKERFOCUS",
	"MECHANIC",
	"\n"
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
    "BRIEF",
    "COMPACT",
    "NO_SHOUT",
    "NO_TELL",
    "D_HP",
    "D_PSI",
    "D_MOVE",
    "D_AMMO",
    "NO_HASS",
    "QUEST",
    "NO_SUMN",
    "NO_REP",
    "LIGHT",
    "C1",
    "C2",
    "NO_WIZ",
    "L1",
    "L2",
    "NO_AUC",
    "NO_GOS",
    "NO_GTZ",
    "RMFLG",
    "AFK",
    "A_CON",
    "A_EXIT",
    "A_WEAR",
    "A_LOOT",
    "A_SPLIT",
    "A_ASSIST",
    "A_GAIN",
    "A_DAM",
    "A_UNITS",
    "AUTOMAP",
    "LDFLEE",
    "NO_NEWBIE",
    "CLS",
    "BLDWALK",
    "R_SNOOP",
    "SPECT",
    "T_MORT",
    "FRZ_TAG",
    "O_SCR",
    "NO_WHO",
    "SHWDBG",
    "STLTH",
    "D_AUTO",
    "NO_TYPO",
    "A_SCAV",
    "NO_FSPAM",
    "AUTOKEY",
    "AUTODOOR",
	"NEWWHO",
	"OLDWHO",
	"OLDEQ",
	"NOMARKET",
	"BLIND",
	"SPAM",
	"NOLEAVE",
	"\n"
};

/** Combat bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline */
const char *combat_bits[] = {
	"\0", // DO NOT REMOVE!!
	"COMBO",
	"COMBO2",
	"\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
    "BLIND",
    "NO_TRACK",
    "DET-PSI",
    "HOLD",
    "STUN",
    "POISON",
    "GROUP",
    "WEAKEN",
    "SLEEP",
    "DODGE",
    "SNEAK",
    "HIDE",
    "CHARM",
    "FOLLOW",
    "WIMPY",
    "COVER FIRE",
    "BODY_BLOCK",
    "DISTRACT",
    "PARALYZE",
    "PETRIFY",
    "WATER_WALK",
    "LEVITATE",
    "SCUBA",
    "CURSE",
    "INFRA",
    "SANCT",
    "SUPER SANCT",
    "MEGA SANCT",
    "INVIS",
    "DET-INVIS",
    "ARMORED AURA",
    "PSI SHIELD",
    "HASTE",
    "SENSE-LIFE",
    "BIO_TRANCE",
    "THERMAL AURA",
    "COOL AURA",
    "2ND SIGHT",
    "DUPLICATES",
    "INDEST AURA",
    "INSPIRE",
    "VORPAL SPEED",
    "FANATICAL FURY",
    "LETH_IMMUNE",
    "BLIND_IMMUNE",
    "COURAGE",
    "PSI-MIRROR",
    "PRIMING",
    "MANIAC",
	"BRONZE LIFE",
	"SILVER LIFE",
	"GOLD LIFE",
	"PLATINUM LIFE",
	"INFINITY LIFE",
	"BIO REGEN",
	"COMBAT MIND",
	"BLEEDING",
	"FLASHED",
	"CHIPJACK",
	"EXARMS",
	"FOOTJET",
	"MATRIX",
	"AUTOPILOT",
	"ANIMAL_STATS",
    "\n"
};

/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Qattrib",
  "Qestra",
  "Qconsent",
  "Ident",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",
  "Custom Edit",
  "Protocol Detection",
  "Dead",
  "ClassHelp",
  "\n"
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \n. */
const char *wear_where[] = {
    "<light>           ",
    "<implant r>       ",
    "<implant l>       ",
    "<neck>            ",
    "<neck>            ",
    "<body>            ",
    "<head>            ",
    "<legs>            ",
    "<feet>            ",
    "<hands>           ",
    "<arms>            ",
    "<shield>          ",
    "<about body>      ",
    "<waist>           ",
    "<wrist r>         ",
    "<wrist l>         ",
    "<face>            ",
    "<ears>            ",
    "<wielded>         ",
    "<held>            ",
    "<floating nearby> ",
    "<nipple r>        ",
    "<nipple l>        ",
    "<finger r>        ",
    "<finger l>        ",
    "<ear ring r>      ",
    "<ear ring l>      ",
    "<nose ring>       ",
    "<expansion>       ",
	"<medical>         "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
    "Used as light",
    "Implant",
    "Implant",
    "First worn around Neck",
    "Second worn around Neck",
    "Worn on body",
    "Worn on head",
    "Worn on legs",
    "Worn on feet",
    "Worn on hands",
    "Worn on arms",
    "Worn as shield",
    "Worn about body",
    "Worn around waist",
    "Worn around right wrist",
    "Worn around left wrist",
    "Worn on face",
    "Worn on ears",
    "Wielded",
    "Held",
    "Floating nearby",
    "Worn on right nipple",
    "Worn on left nipple",
    "Worn on right finger",
    "Worn on left finger",
    "Worn on right ear",
    "Worn on left ear",
    "Worn as nose ring",
    "Expansion Slot",
	"Medical Bracelet",
    "\n"
};
const int wear_order_index[NUM_WEARS] = {
    WEAR_HEAD,
    WEAR_FACE,
    WEAR_RING_NOSE,
    WEAR_EARS,
    WEAR_RING_EAR_R,
    WEAR_RING_EAR_L,
    WEAR_NECK_1,
    WEAR_NECK_2,
    WEAR_ARMS,
    WEAR_WRIST_R,
    WEAR_WRIST_L,
    WEAR_HANDS,
    WEAR_RING_FIN_R,
    WEAR_RING_FIN_L,
    WEAR_IMPLANT_R,
    WEAR_IMPLANT_L,
    WEAR_RING_NIP_R,
    WEAR_RING_NIP_L,
    WEAR_ABOUT,
    WEAR_BODY,
    WEAR_WAIST ,
    WEAR_LEGS,
    WEAR_FEET,
    WEAR_HOLD,
    WEAR_LIGHT,
    WEAR_SHIELD,
    WEAR_WIELD,
    WEAR_EXPANSION,
    WEAR_FLOATING_NEARBY,
	WEAR_MEDICAL
};

/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
    "UNDEFINED",
    "LIGHT",
    "SCROLL",
    "WAND",
    "STAFF",
    "WEAPON",
    "FIRE_WEAPON",
    "MISSILE",
    "TREASURE",
    "ARMOR",
    "POTION",
    "WORN",
    "OTHER",
    "TRASH",
    "TRAP",
    "CONTAINER",
    "NOTE",
    "LIQ_CONTAINER",
    "KEY",
    "FOOD",
    "MONEY",
    "PEN",
    "BOAT",
    "FOUNTAIN",
    "BUTTON",
    "GUN",
    "AMMO",
    "RECALL",
    "LOTTO_TICKET",
    "SCRATCH_TICKET",
    "FLAG",
    "SABER_PIECE",
    "IDENTIFY",
    "MEDKIT",
    "DRUG",
    "EXPLOSIVE",
    "MARBLE_QUEST",
    "GEIGER_COUNTER",
    "AUTOQUEST_ITEM",
    "BAROMETER",
    "LUCKY_CHARMS",
    "TYPE_QUEST_ITEM",
    "EXPANSION_CHIP",
    "FURNITURE",
    "IMBUEABLE",
    "BODY PART",
    "BIONIC_DEVICE",
    "WEAPON_UPGRADE",
    "MEDICAL",
    "VEHICLE",
    "ATTACHMENT",
    "TABLE",
    "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
    "TAKE",
    "IMPLANT",
    "NECK",
    "BODY",
    "HEAD",
    "LEGS",
    "FEET",
    "HANDS",
    "ARMS",
    "SHIELD",
    "ABOUT",
    "WAIST",
    "WRIST",
    "FACE",
    "EARS",
    "WIELD",
    "HOLD",
    "THROW",
    "FLOATING",
    "NIPPLE",
    "FINGER",
    "EARRING",
    "NOSE",
    "EXPANSION",
    "MEDICAL",
    "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
    "GLOW",
    "HUM",
    "MIN_LEV_15",
    "MIN_LEV_25",
    "IMMORT_ONLY",
    "INVISIBLE",
    "PSIONIC",
    "NO_DROP",
    "BLESS",
    "QUEST_ITEM",
    "ANTI_DT",
    "NOT_USED",
    "!RENT",
    "!DONATE",
    "!INVIS",
    "BORG_ONLY",
    "CRAZY_ONLY",
    "STALKER_ONLY",
    "MERCENARY_ONLY",
    "HIGHLANDER_ONLY",
    "PREDATOR_ONLY",
    "!BORG",
    "!CRAZY",
    "!STALKER",
    "!MERCENARY",
    "!HIGHLANDER",
    "!PREDATOR",
    "NOSELL",
    "SEVERS",
    "NOLOCATE",
    "REMORT_ONLY",
    "NO_CALLER",
    "CALLER_ONLY",
	"UNDEAD CORPSE",
	"HUMANOID CORPSE",
	"MECHANICAL CORPSE",
	"HIGHLANDER CORPSE",
	"ANIMAL CORPSE",
	"CANINE/FELINE CORPSE",
	"INSECT CORPSE",
	"REPTILE CORPSE",
	"AQUATIC CORPSE",
	"ALIEN CORPSE",
	"MUTANT CORPSE",
	"PLANT CORPSE",
	"ETHEREAL CORPSE",
	"SHELL CORPSE",
	"BIRD CORPSE",
	"HIDDEN",
	"BINDING",
	"UNUSED",
    "\n"
};
// BODY_PART_X (body part names)
const char *body_parts[] = {
    "RESERVED",
    "head",
    "left eye",
    "right eye",
    "mouth",
    "neck",
    "left arm",
    "right arm",
    "left wrist",
    "right wrist",
    "left hand",
    "right hand",
    "left leg",
    "right leg",
    "left foot",
    "right foot",
    "chest",
    "abdomen",
    "back",
	"neural implant",
	"vascular implant",
	"respiratory implant",
	"skeletal implant",
    "\n"
};

// BODY_PART_X (body part names)
const char *where_body_parts[] = {
    "RESERVED",
    "<head>            ",
    "<left eye>        ",
    "<right eye>       ",
    "<mouth>           ",
    "<neck>            ",
    "<left arm>        ",
    "<right arm>       ",
    "<left wrist>      ",
    "<right wrist>     ",
    "<left hand>       ",
    "<right hand>      ",
    "<left leg>        ",
    "<right leg>       ",
    "<left foot>       ",
    "<right foot>      ",
    "<chest>           ",
    "<abdomen>         ",
    "<back>            ",
    "<neural>          ",
    "<vascular>        ",
    "<respiratory>     ",
    "<skeletal>        ",
    "\n"
};

// body part conditions
const char *part_condition[] = {
    "Excellent",
    "Slightly Damaged",
    "Broken",
    "Badly Bleeding",
    "Missing",
    "Bionic",
    "Bionic Damaged",
    "Bionic Broken",
    "Bionic Busted",
    "Bionic Missing",
    "\n"
};

//  gun types
const char *gun_types[] = {
    "none",
    "ENERGY",
    "CLIP",
    "SHELL",
    "AREA",
    "FUEL",
    "ROCKET",
    "ARROW",
    "BOLT",
    "\n"
};

/*  explosive  */
const char *explosive_types[] = {
    "none",
    "GRENADE",
    "MINE",
    "DESTRUCT",
    "TNT",
    "COCKTAIL",
    "PLASTIQUE",
    "\n"
};

const char *subexplosive_types[] = {
    "none",
    "NORMAL",
    "GAS",
    "FLASHBANG",
    "SONIC",
    "SMOKE",
    "FIRE",
    "CHEMICAL",
    "PSYCHIC",
    "LAG",
    "NUCLEAR",
    "NAPALM",
    "\n"
};

const char *wpn_psionics[] = {
    "NONE",
    "BLINDNESS",
    "PETRIFY",
    "COUNTER_SANCT",
    "PACIFY",
    "PSIONIC_SHACKLE",
    "POISON",
    "LEECH",
    "LETHARGY",
    "PSIONIC_CHANNEL",
    "FRAG_GRENADE",
    "PLASMA_GRENADE",
    "THERM_GRENADE",
    "FLASH_GRENADE",
    "ULTRASONIC_STUN",
    "GAS_GRENADE",
    "NOVA",
    "MEGA_COUNTER",
    "DEGRADE",
    "MEGA_DEGRADE", /*this is #20 same number as in olc.h*/
    "PSI_HAMMER",
    "ELECTROKINETIC-DISCHARGE",
    "SNUFF_LIFEFORCE",
    "PYROKINETIC_ERUPTION",
    "TRANSMUTE",
    "DISRUPT",
    "FIRE",
    "SMOKE",
    "LAG",
    "FIRE_BLADE",
    "ICE_BLADE",
    "\n"
};

/* GET_TEAM */
const char *teams[] =
{
    "None",
    "Red",
    "Blue",
    "Yellow",
    "Green",
    "Cyan",
    "Magenta",
    "White",
    "Black",
    "\n"
};
const char *biotypes[] = {
	"",
	"Interface",
	"Core",
	"Structure",
	"Armor",
	"Chip Jack",
	"Foot Jet",
	"Matrix",
	"\n"
};

/* Bionics */
// these must be in the same order as defined in structs.h
const char *bionic_names[][2] = {
    { "INTERFACE", "interface" },
    { "CHASSIS", "chassis" },
    { "SPINE", "spine" },
    { "SHOULDER", "shoulder" },
    { "RIBCAGE", "ribcage" },
    { "HIP", "hip" },
    { "ARMS", "arms" },
    { "NECK", "neck" },
    { "LUNG", "lung" },
    { "LEGS", "legs" },
    { "SKULL", "cybernetic skull" },
    { "CORE", "core" },
    { "BLASTER", "blaster" },
    { "BLADES", "blades" },
    { "EX_ARMS", "extra arms" },
    { "EYES", "eyes" },
    { "HEADJACK", "headjack" },
    { "VOICE", "voice" },
    { "ARMOR", "armor" },
    { "JETPACK", "jetpack" },
    { "FOOTJETS", "footjets" },
    { "MATRIX", "matrix" },
    { "SELF-DESTRUCT", "self-destruct" },
    { "CHIPJACK", "chipjack" },
    { "\n", "\n"}
};

const char *bionic_level[] = {
    "NONE",
    "BASIC",
    "ADVANCED",
    "REMORT",
    "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
    "NONE", // 0
    "STR",  // 1
    "DEX", // 2
    "INT",  // 3
    "PCN",  // 4
    "CON",  // 5
    "CHA",  // 6
    "SKILLSET",  // 7
    "PSI_MASTERY",  // 8
    "AGE",  // 9
    "CHAR_WEIGHT",  //10
    "CHAR_HEIGHT", //11
    "MAXPSI", //12
    "MAXHIT",//13
    "MAXMOVE",//14
	//"AC",
    "ARMOR",//15
    "HITROLL",
    "DAMROLL",
	"UNUSED",
    "PSI_REGEN",
    "HIT_REGEN",
    "MOVE_REGEN",
    "HIT_N_DAM",
    "REGEN_ALL",
    "H_P_V",
    "PSI2HIT",
    "ALL_STATS",
    "PSIONIC",
	"SAVING_POISON",
	"SAVING_DRUGS",
	"SAVING_FIRE",
	"SAVING_COLD",
	"SAVING_ELECTRICITY",
	"SAVING_EXPLOSION",
	"SAVING_PSIONICS",
	"SAVING_NONORM",
	"POISON_FOCUS",
	"DRUG_FOCUS",
	"FIRE_FOCUS",
	"COLD_FOCUS",
	"ELECTRICITY_FOCUS",
	"EXPLOSION_FOCUS",
	"PSIONIC_FOCUS",
	"ETHEREAL_FOCUS",	
    "\n"
};

const char *damagetype_bits[] = {
	"normal",
	"poisonous",
	"toxic",
	"firey",
	"icey",
	"electrifying",
	"explosive",
	"psionic",
	"ethereal",
	"\n",
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};

/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \n. */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};
const char *drug_names[] =
{
    "none",
    "juice",
    "nuke",
    "glitch",
    "jolt",
    "flash",
    "mirage",
    "god",
    "myst",
    "ko",
    "menace",
    "virgin",
    "ice",
    "peak",
    "velvet",
    "lava",
    "scarlett",
    "death",
    "nectar",
    "tremor",
    "ambrosia",
    "voodoo",
    "emerald",
    "honey",
    "medkit",
    "steroids",
    "morphine",
    "cyanide",
    "antipoison",
    "\n"
};

const char *vehicle_types[] =
{
	"Roadster",
	"Off-Road",
	"Aerial",
	"Marine",
	"Submarine",
	"Land and Aquatic",
	"\n"
};

const char *vehicle_names[] =
{
	"Temp vehicle 1",
	"Temp vehicle 2",
	"Temp vehicle 3",
	"Temp vehicle 4",
	"Temp vehicle 5",
	"Temp vehicle 6",
	"Temp vehicle 7",
	"Temp vehicle 8",
	"Temp vehicle 9",
	"Temp vehicle 10",
	"\n"
};

/** Strength attribute affects.
 * The fields are hit mod, damage mod, weight carried mod, and weight wielded
 * mod. */
cpp_extern struct str_app_type str_app[] = {
    {-5, -10, 0, 0},    /* str = 0 */
    {-5, -9, 3, 1},    /* str = 1 */
    {-4, -8, 3, 2},
    {-4, -7, 10, 3},
    {-3, -6, 25, 4},
    {-3, -5, 55, 5},    /* str = 5 */
    {-2, -4, 80, 6},
    {-2, -3, 90, 7},
    {-1, -2, 100, 8},
    {0, -1, 100, 9},
    {0, 0, 115, 10},    /* str = 10 */
    {1, 1, 115, 11},
    {2, 2, 140, 12},
    {3, 3, 140, 13},
    {4, 4, 170, 14},
    {5, 7, 170, 15},    /* str = 15 */
    {6, 9, 195, 16},
    {7, 12, 220, 18},
    {8, 15, 255, 20},    /* str = 18 */
    {10, 17, 340, 22},
    {12, 20, 450, 24},    /* str = 20 */
    {14, 25, 550, 26},
    {16, 30, 600, 28},
    {18, 35, 700, 30},
    {20, 40, 800, 33},
    {22, 50, 900, 36},    /* str = 25 */
    {22, 60, 900, 36},    /* str = 26 */
    {22, 70, 900, 36},    /* str = 27 */
    {22, 80, 900, 36},    /* str = 28 */
    {22, 90, 900, 40},    /* str = 29 */
    {22, 100, 1000, 40},    /* str = 30 */
    {24, 100, 1000, 40},    /* str = 30/0 - 30/50 */
    {26, 100, 1000, 44},    /* str = 30/51 - 30/75 */
    {28, 100, 1000, 50},    /* str = 30/76 - 30/90 */
    {30, 100, 1000, 50},    /* str = 30/91 - 30/99 */
    {30, 100, 1000, 50}    /* str = 30/100 */
};

const struct attack_rate_type attack_rate[] = {
	{ 0, 5 }, /* 0 */
	{ 1, 8 }, /* 1 */
	{ 1, 4 }, /* 2 */
	{ 1, 3 }, /* 3 */
	{ 3, 7 }, /* 4 */
	{ 3, 6 }, /* 5 */
	{ 3, 5 }, /* 6 */
	{ 3, 4 }, /* 7 */
	{ 5, 7 }, /* 8 */
	{ 5, 6 }, /* 9 */
	{ 5, 5 }, /* 10 */
	{ 5, 5 }, /* 11 */
	{ 5, 4 }, /* 12 */
	{ 5, 4 }, /* 13 */
	{ 5, 3 }, /* 14 */
	{ 10, 7 }, /* 15 */
	{ 10, 7 }, /* 16 */
	{ 10, 6 }, /* 17 */
	{ 10, 6 }, /* 18 */
	{ 10, 6 }, /* 19 */
	{ 10, 5 }, /* 20 */
	{ 10, 5 }, /* 21 */
	{ 10, 5 }, /* 22 */
	{ 10, 5 }, /* 23 */
	{ 10, 5 }, /* 24 */
	{ 10, 4 }, /* 25 */
	{ 10, 4 }, /* 26 */
	{ 10, 4 }, /* 27 */
	{ 10, 4 }, /* 28 */
	{ 10, 3 }, /* 29 */
	{ 10, 3 } /* 30 */
};

/** Dexterity skill modifiers for thieves.
 * The fields are for pick pockets, pick locks, find traps, sneak and hide. */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
    {-99, -99, -90, -99, -60},    /* dex = 0 */
    {-90, -90, -60, -90, -50},    /* dex = 1 */
    {-80, -80, -40, -80, -45},
    {-70, -70, -30, -70, -40},
    {-60, -60, -30, -60, -35},
    {-50, -50, -20, -50, -30},    /* dex = 5 */
    {-40, -40, -20, -40, -25},
    {-30, -30, -15, -30, -20},
    {-20, -20, -15, -20, -15},
    {-15, -10, -10, -20, -10},
    {-10, -5, -10, -15, -5},    /* dex = 10 */
    {-5, 0, -5, -10, 0},
    {0, 0, 0, -5, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},        /* dex = 15 */
    {0, 5, 0, 0, 0},
    {5, 10, 0, 5, 5},
    {10, 15, 5, 10, 10},        /* dex = 18 */
    {15, 20, 10, 15, 15},
    {15, 20, 10, 15, 15},        /* dex = 20 */
    {20, 25, 10, 15, 20},
    {20, 25, 15, 20, 20},
    {25, 25, 15, 20, 20},
    {25, 30, 15, 25, 25},
    {25, 30, 15, 25, 25}, /* dex = 25 */
    {20, 25, 10, 15, 20}, //26
    {20, 25, 15, 20, 20}, //27
    {25, 25, 15, 20, 20}, //28
    {25, 30, 15, 25, 25}, //29
    {25, 30, 15, 25, 25}  //30
};

/** Dexterity attribute affects.
 * The fields are reaction, missile attacks, and defensive (armor class). */
cpp_extern const struct dex_app_type dex_app[] = {
    {-7, -7, 6},        /* dex = 0 */
    {-6, -6, 5},        /* dex = 1 */
    {-4, -4, 5},
    {-3, -3, 4},
    {-2, -2, 3},
    {-1, -1, 2},        /* dex = 5 */
    {0, 0, 1},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},        /* dex = 10 */
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, -1},        /* dex = 15 */
    {1, 1, -2},
    {2, 2, -3},
    {2, 2, -4},        /* dex = 18 */
    {3, 3, -4},
    {3, 3, -4},        /* dex = 20 */
    {4, 4, -5},
    {4, 4, -5},
    {4, 4, -5},
    {5, 5, -6},
    {5, 5, -6},        /* dex = 25 */
    {5, 5, -7},        /* dex = 26 */
    {6, 5, -7},        /* dex = 27 */
    {6, 6, -8},        /* dex = 28 */
    {6, 6, -8},        /* dex = 29 */
    {7, 6, -9},        /* dex = 30 */
};

/** Constitution attribute affects.
 * The field referenced is for hitpoint bonus. */
cpp_extern const struct con_app_type con_app[] = {
    {-4, 20},        /* con = 0 */
    {-3, 25},        /* con = 1 */
    {-2, 30},
    {-2, 35},
    {-1, 40},
    {-1, 45},        /* con = 5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},        /* con = 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},        /* con = 15 */
    {2, 95},
    {2, 97},
    {3, 99},        /* con = 18 */
    {3, 99},
    {4, 99},        /* con = 20 */
    {5, 99},
    {5, 99},
    {5, 99},
    {6, 99},
    {6, 99},        /* con = 25 */
	{6, 99},   //26
	{7, 99},   //27
	{7, 99},   //28
	{7, 99},   //29
	{8, 99}   //30
};

/** Intelligence attribute affects.
 * The field shows how much practicing affects a skill/spell. */
cpp_extern const struct int_app_type int_app[] = {
    {3},        /* int = 0 */
    {5},        /* int = 1 */
    {7},
    {8},
    {9},
    {10},        /* int = 5 */
    {11},
    {12},
    {13},
    {15},
    {17},        /* int = 10 */
    {19},
    {22},
    {25},
    {30},
    {35},        /* int = 15 */
    {40},
    {45},
    {50},        /* int = 18 */
    {53},
    {55},        /* int = 20 */
    {56},
    {57},
    {58},
    {59},
    {60},        /* int = 25 */
	{65},   //26
	{65},   //27
	{70},   //28
	{70},   //29
	{75}   //30
};

/** Wisdom attribute affects.
 * The field represents how many extra practice points are gained per level. */
cpp_extern const struct pcn_app_type pcn_app[] = {
    {0},    /* pcn = 0 */
    {0},  /* pcn = 1 */
    {0},
    {0},
    {0},
    {0},  /* pcn = 5 */
    {0},
    {0},
    {0},
    {0},
    {0},  /* pcn = 10 */
    {0},
    {2},
    {2},
    {3},
    {3},  /* pcn = 15 */
    {3},
    {4},
    {5},    /* pcn = 18 */
    {6},
    {6},  /* pcn = 20 */
    {6},
    {6},
    {7},
    {7},
    {7},  /* pcn = 25 */
	{8},   //26
	{8},   //27
	{9},   //28
	{9},   //29
	{10}   //30
};
// Real NPC classes - Gahan
const char *npc_class_types[] = {
    "None",
    "Undead",
    "Humanoid",
	"Mechanical",
	"Highlander",
	"Animal",
	"Canine/Feline",
	"Insect",
	"Reptile",
	"Aquatic",
    "Alien",
	"Mutant",
	"Plant",
	"Ethereal",
	"Shell",
	"Bird",
    "\n"
};

// Skill trees - Gahan
const char *skill_tree_types[] = {
	"Melee",
	"Ranged",
	"Utility",
	"Defense",
	"Psionic"
};
/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
int movement_loss[] =
{
    1,    /* Inside     */
    2,    /* City       */
    3,    /* Field      */
    4,    /* Forest     */
    5,    /* Hills      */
    8,    /* Mountains  */
    4,    /* Swimming   */
    1,    /* Unswimable */
    1,    /* Flying     */
    8,    /* Underwater */
    1,    /* Pause      */
    10,    /* Desert     */
    1,    /* Death     */
    1,    /* Quest     */
    1,    /* Shop     */
    1,    /* Underground     */
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
    "January",        /* 0 */
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "Quest",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
    "all",
    "say",
    "gossip",
    "wiznet",
    "tell",
    "shout",
    "grats",
    "holler",
    "auction",
    "newbie",
    "radio",
	"event",
	"group",
    "\n"
};
const int sharp[] = {
    0,
    0,
    0,
    1,              /* Slashing */
    0,
    0,
    0,
    0,              /* Bludgeon */
    0,
    0,
    0,
    0               /* Pierce   */
};
/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};
const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "stormy"
};
/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;
	// Number of defined zone bit descriptions.
    size_t zone_bits_count = sizeof(zone_bits) / sizeof(zone_bits[0]) - 1;
