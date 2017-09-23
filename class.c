//////////////////////////////////////////////////////////////////////////////////
// This class file ha been re-written from scratch by Gahan.					//
// As of November 2011, I've changed the way classes and remorts are handled.	//
// Added: A ton of new classes													//
// Added: Character progression subclasses										//
// Added: New automated rebirth system											//
// Added: A ton of new skills													//
// Removed: Flat remort system													//
// Removed: Manual and testing for remorts										//
// - Gahan (NOTE: This code no longer has any relevance to prior codebases.		//
//////////////////////////////////////////////////////////////////////////////////

/** Help buffer the global variable definitions */
#define __CLASS_C__

/* This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class, you
 * should go through this entire file from beginning to end and add the
 * appropriate new special cases for your new class. */

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


// Class Display struct
// [1] - Class Name for display
// [2] - Class abbrev for multiclass display
const struct class_display_type class_display[] = {
	{"Mercenary   ",	"Mer" },
	{"Crazy       ",	"Cra" },
    {"Stalker     ",	"Sta" },
    {"Borg        ",	"Bor" },
    {"Highlander  ",	"Hig" },
    {"Predator    ",	"Pre" },
    {"Caller      ",	"Cal" },
	{"Marksman    ",	"Mar" },
	{"Brawler     ",	"Brw" },
	{"Bountyhunter",	"Bnt" },
	{"Enforcer    ",	"Enf" },
	{"Covert      ",	"Cov" },
	{"Priest      ",	"Pri" },
	{"Sniper      ",	"Sni" },
	{"Saboteur    ",	"Sab" },
	{"Assassin    ",	"Ass" },
	{"Striker     ",	"Str" },
	{"Terrorist   ",	"Ter" },
	{"Spy         ",	"Spy" },
	{"Anarchist   ",	"Ana" },
	{"Summoner    ",	"Sum" },
	{"Psionicist  ",	"Psi" },
	{"Psychotic   ",	"Psy" },
	{"Elementalist",	"Ele" },
	{"Technomancer",	"Tec" },
	{"Feral       ",	"Fer" },
	{"Survivalist ",	"Srv" },
	{"Hunter      ",	"Hun" },
	{"Beastmaster ",	"Bst" },
	{"Vanguard    ",	"Van" },
	{"Shaman      ",	"Sha" },
	{"Drone       ",	"Dro" },
	{"Destroyer   ",	"Dst" },
	{"Assimilator ",	"Asr" },
	{"Guardian    ",	"Grd" },
	{"Juggernaut  ",	"Jug" },
	{"Panzer      ",	"Pan" },
	{"Knight      ",	"Kni" },
	{"Bladesinger ",	"Bls" },
	{"Blademaster ",	"Blm" },
	{"Reaver      ",	"Rea" },
	{"Arcane      ",	"Arc" },
	{"Bard        ",	"Bar" },
	{"YoungBlood  ",	"Yng" },
	{"Badblood	  ",	"Bad" },
	{"Weaponmaster",	"Wpn" },
	{"Elder       ",	"Eld" },
	{"Predalien   ",	"Pda" },
	{"Mech        ",	"Mec" },
    {"\n",				 "\n" }
};

