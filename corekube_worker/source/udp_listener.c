#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "s1ap_handler.h"
#include "core/include/core_general.h"

#define MME_LISTEN_PORT 5566
#define BUFFER_LEN 1024

char *db_ip_address;

int configure_udp_socket(char * mme_ip_address)
{
	int sock_udp;
	struct sockaddr_in listener_addr;

	/****************************/
	/* Initialise socket struct */
	/****************************/
	memset(&listener_addr, 0, sizeof(listener_addr));


	/*****************/
	/* Create socket */
	/*****************/
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	d_assert(sock_udp >= 0, return -1, "Failed to setup UDP socket");


	/***************************/
    /* Set up MME address */
    /***************************/
    listener_addr.sin_addr.s_addr = inet_addr(mme_ip_address); //INADDR_ANY;
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = htons(MME_LISTEN_PORT);
    memset(&(listener_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
	int bind_outcome = bind(sock_udp,(struct sockaddr*)&listener_addr, sizeof(listener_addr));
	d_assert(bind_outcome != -1, close(sock_udp); return -1, "Failed to bind MME socket");

	return sock_udp;
}


void start_listener(char * mme_ip_address)
{
	int sock_udp;
	int n;
	socklen_t from_len;
	struct sockaddr_in client_addr;
	uint8_t buffer[BUFFER_LEN];

	/* Initialise socket structs */
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	from_len = (socklen_t)sizeof(struct sockaddr_in);

	/* Configure the socket */
	sock_udp = configure_udp_socket(mme_ip_address);
	d_assert(sock_udp >= 0, return, "Error configuring UDP socket");
	d_info("UDP socket configured correctly.\n");

	while (1) {

		/* Wait to receive a message */
		n = recvfrom(sock_udp, (char *)buffer, BUFFER_LEN, MSG_WAITALL, ( struct sockaddr *) &client_addr, &from_len); 
		d_assert(n > 0, break, "No longer connected to eNB");
		
		d_print_hex(buffer, n);

		S1AP_handler_response_t response;

		status_t outcome = s1ap_handler_entrypoint(buffer+4, n-4, &response);
		d_assert(outcome == CORE_OK, continue, "Failed to handle S1AP message");		

		if (response.outcome == NO_RESPONSE) {
			d_info("Finished handling NO_RESPONSE message");
			continue;
		}

		// handle the first response, if there is one
		if (response.outcome == HAS_RESPONSE || response.outcome == DUAL_RESPONSE) {
			pkbuf_t *responseBuffer = response.response;

			uint8_t response_out[responseBuffer->len + 5];
			memcpy(response_out, buffer, 4);
			response_out[4] = response.sctpStreamID;
			memcpy(response_out+5, responseBuffer->payload, responseBuffer->len);
			
			int ret = sendto(sock_udp, (void *)response_out, responseBuffer->len + 5,
				MSG_CONFIRM, (const struct sockaddr *) &client_addr,
				from_len);

			pkbuf_free(responseBuffer);

			d_assert(ret != -1, continue, "Failed to send SCTP message");
			d_info("Send %d bytes over UDP", ret);
		}

		// handle the (optional) second response
		if (response.outcome == DUAL_RESPONSE) {
			pkbuf_t *responseBuffer = response.response2;

			uint8_t response_out[responseBuffer->len + 5];
			memcpy(response_out, buffer, 4);
			response_out[4] = response.sctpStreamID;
			memcpy(response_out+5, responseBuffer->payload, responseBuffer->len);
			
			int ret = sendto(sock_udp, (void *)response_out, responseBuffer->len + 5,
				MSG_CONFIRM, (const struct sockaddr *) &client_addr,
				from_len);

			pkbuf_free(responseBuffer);

			d_assert(ret != -1, continue, "Failed to send SCTP message");
			d_info("Send %d bytes over UDP", ret);
		}
	}

	d_assert(n != -1,, "An SCTP error occured");

	/* Close the socket when done */
	close(sock_udp);

}


int main(int argc, char const *argv[])
{
	if(argc != 3) {
		printf("RUN: ./corekube_udp_listener <WORKER_IP_ADDRESS> <DB_IP_ADDRESS>\n");
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
