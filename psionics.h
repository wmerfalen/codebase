/**
* @file psionics.h
* Constants and function prototypes for the psionic system.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/
#ifndef _PSIONICS_H_
#define _PSIONICS_H_

#define DEFAULT_STAFF_LVL       12
#define DEFAULT_WAND_LVL        12

#define CAST_UNDEFINED          (-1)
#define CAST_PSIONIC            0
#define CAST_POTION             1
#define CAST_WAND               2
#define CAST_STAFF              3
#define CAST_SCROLL             4

#define PSI_DAMAGE          (1 << 0)
#define PSI_AFFECTS         (1 << 1)
#define PSI_UNAFFECTS       (1 << 2)
#define PSI_POINTS          (1 << 3)
#define PSI_ALTER_OBJS      (1 << 4)
#define PSI_GROUPS          (1 << 5)
#define PSI_MASSES          (1 << 6)
#define PSI_AREAS           (1 << 7)
#define PSI_SUMMONS         (1 << 8)
#define PSI_CREATIONS       (1 << 9)
#define PSI_MANUAL          (1 << 10)

// This allows us to pass a level into psi_affects to force psionic to be indefinite
#define EQ_PSI_LEVEL          6

#define TYPE_UNDEFINED           (0)
#define PSIONIC_RESERVED_DBC     0      /* PSIONIC NUMBER ZERO -- RESERVED */


// PLAYER PSIONICS -- Numbered from 1 to MAX_PSIONICS
// If you change these, make sure psionic_costs array in spec_procs.cmatches indicies
#define PSIONIC_ARMORED_AURA                  1
#define PSIONIC_ASTRAL_PROJECTION             2
#define PSIONIC_AURA_OF_COOL                  3
#define PSIONIC_BIO_REGEN                     4
#define PSIONIC_BIO_TRANCE                    5
#define PSIONIC_BLINDNESS                     6
#define PSIONIC_BLIND_IMMUNE                  7
#define PSIONIC_CHARM                         8
#define PSIONIC_COMBAT_MIND                   9
#define PSIONIC_COMMUNE                       10
#define PSIONIC_CONJURE                       11
#define PSIONIC_COURAGE                       12
#define PSIONIC_DEGRADE                       13
#define PSIONIC_DETECT_INVISIBILITY           14
#define PSIONIC_DETOX                         15
#define PSIONIC_COUNTER_SANCT                 16
#define PSIONIC_COUNTER_SUPER_SANCT           17
#define PSIONIC_DISRUPTION                    18
#define PSIONIC_DUPLICATES                    19
#define PSIONIC_ELECTROKINETIC_DISCHARGE      20
#define PSIONIC_FANATICAL_FURY                21
#define PSIONIC_FARSEE                        22
#define PSIONIC_GROUP_HEAL                    23
#define PSIONIC_GROUP_SANCT                   24
#define PSIONIC_HASTE                         25
#define PSIONIC_HEAL                          26
#define PSIONIC_IMBUE_ESSENCE                 27
#define PSIONIC_INDESTRUCTABLE_AURA           28
#define PSIONIC_INDUCE_SLEEP                  29
#define PSIONIC_INSPIRE                       30
#define PSIONIC_INVIGORATE                    31
#define PSIONIC_INVISIBILITY                  32
#define PSIONIC_LETHARGY                      33
#define PSIONIC_LETHARGY_IMMUNE               34
#define PSIONIC_LEVITATE                      35
#define PSIONIC_LOCATE_OBJECT                 36
#define PSIONIC_THERAPY                       37
#define PSIONIC_MEGA_DEGRADE                  38
#define PSIONIC_MEGA_COUNTER                  39
#define PSIONIC_MEGA_SANCT                    40
#define PSIONIC_MEGAHEAL                      41
#define PSIONIC_MYSTIC_BULB                   42
#define PSIONIC_NOVA                          43
#define PSIONIC_PACIFY                        44
#define PSIONIC_PANIC                         45
#define PSIONIC_PARALYZE                      46
#define PSIONIC_PETRIFY                       47
#define PSIONIC_POISON                        48
#define PSIONIC_PORTAL                        49
#define PSIONIC_PYROKINETIC_ERUPTION          50
#define PSIONIC_PSI_HAMMER                    51
#define PSIONIC_PSI_MIRROR                    52
#define PSIONIC_PSI_SHIELD                    53
#define PSIONIC_PSI_SHACKLE                   54
#define PSIONIC_PSYCHIC_LEECH                 55
#define PSIONIC_PSYCHIC_STATIC                56
#define PSIONIC_PSYCHIC_SURGERY               57
#define PSIONIC_REMOVE_SHACKLE                58
#define PSIONIC_RESILIANCE                    59
#define PSIONIC_RESIST_FATIGUE                60
#define PSIONIC_RESTORE_SIGHT                 61
#define PSIONIC_REVEAL_LIFE                   62
#define PSIONIC_SANCTUARY                     63
#define PSIONIC_SECOND_SIGHT                  64
#define PSIONIC_SENSE_LIFE                    65
#define PSIONIC_SNUFF_LIFEFORCE               66
#define PSIONIC_SUBLIMINAL_SUGGESTION         67
#define PSIONIC_SUMMON                        68
#define PSIONIC_SUPER_NOVA                    69
#define PSIONIC_SUPER_SANCT                   70
#define PSIONIC_TELEPORT                      71
#define PSIONIC_THERMAL_AURA                  72
#define PSIONIC_TRANSMUTE                     73
#define PSIONIC_VORPAL_SPEED                  74
#define PSIONIC_WEAKEN                        75
// room psionics
#define PSIONIC_FIRE                          76
#define PSIONIC_SMOKE                         77
#define PSIONIC_LAG                           78

