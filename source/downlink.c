#include <stddef.h>
#include "downlink.h"


void * downlink_thread(void * args)
{
	int sock_udp = ((downlink_args *)args)->sock_udp;

	/*
	while recv(msg):
		extract_sock_info(msg)
		send(sock_info, s1ap_msg);
	*/

	return NULL;
}