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
#define NGAP_PPID 60

void dumpDownlink(uint8_t * message, int len)
{
	int i;
	printf("Message dump (%d):", len);
	for(i = 0; i < len; i++) {
		if( i % 16 == 0)
			printf("\n");
		printf("%.2x ", message[i]);
	}
	printf("\n");
}

int array_to_int(uint8_t * buffer)
{
	return (int)((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]);
}

void * downlink_thread(void * args)
{
	int n, sock_enb, sid;
	uint8_t buffer[BUFFER_LEN];
	uint32_t ppid;

	int sock_udp = ((downlink_args *)args)->sock_udp;
	if(((downlink_args *)args)->flag_5g == 1)
		ppid = htonl(NGAP_PPID);
	else
		ppid = htonl(S1AP_PPID);
	free(args);
	#ifdef THREAD_LOGS
	printThread("Downlink thread arguments extracted.\n");
	#endif

	
	while( (n = recv(sock_udp, buffer, BUFFER_LEN, 0)) > 0) {
		#ifdef THREAD_LOGS
		printThread("Downlink message received.\n");
		dumpDownlink(buffer+5, n-5);
		#endif
		sock_enb = array_to_int(buffer);
		sid = (int) buffer[4];
		n -= 5;
		if(sctp_sendmsg(sock_enb, buffer+5, n, NULL, 0, ppid, 0, sid, 0, 0) < 0)
			break;
		#ifdef THREAD_LOGS
		printThread("Downlink message sent.\n");
		#endif
	}

	return NULL;
}
