#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include "downlink.h"
#include "log.h"

#define BUFFER_LEN 1024
#define S1AP_PPID 18

int array_to_int(uint8_t * buffer)
{
	return (int)((buffer[0] << 24) | (buffer[0] << 16) | (buffer[0] << 8) | buffer[3]);
}

void * downlink_thread(void * args)
{
	int n, sock_enb, sid;
	uint8_t buffer[BUFFER_LEN];

	int sock_udp = ((downlink_args *)args)->sock_udp;
	free(args);
	#ifdef THREAD_LOGS
	printThread("Downlink thread arguments extracted.\n");
	#endif

	
	while( (n = recv(sock_udp, buffer, BUFFER_LEN, 0)) >= 0) {
		#ifdef THREAD_LOGS
		printThread("Downlink message received.\n");
		#endif
		sock_enb = array_to_int(buffer);
		sid = (int) buffer[4];
		n -= 5;
		sctp_sendmsg(sock_enb, buffer+5, n, NULL, 0, htonl(S1AP_PPID), 0, sid, 0, 0);
	}

	return NULL;
}