// New class struct
// [1]  - Name  
// [2+] - Array for available skilltrees
const struct pc_class_types pc_class[NUM_CLASSES] = {
	{"Mercenary",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Crazy",			{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Stalker",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Borg",			{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Highlander",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Predator",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Caller",			{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,-1}},
	{"Marksman",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Brawler",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Bountyhunter",	{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},	
	{"Enforcer",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Covert",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Priest",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,20,-1}},
	{"Sniper",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Saboteur",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Assassin",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Striker",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},	
	{"Terrorist",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Spy",				{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,14,15,19,27,-1}},
	{"Anarchist",		{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Summoner",		{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Psionicist",		{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Psychotic",		{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Elementalist",	{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Technomancer",	{ 2, 3, 5, 7, 9,10,11,12,13,14,16,17,18,19,48,69}},
	{"Feral",			{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,-1}},
	{"Survivalist",		{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,69}},
	{"Hunter",			{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,-1}},
	{"Beastmaster",		{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,-1}},
	{"Vanguard",		{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,69}},
	{"Shaman",			{ 1, 2, 3, 4, 5, 6, 7, 9,10,11,12,13,15,19,41,69}},
	{"Drone",			{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Destroyer",		{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Assimilator",		{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Guardian",		{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Juggernaut",		{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Panzer",			{ 0, 1, 3, 4, 5, 6,10,11,12,15,34,-1,-1,-1,-1,-1}},
	{"Knight",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Bladesinger",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Blademaster",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Reaver",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Arcane",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"Bard",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,13,15,19,55,-1}},
	{"YoungBlood",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Badblood",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Weaponmaster",	{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Elder",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Predalien",		{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}},
	{"Mech",			{ 0, 1, 2, 3, 4, 5, 6, 7, 9,10,11,15,19,62,-1,-1}}
};

	// New class parser to set class
unsigned long long int parse_class(char *arg, int remort) {
	int i;
	for (i = 0; arg[i]; i++)
		arg[i] = tolower(arg[ i ]);

        if (!str_cmp(arg, "anarchist")) 
                return (CLASS_ANARCHIST);
			else if (!str_cmp(arg, "arcane"))
                return (CLASS_ARCANE);
			else if (!str_cmp(arg, "assassin"))
                return (CLASS_ASSASSIN);
            else if (!str_cmp(arg, "assimilator"))
                return (CLASS_ASSIMILATOR);
			else if (!str_cmp(arg, "badblood"))
				return (CLASS_BADBLOOD);
            else if (!str_cmp(arg, "bard"))
                return (CLASS_BARD);
            else if (!str_cmp(arg, "beastmaster"))
                return (CLASS_BEASTMASTER);
            else if (!str_cmp(arg, "destroyer"))
                return (CLASS_DESTROYER);
            else if (!str_cmp(arg, "blademaster"))
                return (CLASS_BLADEMASTER);
            else if (!str_cmp(arg, "bladesinger"))
                return (CLASS_BLADESINGER);
            else if (!str_cmp(arg, "borg"))
                return (CLASS_BORG);
            else if (!str_cmp(arg, "bountyhunter"))
                return (CLASS_BOUNTYHUNTER);
            else if (!str_cmp(arg, "brawler"))
                return (CLASS_BRAWLER);
			else if (!str_cmp(arg, "caller"))
				return (CLASS_CALLER);
            else if (!str_cmp(arg, "covert"))
                return (CLASS_COVERT);
            else if (!str_cmp(arg, "crazy"))
                return (CLASS_CRAZY);
            else if (!str_cmp(arg, "drone"))
                return (CLASS_DRONE);
            else if (!str_cmp(arg, "elder"))
                return (CLASS_ELDER);
            else if (!str_cmp(arg, "elementalist"))
                return (CLASS_ELEMENTALIST);
            else if (!str_cmp(arg, "enforcer"))
                return (CLASS_ENFORCER);
            else if (!str_cmp(arg, "feral"))
                return (CLASS_FERAL);
            else if (!str_cmp(arg, "guardian"))
                return (CLASS_GUARDIAN);
            else if (!str_cmp(arg, "highlander"))
                return (CLASS_HIGHLANDER);
            else if (!str_cmp(arg, "hunter"))
                return (CLASS_HUNTER);
            else if (!str_cmp(arg, "juggernaut"))
                return (CLASS_JUGGERNAUT);
            else if (!str_cmp(arg, "knight"))
                return (CLASS_KNIGHT);
            else if (!str_cmp(arg, "marksman"))
                return (CLASS_MARKSMAN);
            else if (!str_cmp(arg, "mech"))
                return (CLASS_MECH);
            else if (!str_cmp(arg, "mercenary"))
                return (CLASS_MERCENARY);
            else if (!str_cmp(arg, "panzer"))
                return (CLASS_PANZER);
            else if (!str_cmp(arg, "predalien"))
                return (CLASS_PREDALIEN);
			else if (!str_cmp(arg, "predator"))
				return (CLASS_PREDATOR);
            else if (!str_cmp(arg, "priest"))
                return (CLASS_PRIEST);
            else if (!str_cmp(arg, "psionicist"))
                return (CLASS_PSIONICIST);
            else if (!str_cmp(arg, "psychotic"))
                return (CLASS_PSYCHOTIC);
            else if (!str_cmp(arg, "reaver"))
                return (CLASS_REAVER);
            else if (!str_cmp(arg, "saboteur"))
                return (CLASS_SABOTEUR);
            else if (!str_cmp(arg, "shaman"))
                return (CLASS_SHAMAN);
            else if (!str_cmp(arg, "sniper"))
                return (CLASS_SNIPER);
            else if (!str_cmp(arg, "spy"))
                return (CLASS_SPY);
			else if (!str_cmp(arg, "stalker"))
				return (CLASS_STALKER);
            else if (!str_cmp(arg, "striker"))
                return (CLASS_STRIKER);
            else if (!str_cmp(arg, "summoner"))
                return (CLASS_SUMMONER);
            else if (!str_cmp(arg, "survivalist"))
                return (CLASS_SURVIVALIST);
            else if (!str_cmp(arg, "technomancer"))
                return (CLASS_TECHNOMANCER);
            else if (!str_cmp(arg, "terrorist"))
                return (CLASS_TERRORIST);
            else if (!str_cmp(arg, "vanguard"))
                return (CLASS_VANGUARD);
            else if (!str_cmp(arg, "weaponmaster"))
                return (CLASS_WEAPONMASTER);
			else if (!str_cmp(arg, "youngblood"))
				return (CLASS_YOUNGBLOOD);
            return (CLASS_UNDEFINED);
}
// New 64-bit class bitvectors
bitvector_c find_class_bitvector(const char *arg)
{
    int class_tree[50][3] = {
        {CLASS_UNDEFINED, CLASS_UNDEFINED, CLASS_UNDEFINED},

        {CLASS_MERCENARY, CLASS_MERCENARY, CLASS_MERCENARY},
        {CLASS_MARKSMAN, CLASS_MERCENARY, CLASS_MERCENARY},
        {CLASS_BRAWLER, CLASS_MERCENARY, CLASS_MERCENARY},
        {CLASS_PRIEST, CLASS_MARKSMAN, CLASS_MERCENARY},
        {CLASS_COVERT, CLASS_MARKSMAN, CLASS_MERCENARY},
        {CLASS_BOUNTYHUNTER, CLASS_BRAWLER, CLASS_MERCENARY},
        {CLASS_ENFORCER, CLASS_BRAWLER, CLASS_MERCENARY},

        {CLASS_STALKER, CLASS_STALKER, CLASS_STALKER},
        {CLASS_SNIPER, CLASS_STALKER, CLASS_STALKER},
        {CLASS_SABOTEUR, CLASS_STALKER, CLASS_STALKER},
        {CLASS_TERRORIST, CLASS_SNIPER, CLASS_STALKER},
        {CLASS_SPY, CLASS_SNIPER, CLASS_STALKER},
        {CLASS_STRIKER, CLASS_SABOTEUR, CLASS_STALKER},
        {CLASS_ASSASSIN, CLASS_SABOTEUR, CLASS_STALKER},

        {CLASS_CRAZY, CLASS_CRAZY, CLASS_CRAZY},
        {CLASS_ANARCHIST, CLASS_CRAZY, CLASS_CRAZY},
        {CLASS_SUMMONER, CLASS_CRAZY, CLASS_CRAZY},
        {CLASS_PSIONICIST, CLASS_ANARCHIST, CLASS_CRAZY},
        {CLASS_PSYCHOTIC, CLASS_ANARCHIST, CLASS_CRAZY},
        {CLASS_ELEMENTALIST, CLASS_SUMMONER, CLASS_CRAZY},
        {CLASS_TECHNOMANCER, CLASS_SUMMONER, CLASS_CRAZY},

        {CLASS_CALLER, CLASS_CALLER, CLASS_CALLER},
        {CLASS_FERAL, CLASS_CALLER, CLASS_CALLER},
        {CLASS_SURVIVALIST, CLASS_CALLER, CLASS_CALLER},
        {CLASS_HUNTER, CLASS_SURVIVALIST, CLASS_CALLER},
        {CLASS_BEASTMASTER, CLASS_SURVIVALIST, CLASS_CALLER},
        {CLASS_VANGUARD, CLASS_FERAL, CLASS_CALLER},
        {CLASS_SHAMAN, CLASS_FERAL, CLASS_CALLER},

        {CLASS_BORG, CLASS_BORG, CLASS_BORG},
        {CLASS_DESTROYER, CLASS_BORG, CLASS_BORG},
        {CLASS_DRONE, CLASS_BORG, CLASS_BORG},
        {CLASS_PANZER, CLASS_DESTROYER, CLASS_BORG},
        {CLASS_JUGGERNAUT, CLASS_DESTROYER, CLASS_BORG},
        {CLASS_GUARDIAN, CLASS_DRONE, CLASS_BORG},
        {CLASS_ASSIMILATOR, CLASS_DRONE, CLASS_BORG},

        {CLASS_HIGHLANDER, CLASS_HIGHLANDER, CLASS_HIGHLANDER},
        {CLASS_KNIGHT, CLASS_HIGHLANDER, CLASS_HIGHLANDER},
        {CLASS_BLADESINGER, CLASS_HIGHLANDER, CLASS_HIGHLANDER},
        {CLASS_BLADEMASTER, CLASS_KNIGHT, CLASS_HIGHLANDER},
        {CLASS_REAVER, CLASS_KNIGHT, CLASS_HIGHLANDER},
        {CLASS_ARCANE, CLASS_BLADESINGER, CLASS_HIGHLANDER},
        {CLASS_BARD, CLASS_BLADESINGER, CLASS_HIGHLANDER},

        {CLASS_PREDATOR, CLASS_PREDATOR, CLASS_PREDATOR},
        {CLASS_YOUNGBLOOD, CLASS_PREDATOR, CLASS_PREDATOR},
        {CLASS_BADBLOOD, CLASS_PREDATOR, CLASS_PREDATOR},
        {CLASS_WEAPONMASTER, CLASS_YOUNGBLOOD, CLASS_PREDATOR},
        {CLASS_ELDER, CLASS_YOUNGBLOOD, CLASS_PREDATOR},
        {CLASS_PREDALIEN, CLASS_BADBLOOD, CLASS_PREDATOR},
        {CLASS_MECH, CLASS_BADBLOOD, CLASS_PREDATOR}
       
    };

    char *tmpstr;
	tmpstr = malloc(strlen(arg)+1);
    strcpy(tmpstr, arg);

    unsigned int rpos, ypos;

    unsigned long long int ret = 0;
    unsigned long long int class_val = parse_class(tmpstr, TRUE);

    for (rpos = 0; rpos < 50; rpos++) {
        if (class_tree[rpos][0] == class_val) {
            for (ypos = 0; ypos < 3; ypos++) {
                ret |= (1ULL << class_tree[rpos][ypos]);
			}
		}
    }
	free(tmpstr);
    return (ret);
} 

// Todo: probably we will not need this
#define PSIONIC    0
#define SKILL    1

/* The appropriate rooms for each guildmaster/guildguard; controls which types
 * of people the various guildguards let through.  i.e., the first line shows
 * that from room 3017, only MAGIC_USERS are allowed to go south. Don't forget
 * to visit spec_assign.c if you create any new mobiles that should be a guild
 * master or guard so they can act appropriately. If you "recycle" the
 * existing mobs that are used in other guilds for your new guild, then you
 * don't have to change that file, only here. */
struct guild_info_type guild_info[] = {

    /* Motown */
    {CLASS_MERCENARY,    3017,    SOUTH},
    {CLASS_CRAZY,     3004,    NORTH},
    {CLASS_STALKER,    3027,    EAST},
    {CLASS_BORG,      3021,    EAST},
    {CLASS_CALLER,      6041,    NORTH},
    /* this must go last -- add new guards above! */
    { -1, NOWHERE, -1}

};

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
    void advance_level(struct char_data * ch);
    struct obj_data *eq1;
    struct obj_data *eq2;
    struct obj_data *eq3;
    struct obj_data *eq4;
    struct obj_data *eq5;
    struct obj_data *eq6;
    GET_LEVEL(ch) = 1;
    GET_EXP(ch) = 1;

    set_title(ch, NULL);
    advance_level(ch);

    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_PSI(ch) = GET_MAX_PSI(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);

    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, DRUNK) = -1;

    ch->player.time.played = 0;
    ch->player.time.logon = time(0);

    GET_UNITS(ch) = 5000;

    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOLOOT);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPPSI);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPAMMO);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOCON);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEQUIP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_1);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR_2);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOSCAVENGER);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_OLDEQ);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTODAM);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOKEY);

	send_to_char(ch, "@n\r\n");                                                                                   
	send_to_char(ch, "@Y######                                        ######  #######    #    ######  ###\r\n");
	send_to_char(ch, "@Y#     # #      ######   ##    ####  ######    #     # #         # #   #     # ###\r\n"); 
	send_to_char(ch, "@Y#     # #      #       #  #  #      #         #     # #        #   #  #     # ###\r\n"); 
	send_to_char(ch, "@Y######  #      #####  #    #  ####  #####     ######  #####   #     # #     #  # \r\n"); 
	send_to_char(ch, "@Y#       #      #      ######      # #         #   #   #       ####### #     #    \r\n"); 
	send_to_char(ch, "@Y#       #      #      #    # #    # #         #    #  #       #     # #     # ### \r\n");
	send_to_char(ch, "@Y#       ###### ###### #    #  ####  ######    #     # ####### #     # ######  ###@n\r\n"); 
	send_to_char(ch, "@n\r\n");                                                                                   
    GET_PAGE_LENGTH(ch) = 43;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOGAIN);
    send_to_char(ch, "\r\n@YWhen you have enough experience to level, you will gain automatically.\r\nTo turn this off and manually gain instead, type @Rtoggle autogain@Y.@n\r\n");

    GET_FREQ(ch) = NEWB_RADIO;
    send_to_char(ch, "\r\n@WYou are now listening to Newbie Information Radio, frequency 10.\r\n"
        "You can TURN OFF the radio by typing @Rradio off@n@W.@n\n\r");
    send_to_char(ch, "@n\n\r");
    if (CONFIG_SITEOK_ALL)
        SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);

        // Items for nubs?
        eq1 = read_object(13039, VIRTUAL);
        eq2 = read_object(13038, VIRTUAL);
        eq3 = read_object(13037, VIRTUAL);
        eq4 = read_object(13036, VIRTUAL);
        eq5 = read_object(13035, VIRTUAL);
        eq6 = read_object(13034, VIRTUAL);
        equip_char(ch, eq1, WEAR_FEET, TRUE);
        equip_char(ch, eq2, WEAR_WIELD, TRUE);
        equip_char(ch, eq3, WEAR_LEGS, TRUE);
        equip_char(ch, eq4, WEAR_BODY, TRUE);
        equip_char(ch, eq5, WEAR_LIGHT, TRUE);
        obj_to_char(eq6, ch);
}

