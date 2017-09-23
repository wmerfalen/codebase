//////////////////////////////////////////////////////////////////////////////////
// skilltree.c file ha been written from scratch by Gahan.      			   	//
// This is the new way skills and psionics are handled.  This is to support our //
// almost 50 classes, and the progression system.                               //
// - Gahan (NOTE: This code no longer has any relevance to prior codebases.		//
//////////////////////////////////////////////////////////////////////////////////

/** Help buffer the global variable definitions */
#define __SKILLTREE_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "psionics.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "comm.h"
#include "handler.h"
#include "ctype.h"
#include "skilltree.h"

const char *skill_tree_names[] = {
	"Melee Weapon Proficiencies  ", // 0
	"Melee Combat Tactics        ",
	"Melee Psionic Enhancements  ",
	"General Melee Skills        ",
	"Ranged Weapon Proficiencies ",
	"Ranged Combat Tactics       ", // 5
	"General Ranged Skills       ",
	"Spirituality                ",
	"Organic Clarity             ",
	"Psionic Transportation      ", 
	"Character Survival          ", // 10
	"General Gadgetry            ",
	"Utility                     ",
	"Aura Projection             ",
	"Healing                     ",
	"Defense Tactics             ", // 15
	"Corruption                  ",
	"Elemental Psionics          ",
	"Dark Forces Psionics        ",
	"Telekenesis Psionics        ",
	"Mercenary General           ", // 20
	"Marksman & Assassin Common  ",
	"Priest                      ",
	"Covert                      ",
	"Bralwer                     ",
	"BountyHunter                ", // 25
	"Enforcer                    ",
	"Stalker General             ",
	"Sniper                      ",
	"Terrorist                   ",
	"Spy                         ", // 30
	"Saboteur                    ",
	"Striker                     ",
	"Assassin                    ",
	"Borg General                ",
	"Drone                       ", // 35
	"Assimilator                 ",
	"Guardian                    ",
	"Destroyer                   ",
	"Juggernaut                  ",
	"Panzer                      ", // 40
	"Caller General              ",
	"Feral                       ",
	"Hunter                      ",
	"Beastmaster                 ",
	"Survivalist                 ", // 45
	"Vanguard                    ",
	"Shaman                      ",
	"Crazy General               ",
	"Anarchist                   ",
	"Psionicist                  ", // 50
	"Psychotic                   ",
	"Summoner                    ",
	"Elementalist                ",
	"Technomancer                ",
	"Highlander General          ", // 55
	"Knight                      ",
	"Blademaster                 ",
	"Reaver                      ",
	"Bladesinger                 ",
	"Arcane                      ", // 60
	"Bard                        ",
	"Predator General            ",
	"Youngblood                  ",
	"Weaponmaster                ",
	"Elder                       ", // 65
	"Badblood                    ",
	"Predalien                   ",
	"Mech                        ",
	"Elemental Protection 		 ", // 69
	"Miscellaneous               ", // 70
};

// Skill tree struct
// [-1] THE EMPTY SPACES BEFORE SKILL NAME REPRESENT THE SPACES NEEDED TO DISPLAY THE SKILL TREES
// [0] - Skill name ""
// [1] - Skill tree (0=Melee, 1=Ranged, 2=Utility, 3=Defense, 4=Psionic)
// [2] - Skill practice cost
// [3] - Skill tier (1=Basic, 2=Intermediate, 3=Proficient, 4=Skilled, 5=Mastered)
// [4] - Class specific (Insert class number, otherwise -1 for universal skill)  //Merc 0 -- Crazy -- 1 Stalker 2 -- Borg 3 -- Highlander 4 -- Pred 5 -- Caller 6
// [5] - Psionic = 0, Skill = 1
// [6] - Minimum Level Requirement
// [7] - Useage cost, Psionic = Psi points, Skill = Movement points
// [8] - This is the skill/psionic number for practicing and sorting purposes
// [9] - Minimum Position (Only used for psionics right now) IE: POS_STANDING
// [10] - Targets (Only used for psionics right now) for skill/psionic use IE: TAR_SELF | TAR_ROOM
// [11] - Routine (Only used for psionics right now) Psi_affects/Psi_damage/Psi_manual etc
// [12] - Wear off message to player(only used for psionics right now)
// [13] - Wear off message to room(only used for psionics right now)
// [13] - Pre-Requisite skill
// //Merc 0 -- Crazy -- 1 Stalker 2 -- Borg 3 -- Highlander 4 -- Pred 5 -- Caller 6