#define PSIONIC_UNUSED1                       79
#define PSIONIC_UNUSED2                       80

// special psionics
#define PSIONIC_PSIONIC_CHANNEL               81
#define PSIONIC_FRAG_GRENADE                  82
#define PSIONIC_PLASMA_GRENADE                83
#define PSIONIC_THERM_GRENADE                 84
#define PSIONIC_FLASH_GRENADE                 85
#define PSIONIC_ULTRA_STUN                    86
#define PSIONIC_GAS_GRENADE                   87
#define PSIONIC_NAPALM                        88
#define PSIONIC_NUKEBLAST                     89
#define PSIONIC_CALLPET                       90
#define PSIONIC_PURIFY						  91

// continue the spells

#define PSIONIC_FIRE_BLADE                    92
#define PSIONIC_ICE_BLADE                     93
#define PSIONIC_BLEED						  94
#define PSIONIC_PSI_BULLET					  95
#define PSIONIC_LIGHTNING_FLASH				  96
#define PSIONIC_INFRAVISION					  97
#define PSIONIC_RESURRECTION_FATIGUE		  98
#define PSIONIC_SLOW_POISON					  99
#define PSIONIC_NEUTRALIZE_DRUG				 100
#define PSIONIC_HEAT_SHIELD					 101
#define PSIONIC_COLD_SHIELD					 102
#define PSIONIC_DISSIPATE					 103
#define PSIONIC_MAGNETIC_FIELD				 104
#define PSIONIC_PSIONIC_SHIELD				 105
#define PSIONIC_SPIRIT_WARD					 106

// Insert new psionics here, up to MAX_PSIONICS
// 2 spells below must not change and should not be assigned levels in 
// spec_procs.c skill or spell assign chart!
#define PSIONIC_IDENTIFY                     107
#define PSIONIC_DG_AFFECT                    108

#define PSIONIC_TIGERCLAW                    109
#define PSIONIC_WOLF_SENSE                   110
#define PSIONIC_PUMA_SPEED                   111
#define PSIONIC_BEAR_RAGE                    112

#define PSIONIC_TURRET                       113

// Total Number of defined psionics
#define NUM_PSIONICS                         114 // Always +1 from last psionic
#define TOP_PSIONIC_DEFINE                   114
#define TOP_MORT_PSIONICS                    114
#define TOP_IMMORT_PSIONICS                  114

