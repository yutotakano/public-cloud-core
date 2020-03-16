#ifndef __enb__
#define __enb__

#include <stdint.h>

#define PLMN_LENGTH 3
#define ENB_ID_LENGHT 4

typedef struct _eNB eNB;

eNB * init_eNB(uint32_t enb_id, char * mcc, char * mnc);
void free_eNB(eNB * enb);
uint8_t * get_plmn(eNB * enb);
uint32_t get_enb_id(eNB * enb);

#endif