/* This simply calculates the backstab multiplier based on a character's level.
 * This used to be an array, but was changed to be a function so that it would
 * be easier to add more levels to your MUD.  This doesn't really create a big
 * performance hit because it's not used very often. */
int backstab_mult(int level)
{
    if (level <= 8)
        return (2);      /* level 1 - 8 */
    else if (level <= 16)
        return (2);      /* level 8 - 16 */
    else if (level <= 24)
        return (3);      /* level 16 - 24 */
    else if (level <= 32)
        return (4);      /* level 24 - 32 */
	else if (level <= 40)
		return (4);		 /* level 32 - 40 */
	else if (level < LVL_IMMORT)
        return (5);      /* all remaining mortal levels */
    else
        return (20);      /* immortals */
}

/* This function controls the change to maxmove, maxmana, and maxhp for each
 * class every time they gain a level. */
void advance_level(struct char_data *ch)
{
	struct gain_data *glist = global_gain;
    int add_hp = 0;
    int add_psi = 0;
    int add_move = 0;
    int add_prac = 0;

    add_hp = GET_CON(ch);
    add_psi = GET_INT(ch);
    add_move = (GET_DEX(ch) + GET_STR(ch)) /3;

    ch->points.max_hit += add_hp;
    ch->points.max_psi += add_psi;
    ch->points.max_move += add_move;

    add_prac = (GET_INT(ch) + GET_PCN(ch) + 25)/5;
    GET_PRACTICES(ch) += add_prac;

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        GET_COND(ch, THIRST) = -1;
        GET_COND(ch, HUNGER) = -1;
        GET_COND(ch, DRUNK) = -1;
        SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }
    while(glist) {
		if (glist->playerid == GET_IDNUM(ch)) {
			glist->playerlevel = GET_LEVEL(ch);
			break;
        } 
        glist = glist->next;
    }

    save_char(ch);

    send_to_char(ch, "@WYou have gained @R%d @Whit points, @C%d@W psi points, @G%d@W movement and @B%d @Wpractice sessions.@n\r\n", add_hp, add_psi, add_move, add_prac);

    snoop_check(ch);

    mudlog(BRF, 0, TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
}

/* invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors. */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
    if (OBJ_FLAGGED(obj, ITEM_ANTI_MERCENARY) && IS_MERCENARY(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_CRAZY) && IS_CRAZY(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_BORG) && IS_BORG(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_STALKER) && IS_STALKER(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_CALLER) && IS_CALLER(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_HIGHLANDER) && IS_HIGHLANDER(ch))
        return (TRUE);

    if (OBJ_FLAGGED(obj, ITEM_ANTI_PREDATOR) && IS_PREDATOR(ch))
        return (TRUE);

    return (FALSE);
}

// init_skill_levels now done inside psionic_parser.c as part of skill assignment
// init_psionic_levels now done inside psionic_parser.c as part of psionic assignment


/* This is the exp given to implementors -- it must always be greater than the
 * exp required for immortality, plus at least 20,000 or so. */
#define EXP_MAX  2040000001


// Class titles lameness should be removed soon
// [0] - Male title
// [1] - Female title
// [2] - Exp to level
// [3] - Level cap

const struct title_type titles[LVL_IMPL + 1] = {
    {"the Man", "the Woman", 0},
	{"the Newbie", "the Newbie", 1000},
    {"the New Recruit", "the New Recruit", 2000},
    {"the Trainee", "the Trainee", 4000},
    {"the Rookie", "the Rookie", 6000},
    {"the Bullet Stopper", "the Bullet Stopper", 8000},
    {"the Grunt", "the Grunt", 12000},
    {"the Serviceman", "the Servicewoman", 18000},
    {"the Guard", "the Guard", 26000},
    {"the Sentry", "the Sentry", 40000},
    {"the Warden", "the Warden", 55000},
    {"the Protector", "the Protector", 72000},
    {"the Escort", "the Escort", 92000},
    {"the Rifleman", "the Riflewoman", 120000},
    {"the Trooper", "the Trooper", 155000},
    {"the Sharpshooter", "the Sharpshooter", 200000},
    {"the Guerilla", "the Guerilla", 260000},
    {"the Campaigner", "the Campaigner", 345000},
    {"the Officer", "the Officer", 450000},
    {"the Career Soldier", "the Career Soldier", 580000},
    {"the Grenadier", "the Grenadier", 760000},
    {"the Elite", "the Elite", 1000000},
    {"the Veteran", "the Veteran", 1150000},
    {"the Gun-for-Hire", "the Gun-for-Hire", 1300000},
    {"the Crusader", "the Crusader", 1500000},
    {"the Defender", "the Defender", 1700000},
    {"the Sentinel", "the Sentinel", 2000000},
    {"the Cyberagent", "the Cyberagent", 2500000},
    {"the Ultimate Fighting Machine","the Ultimate Fighting Machine", 3000000},
    {"the Honored", "the Honored", 4000000},
    {"the Mighty", "the Mighty", 5000000},
	{"the Exhalted", "the Exhalted", 7000000},
	{"the Elusive", "the Elusive", 9000000},
	{"the Captain", "the Captain", 11000000},
	{"the General", "the General", 13000000},
	{"the Elder", "the Elderess", 16000000},
	{"the Spectacular", "the Spectacular", 19000000},
	{"the Master", "the Master", 22000000},
	{"the Legend", "the Legend", 25000000},
	{"the Vengeful", "the Vengeful", 30000000},
	{"the Avatar", "the Avatar", 40000000},
    {"the Immortal", "the Immortal", 40000000},
    {"the Assisstant god", "the Assisstant god", 201000000},
    {"the God", "the Goddess", 202000000},
    {"the Co-Implementor", "the Greater Goddess", 203000000},
    {"the Overlord", "the Overmistress", 204000000}
};

