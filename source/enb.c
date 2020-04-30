#include <libgc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "enb.h"
#include "ue.h"

struct _eNB
{
	uint32_t enb_id;
	uint8_t plmn[PLMN_LENGTH];
	uint8_t enb_identifier[ENB_ID_LENGHT];
	uint8_t enb_ip[IP_LEN];
	int mme_socket;
	int spgw_socket;
};

void generate_plmn_eNB(uint8_t * plmn, char * mcc, char * mnc)
{
	plmn[0] = (mcc[1] - 0x30) << 4 | (mcc[0] - 0x30);
	plmn[1] = 0xF0 | (mcc[2] - 0x30);
	plmn[2] = (mnc[1] - 0x30) << 4 | (mnc[0] - 0x30);
}

eNB * init_eNB(uint32_t enb_id, char * mcc, char * mnc, uint8_t * enb_ip)
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
	memcpy(enb->enb_ip, enb_ip, 4);
	enb->mme_socket = enb->spgw_socket = -1;
	return enb;
}

void free_eNB(eNB * enb)
{
	if(enb->mme_socket != -1)
		close(enb->mme_socket);
	if(enb->spgw_socket != -1)
		close(enb->spgw_socket);
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

uint8_t * get_enb_ip(eNB * enb)
{
	return enb->enb_ip;
}

void set_mme_socket(eNB * enb, int sockfd)
{
	enb->mme_socket = sockfd;
}

int get_mme_socket(eNB * enb)
{
	return enb->mme_socket;
}

void set_spgw_socket(eNB * enb, int sockfd)
{
	enb->spgw_socket = sockfd;
}

int get_spgw_socket(eNB * enb)
{
	return enb->spgw_socket;
}
