#include <libgc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ue.h"

#define UE_DEFAULT_CAPABILITIES 0x8020
#define TUN_NAME_LEN 32

struct _UE
{
	int id;
	uint8_t plmn[UE_PLMN_LENGTH];
	uint8_t msin[UE_MSIN_LENGTH];
	uint8_t net_capabilities[UE_CAPABILITIES_LENGTH];
	uint8_t nas_session_security_algorithms;
	uint32_t ue_s1ap_id;
	uint8_t mme_s1ap_id[4];
	uint8_t mme_s1ap_id_len;
	uint8_t key[KEY_LENGTH];
	uint8_t op_key[KEY_LENGTH];
	uint8_t spgw_ip[IP_LEN];
	uint32_t gtp_teid;
	uint32_t random_gtp_teid;
	char apn_name[32];
	uint8_t pdn_ip[IP_LEN];
	uint8_t guti[GUTI_LEN];
	char tun_name[TUN_NAME_LEN];
	int data_plane_socket;
	int tun_device;
	uint8_t ue_ip[IP_LEN]
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

void generate_tun_name(UE * ue, char * msin)
{
	uint32_t num = atoi(msin);
	ue->id = num;
	sprintf(ue->tun_name, "tun%d", num);
}

UE * init_UE(char * mcc, char * mnc, char * msin, uint8_t * key, uint8_t * op_key, uint8_t * ue_ip)
{
	UE * ue;
	ue = (UE *) GC_malloc(sizeof(UE));
	if(ue == NULL)
		return NULL;
	generate_plmn_UE(ue->plmn, mcc, mnc);
	generate_msin(ue->msin, msin);
	generate_net_capabilities(ue->net_capabilities, UE_DEFAULT_CAPABILITIES);
	generate_tun_name(ue, msin);
	//ue->ue_s1ap_id = (0x00FFFFFF & rand()) | 0x80000000;
	memcpy(ue->key, key, KEY_LENGTH);
	memcpy(ue->op_key, op_key, KEY_LENGTH);
	ue->mme_s1ap_id_len = -1;
	//ue->random_gtp_teid = 0xdde06fca;

	memcpy(ue->ue_ip, ue_ip, IP_LEN);

	/* GTP Teid and UE_S1AP_ID are random numbers dedicated to the UE connection */
	/* Because of that can be generated randomly */
	/* To avoid random number generation collision, these numbers are extracted from the msin value (less significant 24 bits)*/
	ue->ue_s1ap_id = 0x80000000 | (ue->msin[2] << 16) | (ue->msin[3] << 8) | ue->msin[4];
	ue->random_gtp_teid =  (ue->msin[1] << 24) | (ue->msin[2] << 16) | (ue->msin[3] << 8) | ue->msin[4];
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
	if(ue->mme_s1ap_id_len == -1)
		return;
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

void set_nas_session_security_algorithms(UE * ue, uint8_t algs)
{
	ue->nas_session_security_algorithms = algs;
}

uint8_t get_nas_session_enc_alg(UE * ue)
{
	return (ue->nas_session_security_algorithms & 0x70) >> 4;
}

uint8_t get_nas_session_int_alg(UE * ue)
{
	return ue->nas_session_security_algorithms & 0x07;
}

void set_spgw_ip(UE * ue, uint8_t * ip)
{
	memcpy(ue->spgw_ip, ip, IP_LEN);
}

uint8_t * get_spgw_ip(UE * ue)
{
	return ue->spgw_ip;
}

void set_gtp_teid(UE * ue, uint32_t teid)
{
	ue->gtp_teid = teid;
}

uint32_t get_gtp_teid(UE * ue)
{
	return ue->gtp_teid;
}

void set_apn_name(UE * ue, uint8_t * name, uint8_t name_len)
{
	uint8_t len = name_len;
	if(len > 31)
		len = 31;
	memcpy(ue->apn_name, name, len);
	ue->apn_name[len] = 0;
}

char * get_apn_name(UE * ue)
{
	return (char *) ue->apn_name;
}

void set_pdn_ip(UE * ue, uint8_t * ip)
{
	memcpy(ue->pdn_ip, ip, IP_LEN);
}

uint8_t * get_pdn_ip(UE * ue)
{
	return ue->pdn_ip;
}

void set_guti(UE * ue, uint8_t * guti)
{
	memcpy(ue->guti, guti, GUTI_LEN);
}

uint8_t * get_guti(UE * ue)
{
	return ue->guti;
}

uint32_t get_random_gtp_teid(UE * ue)
{
	return ue->random_gtp_teid;
}

char * get_tun_name(UE * ue)
{
	return ue->tun_name;
}

void set_data_plane_socket(UE * ue, int sockfd)
{
	ue->data_plane_socket = sockfd;
}

int get_data_plane_socket(UE * ue)
{
	return ue->data_plane_socket;
}

void set_tun_device(UE * ue, int tun)
{
	ue->tun_device = tun;
}

int get_tun_device(UE * ue)
{
	return ue->tun_device;
}

int get_ue_id(UE * ue)
{
	return ue->id;
}

int get_ue_size()
{
	return sizeof(UE);
}

uint8_t * get_ue_ip(UE * ue)
{
	return ue->ue_ip;
}