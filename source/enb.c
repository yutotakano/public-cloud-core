#include "enb.h"
#include <libgc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

struct _eNB
{
	uint32_t enb_id;
	uint8_t plmn[PLMN_LENGTH];
	uint8_t enb_identifier[ENB_ID_LENGHT];
};

void generate_plmn_eNB(uint8_t * plmn, char * mcc, char * mnc)
{
	plmn[0] = (mcc[1] - 0x30) << 4 | (mcc[0] - 0x30);
	plmn[1] = 0xF0 | (mcc[2] - 0x30);
	plmn[2] = (mnc[1] - 0x30) << 4 | (mnc[0] - 0x30);
}

eNB * init_eNB(uint32_t enb_id, char * mcc, char * mnc)
{
	eNB * enb;
	enb = (eNB *) GC_malloc(sizeof(eNB));
	if(enb == NULL)
		return NULL;
	/* Set eNB ID */
	enb->enb_id = enb_id;
	/* Generate PLMN */
	generate_plmn_eNB(enb->plmn, mcc, mnc);
	/* Set eNB Name */
	/* NOT IMPLEMENTED */
	return enb;
}

void free_eNB(eNB * enb)
{
	GC_free(enb);
}

uint8_t * get_plmn(eNB * enb)
{
	return enb->plmn;
}

uint32_t get_enb_id(eNB * enb)
{
	return enb->enb_id;
}