#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include "log.h"

#include "s1ap_handler.h"
#include "core/include/core_general.h"

#define MME_LISTEN_PORT 36412
#define SCTP_OUT_STREAMS 32
#define SCTP_IN_STREAMS 32
#define SCTP_MAX_ATTEMPTS 2
#define SCTP_TIMEOUT 1
#define SOCKET_READ_TIMEOUT_SEC 1
#define SOCKET_LISTEN_QUEUE 5 /* Extract from openair-cn (openair-cn/SCTP/sctp_primitives_server.c) */
#define BUFFER_LEN 1024


void dumpMessage(uint8_t * message, int len)
{
	int i;
	printf("(%d)\n", len);
	for(i = 0; i < len; i++)
	{
		if( i % 16 == 0)
			printf("\n");
		printf("%.2x ", message[i]);
	}
	printf("\n");
}

int configure_sctp_socket(char * mme_ip_address)
{
	int sock_sctp;
	struct sockaddr_in listener_addr;
	struct sctp_event_subscribe events;
    struct sctp_initmsg init;


	/*****************/
	/* Create socket */
	/*****************/
	sock_sctp = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(sock_sctp < 0) {
		perror("Socket SCTP setup");
		return -1;
	}


	/****************************/
	/* Configure socket options */
	/****************************/
	/* OPT 1 */
    /* Request a number of in/out streams */
    init.sinit_num_ostreams = SCTP_OUT_STREAMS;
    init.sinit_max_instreams = SCTP_IN_STREAMS;
    init.sinit_max_attempts = SCTP_MAX_ATTEMPTS;
    init.sinit_max_init_timeo = SCTP_TIMEOUT;
    if (setsockopt (sock_sctp, IPPROTO_SCTP, SCTP_INITMSG, &init, (socklen_t) sizeof (struct sctp_initmsg)) < 0) {
        perror("setsockopt in/out streams");
        close(sock_sctp);
        return -1;
    }

    /* OPT 2 */
    /* Subscribe to all events */
    memset(&events, 0, sizeof(events));
    events.sctp_data_io_event = 1;
    events.sctp_shutdown_event = 1;
    //memset((void *)&events, 1, sizeof(struct sctp_event_subscribe));
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe)) < 0) {
        perror("setsockopt events");
        close(sock_sctp);
        return -1;
    }

    /* OPT 3 */
    /* Set SCTP_NODELAY option to disable Nagle algorithm */
    int nodelay = 1;
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt no_delay");
        close(sock_sctp);
        return -1;
    }


	/***************************/
    /* Set up SCTP MME address */
    /***************************/
    listener_addr.sin_addr.s_addr = inet_addr(mme_ip_address);
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = htons(MME_LISTEN_PORT);
    memset(&(listener_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
    if (sctp_bindx(sock_sctp,(struct sockaddr*)&listener_addr, 1, SCTP_BINDX_ADD_ADDR) == -1) {
        perror("Bind MME socket");
        close(sock_sctp);
        return -1;
    }
	
	/**********/
	/* Listen */
	/**********/
	if (listen(sock_sctp, SOCKET_LISTEN_QUEUE) < 0) {
    	perror("MME Socket listen");
    	close(sock_sctp);
		return -1;
	}

	return sock_sctp;
}


void start_listener(char * mme_ip_address)
{
	int sock_sctp;
	int sock_enb;
	int flags = 0, n;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	struct sockaddr_in addr;
	uint8_t buffer[BUFFER_LEN];


	sock_sctp = configure_sctp_socket(mme_ip_address);
	if(sock_sctp < 0) {
		printError("Error configuring SCTP socket.\n");
		return;
	}

	printOK("SCTP socket configured correctly.\n");

	/* This prototype only accepts one client (eNB) at the time */
	if ((sock_enb = accept(sock_sctp, NULL, NULL)) < 0) {
		perror("MME Socket accept");
		return;
	}

	printOK("New eNB connected.\n");

	/* Receive S1AP messages and dump the content */
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	from_len = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));

	int stream_id = 0;

	while ( (n = sctp_recvmsg(sock_enb, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sinfo, &flags)) >= 0) {
		if (n == 0) {
			// something has gone wrong, we're no longer connected
			break;
		}

		printf("Received with PPID: %d\n", ntohl(sinfo.sinfo_ppid));
		dumpMessage(buffer, n);

		//////////////////////////////////////////////////////////
		pkbuf_t *setupResponsePkBuf;
		S1AP_handle_outcome_t outcome = s1ap_handler_entrypoint(buffer, n, &setupResponsePkBuf);
		if (outcome == NO_RESPONSE)
			continue;

		int ret = sctp_sendmsg(sock_enb, (void *) setupResponsePkBuf->payload, setupResponsePkBuf->len, NULL, 0, htonl(18), 0, stream_id, 0, 0 );

		stream_id = 1;

		pkbuf_free(setupResponsePkBuf);

		if (ret == -1)
		{
                	printError("Failed to send SCTP message.\n");
		}
		else
		{
			printf("Send %d bytes over SCTP.\n", ret);
		}
		/////////////////////////////////////////////////////////
	}

	close(sock_enb);
	close(sock_sctp);

}


int main(int argc, char const *argv[])
{
	if(argc != 2) {
		printf("RUN: ./listener <MME_IP_ADDRESS>\n");
		return 1;
	}
	core_initialize();

	start_listener((char *)argv[1]);

	return 0;
}
