//////////////////////////////////////////////////////////////////////////////////
// This custom.h file ha been written from scratch by Gahan.					//
// This file is for strict purposes of CyberASSAULT mud, which is a highly		//
// modified TBAMud, a derivitive of CirleMUD.									//
//																				//
// This is the customized equipment system for CyberASSAULT						//
// Anything that has to do with it will be found here or within custom.c		//
//////////////////////////////////////////////////////////////////////////////////
#ifndef _CUSTOM_H_
#define _CUSTOM_H_

struct custom_editor_data {
	int mode;
	int value;
	struct obj_data *obj;
};

#define CUSTOM(d)         ((d)->custom)
#define CUSTOM_MODE(d)    (CUSTOM(d)->mode)
#define CUSTOM_OBJ(d)	  (CUSTOM(d)->obj)
#define CUSTOM_VAL(d)     (CUSTOM(d)->value)

#define CUSTOM_MAIN_MENU	0
#define CUSTOM_KEYWORD		1
#define CUSTOM_SHORTDESC	2
#define CUSTOM_LONGDESC		3
#define CUSTOM_LEVEL		4
#define CUSTOM_TYPE			5
#define CUSTOM_EXTRAS		6
#define CUSTOM_WEAR			7
#define CUSTOM_VALUE_1		8
#define CUSTOM_VALUE_2		9
#define CUSTOM_VALUE_3		10
#define CUSTOM_VALUE_4		11
#define CUSTOM_VALUE_5		12
#define CUSTOM_VALUE_6		13
#define CUSTOM_PROMPT_APPLY	14
#define CUSTOM_APPLY		15
#define CUSTOM_APPLYMOD		16
#define CUSTOM_WPNPSIONIC	17
#define CUSTOM_WPNSKILL		18
#define CUSTOM_AFF			19

void custom_cleanup(struct descriptor_data *d);
void custom_parse(struct descriptor_data *d, char *arg);
bool oset_alias(struct obj_data *obj, char * argument); 
bool oset_apply(struct obj_data *obj, char * argument); 
bool oset_weaponpsionic(struct obj_data *obj, char * argument); 
bool oset_short_description(struct obj_data *obj, char * argument); 
bool oset_itemlevel(struct obj_data *obj, char * argument); 
bool oset_remortlevel(struct obj_data *obj, char * argument); 
bool oset_long_description(struct obj_data *obj, char * argument); 
bool restring_name(struct char_data *ch, struct obj_data *obj, char *argument);
#ifndef __CUSTOM_C__

ACMD(do_oset);
ACMD(do_custom);

#endif
#endif /* _CUSTOM_H_ */