// PLAYER SKILLS - Numbered from MAX_PSIONICS+1 to MAX_SKILLS
// If you change these, make sure skill_costs array in spec_procs.c matches indicies
#define SKILL_AMBUSH                        1
#define SKILL_ASSASSINATE                   2
#define SKILL_AUTO_WPN_PRO                  3
#define SKILL_BACKSTAB                      4
#define SKILL_BERSERK                       5
#define SKILL_BODY_BLOCK                    6
#define SKILL_CHARGE                        7
#define SKILL_COVER_FIRE                    8
#define SKILL_CRITICAL_KNOWLEDGE            9
#define SKILL_DEADLY_AIM                    10
#define SKILL_DODGE                         11
#define SKILL_DOUBLE_HIT                    12
#define SKILL_ENERGY_WPN_PRO                13
#define SKILL_EXPLO_WPN_PRO                 14
#define SKILL_FIRST_AID                     15
#define SKILL_FLANK                         16
#define SKILL_FOCUS                         17
#define SKILL_FUEL_WPN_PRO                  18
#define SKILL_GADGETRY                      19
#define SKILL_HIDE                          20
#define SKILL_MARKSMANSHIP                  21
#define SKILL_MEDITATE                      22
#define SKILL_MELEE_WPN_PRO                 23
#define SKILL_PICK_LOCK                     24
#define SKILL_PROTECT                       25
#define SKILL_RESCUE                        26
#define SKILL_ROCKET_WPN_PRO                27
#define SKILL_SHELL_WPN_PRO                 28
#define SKILL_SNEAK                         29
#define SKILL_SNIPER                        30
#define SKILL_STUN                          31
#define SKILL_TRACK                         32
#define SKILL_EXPLOSIVES                    33
#define SKILL_SLASH                         34
#define SKILL_BLAST							35
#define SKILL_RECON                         36
#define SKILL_LOYALTY                       37
#define SKILL_STRONGARM                     38
#define SKILL_MANIAC                        39
#define SKILL_SEARCH						40
#define SKILL_WHISTLE						41
#define SKILL_DISARM						42
#define SKILL_THROW							43
#define SKILL_RETREAT						44
#define SKILL_CIRCLE						45
#define SKILL_ELECTRIFY						46
#define SKILL_OPTICBLAST					47
#define SKILL_DRAIN							48
#define SKILL_LEAP_ATTACK					49		
#define SKILL_AIM							50
#define SKILL_SNEAK_ATTACKS					51
#define SKILL_COMBAT_TACTICS				52
#define SKILL_INTERNAL_WEAPONS				53
#define SKILL_PSI_ENHANCE					54
#define SKILL_RANGED_WPN_PRO				55
#define SKILL_RNGCOMBAT_TACTICS				56
#define SKILL_SPIRITUALITY					57
#define SKILL_AURA_PROJECTION				58
#define SKILL_ELEMENTAL_LORE				59
#define SKILL_DARK_FORCE					60
#define SKILL_SURVIVAL						61
#define SKILL_UTILITY						62
#define SKILL_DEFENSE_TACTICS				63
#define SKILL_DEBUFFING						64
#define SKILL_TELEKINESIS					65
#define SKILL_RUPTURE						66
#define SKILL_FRENZY						67
#define SKILL_BATTLERAGE					68
#define SKILL_DRAG							69
#define SKILL_PSIONICS_LORE					70
#define SKILL_ARCHERY_WPN_PRO				71
#define SKILL_BASH							72
#define SKILL_KICK							73
#define SKILL_STEAL							74
#define SKILL_BIOBLAST						75
#define SKILL_BIOSLASH						76
#define SKILL_DEFENSE_ROLL						77
#define SKILL_QUERY						78
#define SKILL_REBOOT						79
#define SKILL_EAGLE_EYE						80

// Total Number of defined psionics
#define NUM_SKILLS                          81
#define TOP_SKILL_DEFINE                    81
#define TOP_MORT_SKILLS                     81
#define TOP_IMMORT_SKILLS                   81
// Insert new psionics here, up to MAX_SKILLS
// NEW NPC/OBJECT PSIONICS can be inserted here up to 40

// NON-PLAYER AND OBJECT SKILLS: The practice levels for the skills 
// below are _not_ recorded in the players file; therefore, the 
// intended use is for skills associated with objects or non-players 

