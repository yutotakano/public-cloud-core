#ifndef __ue_emulator__
#define __ue_emulator__

#include <stdint.h>

typedef struct _ue_data
{
	uint8_t id[4];
	char mcc[4];
	char mnc[3];
	char msin[11];
	uint8_t key[16];
	uint8_t op_key[16];
	char * command;
	uint8_t enb_ip[4];
	uint8_t local_ip[4];
	uint8_t ue_ip[4];
	uint16_t spgw_port;
	uint16_t control_plane_len;
	uint8_t * control_plane;
} ue_data;

int ue_emulator_start(ue_data * data);
void ue_copy_id_to_buffer(uint8_t * buffer);

#endif