#include "enb.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct _ENBInfo
{
	uint32_t enb_id;
	uint8_t sock_num[4];
};

ENBInfo * create_enb(uint8_t * enb_id, uint8_t * sock_num)
{
	ENBInfo * enb = (ENBInfo *) malloc(sizeof(ENBInfo));
	if(enb == NULL)
		return NULL;
	enb->enb_id = (enb_id[0] << 24) | (enb_id[1] << 16) | (enb_id[2] << 8) | enb_id[3];
	memcpy(enb->sock_num, sock_num, 4);
	return enb;
}

uint32_t hash_enb_info(void * enb)
{
	ENBInfo * e = (ENBInfo *)enb;
	return e->enb_id;
}

uint8_t * enb_get_socket_number(ENBInfo * enb)
{
	return enb->sock_num;
}


size_t enb_size()
{
	return sizeof(ENBInfo);
}