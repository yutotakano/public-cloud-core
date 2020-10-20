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

enum ITEM_TYPE
{
	IMSI=1,
	MSIN,
	TMSI,
	ENB_UE_S1AP_ID,
	UE_TEID, SPGW_IP,
	ENB_IP,
	PDN_IP,
	UE_NAS_SEQUENCE_NUMBER,
	EPC_NAS_SEQUENCE_NUMBER,
	KEY,
	OPC,
	RAND,
	RAND_UPDATE,
	MME_UE_S1AP_ID,
	EPC_TEID
};
typedef uint8_t ITEM_TYPE;

int db_connect(char * ip_addr, int port);
void db_disconnect(int sock);
int push_items(uint8_t * buffer, ITEM_TYPE id, uint8_t * id_value, int num_items, ...);
int pull_items(uint8_t * buffer, int push_len, int num_items, ...);
void send_request(int sock, uint8_t * buffer, int buffer_len);
int recv_response(int sock, uint8_t * buffer, int buffer_len);

#endif