// Experience Caps by Level - Gahan
// Todo: incorporate this into the above struct, and get rid of titles
const struct exp_cap_type exp_cap[LVL_IMPL + 1] = {
	{0},			// Level 0
	{100},			// Level 1
	{200},			// Level 2
	{400},			// Level 3
	{600},			// Level 4
	{800},			// Level 5
	{1000},			// Level 6
	{1250},			// Level 7
	{1500},			// Level 8
	{2000},			// Level 9
	{2500},			// Level 10
	{3500},			// Level 11
	{4500},			// Level 12
	{6000},			// Level 13
	{9000},			// Level 14
	{13000},		// Level 15
	{17000},		// Level 16
	{25000},		// Level 17
	{30000},		// Level 18
	{40000},		// Level 19
	{50000},		// Level 20
	{60000},		// Level 21
	{70000},		// Level 22
	{80000},		// Level 23
	{90000},		// Level 24
	{100000},		// Level 25
	{110000},		// Level 26
	{120000},		// Level 27
	{130000},		// Level 28
	{140000},		// Level 29
	{150000},		// Level 30
	{165000},		// Level 31
	{180000},		// Level 32
	{195000},		// Level 33
	{210000},		// Level 34
	{225000},		// Level 35
	{240000},		// Level 36
	{255000},		// Level 37
	{270000},		// Level 38
	{285000},		// Level 39
	{300000},		// Level 40
	{300000},		// Level 41
	{300000},		// Level 42
	{300000},		// Level 43
	{300000},		// Level 44
	{300000}		// Level 45
};

