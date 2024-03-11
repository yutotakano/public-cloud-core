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

#endif