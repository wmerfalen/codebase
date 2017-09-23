/**
* @file bionics.h
* Header file for the CyberASSAULT bionics system (bionics.c).
* 
*/
#ifndef _BIONICS_H_
#define _BIONICS_H_
void new_bionics_affect(struct char_data *ch, bool ack);
void new_bionics_unaffect(struct char_data *ch, bool ack);
bool implant_bionic(struct char_data *ch, int location, struct obj_data *device);
struct obj_data *remove_bionic(struct char_data *ch, int location);
void bionics_update_affect(struct char_data *ch, struct obj_data *device, bool on, bool ack);
int bionics_save(struct char_data *ch);
void bionics_restore(struct char_data *ch);
extern int bionic_psi_loss[];
extern const struct bionic_info_type bionic_info[];
extern const struct bionic_info_type bionic_info_advanced[];
void salvagebionic(struct char_data *ch, struct obj_data *obj);

#endif /* _BIONICS_H_ */
