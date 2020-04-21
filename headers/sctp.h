#ifndef __sctp__
#define __sctp__

#include "enb.h"

int sctp_get_sock_info (int sock);
int sctp_get_peer_addresses (int sock);
int sctp_get_local_addresses(int sock);

int sctp_connect_enb_to_mme(eNB * enb, uint8_t * mme_ip);

#endif