// WEAPON ATTACK TYPES
#define TYPE_HIT                    300
#define TYPE_STING                  301
#define TYPE_WHIP                   302
#define TYPE_SLASH                  303
#define TYPE_BITE                   304
#define TYPE_BLUDGEON               305
#define TYPE_CRUSH                  306
#define TYPE_POUND                  307
#define TYPE_CLAW                   308
#define TYPE_MAUL                   309
#define TYPE_THRASH                 310
#define TYPE_PIERCE                 311
#define TYPE_BLAST                  312
#define TYPE_PUNCH                  313
#define TYPE_STAB                   314
#define TYPE_BASH                   315
#define TYPE_SHOOT                  316
#define TYPE_EXPLOSIVE              317
#define TYPE_CHOP		            318
#define TYPE_FLAIL                  319
#define TYPE_SLICE                  320
#define TYPE_SPLAT                  321
#define TYPE_ATTACK                 322
#define TYPE_SCRATCH                323
#define TYPE_CLEAVE                 324
#define TYPE_HACK                   325
#define TYPE_SAW                    326
#define TYPE_GOUGE                  327
#define TYPE_GRIND                  328
#define TYPE_DRILL                  329
#define TYPE_MAIM                   330
#define TYPE_SLAP                   331
#define TYPE_KICK                   332

#define TYPE_MIN					300
#define TYPE_MAX					332
// The total number of attack types
#define NUM_ATTACK_TYPES            33

// new attack types can be added here - up to TYPE_SUFFERING
#define TYPE_SUFFERING             399

// These are the new damage types for saving throws
#define DMG_NORMAL					0
#define DMG_POISON					1
#define DMG_DRUG					2
#define DMG_FIRE					3
#define DMG_COLD					4
#define DMG_ELECTRICITY				5
#define DMG_EXPLOSIVE				6
#define DMG_PSIONIC					7
#define DMG_ETHEREAL				8

#define MAX_DMG						9

// Possible Targets:
// **  TAR_IGNORE    : IGNORE TARGET.
// **  TAR_CHAR_ROOM : PC/NPC in room.
// **  TAR_CHAR_WORLD: PC/NPC in world.
// **  TAR_FIGHT_SELF: If fighting, and no argument, select tar_char as self.
// **  TAR_FIGHT_VICT: If fighting, and no argument, select tar_char as victim (fighting).
// **  TAR_SELF_ONLY : If no argument, select self, if argument check that it IS self.
// **  TAR_NOT_SELF  : Target is anyone else besides self.
// **  TAR_OBJ_INV   : Object in inventory.
// **  TAR_OBJ_ROOM  : Object in room.
// **  TAR_OBJ_WORLD : Object in world.
// **  TAR_OBJ_EQUIP : Object held.
#define TAR_IGNORE              (1 << 0)
#define TAR_CHAR_ROOM           (1 << 1)
#define TAR_CHAR_WORLD          (1 << 2)
#define TAR_FIGHT_SELF          (1 << 3)
#define TAR_FIGHT_VICT          (1 << 4)
#define TAR_SELF_ONLY           (1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF            (1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV             (1 << 7)
#define TAR_OBJ_ROOM            (1 << 8)
#define TAR_OBJ_WORLD           (1 << 9)
#define TAR_OBJ_EQUIP           (1 << 10)
#define TAR_CHAR_SELF           (1 << 11)

#define NOT_REMORT              0
#define REMORT                  1

struct psionics_info_type 
{
    byte min_position;          /* Position for caster     */
    int psi;                    /* amount of psi used by a psionic (highest lev) */
    bool remort;
    int min_level;
    int routines;
    byte violent;
    int targets;                /* See below for use with TAR_XXX  */
    const char *name;           /* Input size not limited. Originates from string constants. */
    const char *wear_off_msg;   /* Input size not limited. Originates from string constants. */
	const char *rwear_off_msg;
	int prac_cost;
	int tree;
	int prereq;
	int prereqid;
	const char *display;
	int pcclass;
	int tier;
};

// todo: most of these fields are not currently used
struct skills_info_type 
{
    byte min_position;    /* Position for performer     */
    bool remort;
    int min_level;
    int routines;
    byte violent;
    int targets;         /* See below for use with TAR_XXX  */
    const char *name;           /* Input size not limited. Originates from string constants. */
    const char *wear_off_msg;   /* Input size not limited. Originates from string constants. */
	const char *rwear_off_msg;
	int prac_cost;
	int tree;
	int prereq;
	int prereqid;
	const char *display;
	int pcclass;
	int tier;
};

