#ifndef __enb_emulator__
#define __enb_emulator__

#include <stdint.h>

typedef struct _enb_data
{
	uint8_t id[4];
	uint8_t mcc[4];
	uint8_t mnc[4];
	uint8_t epc_ip[4];
	uint8_t enb_ip[4];
} enb_data;

int enb_emulator_start(enb_data * data);
void enb_copy_id_to_buffer(uint8_t * buffer);
void enb_copy_port_to_buffer(uint8_t * buffer);

#endif