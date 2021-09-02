#ifndef __LIBCK_H__
#define __LIBCK_H__

#include <stdint.h>

#define OK 0
#define ERROR -1

#define IMSI_LEN 16
#define MSIN_LEN 11
#define KEY_LEN 16
#define ENB_UE_S1AP_ID_LEN 4
#define MME_UE_S1AP_ID_LEN 4
#define IP_LEN 4
#define TMSI_LEN 4
#define TEID_LEN 4
#define RAND_LEN 16
#define AUTH_RES_LEN 8

enum ITEM_TYPE
{
	IMSI=1,
	MSIN,
	TMSI,
	ENB_UE_S1AP_ID,
	UE_TEID,
	SPGW_IP,
	ENB_IP,
	PDN_IP,
	UE_NAS_SEQUENCE_NUMBER,
	EPC_NAS_SEQUENCE_NUMBER,
	UE_NAS_SEQUENCE_NUMBER_NO_INC,
	KEY,
	OPC,
	RAND,
	RAND_UPDATE,
	MME_UE_S1AP_ID,
	EPC_TEID,
	AUTH_RES,
	ENC_KEY,
	INT_KEY,
	KASME_1,
	KASME_2,
	EOM
};
typedef uint8_t ITEM_TYPE;

typedef struct corekubedb_pulls {
	uint8_t *head;
	uint8_t *imsi;
	uint8_t *msin;
	uint8_t *tmsi;
	uint8_t *enb_ue_s1ap_id;
	uint8_t *ue_teid;
	uint8_t *spgw_ip;
	uint8_t *enb_ip;
	uint8_t *pdn_ip;
	uint8_t *ue_nas_sequence_number;
	uint8_t *epc_nas_sequence_number;
	uint8_t *key;
	uint8_t *opc;
	uint8_t *rand;
	uint8_t *rand_update;
	uint8_t *mme_ue_s1ap_id;
	uint8_t *epc_teid;
	uint8_t *auth_res;
	uint8_t *enc_key;
	uint8_t *int_key;
	uint8_t *kasme1;
	uint8_t *kasme2;
} corekube_db_pulls_t;

void extract_db_values(uint8_t *buffer, int n, corekube_db_pulls_t *db_pulls);

int db_connect(char * ip_addr, int port);
void db_disconnect(int sock);
uint8_t * add_identifier(uint8_t * buffer, ITEM_TYPE id, uint8_t * value);
int push_items(uint8_t * buffer, ITEM_TYPE id, uint8_t * id_value, int num_items, ...);
uint8_t * push_item(uint8_t * buffer, ITEM_TYPE item, uint8_t * value);
int pull_items(uint8_t * buffer, int push_len, int num_items, ...);
uint8_t * pull_item(uint8_t * buffer, ITEM_TYPE item);
int send_request(int sock, uint8_t * buffer, int buffer_len);
int recv_response(int sock, uint8_t * buffer, int buffer_len);

#endif