// Set character to base stats of their class
// To be used before character enters bonus stat adding
// in interpreter.c (create and remort character) -Lump

void set_basestats(struct char_data *ch)
{
    int str;
    int add;
    int intel;
    int pcn;
    int dex;
    int con;
    int cha;

    if (IS_BORG(ch)) {
	    str = 14;
        add = 0;
        intel = 5;
        pcn = 9;
        dex = 7;
        con = 15;
        cha = 10;
	}
	else if (IS_CRAZY(ch)) {
	    str = 8;
        add = 0;
        intel = 15;
        pcn = 12;
        dex = 9;
        con = 6;
        cha = 10;
	}
	else if (IS_STALKER(ch)) {
		str = 7;
        add = 0;
        intel = 10;
        pcn = 11;
        dex = 15;
        con = 7;
        cha = 10;
	}
	else if (IS_MERCENARY(ch)) {
		str = 10;
        add = 0;
        intel = 10;
        pcn = 10;
        dex = 10;
        con = 10;
        cha = 10;
	}
	else if (IS_CALLER(ch)) {
        str = 9;
        add = 0;
        intel = 10;
        pcn = 10;
        dex = 8;
        con = 9;
        cha = 14;
	}
	else if (IS_HIGHLANDER(ch)) {
	    str = 13;
        add = 0;
        intel = 10;
        pcn = 12;
        dex = 10;
        con = 12;
        cha = 10;
		GET_NUM_ESSENCES(ch) = 100;
	}
	else if (IS_PREDATOR(ch)) {
		str = 15;
        add = 0;
        intel = 9;
        pcn = 10;
        dex = 13;
        con = 10;
        cha = 10;
	}
	else {
		str = 15;
        add = 0;
        intel = 9;
        pcn = 10;
        dex = 13;
        con = 10;
        cha = 10;
	}

    ch->real_abils.str = str;
    ch->real_abils.str_add = add;
    ch->real_abils.intel = intel;
    ch->real_abils.pcn = pcn;
    ch->real_abils.dex = dex;
    ch->real_abils.con = con;
    ch->real_abils.cha = cha;

}

