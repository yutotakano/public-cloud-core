#ifndef __ENB_H__
#define __ENB_H__

#include <stdint.h>
#include <stddef.h>

typedef struct _ENBInfo ENBInfo;

ENBInfo * create_enb(uint8_t * enb_id, uint8_t * sock_num);
uint32_t hash_enb_info(void * enb);
uint8_t * enb_get_socket_number(ENBInfo * enb);
size_t enb_size();

#endif