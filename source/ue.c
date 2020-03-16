#include <libgc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ue.h"

struct _UE
{
	uint8_t plmn[UE_PLMN_LENGTH];
	uint8_t msin[UE_MSIN_LENGTH];
	uint8_t net_capabilities[UE_CAPABILITIES_LENGTH];
	uint32_t ue_s1ap_id;
	uint8_t mme_s1ap_id[4];
	uint8_t mme_s1ap_id_len;
	uint8_t key[KEY_LENGTH];
	uint8_t op_key[KEY_LENGTH];
};

void printUE(UE * ue)
{
	printf("PLMN: %.2x %.2x %.2x\n", ue->plmn[0], ue->plmn[1], ue->plmn[2]);
	printf("MSIN: %.2x %.2x %.2x %.2x %.2x\n", ue->msin[0], ue->msin[1], ue->msin[2], ue->msin[3], ue->msin[4]);
	printf("CAPABILITIES: %.2x %.2x %.2x\n", ue->net_capabilities[0], ue->net_capabilities[1], ue->net_capabilities[2]);
}

void generate_plmn_UE(uint8_t * plmn, char * mcc, char * mnc)
{
	plmn[0] = (mcc[0] - 0x30) << 4;
	plmn[1] = (mcc[2] - 0x30) << 4 | (mcc[1] - 0x30);
	plmn[2] = (mnc[1] - 0x30) << 4 | (mnc[0] - 0x30);
}

void generate_msin(uint8_t * msin, char * msinText)
{
	uint8_t i;
	for(i = 0; i < 10; i += 2)
	{
		msin[i/2] = (msinText[i+1] - 0x30) << 4 | (msinText[i] - 0x30);
	}
}

void generate_net_capabilities(uint8_t * net_capabilities, uint16_t capabilities)
{
	net_capabilities[0] = 0x02;
	net_capabilities[1] = (capabilities >> 8) & 0xFF;
	net_capabilities[2] = capabilities & 0xFF;
}

UE * init_UE(char * mcc, char * mnc, char * msin, uint16_t capabilities, uint32_t ue_s1ap_id, uint8_t * key, uint8_t * op_key)
{
	UE * ue;
	ue = (UE *) GC_malloc(sizeof(UE));
	if(ue == NULL)
		return NULL;
	generate_plmn_UE(ue->plmn, mcc, mnc);
	generate_msin(ue->msin, msin);
	generate_net_capabilities(ue->net_capabilities, capabilities);
	ue->ue_s1ap_id = ue_s1ap_id;
	memcpy(ue->key, key, KEY_LENGTH);
	memcpy(ue->op_key, op_key, KEY_LENGTH);
	return ue;
}

void free_UE(UE * ue)
{
	GC_free(ue);
}

uint8_t * get_ue_plmn(UE * ue)
{
	return ue->plmn;
}

uint8_t * get_ue_msin(UE * ue)
{
	return ue->msin;
}

uint8_t * get_ue_capabilities(UE * ue)
{
	return ue->net_capabilities;
}

uint32_t get_ue_s1ap_id(UE * ue)
{
	return ue->ue_s1ap_id;
}

uint8_t * get_mme_s1ap_id(UE * ue, uint8_t * len)
{
	*len = ue->mme_s1ap_id_len;
	return ue->mme_s1ap_id;
}

void set_mme_s1ap_id(UE * ue, uint8_t * mme_s1ap_id, uint8_t len)
{
	memcpy(ue->mme_s1ap_id, mme_s1ap_id, len);
	ue->mme_s1ap_id_len = len;
}

uint8_t * get_ue_key(UE * ue)
{
	return ue->key;
}

uint8_t * get_ue_op_key(UE * ue)
{
	return ue->op_key;
}