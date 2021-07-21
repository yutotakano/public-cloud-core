#ifndef __UPLINK_H__
#define __UPLINK_H__

#include <stdint.h>

typedef struct _uplink_args
{
	int sock_enb;
	pthread_t d_thread_id;
	uint32_t epc_addr;
	uint32_t frontend_ip;
} uplink_args;

void * uplink_thread(void * args);

#endif