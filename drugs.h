/**************************************************************************
*   File: drugs.h                                    Part of CyberASSAULT *
*  Usage: header file: constants and fn prototypes for drug system        *
*                                                                         *
************************************************************************ */

// Our new drugs go here
#define DRUG_NONE               0
#define DRUG_JUICE              1
#define DRUG_NUKE               2
#define DRUG_GLITCH             3
#define DRUG_JOLT               4
#define DRUG_FLASH              5
#define DRUG_MIRAGE             6
#define DRUG_GOD                7
#define DRUG_MYST               8
#define DRUG_KO                 9
#define DRUG_MENACE             10
#define DRUG_VIRGIN             11
#define DRUG_ICE                12
#define DRUG_PEAK               13
#define DRUG_VELVET             14
#define DRUG_LAVA               15
#define DRUG_SCARLETT           16
#define DRUG_DEATH              17
#define DRUG_NECTAR             18
#define DRUG_TREMOR             19
#define DRUG_AMBROSIA           20
#define DRUG_VOODOO             21
#define DRUG_EMERALD            22
#define DRUG_HONEY              23
#define DRUG_MEDKIT             24
#define DRUG_STEROIDS           25
#define DRUG_MORPHINE           26
#define DRUG_CYANIDE            27
#define DRUG_ANTIPOISON         28
#define MAX_DRUGS               28

struct drug_info_type {
   byte min_position;   /* Position for performer */
   bool remort;
   int min_level;
   bool overdose;       /* if true, can only self-inject or be injected by group member */
   int targets;         /* See below for use with TAR_XXX  */
   byte beats;          /* The delay that is caused from injecting */
};

// Possible Targets:
//
//   TAR_IGNORE        : IGNORE TARGET
//   TAR_CHAR_ROOM    : PC/NPC in room
//   TAR_CHAR_WORLD    : PC/NPC in world
//   TAR_FIGHT_SELF    : If fighting, and no argument, select tar_char as self 
//   TAR_FIGHT_VICT    : If fighting, and no argument, select tar_char as victim (fighting)
//   TAR_CHAR_SELF     : If no argument, select self, if argument check that it IS self.   
//  

#define ADRUG(drugname) void drugname(struct char_data *ch)

// Insert new DRUG functions here
ADRUG(drug_juice);
ADRUG(drug_nuke);
ADRUG(drug_glitch);
ADRUG(drug_jolt);
ADRUG(drug_flash);
ADRUG(drug_mirage);
ADRUG(drug_god);
ADRUG(drug_myst);
ADRUG(drug_ko);
ADRUG(drug_menace);
ADRUG(drug_virgin);
ADRUG(drug_ice);
ADRUG(drug_peak);
ADRUG(drug_velvet);
ADRUG(drug_lava);
ADRUG(drug_scarlett);
ADRUG(drug_death);
ADRUG(drug_nectar);
ADRUG(drug_tremor);
ADRUG(drug_ambrosia);
ADRUG(drug_voodoo);
ADRUG(drug_emerald);
ADRUG(drug_honey);
ADRUG(drug_medkit);

// Mac's drug code from 2002 */
// To clarify some of the things here...
// Object Values for Drugs -->
// 1 = DAFFECT = tells WHICH drug they are on...
// 2 = Duration
// 3 = Smoked or Injected?
// GET_ADDICTION = Their addiction rating.  100+ = dependent

#define GET_ADDICTION(ch)  ((ch)->player_specials->saved.addiction)
#define GET_OVERDOSE(ch)   ((ch)->player_specials->saved.overdose)
#define IS_ON_DRUGS(ch)    ((ch)->drugs_used)

#define GET_DAFFECT(obj)   GET_OBJ_VAL((obj), 0) // For drug type
#define GET_DTIMER(obj)    GET_OBJ_VAL((obj), 1) // For duration

void drug_to_char(struct char_data *ch, struct affected_type * af);
void drug_remove(struct char_data * ch, struct affected_type * af);
void drug_from_char(struct char_data * ch, int type);
void drug_join(struct char_data * ch, struct affected_type * af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);
bool affected_by_drug(struct char_data * ch, int type);

