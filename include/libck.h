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
	EOM=1, /* 0x01 */
	IMSI, /* 0x02 */
	MSIN, /* 0x03 */
	TMSI, /* 0x04 */
	ENB_UE_S1AP_ID, /* 0x05 */
	UE_TEID, /* 0x06 */
	SPGW_IP, /* 0x07 */
	ENB_IP, /* 0x08 */
	PDN_IP, /* 0x09 */
	UE_NAS_SEQUENCE_NUMBER, /* 0x0A */
	EPC_NAS_SEQUENCE_NUMBER, /* 0x0B */
	UE_NAS_SEQUENCE_NUMBER_NO_INC, /* 0x0C */
	KEY, /* 0x0D */
	OPC, /* 0x0E */
	RAND, /* 0x0F */
	RAND_UPDATE, /* 0x10 */
	MME_UE_S1AP_ID, /* 0x11 */
	EPC_TEID, /* 0x12 */
	AUTH_RES, /* 0x13 */
	ENC_KEY, /* 0x14 */
	INT_KEY, /* 0x15 */
	KASME_1, /* 0x16 */
	KASME_2, /* 0x17 */
	NEW_ENB, /* 0x18 */
	GET_ENB, /* 0x19 */
	KNH_1, /* 0x1A */
	KNH_2, /* 0x1B */
	Target_ENB_UE_S1AP_ID, /* 0x1C */
	NEXT_HOP_CHAINING_COUNT /* 0x1D */
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
	uint8_t *knh1;
	uint8_t *knh2;
	uint8_t *ncc;
	uint8_t *target_enb_ue_s1ap_id;
	uint8_t *get_enb;
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
