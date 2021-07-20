#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "uplink.h"
#include "log.h"

#define S1AP_PPID 18
#define BUFFER_LEN 1020
#define EPC_UDP_PORT 5566

void dumpMessage(uint8_t * message, int len)
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

void int_to_array(int num, uint8_t * buffer)
{
	buffer[0] = (num >> 24) & 0xFF;
	buffer[1] = (num >> 16) & 0xFF;
	buffer[2] = (num >> 8) & 0xFF;
	buffer[3] = num & 0xFF;
}

void reset_sctp_structures(struct sockaddr_in * addr, socklen_t * from_len, struct sctp_sndrcvinfo * sinfo, int * flags)
{
	memset((void *)addr, 0, sizeof(struct sockaddr_in));
	*from_len = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)sinfo, 0, sizeof(struct sctp_sndrcvinfo));
	*flags = 0;
}

void * uplink_thread(void * args)
{
	int flags = 0, n;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	struct sockaddr_in addr, epc_addr;
	uint8_t buffer[BUFFER_LEN+4];

	/* Extract the eNB socket from args and free the args structure */
	int sock_enb = ((uplink_args *)args)->sock_enb;
	int sock_epc = ((uplink_args *)args)->epc_sock;
	uint32_t epc_ip_addr = ((uplink_args *)args)->epc_addr;
	uint32_t frontend_ip = ((uplink_args *)args)->frontend_ip;
	free(args);
	#ifdef THREAD_LOGS
	printThread("Uplink thread arguments extracted.\n");
	#endif

	/* Configure EPC UDP address */
	epc_addr.sin_addr.s_addr = epc_ip_addr;
    epc_addr.sin_family = AF_INET;
    epc_addr.sin_port = htons(EPC_UDP_PORT);
    memset(&(epc_addr.sin_zero), 0, sizeof(struct sockaddr));
    #ifdef THREAD_LOGS
	printThread("Kubernetes loadbalancer information initialized.\n");
	#endif

	/* Reset structures */
	reset_sctp_structures(&addr, &from_len, &sinfo, &flags);

	/* Copy the socket number into the UDP/S1AP message to be used in the downlink thread */
	int_to_array(sock_enb, buffer);
	#ifdef THREAD_LOGS
	printThread("Socket data added to the buffer.\n");
	#endif

	while ( (n = sctp_recvmsg(sock_enb, (void *)(buffer + 4), BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sinfo, &flags)) > 0) {
		/* Only messages with PPID 18 are accepted */
		if(ntohl(sinfo.sinfo_ppid) != S1AP_PPID) {
			reset_sctp_structures(&addr, &from_len, &sinfo, &flags);
			continue;
		}

		#ifdef THREAD_LOGS
		printThread("Uplink message received.\n");
		dumpMessage(buffer+4, n);
		#endif

		int tmp_sock;
		struct sockaddr_in udp_addr;

		/*****************/
		/* Create socket */
		/*****************/
		tmp_sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (tmp_sock < 0) { 
			perror("socket creation failed"); 
			return NULL;
		}

		udp_addr.sin_addr.s_addr = frontend_ip;
		udp_addr.sin_family = AF_INET;
		udp_addr.sin_port = 0;

		/***********/
		/* Binding */
		/***********/
		if(bind(tmp_sock, (const struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
		{ 
			perror("Bind MME UDP socket"); 
			close(tmp_sock);
			return NULL;
		}

		n += 4;
		/* Send message to Kubernetes Loadbalancer (EPC Address) using UDP */
		sendto(tmp_sock, buffer, n, 0, (const struct sockaddr *) &epc_addr, sizeof(epc_addr));
		#ifdef THREAD_LOGS
		printThread("UDP message sent.\n");
		#endif
		close(tmp_sock);

		/* Reset structures */
		reset_sctp_structures(&addr, &from_len, &sinfo, &flags);
	}

	close(sock_enb);

	return NULL;
}
