#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "user.h"

#define GATEWAY_IP 0xAC100001

#define UINT32_TO_ARRAY(array, u32)	\
{					\
	array[0] = (u32 >> 24) & 0xFF;	\
	array[1] = (u32 >> 16) & 0xFF;	\
	array[2] = (u32 >> 8) & 0xFF;	\
	array[3] = (u32) & 0xFF;	\
}

#define ARRAY_TO_UINT32(u32, array)							\
{											\
	u32 = (array[0] << 24) | (array[1] << 16) | (array[2] << 8) | (array[3]);	\
}

void initRandSeed()
{
	srand(time(0));
}

struct _UserInfo
{
	char imsi[IMSI_LEN];
	uint8_t key[KEY_LEN];
	uint8_t opc[KEY_LEN];
	uint8_t rand[RAND_LEN];
	uint8_t enb_ue_s1ap_id[ENB_UE_S1AP_ID_LEN];
	uint8_t target_enb_ue_s1ap_id[ENB_UE_S1AP_ID_LEN];
	uint8_t mme_ue_s1ap_id[MME_UE_S1AP_ID_LEN];
	uint8_t tmsi[TMSI_LEN]; /* Temporal ID generated from the IMSI */
	uint8_t ue_teid[TEID_LEN]; /* Downlink */
	uint8_t epc_teid[TEID_LEN]; /* Uplink */

	uint8_t spgw_ip[IP_LEN]; /* SPGW IP */
	uint8_t enb_ip[IP_LEN]; /* ENB IP */
	uint8_t pdn_ipv4[IP_LEN]; /* Internal LTE UE IP */

	uint8_t auth_res[AUTH_RES_LEN]; /* Challenge result */
	uint8_t enc_key[KEY_LEN]; /* Key used in the NAS encryption */
	uint8_t int_key[KEY_LEN]; /* Key used in the NAS integrity */

	uint8_t kasme1[KEY_LEN]; /* Kasme[0:16]  */
	uint8_t kasme2[KEY_LEN]; /* Kasme[16:32] */

	uint8_t epc_nas_sequence_number;
	uint8_t ue_nas_sequence_number;

	uint8_t next_hop_chaining_count;

	uint8_t knh1[KEY_LEN]; /* KNH[0:16] */
	uint8_t knh2[KEY_LEN]; /* KNH[16:32] */

	uint8_t enb_source_socket[SOCKET_LEN];
	uint8_t enb_target_socket[SOCKET_LEN];
};

UserInfo * new_user_info()
{
	return (UserInfo *)malloc(sizeof(UserInfo));
}

void free_user_info(void * user)
{
	free(user);
}

void set_user_imsi(UserInfo * user, char  * imsi)
{
	memcpy(user->imsi, imsi, IMSI_LEN);
	user->imsi[15] = 0;
	return;
}

char * get_user_imsi(UserInfo * user)
{
	return user->imsi;
}

char * get_user_msin(UserInfo * user)
{
	return user->imsi+5;
}

uint8_t * get_user_key(UserInfo * user)
{
	return user->key;
}

uint8_t * get_user_opc(UserInfo * user)
{
	return user->opc;
}

uint8_t * get_user_rand(UserInfo * user)
{
	/* Generate a new RAND */
	return user->rand;
}

uint8_t * get_user_enb_ue_id(UserInfo * user)
{
	return user->enb_ue_s1ap_id;
}

uint8_t * get_user_mme_ue_id(UserInfo * user)
{
	return user->mme_ue_s1ap_id;
}

uint8_t * get_user_target_enb_ue_id(UserInfo * user)
{
	return user->target_enb_ue_s1ap_id;
}

uint8_t * get_user_tmsi(UserInfo * user)
{
	return user->tmsi;
}

uint8_t * get_user_ue_teid(UserInfo * user)
{
	return user->ue_teid;
}

uint8_t * get_user_epc_teid(UserInfo * user)
{
	return user->epc_teid;
}

uint8_t * get_user_spgw_ip(UserInfo * user)
{
	return user->spgw_ip;
}

uint8_t * get_user_enb_ip(UserInfo * user)
{
	return user->enb_ip;
}

uint8_t * get_user_pdn_ipv4(UserInfo * user)
{
	return user->pdn_ipv4;
}

uint8_t get_user_epc_nas_sequence_number(UserInfo * user)
{
	return user->epc_nas_sequence_number;
}

void set_user_epc_nas_sequence_number(UserInfo * user, uint8_t value)
{
	user->epc_nas_sequence_number = value;
}

uint8_t get_user_ue_nas_sequence_number(UserInfo * user)
{
	return user->ue_nas_sequence_number;
}

void set_user_ue_nas_sequence_number(UserInfo * user, uint8_t value)
{
	user->ue_nas_sequence_number = value;
}

uint8_t get_user_next_hop_chaining_count(UserInfo * user)
{
	return user->next_hop_chaining_count;
}

void set_user_next_hop_chaining_count(UserInfo * user, uint8_t value)
{
	user->next_hop_chaining_count = value;
}

