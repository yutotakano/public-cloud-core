#ifndef __message__
#define __message__

#include <stdint.h>

#define OK_CODE 0x80
#define ERROR_CODE 0x7F


#define INIT_CODE 0x01
typedef struct _init_msg
{
	uint8_t id[4];
	uint8_t mcc[4];
	uint8_t mnc[3];
	uint8_t msin[11];
	uint8_t key[16];
	uint8_t op_key[16];
	uint8_t ue_ip[4];
}init_msg;


typedef struct _init_response_msg
{
	uint8_t teid[4];
	uint8_t ue_ip[4];
	uint8_t spgw_ip[4];
}init_response_msg;

#endif