// This is a function to calculate the exp needed to level based on
// the amount of progressions and then round the number to better
// present it.  I know this is a pretty sloppy function but it works
// and I have yet to find a better way to do this

int exptolevel(struct char_data *ch, int mod)
{
	int calc;
	int newcalc;
	int base;
	int exp;
	int i;

	if (GET_PROGRESS(ch) <= 1) {
		base = titles[mod].exp;
		return (base);
	}
	else if (GET_PROGRESS(ch) >= 2) {
		exp = GET_PROGRESS(ch) - 1;
		base = titles[mod].exp;
		
		for (i = 1; i <= exp; i++)
			base = base * 1.25;

		if (base < 1000) {
			calc = base / 10;
			newcalc = calc * 10;
			return (newcalc);
		}
		else if (base < 10000) {
			calc = base / 100;
			newcalc = calc * 100;
			return (newcalc);
		}
		else if (base < 100000) {
			calc = base / 1000;
			newcalc = calc * 1000;
			return (newcalc);
		}
		else if (base < 1000000) {
			calc = base / 10000;
			newcalc = calc * 10000;
			return (newcalc);
		}
		else if (base < 10000000) {
			calc = base / 100000;
			newcalc = calc * 100000;
			return (newcalc);
		}
		else if (base < 100000000) {
			calc = base / 1000000;
			newcalc = calc * 1000000;
			return (newcalc);
		}
		else if (base > 100000000) {
			calc = base / 1000000;
			newcalc = calc * 1000000;
			return (newcalc);
		}
		else
			return (base);
	}
	else
		return (0);
}