uint8_t * get_user_auth_res(UserInfo * user)
{
	return user->auth_res;
}

uint8_t * get_user_enc_key(UserInfo * user)
{
	return user->enc_key;
}

uint8_t * get_user_int_key(UserInfo * user)
{
	return user->int_key;
}

uint8_t * get_user_kasme1_key(UserInfo * user)
{
	return user->kasme1;
}

uint8_t * get_user_kasme2_key(UserInfo * user)
{
	return user->kasme2;
}

uint8_t * get_user_knh1_key(UserInfo * user)
{
	return user->knh1;
}

uint8_t * get_user_knh2_key(UserInfo * user)
{
	return user->knh2;
}

uint8_t * get_user_enb_source_socket(UserInfo * user)
{
	return user->enb_source_socket;
}

uint8_t * get_user_enb_target_socket(UserInfo * user)
{
	return user->enb_target_socket;
}

void generate_rand(UserInfo * user)
{
	/* TODO: only for testing, remove this */
	//uint8_t nonrand[] = "abcdefghijklmnop";
	//memcpy(user->rand, nonrand, 16);
	//return;

	uint32_t num;

	num = (uint32_t)rand();
	UINT32_TO_ARRAY(user->rand, num);
	num = (uint32_t)rand();
	UINT32_TO_ARRAY((user->rand+4), num);
	num = (uint32_t)rand();
	UINT32_TO_ARRAY((user->rand+8), num);
	num = (uint32_t)rand();
	UINT32_TO_ARRAY((user->rand+12), num);
}

void complete_user_info(UserInfo * user)
{
	/* TODO: generate the seed randomly */ 
	uint32_t hash;

	/* This function initialize the uncompleted user information */
	user->epc_nas_sequence_number = 0;
	user->next_hop_chaining_count = 1;
	/* To simplify the process, EPC TEID, MME UE S1AP ID, TMSI and PDN IP are going to be equal and generated from the IMSI */
	hash = hash_imsi_user_info(user);
	UINT32_TO_ARRAY(user->mme_ue_s1ap_id, hash);
	UINT32_TO_ARRAY(user->tmsi, hash);
	UINT32_TO_ARRAY(user->epc_teid, hash);
	UINT32_TO_ARRAY(user->epc_teid, hash);
	UINT32_TO_ARRAY(user->pdn_ipv4, (GATEWAY_IP + (hash & 0x0000FFFF))); /* PDN IP = GATEWAY IP + IMSI */
	/* Generate initial RAND */
	generate_rand(user);
}

void show_user_info(UserInfo * user)
{
	int i;
	uint32_t epc_teid, tmsi, mme_ue_s1ap_id;

	ARRAY_TO_UINT32(epc_teid, user->epc_teid);
	ARRAY_TO_UINT32(tmsi, user->tmsi);
	ARRAY_TO_UINT32(mme_ue_s1ap_id, user->mme_ue_s1ap_id);

	printf("\n");
	if(user == NULL) {
		printf("NULL UserInfo pointer\n");
		return;
	}
	printf("IMSI: %s\n", user->imsi);
	printf("KEY: ");
	for(i = 0; i < 16; i++) printf("%.2x ", user->key[i]);
	printf("\n");
	printf("OPC: ");
	for(i = 0; i < 16; i++) printf("%.2x ", user->opc[i]);
	printf("\n");
	printf("RAND: ");
	for(i = 0; i < 16; i++) printf("%.2x ", user->rand[i]);
	printf("\n");
	printf("EPC TEID: 0x%.8x\n", epc_teid);
	printf("TMSI: 0x%.8x\n", tmsi);
	printf("MME-UE-S1AP-ID: 0x%.8x\n", mme_ue_s1ap_id);
	printf("PDN IP: %d.%d.%d.%d\n", user->pdn_ipv4[0], user->pdn_ipv4[1], user->pdn_ipv4[2], user->pdn_ipv4[3]);
}

uint32_t hash_imsi(char * imsi)
{
	/* This function returns as a 32-bit integer the last 9 digits of the IMSI */
	return atoi(imsi+6);
}

uint32_t hash_imsi_user_info(void * user)
{
	return hash_imsi(((UserInfo *)user)->imsi);
}

uint32_t hash_msin(char * imsi)
{
	/* This function returns as a 32-bit integer the last 9 digits of the MSIN */
	return atoi(imsi+1);
}

uint32_t hash_msin_user_info(void * user)
{
	return hash_imsi(((UserInfo *)user)->imsi);
}

uint32_t hash_tmsi(uint8_t * tmsi)
{
	/* This function returns as a 32-bit integer the TMSI */
	uint32_t hash;
	ARRAY_TO_UINT32(hash, tmsi);
	return hash;
}

uint32_t hash_tmsi_user_info(void * user)
{
	return hash_tmsi(((UserInfo *)user)->tmsi);
}

uint32_t hash_mme_ue_s1ap_id(uint8_t * id)
{
	uint32_t hash;
	ARRAY_TO_UINT32(hash, id);
	return hash;
}

size_t user_info_size()
{
	return sizeof(UserInfo);
}
