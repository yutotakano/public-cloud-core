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

char *db_ip_address;

int configure_sctp_socket(char * mme_ip_address)
{
	d_info("Configuring SCTP socket");

	int sock_sctp;
	struct sockaddr_in listener_addr;
	struct sctp_event_subscribe events;
    struct sctp_initmsg init;


	/*****************/
	/* Create socket */
	/*****************/
	sock_sctp = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	d_assert(sock_sctp >= 0, return -1, "Failed to setup SCTP socket");


	/****************************/
	/* Configure socket options */
	/****************************/
	/* OPT 1 */
    /* Request a number of in/out streams */
    init.sinit_num_ostreams = SCTP_OUT_STREAMS;
    init.sinit_max_instreams = SCTP_IN_STREAMS;
    init.sinit_max_attempts = SCTP_MAX_ATTEMPTS;
    init.sinit_max_init_timeo = SCTP_TIMEOUT;
	int sockopt_outcome = setsockopt (sock_sctp, IPPROTO_SCTP, SCTP_INITMSG, &init, (socklen_t) sizeof (struct sctp_initmsg));
	d_assert(sockopt_outcome >= 0, close(sock_sctp); return -1, "Failed to set SCTP socket in/out streams");

    /* OPT 2 */
    /* Subscribe to all events */
    memset(&events, 0, sizeof(events));
    events.sctp_data_io_event = 1;
    events.sctp_shutdown_event = 1;
    //memset((void *)&events, 1, sizeof(struct sctp_event_subscribe));
	sockopt_outcome = setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe));
	d_assert(sockopt_outcome >= 0, close(sock_sctp); return -1, "Failed to set SCTP socket events");

    /* OPT 3 */
    /* Set SCTP_NODELAY option to disable Nagle algorithm */
    int nodelay = 1;
	sockopt_outcome = setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay));
	d_assert(sockopt_outcome >= 0, close(sock_sctp); return -1, "Failed to set SCTP socket no_delay");


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
	int sock_bind = sctp_bindx(sock_sctp,(struct sockaddr*)&listener_addr, 1, SCTP_BINDX_ADD_ADDR);
	d_assert(sock_bind != -1, close(sock_sctp); return -1, "Failed to bind MME socket");
	
	/**********/
	/* Listen */
	/**********/
	int sock_listen = listen(sock_sctp, SOCKET_LISTEN_QUEUE);
	d_assert(sock_listen >= 0, close(sock_sctp); return -1, "Failed to listen on MME socket");

	return sock_sctp;
}


void start_listener(char * mme_ip_address)
{
	d_info("Starting SCTP listener");

	int sock_sctp;
	int sock_enb;
	int flags = 0, n;
	socklen_t from_len;
	struct sctp_sndrcvinfo sinfo;
	struct sockaddr_in addr;
	uint8_t buffer[BUFFER_LEN];


	sock_sctp = configure_sctp_socket(mme_ip_address);
	d_assert(sock_sctp >= 0, return, "Error configuring SCTP socket");
	d_info("SCTP socket configured correctly");

	/* This prototype only accepts one client (eNB) at the time */
	sock_enb = accept(sock_sctp, NULL, NULL);
	d_assert(sock_enb >= 0, return, "MME socket failed to accept");
	d_info("New eNB connected");

	/* Receive S1AP messages and dump the content */
	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	from_len = (socklen_t)sizeof(struct sockaddr_in);
	memset((void *)&sinfo, 0, sizeof(struct sctp_sndrcvinfo));

	while ( (n = sctp_recvmsg(sock_enb, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sinfo, &flags)) >= 0) {
		d_assert(n > 0, break, "No longer connected to eNB");

		d_info("Received with PPID: %d", ntohl(sinfo.sinfo_ppid));
		d_print_hex(buffer, n);

		//////////////////////////////////////////////////////////
		S1AP_handler_response_t response;
		status_t outcome = s1ap_handler_entrypoint(buffer, n, &response);
		d_assert(outcome == CORE_OK, continue, "Failed to handle S1AP message");

		if (response.outcome == NO_RESPONSE) {
			d_info("Finished handling NO_RESPONSE message");
			continue;
		}

		// handle the first response, if there is one
		if (response.outcome == HAS_RESPONSE || response.outcome == DUAL_RESPONSE) {
			d_info("Handling a message with a response");
			pkbuf_t *setupResponsePkBuf = response.response;
			int ret = sctp_sendmsg(sock_enb, (void *) setupResponsePkBuf->payload, setupResponsePkBuf->len, NULL, 0, htonl(18), 0, (uint16_t) response.sctpStreamID, 0, 0 );
			pkbuf_free(setupResponsePkBuf);
			d_assert(ret != -1, continue, "Failed to send SCTP message");
			d_info("Sent %d bytes over SCTP", ret);
		}

		// handle the (optional) second response
		if (response.outcome == DUAL_RESPONSE) {
			d_info("Handling the second response");
			pkbuf_t *setupResponsePkBuf = response.response2;
			int ret = sctp_sendmsg(sock_enb, (void *) setupResponsePkBuf->payload, setupResponsePkBuf->len, NULL, 0, htonl(18), 0, (uint16_t) response.sctpStreamID, 0, 0 );
			pkbuf_free(setupResponsePkBuf);
			d_assert(ret != -1, continue, "Failed to send SCTP message");
			d_info("Sent %d bytes over SCTP", ret);
		}

		

		/////////////////////////////////////////////////////////
	}

	d_assert(n != -1,, "An SCTP error occured");

	close(sock_enb);
	close(sock_sctp);

}


int main(int argc, char const *argv[])
{
	if(argc != 3) {
		printf("RUN: ./corekube_sctp_listener <WORKER_IP_ADDRESS> <DB_IP_ADDRESS>\n");
		return 1;
	}
	core_initialize();

	// setup the DB IP address
	db_ip_address = (char*) core_calloc(strlen((char *)argv[2]), sizeof(char));
	memcpy(db_ip_address, (char *)argv[2], strlen((char *)argv[2]));

	start_listener((char *)argv[1]);

	core_free(db_ip_address);

	return 0;
}