// Class Menu called at character creation in interpreter.c
const char *class_menu =
    "\r\n"
    "@DSelect a class:@n\r\n\r\n"
    "  @D@CCrazy\r\n@D  ------\r\n  Altered by mind enhancement drugs and prosthetics,\n\r  these guys are ultra-intelligent and possess amazing psionic ability.  Yet,\r\n  they are not as physically tough as the other classes.@n\n\r\r\n"
    "  @D@CStalker\r\n@D  ------\r\n  Quiet and deadly, these low-key individuals are\n\r  incredibly agile: able to perform some skills others would find impossible.@n\n\r\r\n"
    "  @D@CBorg\r\n@D  ------\r\n  Enhanced by bionic and cybernetic augmentation, these\r\n  machines have lost most of their original human qualities.  As a result,\r\n  they make great fighters but are lacking in intelligence.@n\n\r\r\n"
    "  @D@CMercenary\r\n@D  ------\r\n  A freelance operative and the basic class. \r\n  Mercenaries have no advantages or disadvantages.@n\n\r\r\n"
    "  @D@CCaller\r\n@D  ------\r\n  A new breed of mutant, blessed with the ability\r\n  to call upon animals to assist them. Callers look just like\r\n  regular human, but they can make slaves out of animals with the power of\r\n  their minds alone.@n\n\r\r\n"
    "  @D@CPredator\r\n@D  ------\r\n  Predators have the skills of a true hunter. Most \r\n  Predators wear special armor for the hunt, and carry a variety \r\n  of devices to aid in stalking thier prey.@n\r\n\r\n"
	"  @D@CHighlander @R(HARDCORE CLASS)\r\n@D  ------\r\n  No one really knows much about Highlanders, only\r\n  that rarely a mortal is deemed worthy enough to join their \r\n  battle. That mortal gains abilities almost too mighty to contain, but \r\n  also joins the race and the eternal battle to be the last...\r\n@R  WARNING:  HIGHLANDERS HAVE LIMITED DEATHS BEFORE PERMANENT DELETION\n\r\n@n";