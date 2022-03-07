#include <stdarg.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libck.h"
#include "log.h"

#define DEFAULT_DB_PORT 7788

int db_connect(char * ip_addr, int port)
{
	int flag = 1;
	struct sockaddr_in db_addr;
	int sock;
	int res;

	/* Configure DB */
	db_addr.sin_family = AF_INET;
	db_addr.sin_addr.s_addr = inet_addr(ip_addr);
	if(port == 0)
		db_addr.sin_port = htons(DEFAULT_DB_PORT);
	else
		db_addr.sin_port = htons(port);
	memset(&(db_addr.sin_zero), '\0', 8);

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		return -1;

	/* Disable Nagle's algorithm */
	res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if(res < 0) {
		printError("Error disabling Nagle's algorithm\n");
		close(sock);
		return -1;
	}

	/* Connect with DB */
	if(connect(sock, (struct sockaddr *)&db_addr, sizeof(struct sockaddr)) == -1) {
		printError("Unable to connect with %s:%d.\n", ip_addr, port);
		close(sock);
		return -1;
	}
	return sock;
}

void db_disconnect(int sock)
{
	close(sock);
}

uint8_t * add_identifier(uint8_t * buffer, ITEM_TYPE id, uint8_t * value)
{
	switch(id) {
		case IMSI:
			buffer[0] = IMSI;
			memcpy(buffer+1, value, IMSI_LEN);
			buffer[16] = 0;
			break;
		case MSIN:
			buffer[0] = MSIN;
			memcpy(buffer+1, value, MSIN_LEN);
			break;
		case TMSI:
			buffer[0] = TMSI;
			memcpy(buffer+1, value, TMSI_LEN);
			break;
		case MME_UE_S1AP_ID:
			buffer[0] = MME_UE_S1AP_ID;
			memcpy(buffer+1, value, MME_UE_S1AP_ID_LEN);
			break;
		default:
			return buffer;
	}
	return buffer+17;
}

uint8_t * push_item(uint8_t * buffer, ITEM_TYPE item, uint8_t * value)
{
	switch(item) {
		case MME_UE_S1AP_ID:
			buffer[0] = MME_UE_S1AP_ID;
			memcpy(buffer+1, value, MME_UE_S1AP_ID_LEN);
			break;
		case ENB_UE_S1AP_ID:
			buffer[0] = ENB_UE_S1AP_ID;
			memcpy(buffer+1, value, ENB_UE_S1AP_ID_LEN);
			break;
		case Target_ENB_UE_S1AP_ID:
			buffer[0] = Target_ENB_UE_S1AP_ID;
			memcpy(buffer+1, value, ENB_UE_S1AP_ID_LEN);
			break;
		case UE_TEID:
			buffer[0] = UE_TEID;
			memcpy(buffer+1, value, TEID_LEN);
			break;
		case SPGW_IP:
			buffer[0] = SPGW_IP;
			memcpy(buffer+1, value, IP_LEN);
			break;
		case ENB_IP:
			buffer[0] = ENB_IP;
			memcpy(buffer+1, value, IP_LEN);
			break;
		case PDN_IP:
			buffer[0] = PDN_IP;
			memcpy(buffer+1, value, IP_LEN);
			break;
		case EPC_NAS_SEQUENCE_NUMBER:
			buffer[0] = EPC_NAS_SEQUENCE_NUMBER;
			buffer[1] = *value;
			break;
		case UE_NAS_SEQUENCE_NUMBER:
			buffer[0] = UE_NAS_SEQUENCE_NUMBER;
			buffer[1] = *value;
			break;
		case NEXT_HOP_CHAINING_COUNT:
			buffer[0] = NEXT_HOP_CHAINING_COUNT;
			buffer[1] = *value;
			break;
		case AUTH_RES:
			buffer[0] = AUTH_RES;
			memcpy(buffer+1, value, AUTH_RES_LEN);
			break;
		case ENC_KEY:
			buffer[0] = ENC_KEY;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case INT_KEY:
			buffer[0] = INT_KEY;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case KASME_1:
			buffer[0] = KASME_1;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case KASME_2:
			buffer[0] = KASME_2;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case KNH_1:
			buffer[0] = KNH_1;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case KNH_2:
			buffer[0] = KNH_2;
			memcpy(buffer+1, value, KEY_LEN);
			break;
		case ENB_SOURCE_SOCKET:
			buffer[0] = ENB_SOURCE_SOCKET;
			memcpy(buffer+1, value, 4);
			break;
		case ENB_TARGET_SOCKET:
			buffer[0] = ENB_TARGET_SOCKET;
			memcpy(buffer+1, value, 4);
			break;
		default:
			/* Undefined Item */
			return buffer;
	}
	return buffer+17;
}