// Possible Targets:
//   bit 0 : IGNORE TARGET
//   bit 1 : PC/NPC in room
//   bit 2 : PC/NPC in world
//   bit 3 : Object held
//   bit 4 : Object in inventory
//   bit 5 : Object in room
//   bit 6 : Object in world
//   bit 7 : If fighting, and no argument, select tar_char as self
//   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
//   bit 9 : If no argument, select self, if argument check that it IS self. */
#define PSI_TYPE_PSIONIC    0
#define PSI_TYPE_POTION     1
#define PSI_TYPE_WAND       2
#define PSI_TYPE_STAFF      3
#define PSI_TYPE_SCROLL     4

#define APSI(psiname) void psiname(int psi_level, struct char_data *ch, struct char_data *victim, struct obj_data *obj)
#define MANUAL_PSI(psiname)    psiname(psi_level, ch, victim, obj);

APSI(psionic_astral_projection);
APSI(psionic_charm);
APSI(psionic_commune);
APSI(psionic_conjure);
APSI(psionic_detox);
APSI(psionic_duplicates);
APSI(psionic_farsee);
APSI(psionic_locate_object);
APSI(psionic_pacify);
APSI(psionic_callpets);
APSI(psionic_panic);
APSI(psionic_portal);
APSI(psionic_psychic_leech);
APSI(psionic_psychic_static);
APSI(psionic_reveal_life);
APSI(psionic_summon);
APSI(psionic_teleport);
APSI(psi_psionic_channel);
APSI(psi_flash_grenade);
APSI(psi_ultra_stun);
APSI(psi_gas_grenade);
APSI(psionic_identify);
APSI(psi_fire);
APSI(psi_smoke);
APSI(psi_turret);
APSI(psi_lag);
APSI(psi_fire_blade);
APSI(psi_ice_blade);
APSI(psionic_lightning_flash);
APSI(psionic_psi_bullet);
APSI(psionic_tigerclaw);

// basic psionic calling functions

int find_skill_num(char *name);
int find_psionic_num(char *name);

int psi_damage(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj);
void psi_affects(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, bool ack, struct obj_data *obj);

// todo: following removed?
void perform_group_psionic(int psi_level, struct char_data *ch, struct char_data *tch, int psi_num); 

void psi_groups(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj);
void psi_masses(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj);
void psi_areas(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj);
void psi_summons(int psi_level, struct char_data *ch, struct obj_data *obj, int psi_num);
void psi_regen(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj);
void psi_points(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj);
void psi_unaffects(int psi_level, struct char_data *ch, struct char_data *victim, int psi_num, struct obj_data *obj);
void psi_alter_objs(int psi_level, struct char_data *ch, struct obj_data *obj, int psi_num);
void psi_creations(int psi_level, struct char_data *ch, int psi_num, struct obj_data *obj);

int call_psionic(struct char_data *ch, struct char_data *victim, struct obj_data *obj, int psi_num, int level, int casttype, bool ack);
void psi_object_psionics(struct char_data *ch, struct obj_data *obj, char *argument);
int cast_psionic(struct char_data *ch, struct char_data *tch, struct obj_data *tobj, int psi_num);

// other prototypes
const char *skill_name(int num);

int psi_savingthrow(struct char_data *ch, struct char_data *victim, int type, int modifier);
void affect_update(void);
void room_affect_update(void);

bool group_member_check(struct char_data *ch, struct char_data *dude);

// from psionic_parser.c
ACMD(do_psi);
void unused_psionic(int spl);
void psi_assign_psionics(void);
void skill_assign_skills(void);
const char *psionic_name(int num);

// Global variables exported
#ifndef __PSIONIC_PARSER_C__

extern struct psionics_info_type psionics_info[];
extern struct skills_info_type skills_info[];
extern char cast_arg2[];
extern const char *unused_psiname;
extern const char *unused_skillname;
extern struct affected_rooms *room_af_list;

#endif /* __PSIONIC_PARSER_C__ */

#endif /* _PSIONICS_H_ */
