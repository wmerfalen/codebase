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

#ifndef _CLASS_H_
#define _CLASS_H_

/* Functions available through class.c */
unsigned long long parse_class(char *arg, int remort);
void do_start(struct char_data *ch);
void advance_level(struct char_data *ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
void set_basestats(struct char_data *ch);
void make_crazy(struct char_data *ch);
void make_borg(struct char_data *ch);
void make_stalker(struct char_data *ch);
void make_mercenary(struct char_data *ch);
void make_highlander(struct char_data *ch);
void make_predator(struct char_data *ch);

/* Global variables */

#ifndef __CLASS_C__  
extern int exptolevel(struct char_data *ch, int mod);
extern const char *class_menu;
extern struct guild_info_type guild_info[];
extern const struct title_type titles[LVL_IMPL + 1];
extern const struct exp_cap_type exp_cap[LVL_IMPL + 1];

#endif /* __CLASS_C__ */

#endif /* _CLASS_H_*/
