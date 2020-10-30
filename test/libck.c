#include <stdarg.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "libck.h"
#include "log.h"

#define DEFAULT_DB_PORT 7788

int db_connect(char * ip_addr, int port)
{
	struct sockaddr_in db_addr;
	int sock;

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
		return ERROR;

	/* Connect with DB */
	if(connect(sock, (struct sockaddr *)&db_addr, sizeof(struct sockaddr)) == -1) {
		printError("Unable to connect with %s:%d.\n", ip_addr, port);
		close(sock);
		return ERROR;
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
		default:
			return buffer;
	}
	return buffer+17;
}

uint8_t * push_item(uint8_t * buffer, ITEM_TYPE item, uint8_t * value)
{
	switch(item) {
		case ENB_UE_S1AP_ID:
			buffer[0] = ENB_UE_S1AP_ID;
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
	ptr = add_identifier(buffer, id, id_value);

	printf("1\n");

	for(i = 0; i < num_items; i++) {
		type = (ITEM_TYPE) va_arg(ap, int);
		item = va_arg(ap, uint8_t*);
		ptr = push_item(ptr, type, item);
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
	uint8_t * ptr;
	ITEM_TYPE item;
	va_list ap;

	va_start(ap, num_items);
	buffer[push_len] = 0;
	ptr = (uint8_t *)(buffer + push_len + 1);

	for(i = 0; i < num_items; i++) {
		item = (ITEM_TYPE) va_arg(ap, int);
		ptr = pull_item(ptr, item);
	}

	va_end(ap);

	return (int)(ptr - buffer);
}

void send_request(int sock, uint8_t * buffer, int buffer_len)
{
	send(sock, buffer, buffer_len, 0);
}

int recv_response(int sock, uint8_t * buffer, int buffer_len)
{
	return recv(sock, buffer, buffer_len, 0);
}