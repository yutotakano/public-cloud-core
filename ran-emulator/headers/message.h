#ifndef __message__
#define __message__

#include <stdint.h>

#define OK_CODE 0x80
#define ERROR_CODE 0x7F


#define INIT_CODE 0x01
#define MOVE_TO_IDLE 0x02
#define UE_DETACH 0x03
#define UE_SWITCH_OFF_DETACH 0x04
#define UE_ATTACH 0x05
#define MOVE_TO_CONNECTED 0x06
#define HO_SETUP 0x07
#define HO_REQUEST 0x08
#define UE_TRANFSER 0x09
#define UE_S1_HANDOVER 0x0a
#define S1_SYNC 0x0b
#define S1_SYNC2 0x0c
#define S1_ERROR 0x0d


typedef struct _init_msg
{
	uint8_t id[4];
	uint8_t mcc[4];
	uint8_t mnc[4];
	uint8_t msin[12];
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


typedef struct _idle_msg
{
	uint8_t msin[16];
}idle_msg;

typedef struct _ho_msg
{
	uint8_t msin[16];
	uint8_t target_enb_ip[4];
}ho_msg;

typedef struct _idle_response_msg
{
	uint8_t teid[4];
	uint8_t spgw_ip[4];
}idle_response_msg;

#endif
