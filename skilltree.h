//////////////////////////////////////////////////////////////////////////////////
// skilltree.h file ha been written from scratch by Gahan.      			   	//
// This is the new way skills and psionics are handled.  This is to support our //
// almost 50 classes, and the progression system.                               //
// - Gahan (NOTE: This code no longer has any relevance to prior codebases.		//
//////////////////////////////////////////////////////////////////////////////////

#ifndef _SKILLTREE_H_
#define _SKILLTREE_H_

#define MAX_SKILL_TREES 		71
#define MAX_SKILLSNPSIONIC 		183
#define MAX_CLASS_SKILLTREES	16
#define MAX_COMBOS				51

struct pc_class_types {
	char *class_name;
	int skill_tree[MAX_CLASS_SKILLTREES];
};

ACMD(do_combo);

extern const struct pc_class_types pc_class[NUM_CLASSES];
extern const struct skill_tree_type full_skill_tree[MAX_SKILLSNPSIONIC];
extern const char *skill_tree_names[MAX_SKILL_TREES];
extern const struct combo_type combo[MAX_COMBOS];

#ifndef __SKILLTREE_C__ 

#endif /* __SKILLTREE_C__ */

#endif /* _SKILLTREE_H_*/