int push_items(uint8_t * buffer, ITEM_TYPE id, uint8_t * id_value, int num_items, ...)
{
	int i;
	uint8_t * item, * ptr;
	ITEM_TYPE type;
	va_list ap;

	va_start(ap, num_items);
	/* Special case where the ID is NEW_ENB */
	if(id == NEW_ENB) {
		ptr = buffer;
		ptr[0] = NEW_ENB;
		ptr += 17;
	}
	else {
		ptr = add_identifier(buffer, id, id_value);
	}

	for(i = 0; i < num_items; i++) {
		type = (ITEM_TYPE) va_arg(ap, int);
		/* Special case: NEW_ENB (arg0) + eNB_ID (arg1) + eNB_SOCK_NUM (arg2) */
		if(type == NEW_ENB) {
			ptr[0] = NEW_ENB;
			item = va_arg(ap, uint8_t*); /* eNB_ID */
			memcpy(ptr+1, item, 4);
			item = va_arg(ap, uint8_t*); /* eNB_SOCK_NUM */
			memcpy(ptr+5, item, 4);
			ptr += 17;
		}
		else {
			item = va_arg(ap, uint8_t*);
			ptr = push_item(ptr, type, item);
		}
	}

	va_end(ap);

	return (int)(ptr - buffer);
}

uint8_t * pull_item(uint8_t * buffer, ITEM_TYPE item)
{
	buffer[0] = item;
	return buffer+1;
}

int pull_items(uint8_t * buffer, int push_len, int num_items, ...)
{
	int i;
	uint8_t * ptr, * enb;
	ITEM_TYPE item;
	va_list ap;

	va_start(ap, num_items);
	buffer[push_len] = 0;
	ptr = (uint8_t *)(buffer + push_len + 1);

	for(i = 0; i < num_items; i++) {
		item = (ITEM_TYPE) va_arg(ap, int);
		ptr = pull_item(ptr, item);
		/* Special case: GET_ENB (1Byte) + ENB_ID (4Bytes) */
		if(item == GET_ENB) {
			enb = va_arg(ap, uint8_t*);
			memcpy(ptr, enb, 4);
			ptr += 4;
		}

	}
	/* Add End-Of-Message byte */
	ptr = pull_item(ptr, EOM);

	va_end(ap);

	return (int)(ptr - buffer);
}

void extract_db_values(uint8_t *buffer, int n, corekube_db_pulls_t *db_pulls) {

	for (int i = 0; i < n; i += 17) {
		switch(buffer[i]) {
			case IMSI:
				db_pulls->imsi = buffer+i+1;
				break;
			case MSIN:
				db_pulls->msin = buffer+i+1;
				break;
			case TMSI:
				db_pulls->tmsi = buffer+i+1;
				break;
			case ENB_UE_S1AP_ID:
				db_pulls->enb_ue_s1ap_id = buffer+i+1;
				break;
			case UE_TEID:
				db_pulls->ue_teid = buffer+i+1;
				break;
			case SPGW_IP:
				db_pulls->spgw_ip = buffer+i+1;
				break;
			case ENB_IP:
				db_pulls->enb_ip = buffer+i+1;
				break;
			case PDN_IP:
				db_pulls->pdn_ip = buffer+i+1;
				break;
			case UE_NAS_SEQUENCE_NUMBER:
				db_pulls->ue_nas_sequence_number = buffer+i+1;
				break;
			case EPC_NAS_SEQUENCE_NUMBER:
				db_pulls->epc_nas_sequence_number = buffer+i+1;
				break;
			case KEY:
				db_pulls->key = buffer+i+1;
				break;
			case OPC:
				db_pulls->opc = buffer+i+1;
				break;
			case RAND:
				db_pulls->rand = buffer+i+1;
				break;
			case RAND_UPDATE:
				// RAND_UPDATE is a special case, it just returns
				// RAND but ensures the next RAND returns is random
				// (as opposed to the value saved in the DB)
				db_pulls->rand = buffer+i+1;
				break;
			case MME_UE_S1AP_ID:
				db_pulls->mme_ue_s1ap_id = buffer+i+1;
				break;
			case EPC_TEID:
				db_pulls->epc_teid = buffer+i+1;
				break;
			case AUTH_RES:
				db_pulls->auth_res = buffer+i+1;
				break;
			case ENC_KEY:
				db_pulls->enc_key = buffer+i+1;
				break;
			case INT_KEY:
				db_pulls->int_key = buffer+i+1;
				break;
			case KASME_1:
				db_pulls->kasme1 = buffer+i+1;
				break;
			case KASME_2:
				db_pulls->kasme2 = buffer+i+1;
				break;
			default:
				printError("Unre] item\n");
				break;
		}
	}
}

int send_request(int sock, uint8_t * buffer, int buffer_len)
{
	return  send(sock, buffer, buffer_len, 0);
}

int recv_response(int sock, uint8_t * buffer, int buffer_len)
{
	return recv(sock, buffer, buffer_len, 0);
}