// SKILL/PSIONIC TREE LIST - GAHAN 2013 - UNIFORM STRUCTURE FOR ALL SKILLS / PSIONICS FOR USE WITH MORE IN DEPTH SKILL SYSTEM IF EVER IMPLEMENTED
const struct skill_tree_type full_skill_tree[] = {
// "Name",															Tree/
//Prac  Tier/Class  Skill/Lvl/Useage  Skill Number, Minimum Pos, Targets, Violent, Routine, Wear off message 
// TREE 0 - GENERAL MELEE WEAPON PROFICIENCY
{ "+ Melee Weapon Proficiency         ","Melee Weapon Proficiency",	0,	5,		1,	-1,		1,	1,	2,		SKILL_MELEE_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Strongarm                     ","Strongarm",				0,	10,		1,	-1,		1,	8,	2,		SKILL_STRONGARM, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_MELEE_WPN_PRO, 1 },
{ "        Maniac                     ","Maniac",					0,	20,		1,	-1,		1,	16, 20,		SKILL_MANIAC, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_STRONGARM, 1 },
{ "           Berserk                 ","Berserk",					0,	25,		1,	-1,		1,	25,	150,	SKILL_BERSERK, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_MANIAC, 1 },
{ "              Frenzy               ","Frenzy",					0,  30,		1,	-1,     1,  45, 170,	SKILL_FRENZY, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_BERSERK, 1 },
{ "                 Battle Rage       ","Battle Rage",				0,  35,		1,  -1,     1,  45, 200,	SKILL_BATTLERAGE, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_FRENZY, 1 },
// TREE 1 - GENERAL MELEE COMBAT TACTICS
{ "+ Melee Combat Tactics             ","Melee Combat Tactics",		1,  5,		1,  -1,		1,  5,  2,		SKILL_COMBAT_TACTICS, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Double Hit                    ","Double Hit",				1,	10,		1,	-1,		1,	6,	2,		SKILL_DOUBLE_HIT, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_COMBAT_TACTICS, 1 },
{ "        Critical Knowledge         ","Critical Knowledge",		1,	15,		1,	-1,		1,	10,	20,		SKILL_CRITICAL_KNOWLEDGE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_DOUBLE_HIT, 1 },
{ "           Charge                  ","Charge",					1,	20,		1,	-1,		1,	15,	80,		SKILL_CHARGE, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_CRITICAL_KNOWLEDGE, 1 },
{ "              Slash                ","Slash",					1,	-1,		1,	-1,		1,	45, 90,		SKILL_SLASH, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_CHARGE, 1 },
// TREE 2 - GENERAL MELEE PSIONIC ENHANCEMENTS
{ "+ Psionic Enhancements             ","Psionic Enhancements",		2,	5,		1,	-1,		1,	8,	2,		SKILL_PSI_ENHANCE,	POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Haste                         ","Haste",					2,	10,		1,	-1,		0,	10, 40,		PSIONIC_HASTE, POS_FIGHTING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You are no longer moving as fast.", "$n begins to return to normal speed.", SKILL_PSI_ENHANCE, 1 },
{ "        Vorpal Speed               ","Vorpal Speed",				2,	35,		1,	-1,		0,	30, 120,	PSIONIC_VORPAL_SPEED, POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel much slower.","$n begins to return to normal speed.", PSIONIC_HASTE, 0 },
// TREE 3 - GENERAL MELEE SKILLS
{ "  Stun                             ","Stun",						3,	25,		1,	-1,		1,	32, 100,	SKILL_STUN, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "  Drag                             ","Drag",						3,  15,     1,  -1,     1,  33, 50,		SKILL_DRAG, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "  Bio Blast                        ","Bio Blast",				3,	-1,		1,	-1,		1,	45, 1,		SKILL_BIOBLAST, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "  Bio Slash                        ","Bio Slash",				3,	-1,		1,	-1,		1,	45, 1,		SKILL_BIOSLASH, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, -1 },
// TREE 4 - GENERAL RANGED WEAPONS PROFICIENCY
{ "+ Ranged Weapon Proficiency        ","Ranged Weapon Proficiency",4,	2,		1,	-1,		1,	1,	2,		SKILL_RANGED_WPN_PRO, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Automatic Weapons             ","Automatic Weapons",		4,	20,		1,	-1,		1,	5,	2,		SKILL_AUTO_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Energy Weapons                ","Energy Weapons",			4,	20,		1,	-1,		1,	14, 2,		SKILL_ENERGY_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Explosive Weapons             ","Explosive Weapons",		4,	20,		1,	-1,		1,	15, 2,		SKILL_EXPLO_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Fuel Weapons                  ","Fuel Weapons",				4,	20,		1,	-1,		1,	16, 2,		SKILL_FUEL_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Rocket Weapons                ","Rocket Weapons",			4,	20,		1,	-1,		1,	20, 2,		SKILL_ROCKET_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Shell Weapons                 ","Shell Weapons",			4,	20,		1,	-1,		1,	7,	2,		SKILL_SHELL_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
{ "     Archery Weapons               ","Archery Weapons",			4,  20,		1,  -1,		1,  10, 2,		SKILL_ARCHERY_WPN_PRO, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RANGED_WPN_PRO, 1 },
// TREE 5 - GENERAL RANGED COMBAT TACTICS
{ "+ Ranged Combat Tactics            ","Ranged Combat Tactics",	5,	2,		1,	-1,		1,	10,	2,		SKILL_RNGCOMBAT_TACTICS, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
{ "     Ambush                        ","Ambush",					5,	20,		1,	-1,		1,	10,	100,	SKILL_AMBUSH, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RNGCOMBAT_TACTICS, 1 },
{ "        Coverfire                  ","Coverfire",			5,	20,		1,	-1,		1,	15, 60,		SKILL_COVER_FIRE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_AMBUSH, 1 },
{ "           Throw                   ","Throw",			5,	15,		1,	-1,		1,	18, 85,		SKILL_THROW, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_COVER_FIRE, 1 },
// TREE 6 - GENERAL RANGED SKILLS
{ "+ Marksmanship                     ","Marksmanship",			6,	10,		1,	-1,		1,	5, 2,		SKILL_MARKSMANSHIP, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
{ "     Deadly Aim                    ","Deadly Aim",			6,	15,		1,	-1,		1,	12,	2,		SKILL_DEADLY_AIM, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_MARKSMANSHIP, 1 },
{ "        Aim                        ","Aim",						6,	10,		1,	-1,		1,	22, 40,		SKILL_AIM, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_DEADLY_AIM, 1 },
{ "        Sniper                     ","Sniper",			6,	30,		1,	-1,		1,	22, 100,	SKILL_SNIPER, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_DEADLY_AIM, 1 },
{ "  Blast                            ","Blast",					6,	-1,		1,	-1,		1,	45, 100,	SKILL_BLAST, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
// TREE 7 - GENERAL UTILITY - SPIRITUALITY TREE
{ "+ Spirituality                     ","Spirituality",				7,	2,		1,	-1,		1,	1,	2,		SKILL_SPIRITUALITY, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Meditate                      ","Meditate",					7,	15,		1,	-1,		1,	15, 40,		SKILL_MEDITATE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, PSIONIC_ASTRAL_PROJECTION, 0 },
{ "        Focus                      ","Focus",					7,	30,		1,	-1,		1,	33, 50,		SKILL_FOCUS, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_MEDITATE, 1 },
{ "+ Astral Projection                ","Astral Projection",		7,	15,		1,	-1,		0,	8, 90,		PSIONIC_ASTRAL_PROJECTION, POS_STANDING, TAR_CHAR_WORLD, TRUE, PSI_MANUAL, "!Astral projection!", NULL, -1, -1 },
{ "     Locate Object                 ","Locate Object",			7,	10,		1,	-1,		0,	12,	100,	PSIONIC_LOCATE_OBJECT, POS_STANDING, TAR_OBJ_WORLD, FALSE, PSI_MANUAL, "!Locate object!", NULL, PSIONIC_ASTRAL_PROJECTION, 0 },
{ "        Farsee                     ","Farsee",					7,	10,		1,	-1,		0,	15,	1,		PSIONIC_FARSEE, POS_STANDING, TAR_CHAR_WORLD, TRUE, PSI_MANUAL, "!Farsee!", NULL, PSIONIC_LOCATE_OBJECT, 0 },
// TREE 9 - GENERAL UTILITY - PSIONIC TRANSPORTATION
{ "+ Teleport                         ","Teleport",					9,	20,		1,	-1,		0,	8, 100,		PSIONIC_TELEPORT, POS_STANDING, TAR_IGNORE | TAR_CHAR_ROOM, FALSE, PSI_MANUAL, "!Teleport!", NULL,  -1, -1 },
{ "     Summon                        ","Summon",					9,	25,		1,	-1,		0,	15,	100,	PSIONIC_SUMMON, POS_STANDING, TAR_CHAR_WORLD, FALSE, PSI_MANUAL, "!Summon!", NULL, PSIONIC_TELEPORT, 0},
{ "        Portal                     ","Portal",					9,	20,		1,	-1,		0,	30, 80,		PSIONIC_PORTAL, POS_STANDING, TAR_CHAR_WORLD, FALSE, PSI_MANUAL, "!Portal!", NULL, PSIONIC_SUMMON, 0 },
// TREE 10 - GENERAL UTILITY - SURVIVAL SKILLS/PSIONICS
{ "+ Survival                         ","Survival",					10,	2,		1,	-1,		1,	1,	2,		SKILL_SURVIVAL, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     First Aid                     ","First Aid",				10,	10,		1,	-1,		1,	3,	60,		SKILL_FIRST_AID, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SURVIVAL, 1 },
{ "     Sneak                         ","Sneak",					10,	10,		1,	-1,		1,	3,	20,		SKILL_SNEAK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SURVIVAL, 1 },
{ "        Hide                       ","Hide",						10,	15,		1,	-1,		1,	4,	40,		SKILL_HIDE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SNEAK, 1 },
{ "     Psionics Lore                 ","Psionics Lore",			10, 10,		1,	-1,		1,	10,	0,		SKILL_PSIONICS_LORE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SURVIVAL, 1 },
{ "     Track                         ","Track",					10,	30,		1,	-1,		1,	35, 20,		SKILL_TRACK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SURVIVAL, 1 },
{ "     Recon                         ","Recon",					10,	10,		1,	-1,		1,	8,	50,		SKILL_RECON, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SURVIVAL, 1 },
{ "        Eagle Eye                  ","Eagle Eye",			10,	10,		1,	6,		1,	8,	50,		SKILL_EAGLE_EYE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RECON, 1 },
{ "        Search                     ","Search",					10,	5,		1,	-1,		1,	10,	40,		SKILL_SEARCH, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RECON, 1 },
{ "     Sense Life                    ","Sense Life",				10,	10,		1,	-1,		0,	7,	50,		PSIONIC_SENSE_LIFE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You are not as able to detect others.", NULL, SKILL_SURVIVAL, 1 },
{ "        Infravision                ","Infravision",				10,	15,		1,	-1,		0,	10, 55,		PSIONIC_INFRAVISION, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The red vision in your eyes subside.", NULL, PSIONIC_SENSE_LIFE, 0 },
{ "           Second Sight            ","Second Sight",				10,	20,		1,	-1,		0,	30, 80,		PSIONIC_SECOND_SIGHT, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The red in your eyes fade.", NULL, PSIONIC_INFRAVISION, 0 },
{ "        Reveal Life                ","Reveal Life",				10,	10,		1,	-1,		0,	45, 1,		PSIONIC_REVEAL_LIFE, POS_STANDING, TAR_IGNORE, FALSE, PSI_AFFECTS, "!Reveal Life!", NULL, PSIONIC_SENSE_LIFE, 0 },
// TREE 11 - GENERAL UTILITY - GADGETRY SKILLS
{ "+ Gadgetry                         ","Gadgetry",					11,	5,		1,	-1,		1,	1,	10,		SKILL_GADGETRY, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Pick Locks                    ","Pick Locks",				11,	20,		1,	-1,		1,	5,	50,		SKILL_PICK_LOCK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_GADGETRY, 1 },
{ "        Explosives                 ","Explosives",				11,	30,		1,	-1,		1,	20, 100,	SKILL_EXPLOSIVES, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_PICK_LOCK, 1 },
// TREE 12 - GENERAL UTILITY - GENERAL UTILITY SKILLS/PSIONICS
{ "  Bio Trance                       ","Bio Trance",				12,	15,		1,	-1,		0,	10,	80,		PSIONIC_BIO_TRANCE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "Your respiration returns to normal.", NULL, -1, -1 },
{ "  Conjure                          ","Conjure",					12,	10,		1,	-1,		0,	3,	1,		PSIONIC_CONJURE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "!Conjure!", NULL, -1, -1 },
{ "  Courage                          ","Courage",					12,	20,		1,	-1,		0,	15, 70,		PSIONIC_COURAGE, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You do not feel as brave as before.", NULL, -1, -1 },
{ "  Detox                            ","Detox",					12,	15,		1,	-1,		0,	6,	150,	PSIONIC_DETOX, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_MANUAL, "!Detox!", NULL, -1, -1 },
{ "  Inspire                          ","Inspire",					12,	20,		1,	-1,		0,	18, 85,		PSIONIC_INSPIRE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel less inspired.", NULL, -1, -1 },
{ "  Resist Fatigue                   ","Resist Fatigue",			12,	30,		1,	-1,		0,	20, 50,		PSIONIC_RESIST_FATIGUE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_POINTS, "!Resist Fatigue!", NULL, -1, -1 },
{ "  Levitate                         ","Levitate",					12,	10,		1,	-1,		0,	10, 80,		PSIONIC_LEVITATE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You no longer float above the ground.","$n floats gently to the ground.", SKILL_SPIRITUALITY, 1 },
// TREE 13 - GENEREAL DEFENSIVE - AURA PROJECTION
{ "+ Aura Projection                  ","Aura Projection",			13,	2,		1,	-1,		1,	1,	2,		SKILL_AURA_PROJECTION, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "    Armored Aura                   ","Armored Aura",				13,	15,		1,	-1,		0,	5,	60,		PSIONIC_ARMORED_AURA, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel less protected.","$n looks less protected.", SKILL_AURA_PROJECTION, 1 },
{ "       Aura of Cool                ","Aura of Cool",				13,	15,		1,	-1,		0,	6,	65,		PSIONIC_AURA_OF_COOL, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel less protected by the heat.","$n appears to be less protected by heat.", PSIONIC_ARMORED_AURA, 0 },
{ "       Thermal Aura                ","Thermal Aura",				13,	15,		1,	-1,		0,	8,	90,		PSIONIC_THERMAL_AURA, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel less protected from the cold.","$n appears to be less protected by the cold.", PSIONIC_ARMORED_AURA, 0 },
{ "       Psi Shield                  ","Psi Shield",				13,	20,		1,	-1,		0,	12, 95,		PSIONIC_PSI_SHIELD, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "Your psionic shield disappears.","$n is no longer surrounded by a psionic shield.", PSIONIC_ARMORED_AURA, 0 },
{ "          Sanctuary                ","Sanctuary",				13,	20,		1,	-1,		0,	15,	100,	PSIONIC_SANCTUARY, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The white aura around your body fades.","The white aura surrounding $n fades.", PSIONIC_PSI_SHIELD, 0 },
{ "             Group Sanctuary       ","Group Sanctuary",			13,	30,		1,	-1,		0,	22, 250,	PSIONIC_GROUP_SANCT, POS_STANDING, TAR_IGNORE, FALSE, PSI_GROUPS, "!Group Sanct!", NULL, PSIONIC_SANCTUARY, 0 },
{ "             Super Sanctuary       ","Super Sanctuary",			13,	25,		1,	-1,		0,	25, 140,	PSIONIC_SUPER_SANCT, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The silver aura around your body fades.","The silver aura surrounding $n fades.", PSIONIC_SANCTUARY, 0 },
{ "                Mega Sanctuary     ","Mega Sanctuary",			13,	25,		1,	-1,		0,	30, 150,	PSIONIC_MEGA_SANCT, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The golden aura around your body fades.","The golden aura surrounding $n fades", PSIONIC_SUPER_SANCT, 0 },
{ "                Indestructable Aura","Indestructable Aura",		13,	25,		1,	-1,		0,	37, 100,	PSIONIC_INDESTRUCTABLE_AURA, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "Your telekinetic shield shimmers away.","$n is no longer protected by a telekinetic shield.", PSIONIC_SUPER_SANCT, 0 },
// TREE 14 - GENERAL DEFENSIVE - HEALING
{ "+ Heal                             ","Heal",						14,	15,		1,	-1,		0,	10, 85,		PSIONIC_HEAL, POS_FIGHTING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_POINTS, NULL, "!Heal!", -1, -1 },
{ "     Psychic Surgery               ","Psychic Surgery",			14,	20,		1,	-1,		0,	15,	100,	PSIONIC_PSYCHIC_SURGERY, POS_STANDING, TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, PSI_POINTS, NULL, "!Psychic Surgery!", PSIONIC_HEAL, 0 },
{ "        Restore Sight              ","Restore Sight",			14,	10,		1,	-1,		0,	22,	100,	PSIONIC_RESTORE_SIGHT, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_UNAFFECTS, NULL, "!Restore Sight!", PSIONIC_PSYCHIC_SURGERY, 0 },
{ "           Mega Heal               ","Mega Heal",				14,	30,		1,	-1,		0,	27, 350,	PSIONIC_MEGAHEAL, POS_FIGHTING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_POINTS, "!MegaHeal!", NULL, PSIONIC_RESTORE_SIGHT, 0 },
{ "              Group Heal           ","Group Heal",				14,	30,		1,	-1,		0,	33, 250,	PSIONIC_GROUP_HEAL, POS_FIGHTING, TAR_IGNORE, FALSE, PSI_GROUPS, "!Group Heal!", NULL, PSIONIC_MEGAHEAL, 0 },

// TREE 15 - GENERAL DEFENSIVE - DEFENSE TACTICS                                Tree    Prac	Tier   Specific Sk1Ps0  Lvl     mv-psi 
{ "+ Bodyblock                        ","Bodyblock",				15,	20,	1,	-1,	1,	10,	90,		SKILL_BODY_BLOCK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Dodge                         ","Dodge",				15,	30,	1,	-1,	1,	20, 	50,		SKILL_DODGE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_BODY_BLOCK, 1 },
{ "     Defensive Roll                ","Defensive Roll",			15,	25,	1,	-1,	1,	20, 	50,		SKILL_DEFENSE_ROLL, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
{ "        Flank                      ","Flank",				15,	20,	1,	-1,	1,	11, 	80,		SKILL_FLANK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_DODGE, 1 },
{ "           Protect                 ","Protect",				15,	10,	1,	-1,	1,	17, 	20,		SKILL_PROTECT, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_FLANK, 1 },
{ "               Rescue              ","Rescue",				15,	15,	1,	-1,	1,	20, 	50,		SKILL_RESCUE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_PROTECT, 1 },
// TREE 16 - GENERAL DEFENSIVE - CORRUPTION
{ "+ Corruption                       ","Corruption",				16,	2,		1,	-1,		1,	1,	2,		SKILL_DEBUFFING, POS_STANDING, TAR_IGNORE, FALSE, PSI_POINTS, NULL, NULL, -1, -1 },
{ "     Counter Sanctuary             ","Counter Sanctuary",			16,	20,		1,	-1,		0,	15, 100,	PSIONIC_COUNTER_SANCT, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, PSI_UNAFFECTS, "!Counter Sanct!", NULL, SKILL_DEBUFFING, 1 },
{ "        Counter Super Sanct        ","Counter Super Sanct",			16,	20,		1,	-1,		0,	25, 120,	PSIONIC_COUNTER_SUPER_SANCT, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, PSI_UNAFFECTS, "!Counter Super Sanct!", NULL, PSIONIC_COUNTER_SANCT, 0 },
{ "           Mega Counter            ","Mega Counter",				16,	30,		1,	-1,		0,	30, 150,	PSIONIC_MEGA_COUNTER, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, PSI_UNAFFECTS, "!Mega Counter!", NULL, PSIONIC_COUNTER_SUPER_SANCT, 0 },
{ "              Degrade              ","Degrade",				16,	20,		1,	 1,		0,	34, 120,	PSIONIC_DEGRADE, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_AFFECTS, "You no longer feel as vulnerable to attack.", NULL, PSIONIC_MEGA_COUNTER, 0 },
{ "                 Mega Degrade      ","Mega Degrade",				16,	30,		1,	 1,		0,	38, 150,	PSIONIC_MEGA_DEGRADE, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_AFFECTS, "You no longer feel as greatly vulnerable to attack.", NULL, PSIONIC_DEGRADE, 0 },
{ "     Pacify                        ","Pacify",					16,	20,		1,	 1,		0,	21, 100,	PSIONIC_PACIFY, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "!Pacify!", NULL, SKILL_DEBUFFING, 1 },
{ "        Panic                      ","Panic",					16,	20,		1,	 1,		0,	28, 110,	PSIONIC_PANIC, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_MANUAL, "!Panic!", NULL, PSIONIC_PACIFY, 0 },
{ "           Petrify                 ","Petrify",					16,	30,		1,	 1,		0,	45, 400,	PSIONIC_PETRIFY, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "You can move!", NULL, PSIONIC_PANIC, 0 },
{ "     Weaken                        ","Weaken",					16,	20,		1,	 1,		0,	10, 100,	PSIONIC_WEAKEN, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_AFFECTS, "You no longer feel as weak.", NULL, SKILL_DEBUFFING, 1 },
{ "        Lethargy                   ","Lethargy",					16,	20,		1,	 1,		0,	19, 100,	PSIONIC_LETHARGY, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, PSI_AFFECTS, "You feel more alert.", NULL, PSIONIC_WEAKEN, 0 },
{ "           Lethargy Immunity       ","Lethargy Immunity",		16,	10,		1,	 1,		0,	45, 1,		PSIONIC_LETHARGY_IMMUNE, POS_STANDING, TAR_CHAR_ROOM |TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "Your skin stops twitching.", NULL, PSIONIC_LETHARGY, 0 },
// TREE 17 - GENERAL PSIONICS - ELEMENTAL PSIONICS TREE
{ "+ Elemental Lore                   ","Elemental Lore",			17,	2,		1,	-1,		1,	1,	2,		SKILL_ELEMENTAL_LORE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Bio Regen                     ","Bio Regen",				17,	15,		1,	-1,		0,	5,	100,	PSIONIC_BIO_REGEN, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "The blue energy about you slowly dissapates.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "        Psionic Duplicates         ","Duplicates",				17,	30,		1,	-1,		0,	34, 190,	PSIONIC_DUPLICATES, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "You are no longer surrounded by images.","$n is no longer protected by duplicate images.",  PSIONIC_BIO_REGEN, 0 },
{ "           Fire Blade              ","Fire Blade",				17,	-1,		1,	-1,		0,	45, 1,		PSIONIC_FIRE_BLADE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, PSIONIC_DUPLICATES, 0 },
{ "           Ice Blade               ","Ice Blade",				17,	-1,		1,	-1,		0,	45, 1,		PSIONIC_ICE_BLADE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, PSIONIC_DUPLICATES, 0 },
{ "     Electrokinetic Discharge      ","Electrokinetic Discharge",	17,	15,		1,	 1,		0,	5,	90,		PSIONIC_ELECTROKINETIC_DISCHARGE, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, PSI_DAMAGE, "!Electrokinetic Discharge!", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "        Pyrokinetic Eruption       ","Pyrokinetic Eruption",		17,	30,		1,	 1,		0,	27, 550,	PSIONIC_PYROKINETIC_ERUPTION, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_AREAS, "!Pyrokinetic Eruption!", NULL, PSIONIC_ELECTROKINETIC_DISCHARGE, 0 },
{ "           Nova                    ","Nova",						17,	35,		1,	 1,		0,	35, 500,	PSIONIC_NOVA, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, "!Nova!", NULL, PSIONIC_PYROKINETIC_ERUPTION, 0 },
{ "              Super Nova           ","Super Nova",				17,	50,		1,	 1,		0,	38, 1500,	PSIONIC_SUPER_NOVA, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_AREAS, "!Super Nova!", NULL, PSIONIC_NOVA, 0 },
{ "     Detect Invisibility           ","Detect Invisibility",		17,	10,		1,	-1,		0,	9,	50,		PSIONIC_DETECT_INVISIBILITY, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You can no longer see invisible.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "         Invisibility              ","Invisibility",				17,	10,		1,	-1,		0,	11,	60,		PSIONIC_INVISIBILITY, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_OBJ_EQUIP, FALSE, PSI_AFFECTS | PSI_ALTER_OBJS, "You feel yourself exposed.", NULL, PSIONIC_DETECT_INVISIBILITY, 0 },
{ "     Mystic Bulb                   ","Mystic Bulb",				17,	10,		1,	-1,		0,	1,	1,		PSIONIC_MYSTIC_BULB, POS_STANDING, TAR_IGNORE, FALSE, PSI_CREATIONS, "!Mystic Bulb!", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "     Commune                       ","Commune",					17,	10,		1,	-1,		0,	10,	100,	PSIONIC_COMMUNE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "!Commune!", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "     Fanatical Fury                ","Fanatical Fury",			17,	30,		1,	-1,		0,	37, 200,	PSIONIC_FANATICAL_FURY, POS_FIGHTING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You do not feel as focused as before.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "     Psionic Imbue                 ","Imbue Essence",			17,	50,		1,	-1,		0,	40, 500,	PSIONIC_IMBUE_ESSENCE, POS_STANDING, TAR_OBJ_INV, FALSE, PSI_ALTER_OBJS, "!Imbue Essence!", NULL, SKILL_ELEMENTAL_LORE, 1 },
// TREE 18 - GENERAL PSIONICS - DARK FORCESS PSIONICS TREE
{ "+ Dark Force                       ","Dark Force",				18,	2,		1,	-1,		1,	2,	2,		SKILL_DARK_FORCE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Blindness                     ","Blindness",				18,	20,		1,	 1,		0,	16, 250,	PSIONIC_BLINDNESS, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, PSI_AFFECTS, "You feel a cloak of blindness dissolve.", NULL, SKILL_DARK_FORCE, 1 },
{ "     Blind Immunity                ","Blind Immunity",			18,	-1,		1,	-1,		0,	45, 1,		PSIONIC_BLIND_IMMUNE, POS_STANDING, TAR_CHAR_ROOM |TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You relax your eyes.", NULL, SKILL_DARK_FORCE, 1 },
{ "     Poison                        ","Poison",					18,	20,		1,	-1,		0,	23, 100,	PSIONIC_POISON, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_AFFECTS, "You no longer feel poisoned.","$n no longer appears to be poisoned.", SKILL_DARK_FORCE, 1 },
{ "     Disruption                    ","Disruption",				18,	25,		1,	 1,		0,	22, 165,	PSIONIC_DISRUPTION, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, PSI_DAMAGE, "!Disruption!", NULL, SKILL_DARK_FORCE, 1 },
{ "        Psychic Leech              ","Psychic Leech",			18,	30,		1,	 1,		0,	30, 100,	PSIONIC_PSYCHIC_LEECH, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_MANUAL, "!Psychic Leech!", NULL, PSIONIC_DISRUPTION, 0 },
{ "        Transmute                  ","Transmute",				18,	30,		1,	 1,		0,	31, 300,	PSIONIC_TRANSMUTE, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, "!Transmute!", NULL, PSIONIC_DISRUPTION, 0 },
{ "		      Psi Shackle             ","Psi Shackle",				18,	-1,		1,	 1,		0,	25, 1,		PSIONIC_PSI_SHACKLE, POS_FIGHTING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "You are now able to move about.", NULL, PSIONIC_TRANSMUTE, 0 },
{ "              Remove Shackle       ","Remove Shackle",			18,	30,		1,	 1,		0,	30, 100,	PSIONIC_REMOVE_SHACKLE, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "!Remove Shackle!", NULL, PSIONIC_PSI_SHACKLE, 0 },
{ "              Paralyze             ","Paralyze",					18,	30,		1,	 1,		0,	32, 300,	PSIONIC_PARALYZE, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, FALSE, PSI_AFFECTS, "You can move again!", "$n is no longer immobile and can move again.", PSIONIC_PSI_SHACKLE, 0 },
{ "                 Drain             ","Drain",					18,	30,		1,	 1,		1,	35, 150,	SKILL_DRAIN, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, PSIONIC_PARALYZE, 0 },
// TREE 19 - GENERAL PSIONICS - TELEKENISIS PSIONCIS TREE
{ "+ Telekinesis                      ","Telekinesis",				19,	2,		1,	-1,		1,	1,	2,		SKILL_TELEKINESIS, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Combat Mind                   ","Combat Mind",				19,	35,		1,	-1,		0,	29, 300,	PSIONIC_COMBAT_MIND, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "Your mind is no longer focused on combat.", NULL, SKILL_TELEKINESIS, 1 },
{ "     Induce Sleep                  ","Induce Sleep",				19,	30,		1,	 1,		0,	31, 250,	PSIONIC_INDUCE_SLEEP, POS_STANDING, TAR_CHAR_ROOM, TRUE, PSI_AFFECTS, "You no longer feel as sleepy.", NULL, SKILL_TELEKINESIS, 1 },
{ "     Psi Bullets                   ","Psi Bullets",				19,	20,		1,	 1,		0,	6,	80,		PSIONIC_PSI_BULLET, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_MANUAL, "!Psi Bullet!", NULL, SKILL_TELEKINESIS, 1 },
{ "        Psi Hammer                 ","Psi Hammer",				19,	20,		1,	 1,		0,	12,	70,		PSIONIC_PSI_HAMMER, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, "!Psi Hammer!", NULL, PSIONIC_PSI_BULLET, 0 },
{ "           Snuff Lifeforce         ","Snuff Lifeforce",			19,	30,		1,	 1,		0,	28, 225,	PSIONIC_SNUFF_LIFEFORCE, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, "!Snuff Lifeforce!", NULL, PSIONIC_PSI_HAMMER, 0 },
// TREE 20 - MERCENARY GENERAL
{ "+ Disarm                           ","Disarm",					20,	20,		1,	 0,		1,	24, 80,		SKILL_DISARM, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
{ "     Retreat                       ","Retreat",					20,	20,		1,	 0,		1,	28, 95,		SKILL_RETREAT, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_DISARM, 1 },
{ "        Resiliance                 ","Resiliance",				20,	20,		1,	 0,		0,	34, 100,	PSIONIC_RESILIANCE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_UNAFFECTS, "!Resiliance!", NULL, SKILL_RETREAT, 1 },
// TREE 21 - MARKSMAN TREE
// TREE 22 - PRIEST TREE
// TREE 23 - COVERT TREE
// TREE 24 - BRAWLER TREE
// TREE 25 - BOUNTYHUNTER TREE
// TREE 26 - ENFORCER TREE
// TREE 27 - STALKER GENERAL
{ "  Sneak Attacks                    ","Sneak Attacks",			27,  2,		1,   2,		1,	2,	2,		SKILL_SNEAK_ATTACKS, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Backstab                      ","Backstab",					27,	15,		1,	 2,		1,	5,	35,		SKILL_BACKSTAB, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_SNEAK_ATTACKS, 1 },
{ "        Circle                     ","Circle",					27,	25,		1,	 2,		1,	25, 120,	SKILL_CIRCLE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_BACKSTAB, 1 },
{ "           Rupture                 ","Rupture",					27,  25,	1,	15,		1,	39, 150,	SKILL_RUPTURE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_CIRCLE, 1 },
{ "              Assassinate          ","Assassinate",			    27,  30,    1,  15,     1,  39, 100,	SKILL_ASSASSINATE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_RUPTURE, 1 },
// TREE 28 - SNIPER TREE
// TREE 29 - TERRORIST TREE
// TREE 30 - SPY TREE
// TREE 31 - SABOTEUR TREE
// TREE 32 - STRIKER TREE
// TREE 33 - ASSASSIN TREE

// Merc 0 -- Crazy -- 1 Stalker 2 -- Borg 3 -- Highlander 4 -- Pred 5 -- Caller 6
//                                                                              Tree    Prac	Tier   Specific Sk1Ps0  Lvl     mv-psi 
//{ "  Mutilate                         ","Mutilate",				62,	20,	1,	 5,	1,	15,	45,		SKILL_MUTILATE, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },
// TREE 34 - BORG GENERAL
{ "+ Internal Weapons Systems         ","Internal Weapons Systems",		34,  	2,	1,	 3,		1,  5,	2,		SKILL_INTERNAL_WEAPONS, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "     Electrify                     ","Electrify",				34,	15,	1,	 3,		1,	5,	45,		SKILL_ELECTRIFY, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_INTERNAL_WEAPONS, 1 },
{ "        Optic Blast                ","Optic Blast",				34,	20,	1,	 3,		1,	30, 120,	SKILL_OPTICBLAST, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_ELECTRIFY, 1 },
{ "  Query                            ","Query      ",				34,  	20,	1,	 3,	1,  	10,	50,		SKILL_QUERY, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
{ "  Reboot                           ","Reboot",				34,	20,	1,	 3,	1,	20, 	50,		SKILL_REBOOT, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },

// TREE 35 - DRONE TREE
// TREE 36 - ASSIMILATOR TREE
// TREE 37 - GUARDIAN TREE
// TREE 38 - DESTROYER TREE
// TREE 39 - JUGGERNAUT TREE
// TREE 40 - PANZER TREE
// TREE 41 - CALLER GENERAL - CHARM SKILLI TREE
{ "+ Charm                            ","Charm",					41,	20,		1,	 6,		0,	1,	85,		PSIONIC_CHARM, POS_STANDING , TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, PSI_MANUAL, "You take orders from no one now.", NULL, -1, -1 },	
{ "     Loyalty                       ","Loyalty",					41,	20,		1,	 6,		1,	5,	100,	SKILL_LOYALTY, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, PSIONIC_CHARM, 0 },
{ "        Whistle                    ","Whistle",					41,	20,		1,	 6,		1,	8,	200,	SKILL_WHISTLE, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, SKILL_LOYALTY, 1 },
{ "           Callpet                 ","Callpet",					41,	10,		1,	 6,		0,	8,	100,	PSIONIC_CALLPET, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, "!Callpet!", NULL, SKILL_WHISTLE, 1 },
{ "  Lightning Flash                  ","Lightning Flash",			41,	20,		1,	 6,		0,	18, 100,	PSIONIC_LIGHTNING_FLASH, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, "!Lightning Flash!", NULL, -1, -1 },
{ "  Tigerclaw                        ","Tigerclaw",				41,	20,		1,	 6,		0,	6,	80,		PSIONIC_TIGERCLAW, POS_FIGHTING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_MANUAL, "!Tigerclaw!", NULL, -1, 1 },
{ "  Wolf Sense                       ","Wolf Sense",				41,	15,		1,	-1,		0,	25, 200,	PSIONIC_WOLF_SENSE, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel your animal senses fade.", NULL, -1, 1 },
{ "  Puma Speed                       ","Puma Speed",				41,	15,		1,	-1,		0,	25, 200,	PSIONIC_PUMA_SPEED, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel your animal senses fade.", NULL, -1, 1 },
{ "  Bear Rage                        ","Bear Rage",				41,	15,		1,	-1,		0,	25, 200,	PSIONIC_BEAR_RAGE, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel your animal senses fade.", NULL, -1, 1 },

// TREE 42 - FERAL TREE
// TREE 43 - HUNTER TREE
// TREE 44 - BEASTMASTER
// TREE 45 - SURVIVALIST TREE
// TREE 46 - VANGUARD TREE
// TREE 47 - SHAMAN TREE
// TREE 48 - CRAZY GENERAL
{ "  Invigorate                       ","Invigorate",				48,	20,		1,	 1,		0,	35, 100,	PSIONIC_INVIGORATE, POS_STANDING, TAR_CHAR_ROOM | TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You feel less invigorated.", NULL, -1, -1 },
{ "  Therapy                          ","Therapy",					48,	30,		1,	 1,		0,	37, 100,	PSIONIC_THERAPY, POS_STANDING, TAR_SELF_ONLY | TAR_CHAR_SELF, FALSE, PSI_POINTS, "!Therapy!", NULL, -1, -1 },
{ "  Purify                           ","Purify",					48,	10,		1,	 1,		0,	16,	100,	PSIONIC_PURIFY, POS_STANDING, TAR_OBJ_INV, FALSE, PSI_ALTER_OBJS, "!Purify!", NULL, -1, -1 },
// TREE 49 - ANARCHIST TREE
// TREE 50 - PSIONICIST TREE
// TREE 51 - PSYCHOTIC TREE
// TREE 52 - SUMMONER TREE
// TREE 53 - ELEMENTALIST TREE
// TREE 54 - TECHNOMANCER TREE
// TREE 55 - HIGHLANDER GENERAL
{ "  Leap Attack                      ","Leap attack",				55,	20,		1,	 4,		1,	22, 100,	SKILL_LEAP_ATTACK, POS_STANDING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, -1 },
// TREE 56 - KNIGHT TREE
// TREE 57 - BLADEMASTER TREE
// TREE 58 - REAVER
// TREE 59 - BLADESINGER
// TREE 60 - ARCANE
// TREE 61 - BARD

// Merc 0 -- Crazy -- 1 Stalker 2 -- Borg 3 -- Highlander 4 -- Pred 5 -- Caller 6
//                                                                              Tree    Prac	Tier   Specific Sk1Ps0  Lvl     mv-psi 
// TREE 62 - PREDATOR GENERAL
//{ "  Mutilate                         ","Mutilate",				62,	20,	1,	 5,	1,	15,	45,		SKILL_MUTILATE, POS_FIGHTING, TAR_IGNORE, TRUE, PSI_MANUAL, NULL, NULL, -1, 1 },

// TREE 63 - YOUNGBLOOD
// TREE 64 - WEAPONMASTER
// TREE 65 - ELDER
// TREE 66 - BADBLOOD
// TREE 67 - PREDALIEN
// TREE 68 - MECH
// TREE 69 - ELEMENTAL PROTECTION
{ "  Slow Poison                      ","Slow Poison",				69, 15,		1,  -1,     0,   6,  30,    PSIONIC_SLOW_POISON, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to poison lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Neutralize Drug                  ","Neutralize Drug",			69, 15,		1,  -1,     0,   8,  30,    PSIONIC_NEUTRALIZE_DRUG, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to drugs lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Heat Shield                      ","Heat Shield",				69, 20,		1,  -1,     0,  10,  30,    PSIONIC_HEAT_SHIELD, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to the heat lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Cold Shield                      ","Cold Shield",				69, 20,		1,  -1,     0,  14,  30,    PSIONIC_COLD_SHIELD, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to the cold lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Dissipate                        ","Dissipate",				69, 20,		1,  -1,     0,  18,  30,    PSIONIC_DISSIPATE, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to electricity lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Magnetic Field                   ","Magnetic Field",			69, 25,		1,  -1,     0,  24,  30,    PSIONIC_MAGNETIC_FIELD, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to explosives lower slightly as the psionic wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Psionic Shield                   ","Psionic Shield",			69, 25,		1,  -1,     0,  30,  30,    PSIONIC_PSIONIC_SHIELD, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences to psionics lower slightly as your psionic shield wears off.", NULL, SKILL_ELEMENTAL_LORE, 1 },
{ "  Spirit Ward                      ","Spirit Ward",				69, 30,		1,  -1,     0,  36,  30,    PSIONIC_SPIRIT_WARD, POS_STANDING, TAR_CHAR_ROOM, FALSE, PSI_AFFECTS, "Your defences toward ethereal lower slightly as your spirit ward dissipates.", NULL, SKILL_ELEMENTAL_LORE, 1 },

// LAST TREE - UNLEARNABLE SKILL TREE
{ "+ Resurrection Fatigue             ","Resurrection Fatigue",     70,  99,    -1,   1,     0,  45, 999,	PSIONIC_RESURRECTION_FATIGUE, POS_FIGHTING, TAR_CHAR_ROOM, TRUE, PSI_AFFECTS, "Your infections from resurrection have subsided.", NULL, PSIONIC_RESURRECTION_FATIGUE, 0 },
{ "","Psychic Static",				70,	-1,		1,	-1,		0,	45,	1,		PSIONIC_PSYCHIC_STATIC, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "!Psychic Static!", NULL, -1, 0 },
{ "","Frag Grenade",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_FRAG_GRENADE, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, NULL, NULL, -1, 0 },
{ "","Plasma Grenade",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_PLASMA_GRENADE, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, NULL, NULL, -1, 0 },
{ "","Thermal Grenade",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_THERM_GRENADE, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, NULL, NULL, -1, 0 },
{ "","Psionic Mirror",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_PSI_MIRROR, POS_STANDING, TAR_CHAR_ROOM |TAR_CHAR_SELF, FALSE, PSI_AFFECTS, "You are no longer protected from violent psionics!", NULL, -1, 0 },
{ "","Fire",						70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_FIRE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Smoke",						70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_SMOKE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Lag",							70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_LAG, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Psionic Channel",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_PSIONIC_CHANNEL, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Flash Grenade",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_FLASH_GRENADE, POS_STANDING, TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, PSI_DAMAGE, NULL, NULL, -1, 0 },
{ "","Ultra Stun",					70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_ULTRA_STUN, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Gas Grenade",					70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_GAS_GRENADE, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Identify",					70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_IDENTIFY, POS_STANDING, TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 },
{ "","Alien Warfare",				70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_DG_AFFECT, POS_STANDING, TAR_IGNORE, TRUE, 0, NULL, NULL, -1, 0 },
{ "","Turret",						70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_TURRET, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, "Test", NULL, -1, 0 },
{ "","Bleed",						70,	-1,		1,	-1,		0,	45, 1,		PSIONIC_BLEED, POS_STANDING, TAR_IGNORE, FALSE, PSI_MANUAL, NULL, NULL, -1, 0 }
};

// COMBO CODE - GAHAN - 2013

const struct combo_type combo[] = {
	{ 0,	"", "", "",		{999,999,999,999,999}},
	{ 1,	"@GYou end your series of attacks by slashing $N hard with your unsheathed claws!@n", 
			"@GStaggering beneath $N's assault, you do not see the claw strike coming in time to dodge it!@n", 
			"@G$n ends $s series of brutal attacks on $N by driving $s unsheathed claws into $M!@n",		
			{SKILL_SLASH,	SKILL_STUN,			SKILL_BACKSTAB,		0,					0}},
	{ 2,	"@GYou come out of your charge, circle behind $N, and score several deep cuts with\nyour unsheathed claws!@n", 
			"@G$N suddenly circle behind you, and before you can turn to face $M, you feel\nfiery pain traveling down your spine as deadly claws strike!@n", 
			"@GAt the end of $s charge, $n nimbly steps behind $N, unsheaths\n$s's claws and executes a series of painful strikes!@n",		
			{SKILL_CHARGE,		SKILL_CIRCLE,		SKILL_BACKSTAB,		0,					0}},
	{ 3,	"@GCircling one last time, you melt into the shadows! Reappearing right before $N!\nYou thrust your weapon into $M, dead center!@n",
			"@G$N suddenly vanishes from view as $M becomes one with the shadows!\nBefore you know what's happening, $s steps in close and you feel $S's\nweapon stab deep into your body!@n", 
			"@GVanishing from view, $n becomes one with the shadows!  $N shutters\nviolently as $n reappears and embeds $s weapon deeply into $S center mass!@n",		
			{SKILL_CIRCLE,		SKILL_STUN,			SKILL_BACKSTAB,		0,					0}},
	{ 4,	"@GCapitalizing on the surprising series of attacks, you execute a series of slashes\nwith your bionic blade, scoring multiple hits!@n", 
			"@GYou have no time to defend yourself as $N slashes madly again and again!@n", 
			"@GNot letting up on $s attack, $n presses forward at $N with a series of mad slashes!@n",
			{SKILL_SLASH,		SKILL_CHARGE,		SKILL_AMBUSH,		0,					0}},
	{ 5,	"@GYou cap off your stunning ambush with a series of lightning quick strikes that nearly destroys $N!@n", 
			"@GUnable to react, you take the full force of $N's stunning series of strikes!@n", 
			"@G$N appears too dazed to react as $n rips into $M with a stunning series of strikes!@n",		
			{SKILL_CIRCLE,		SKILL_STUN,			SKILL_AMBUSH,		0,					0}},
	{ 6,	"@GYou turn and charge again, not giving $N time to recover from your stunning attacks!@n", 
			"@GToo stunned to react, you stand there like a dummy as $N turns and charges again!@n", 
			"@GReversing direction, $n charges $N a second time while $e is too stunned to avoid it!@n",
			{SKILL_CHARGE,		SKILL_STUN,			SKILL_AMBUSH,		0,					0}},
	{ 7,	"@GLanding atop $N, you let your berserker rage take hold of you and begin\nmercilessly ripping into $M!@n", 
			"@GYou are driven to the ground by $N's raging attacks!@n", 
			"@G$n rides $N all the way to the ground after landing, mercilessly ripping and\ntearing all the way down!@n",		
			{SKILL_BERSERK,		SKILL_THROW,		SKILL_LEAP_ATTACK,	0,					0}},
	{ 8,	"@GYou finish up your combo by giving your blade a vicious twist before ripping it free!@n", 
			"@GYou feel $N's blade twisting in your guts before ripping free!@n", 
			"@G$n finishes $s combo by twisting $s blade viciously before ripping it free of $N!@n",		
			{SKILL_BERSERK,		SKILL_STUN,			SKILL_SLASH,		0,					0}},
	{ 9,	"@GYou grin at a stunned $N as you pour your rage into prolonging the energy\nblast which engulfs $M!@n", 
			"@GThe last thing you see is $N's grinning visage before $S\nsupercharged blast washes over you!@n", 
			"@G$n grins as $e delivers a supercharged blast at $N, bio-matters\nand flames trailing behind the attack!@n",
			{SKILL_BERSERK,		SKILL_BLAST,		SKILL_STUN,			0,					0}},
	{ 10,	"@GLightning crackles around you as you slams bodily into $N!@n", 
			"@GLightning crackels around $N as $E slams into you!@n", 
			"@GLightning crackles around $n as $e charges into $N, smashing\nthrough all $S defenses!@n",		
			{SKILL_THROW,		SKILL_LEAP_ATTACK,	SKILL_CHARGE,		0,					0}},
	{ 11,	"@GYour legs lash out in a series of devistating kicks as you land, your heels impacting\n$N again and again!@n", 
			"@G$N hits you with a series of rapid firing kicks as $E lands, driving you back!@n", 
			"@G$n lands upon $N with a series of rapid firing kicks too fast for your eyes to follow!@n",
			{SKILL_THROW,		SKILL_SLASH,		SKILL_LEAP_ATTACK,	0,					0}},
	{ 12,	"@GYou salute $N with your blaster before blasting $M again at point blank!@n", 
			"@GLifting $S blaster in a salute, $N aims and blasts you point blank!@n", 
			"@GSaluting $N with $s blaster, $n delivers a final blast at point blank!@n",		
			{SKILL_THROW,		SKILL_DISARM,		SKILL_BLAST,		0,					0}},
	{ 13,	"@GEasily dodging $N's attack, you rush in, claws extended, and tear into $M like a\nwolverine on steroids!@n", 
			"@GBefore you can recover from your failed attack, $N rushes you with claws extended,\nand you find your self torn apart by the assault!@n", 
			"@GSidestepping $N's attack, $n rushes in with claws extended, and begins shredding\n$N with a series of wicked slashes!@n",		
			{SKILL_CIRCLE,		SKILL_COVER_FIRE,	SKILL_BODY_BLOCK,	0,					0}},
	{ 14,	"@GSensing an opening, you dart in with a series of claw strikes before circling\nback out again!@n", 
			"@GAs your attention wanders, $N leaps at you out of nowhere with a series of claw\nstrikes!@n", 
			"@G$n darts in like a wolf sensing an opening, and after a series of claw strikes,\ncircles back out again to wait for the next opening!@n",		
			{SKILL_CIRCLE,		SKILL_FLANK,		SKILL_CHARGE,		0,					0}},
	{ 15,	"@GStepping around behind $N, you aim and deliver a blast so powerful that it throws\n $N forward!@n", 
			"@GBefore you can recover from the blast, $N circles behind you and you feel yourself\nthrown forward by another blast from behind!@n", 
			"@G$n circles behind $N and triggers a mighty blast that tosses $M forward like a rag doll!@n",		
			{SKILL_CIRCLE,		SKILL_SLASH,		SKILL_BLAST,		0,					0}},
	{ 16,	"@GBefore $N can recover from your stunning ambush, you leap in and begin to pommel\n$M with a storm of punches and kicks!@n", 
			"@GYou are stunned by the ambush and can do nothing while being brutally pommeled by\n$N's combo!@n", 
			"@G$N shutters like a punching bag, too stunned to react to $n's storm of punches\nand kicks!@n",
			{SKILL_CHARGE,		SKILL_LEAP_ATTACK,	SKILL_STUN,			SKILL_AMBUSH,		0}},
	{ 17,	"@GSheets of lightning blast from your hands and eyes, and into $N as your core\nreaches critical heat!@n", 
			"@GSheets of lightning blast into you, leaping out of $N's hands and eyes!@n", 
			"@G$n begins to glow fiercely as sheets of lightning pour from $m and into $N!@n",
			{SKILL_CHARGE,		SKILL_ELECTRIFY,	SKILL_OPTICBLAST,	SKILL_AMBUSH,		0}},
	{ 18,	"@GRipping your blade free, you slash again at $N, and then a third time! Crouching\nlow, you uppercut $N, your blade ripping upward in an attempt to open $M up!@n", 
			"@GYou double over in pain from $n's brutal slashes, only to be lifted back up by $s\nfinal uppercut!@n", 
			"@G$n rips $s blade free before repeatedly slashing $N! The attack ends with a brutal\nuppercut, $s blade leading the way!@n", 	
			{SKILL_SLASH,		SKILL_BERSERK,		SKILL_STUN,			SKILL_BACKSTAB,		0}},
	{ 19,	"@GPsionic energy leaps from your unsheathed claws and slams into $N!@n",
			"@GA wave of psionic energy leaps from $N's claws and slams into you!@n",
			"@GA wave of psionic energy leaps from $n's unsheathed claws and blasts into $N!@n", 		
			{SKILL_CHARGE,		SKILL_SLASH,		SKILL_STUN,			SKILL_BACKSTAB,		0}},
	{ 20,	"@GA fiery blast of psi energy travels down your arm and through your blade, scorching\n$N from inside out!@n",
			"@GA fiery wave of psi power passes from $N's blade into your body, and you feel your\norgans burning!@n", 
			"@GSmokes pour from $N's body as fire blasts down $n's arm and blade, and sets $M ablaze!@n",
			{SKILL_STUN,		SKILL_AMBUSH,		SKILL_CHARGE,		SKILL_SLASH,		0}},
	{ 21,	"@GFocusing your berserker rage upon a single target, you unload your fury upon $N\nin a whirlwind of slashes and stabs!@n",
			"@G$N goes berserk and deals you a whirlwind of slashes!@n",
			"@GFocusing upon $N, the berserker that is $n unleashes a whirlwind of slashes and stabs!@n",
			{SKILL_STUN,		SKILL_LEAP_ATTACK,	SKILL_SLASH,		SKILL_BERSERK,		0}},
	{ 22,	"@GYou unleash your berserker rage upon $N and begin ripping into $M with lightning\nfast claw strikes!@n",
			"@GPain tear through you as a berserked $N's lightning quick claw strikes connect!@n",
			"@GWith an animal like roar, $n charges $N, $s claws slashing and dicing!@n",
			{SKILL_CIRCLE,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_BERSERK,		0}},
	{ 23,	"@GYou circle $N again, blasting $M from behind! Circling to the side, you score a\nnasty wound with your claws!@n",
			"@GYou are blasted forward from behind by $N! Before you can recover, you feel the\nsharp pain in your side as $S claws rip through you!@n",
			"@GCircling behind $N, $n blasts $M with $S own blaster! Circling to one side, $e\nlashes out with razor sharp claws, scoring yet another deep gash!@n",
			{SKILL_CIRCLE,		SKILL_BLAST,		SKILL_SLASH,		SKILL_CHARGE,		0}},
	{ 24,	"@GCharging up your blaster, you throw shots after shots into $N's flank!@n",
			"@GCatching an opening, $N charges in, throwing a series of blasts directed at your flank!@n",
			"@GMoving in from the flank, $n charges up $s blaster and throws a series of shots at $N!@n",
			{SKILL_THROW,		SKILL_FLANK,		SKILL_CHARGE,		SKILL_BLAST,		0}},
	{ 25,	"@GIgniting your nuclear powered core, you bodyslam $N, throwing $M back in a shower\nof dancing sparks!@n",
			"@GSparks leap and dance as $N bodyslams you, $S nuclear powered core glowing brightly!@n",
			"@G$n's power core glows brightly as $e bodyslams $N, tossing $M back in a shower of\ndancing sparks!@n",
			{SKILL_THROW,		SKILL_BODY_BLOCK,	SKILL_COVER_FIRE,	SKILL_OPTICBLAST,	0}},
	{ 26,	"@GCharging your psi power into your bladed hands, you let loose a pure blade of energy\nwhich scythes through $N!@n",
			"@GA blade of pure psi energy flies from $N's hands and scythes through your armor as\nif they're not there!@n",
			"@G$n's hands begins to glow as $e charges them up with psi power! Throwing $s's hands\nforward, $n unleashes a pure blade of energy which scythes cleanly through $N!@n",
			{SKILL_COVER_FIRE,	SKILL_CHARGE,		SKILL_STUN,			SKILL_CHARGE,		0}},
	{ 27,	"@GYou make a throwing motion, pouring your psionic powered rage into the gesture! A\n series of flaming playing cards form in the air and explode upon $N!@n",
			"@G$N gestures, and a set of flaming playing cards appears in the air before $M!\nBefore you have time to dodge, the cards explode into you with shocking force!@n",
			"@G$n gestures, and a set of flaming playing cards appears out of nowhere, hovering\nin midair! $N is blasted backward by the force as the cards explode upon $M!@n",
			{SKILL_COVER_FIRE,	SKILL_BLAST,		SKILL_DISARM,		SKILL_THROW,		0}},
	{ 28,	"@GDuplicates of your self descend upon a stunned $N and begin pommeling $M to a pulp!@n",
			"@GYou are stunned as duplicates of $N suddenly appear out of nowhere and descend upon\nyou with a storm of punches and kicks!@n",
			"@GDuplicate images of $n blink into existence and fall upon $N in a storm of punishing blows!@n",
			{SKILL_BLAST,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_STUN,			0}},
	{ 29,	"@GIce crystals form around your weapon, becoming ghostly copies that slash and\ntear as you continue to circle $N!@n",
			"@GIcy copies of $N's weapon form out of nowhere and rip into you from all directions!@n",
			"@GIce crystals form in the air around $N and create ghostly copies of $n's weapon!\nAs $n continues to circle, they rip into $N from all sides!@n",
			{SKILL_BLAST,		SKILL_CHARGE,		SKILL_STUN,			SKILL_CIRCLE,		0}},
	{ 30,	"@GYou leap straight up into the air before dropping upon $N blade first like\na falling star!@n",
			"@G$N leaps up and falls upon your head with $S blade leading the way!@n",
			"@G$n leaps high into the air before falling upon $N blade-first like a falling star!@n",
			{SKILL_DISARM,		SKILL_BERSERK,		SKILL_CHARGE,		SKILL_SLASH,		0}},
	{ 31,	"@GA shock of psi power shakes the universe as you release an ultimate combo!\nSeemingly everywhere at once, you blur in and out of this reality as your\npsi-charged claws take $N apart piece by piece!@n",
			"@GA shockwave travels through the universe as $N unleashes an ultimate combo!\nReality itself warps around $N as $E rips through you with psi-charged claws!@n",
			"@GA shockwave travels through the universe as $n unleashes an ultimate combo!\nSeemingly everywhere at once, $n blues through time and space, ripping $N apart piece by piece with psi-powered claws!@n",
			{SKILL_BLAST,		SKILL_CIRCLE,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_BACKSTAB}},
	{ 32,	"@GDark energy forms around you as you unleashes an ultimate technique! Dashing forward,\nyou impale $N on your claws before lifting $M up into the air with ridiculous ease!\nWith a bestial roar, you smash $N down as a massive wave of destructive energy\npasses into $M through your contact!@n",
			"@GDark energy forms around $N as an ultimate technique is unveiled! Not ducking in time,\nyou are impaled by $N's claws and lifted up into the air with ridiculous ease!\nYour atoms seem to be pulled apart as a wave of destruction passes into your body!@n",
			"@GDark energy forms around $n as an ultimate technique is unveiled! Dashing forward with\nincredible speed, $n impales $N on $s's claws and lifts $M up with ridiculous ease!\n$N almost seems to explode as a riptide of destruction passes into $M from $n's hand!@n",
			{SKILL_BERSERK,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_STUN,			SKILL_BACKSTAB}},
	{ 33,	"@GYour power cycles up to a crescendo as you executes an ultimate combo! Slashing\ndownward with your blade, you use your psi to create a pillar of black energy which engulfs $N!\nGhostly faces and skulls can be seen swimming within the energy construct\nbefore it finally fades!@n",
			"@GGigantic pressure builds around you and $N as an ultimate combo is successfuly\nexecuted! Slashing down with $S bionic blade, $N creates a massive pillar of energy which\nengulfs you! You feel thousands of blades and claws rip into you, through you, before\nthe tormenting energy finally fades away!@n",
			"@GGigantic pressure builds around $n as an ultimate combo is successfuly executed!\nSlashing downward, $n summons up a massive pillar of dark energy which engulfs $N!\nSkulls and ghostly faces swim in and out of focus within the energy construct, growing dimmer\nas the pillar fades away!@n",
			{SKILL_THROW,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_SLASH}},
	{ 34,	"@GA storm of razors is pulled into view as you successfuly perform an ultimate\ntechnique! Spinning around $N, the razor blades slash $M from all angles before vanishing from view!@n",
			"@GAs $N successfuly performs an ultimate technique, a storm of razor blades appears\naround you! You are slashed from all angles by the spinning blades before the storm vanishes from view!@n",
			"@GA storm of razors is called into existence as an ultimate technique is performed!\nSpinning around $N, the razor blades slash $M from all angles before vanishing from view!@n",
			{SKILL_CIRCLE,		SKILL_SLASH,		SKILL_CIRCLE,		SKILL_SLASH,		SKILL_CIRCLE}},
	{ 35,	"@GFocusing your energy, you prepare for the ultimate assault! Rolling beneath $N's\nguard, you blast upward with a rising slash that lifts $M into the air! Leaping after $N,\nyou slash across twice in mid-flight, stab your blade into $M, your other hand\nslash down with psi-charged claws! $N is tossed away from you by a massive explosion as\nyour strike connects!@n",
			"@G$N's eyes roll back as $E prepares for the ultimate assault! Before you can blink,\n$N rushes you, and you find yourself airborn while being slashed and diced as if in slow\nmotion! You don't even have time to scream before $N's psi-charged claws slam into\nyou, and a massive explosion tosses you head over heels away from $N!@n",
			"@G$n's eyes roll back as $e prepares for the ultimate assault! Darting in with\nincredible speed, $n blasts into $N with a rising slash that lifts $M into the air! Leaping after\n$N, $n slashes twice in mid-air before striking $M with psionic-powered claws!\nA massive explosion shakes the area as $N flies backward away from $n!@n",
			{SKILL_CIRCLE,		SKILL_COVER_FIRE,	SKILL_SLASH,		SKILL_CHARGE,		SKILL_CIRCLE}},
	{ 36,	"@GThe power of the arcane fills your body as you execute an ultimate attack!\n Time seems to slow down as you charge forward, and each step seems to be accompanied by a clap of\nthunder! Rising your blade as if to strike, you carve ancient symbols in the air before $N!\n The symbols for into a massive shape that rushes forward to devour $N!\nYou watch in satisfaction as the summoned power passes through $N in a ripping, tearing impact!@n",
			"@G$N's eyes roll back as $E invokes an ancient power! Time seems to come to a\n stop as $N charges toward you, thunderclaps following in $N's wake! The blade comes up, but instead\nof striking you, begins to carve a series of strange symbols in the air!\n The symbols form a horrific dragon shape that rushes towards you! Every nerve ending in\nyour body shrieks as the energy rips through your being!@n",
			"@G$n's eyes roll back as $e invokes an ancient power! Time seems to come to a\nstop as $n charges toward $N, thunderclaps following in $s wake! Stopping short of $N, $n's blade\nmoves in the air, forming strange symbols of power! A massive dragon shape rushes\n out of the runes and blasts through $N!@n",
			{SKILL_LEAP_ATTACK,	SKILL_SLASH,		SKILL_STUN,			SKILL_LEAP_ATTACK,	SKILL_CHARGE}},
	{ 37,	"@GThe metal in your blade screams in protest as you channel your psi power into\n it for the ultimate slash! Flames leap to life around your blade as you bring it down upon $N!\nSpinning, you sweep the flaming blade low, the fire whipping out in a wide arc!\nLunging forward, you impale $N, and the fire engulfs $M!@n",
			"@GThe blade in $N's hand begins to vibrate, and a strange shriek can be heard\nas $E prepares for an ultimate slash! Flames leap up around the blade as $N slashes at you several\ntime before impaling you! You are burning!@n",
			"@GThe blade in $n's hand begins to vibrate, and a strange shriek can be heard\nas $e prepares for an ultimate slash! Rushing in, $n slashes down before sweeping the blade wide, the\nfire whipping in an arc! Lunging, $n impales $N on the flaming blade, setting $M on fire!@n",
			{SKILL_LEAP_ATTACK,	SKILL_SLASH,		SKILL_BLAST,		SKILL_LEAP_ATTACK,	SKILL_SLASH}},
	{ 38,	"@GYour bionic blade begins to grow and change as you focus your psi for an\nultimate blow! A massive psi-constructed cleaver appear on the end of your arm, weightless and titanic!\nLeaping towards $N, you swing the psi-construct in a cleaving stroke!\nArmor begins to part as your cleave as the blade bites deep!@n",
			"@GYou watch, stunned, as the bionic blade in $N's hand begins to change shape!\nThe blade grows into a massive psionically constructed cleaver, and $N leaps forward while swinging it at you!\nYour armor is no defense against the weapon as it bites into your body deep!@n",
			"@G$n's bionic blade begins to grow and warp as $e pulls in all $s psi power for\nan ultimate blow! A massive psi-constructed cleaver forms on the end of $n's arm, and $e swings\nit hard while leaping at $N! The cleaving cut bites deep as $N's armor like wet paper\nbeneath a knife!@n",
			{SKILL_SLASH,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_AMBUSH}},
	{ 39,	"@GYou rear back, gathering your psionic energy and anger for one ultimate strike!\nA ring of energy forms around your uplifted fist, and you swing that fist forward in one simple\npunch- even though you're too far away to connect! A huge fist-shaped psi construct\nblast from your fist, and rockets towards $N! Your galactic phantom punch smashes into $N with enough force to shake the planet!@n",
			"@G$N rears back $S fist for an ultimate strike! A ring of psi energy surround $N's\nfist, and $E throws a punch towards you- clearly too far away to be of concern... You cringe as\na massive fist shaped psi construct, big as a train, rockets towards you from\n$N's fist! Unable to get out of the way, you feel as if you have been reduced to individual atoms as the punch connects!@n",
			"@G$n rears back $s fist for an ultimate strike! A ring of psi energy surround $n's\nfist, and $e throws a punch towards $N- clearly too far away land the hit... A massive fist shaped\npsi construct, as large as a train, rockets from $n's fist and blasts into\n$N with enough force to shake the planet!@n",
			{SKILL_SLASH,		SKILL_SLASH,		SKILL_DISARM,		SKILL_THROW,		SKILL_AMBUSH}},
	{ 40,	"@GYour blast opens up a rift in space, right before $N! Something huge and\nserpentine darts from the rift and grabs ahold of $N, attempting to pull $M through to\nthe other side! A flash of lightning sparks from the rift, and the creature retreats,\nleaving a badly injured $N behind!@n",
			"@G$N's blast appears to have triggered a space/time portal as a rift opens up\n before you! A massive serpentine shape darts out, its toothy maw closing upon you and\npulls you towards the space beyond! Just as you think all is lost, a flash of lightning\nsparks from the rift, and the creature drops you and retreats!@n",
			"@G$n's blast appears to have triggered a space/time portal as a rift opens up\n before $N! A massive pit-worm darts out of the rift, its toothy maw closing upon $N,\npulling $M back towards certain doom! A flash of lightning sparks from the rift, and\nthe creature suddenly retreats, leaving a badly damaged $N behind!@n",
			{SKILL_COVER_FIRE,	SKILL_BODY_BLOCK,	SKILL_STUN,			SKILL_CHARGE,		SKILL_BLAST}},
	{ 41,	"@GYou send out a signal, and light years away, an assault platform swivels its\nmany guns towards your enemy! A salvo of plasma fire rains from space, obliterating $N!@n",
			"@GSomething bleeps within $N's body, and you get a very bad feeling! Out of the\nclear blue sky, a salvo of plasma fire blasts into you, nearly melting you down to slag!@n",
			"@GA salvo of plasma fire blasts down from the heaven as a killer satelite homes\nin upon $N!@n",
			{SKILL_COVER_FIRE,	SKILL_BLAST,		SKILL_ELECTRIFY,	SKILL_CHARGE,		SKILL_BERSERK}},
	{ 42,	"@GYou grin as you execute your ultimate combo!\nRiding your berserker rage, you lift a stunned $N up and tosses $M straight up into the air!\nLeaping after $M, you slam your body into $S in mid-air! Kicking $N away from\nyou, you blast $M with a bolt of psi energy strong enough to vaporize rock!@n",
			"@G$N grins as $E execute $S ultimate technique!\nBefore you can recover from the stun, you are thrown into the air, $N leaping after you!\n You feel the massive impact as $N slams into you before booting you away like a football!\nA shocking bolt of psi energy nearly vaporizes you as you fall back down\nin a battered heap!@n",
			"@G$n grins as $e execute $s ultimate technique!\nGrabbing a stunned $N, $n tosses $M high into the air before leaping up after $M!\nA massive impact can be felt as the two slam together, and then $n brutally kicks $N,\nsending $M tumbling away from $n! A killing bolt of psi energy blasts from $n's\nhand and obliterates $N!@n",
			{SKILL_BODY_BLOCK,	SKILL_STUN,			SKILL_CHARGE,		SKILL_BERSERK,		SKILL_CHARGE}},
	{ 43,	"@GCatching an opening, you charge in at $N, pulling a fragmentation mine from your pocket!\nSlapping the mine onto $N, you shout a warning to your team mates before backing away.\nGrinning, you depress the detonator and have the satisfaction of seeing the mine explode,\nshowering $N with a storm of jagged fragments!@n",
			"@GYou feel a smack as $N darts in from the side. $N shouts something to $S team,\nand you watch, confused, as they back away from you...\nYour world suddenly turns into a world of pain as something explodes on your back!\nThousands of jagged fragments tear into you, ripping you apart!@n",
			"@G$n darts in at $N, slapping something that sticks to $S back! 'FIRE IN THE\nHOLE!' $n shouts, and you quickly back out of the way.\nGrinning, $n depresses the detonator, and the fragmentation mine attached to $N\nexplodes in a storm of jagged fragments!@n",
			{SKILL_STUN,		SKILL_FLANK,		SKILL_CHARGE,		SKILL_DISARM,		SKILL_BERSERK}},
	{ 44,	"","","",		{SKILL_STUN,		SKILL_LEAP_ATTACK,	SKILL_OPTICBLAST,	SKILL_LEAP_ATTACK,	SKILL_DISARM}},
	{ 45,	"@GCharging up your psi power, you throw a ethereal harpoon at $N!\nThe barbs on the harpoon bite deep, and you make a sharp gesture as if\nreeling a fish in! $N is pulled towards you, right into your follow up psionic\nbolt which sends $M crashing down in a heap!@n",
			"@GA strange ethereal harpoon spears you, thrown by $N!\nYou struggle mightily, but are reeled in like a fish on a line!\nThe last thing yousee is $N's grim visage before $S psionic bolt blasts you back!@n",
			"@G$n makes an odd gesture and throws an ethereal harpoon at $N!\nThe harpoon spears into $N's side, and $n proceeds to reel $M in!\n$N is pulled right into $n's psionic bolt which blasts $M back to land in a heap!@n",
			{SKILL_FLANK,		SKILL_CHARGE,		SKILL_DISARM,		SKILL_CHARGE,		SKILL_THROW}},
	{ 46,	"@GYou grin as you execute your ultimate technique! Charging past $N, you leap\nonto $S back, driving down with all your strength! Riding $N down, you rise your bionic\nblade and plunges it in again and again, laughing madly while doing so!\nYanking your blade free, you channel your psi into your fist and smash it down\nupon $N in a fountain of black flames!@n",
			"@GGrinning madly, $N charges and takes you to the ground! Unable to get up, you\nfeel $N's weight on your back and you scream as the bionic blade stabs in again and again!\nRearing back, $N smashes down with a psi charged fist, and black flames devour you whole!@n",
			"@GCharging past $N, $n spins and leaps onto $N's back! Riding $M down, $n plunges\n$s bionic blade in again and again, laughing madly! Retracting the blade, $n rears up,\npsi energy glowing around $s fist as $e brings it crashing down upon $N!\nA fountain of black flames devours $N!@n",
			{SKILL_CHARGE,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_CHARGE}},
	{ 47,	"@GYour body begins to glow a bright white as you charge at $N, your psi power\nprimed for the ultimate assault! Electric tendrils lash out around you as you slam\ninto $N, causing a massive electric storm to build up around you two!\n$N jitters madly as the bolts of energy wash over $M!@n",
			"@G$N suddenly begins to glow a bright white, tendrils of lightning lashing out\nfrom $S body! Charging directly in your line, $N slams into you, and you jitter\nand dance as the electrical storm washes over you!@n",
			"@G$n begins to glow a bright white as $e charges $N! Tendrils of electricity\nlash around $m as $e slams into $N! A massive electrical storm washes over $N,\ncausing $M to jitter and shake!@n",
			{SKILL_CHARGE,		SKILL_BLAST,		SKILL_CHARGE,		SKILL_FLANK,		SKILL_CHARGE}},
	{ 48,	"@GYou slash upward, opening $N up like a can of tuna!\n Leaping up high, you stab down, piercing $N from above!\nKicking $M over, you aim carefully and open up a neat gash with your blade!\nGrinning, you prime your blaster, reach inside the hole you just made, and fire!@n",
			"@G$N disembowels you with a low slash!\nLeaping up, $E drops on top of you, the blade piercing you from above!\nA sweeping kick knocks you down, and you scream as $N opens you up from behind!\nGrinning madly, $N primes $S blaster and fires a shot right into your wound!@n",
			"@G$n's first slash nearly cored $N like an apple!\n Leaping up high, $n nails $N with a downward stab!\nKicking $M over, $n cuts a neat hole in $N's back with $s blade before\nfiring a fatalshot right into the wound!@n",
			{SKILL_SLASH,		SKILL_BLAST,		SKILL_SLASH,		SKILL_BLAST,		SKILL_SLASH}},
	{ 49,	"@GGathering your energy, you punch forward, your blade extended!\nYour arm sinks into $N up to the elbow, and you begin tearing and ripping from the inside,\ncausing massive damage!@n",
			"@GWith a powerful punch, $N sinks $S arm up to the elbow in your guts!\nAgony grips you as $N twists $S hand, the blade ripping and tearing from the inside!@n",
			"@GLunging in, $n punches forward, bionic blade extended!\n$n's arm sinks into $N up to the elbow, causing massive damage!@n",
			{SKILL_SLASH,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_CHARGE,		SKILL_SLASH}},
	{ 50,	"@GYou rush $N and begin raining thounderous blows upon $M!\nAfter pommeling $N for awhile, you rear back, before smashing forward with a punch that'd stop a train!\n$N is sent flying!@n",
			"@GRushing you madly, $N rains thunderous blows down upon you\nthe hits too hard and fast to block! Suddenly, the impacts stop coming, and you look up just in time to\nsee $N's psi-charged fist coming! The fist hits you with the force of a moving train, sending you flying back!@n",
			"@G$n rushes $N, raining thunderous blows down upon $M! After\npommeling $N for awhile, $n rears back and delivers a punch strong enough to stop a train!\n$N is sent flying!@n",
			{SKILL_BLAST,		SKILL_CHARGE,		SKILL_SLASH,		SKILL_DISARM,		SKILL_BERSERK}},
};

// COMBO COMMAND - GAHAN 2013

ACMD(do_combo) {
	int i,j,z,skillnum;
	send_to_char(ch, "You know the following combos:\r\n");
	
	for (i=0;i<MAX_COMBOLEARNED;i++) {
		j = (MAX_COMBOLENGTH -1);
		if (GET_COMBO_LEARNED(ch, i)) {
			send_to_char(ch, "%d ) |", i+1);
			z = combo[GET_COMBO_LEARNED(ch, i)].id;
			for (j=(MAX_COMBOLENGTH -1);j >= 0;j--) {
				if (combo[z].combo[j] > 0) {
					skillnum = combo[z].combo[j];
					send_to_char(ch, " %s |", skills_info[skillnum].name);
				}
			}
			send_to_char(ch, "\r\n");
		}
	}
	send_to_char(ch, "\r\nThe last combo moves used were:\r\n\r\n");
	for (i=0;i<MAX_COMBOLENGTH;i++)
		if (COMBOCOUNTER(ch, i))
			send_to_char(ch, "%s\r\n", skills_info[COMBOCOUNTER(ch, i)].name); 
		
}

