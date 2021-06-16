#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libck.h>
#include <pthread.h>

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"

#define MME_LISTEN_PORT 5566
#define BUFFER_LEN 1024

// TODO: allow this file to compile
typedef enum S1AP_handle_outcome {NO_RESPONSE, HAS_RESPONSE, DUAL_RESPONSE} S1AP_handle_outcome_t;

typedef struct S1AP_handler_response {
    S1AP_handle_outcome_t outcome;
    // the SCTP stream ID differs depending on whether
    // this is a S1Setup message or a UE message
    uint8_t sctpStreamID;
    // the response can either be a pointer to an s1ap_message_t
    // or a pointer to a pkbuf_t, so use void* to allow it to
    // take on both types
    void * response;
    // an (optional) second response, for cases where a single
    // incoming messages must be responded with two outgoing messages
    void * response2;
} S1AP_handler_response_t;

int s1ap_handler_entrypoint(void *incoming, int incoming_len, S1AP_handler_response_t *response) {
	ogs_ngap_message_t msg;

	ogs_pkbuf_t * pkbuf = NULL;
	pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
	pkbuf->len = incoming_len;
	memcpy(pkbuf->data, incoming, pkbuf->len);

	int status = ogs_ngap_decode(&msg, pkbuf);
	ogs_pkbuf_free(pkbuf);

	return status;
}
//

int db_sock;
pthread_mutex_t db_sock_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	ogs_assert(sock_udp >= 0); // Failed to setup UDP socket


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
	ogs_assert(bind_outcome != -1); // Failed to bind MME socket

	return sock_udp;
}

// TODO: move to separate header
// or perhaps separate file (since
// process_message() might be able
// to be made SCTP / UDP independent?)
typedef struct process_message_args {
	int sock_udp;
	socklen_t from_len;
	struct sockaddr_in *client_addr;
	uint8_t *buffer;
	int num_bytes_received;
} process_message_args_t;

void *process_message(void *raw_args) {
	process_message_args_t *args = (process_message_args_t *) raw_args;

	// free the dynamically-allocated buffer as soon as possible
	// since it takes up a relatively large amount of the limited
	// amount of available dynamically-allocatable memory (1KB of 7MB)
	uint8_t buffer[args->num_bytes_received];
	memcpy(buffer, args->buffer, args->num_bytes_received);
	ogs_free(args->buffer);

	ogs_info("New SCTP message received:");
	ogs_log_hexdump(OGS_LOG_INFO, buffer, args->num_bytes_received);

	S1AP_handler_response_t response;

	int outcome = s1ap_handler_entrypoint(buffer+4, (args->num_bytes_received)-4, &response);
	ogs_assert(outcome == OGS_OK); // Failed to handle S1AP message

	if (response.outcome == NO_RESPONSE)
		ogs_info("Finished handling NO_RESPONSE message");

	args->client_addr->sin_port = htons(32566);

	// handle the first response, if there is one
	if (response.outcome == HAS_RESPONSE || response.outcome == DUAL_RESPONSE) {
		ogs_pkbuf_t *responseBuffer = response.response;

		uint8_t response_out[responseBuffer->len + 5];
		memcpy(response_out, buffer, 4);
		response_out[4] = response.sctpStreamID;
		memcpy(response_out+5, responseBuffer->data, responseBuffer->len);
		
		int ret = sendto(args->sock_udp, (void *)response_out, responseBuffer->len + 5,
			MSG_CONFIRM, (const struct sockaddr *) args->client_addr,
			args->from_len);

		ogs_pkbuf_free(responseBuffer);

		ogs_assert(ret != -1); // Failed to send UDP message
		ogs_info("Send %d bytes over UDP", ret);
	}

	// handle the (optional) second response
	if (response.outcome == DUAL_RESPONSE) {
		ogs_pkbuf_t *responseBuffer = response.response2;

		uint8_t response_out[responseBuffer->len + 5];
		memcpy(response_out, buffer, 4);
		response_out[4] = response.sctpStreamID;
		memcpy(response_out+5, responseBuffer->data, responseBuffer->len);
		
		int ret = sendto(args->sock_udp, (void *)response_out, responseBuffer->len + 5,
			MSG_CONFIRM, (const struct sockaddr *) args->client_addr,
			args->from_len);

		ogs_pkbuf_free(responseBuffer);

		ogs_assert(ret != -1); // Failed to send UDP message
		ogs_info("Send %d bytes over UDP", ret);
	}

	// free the dynamically-allocated structures
	ogs_free(args->client_addr);
	ogs_free(args);

	ogs_info("Finished processing message");

	return NULL;
}


void start_listener(char * mme_ip_address)
{
	int sock_udp;
	int n;
	socklen_t from_len;

	/* Initialise socket structs */
	from_len = (socklen_t)sizeof(struct sockaddr_in);

	/* Configure the socket */
	sock_udp = configure_udp_socket(mme_ip_address);
	ogs_assert(sock_udp >= 0); // Error configuring UDP socket
	ogs_info("UDP socket configured correctly.\n");

	while (1) {

		// setup variables for receiving a message
		process_message_args_t *args = ogs_calloc(1, sizeof(process_message_args_t));
		struct sockaddr_in *client_addr = ogs_calloc(1, sizeof(struct sockaddr_in));
		uint8_t *buffer = ogs_malloc(BUFFER_LEN);

		/* Wait to receive a message */
		n = recvfrom(sock_udp, (char *)buffer, BUFFER_LEN, MSG_WAITALL, ( struct sockaddr *) client_addr, &from_len); 
		ogs_assert(n > 0); // No longer connected to eNB

		// setup the arguments to be passed
		// to the multithreaded function
		args->buffer = buffer;
		args->client_addr = client_addr;
		args->from_len = from_len;
		args->num_bytes_received = n;
		args->sock_udp = sock_udp;

		pthread_t thread;
		int thread_create = pthread_create(&thread, NULL, process_message, (void *) args);
		ogs_assert(thread_create == 0); // Failed to create thread
	}

	ogs_assert(n != -1); // An UDP error occured

	/* Close the socket when done */
	close(sock_udp);

}


int main(int argc, char const *argv[])
{
	if(argc != 3 && argc != 4) {
		printf("RUN: ./corekube_udp_listener <WORKER_IP_ADDRESS> <DB_IP_ADDRESS> [PRODUCTION=0]\n");
		return 1;
	}

	// initialise the Open5GS core library
	ogs_core_initialize();

	// initialise the logs for the libraries being used
    ogs_log_install_domain(&__ogs_ngap_domain, "ngap", OGS_LOG_TRACE);

	// create the default pkbuf size config
	ogs_pkbuf_config_t config;
	ogs_pkbuf_default_init(&config);

	// initialise the pkbuf pools with the above config
	ogs_pkbuf_default_create(&config);

	// in production, turn off info logs
	if (argc == 4 && atoi(argv[3]))
		ogs_log_set_domain_level(OGS_LOG_DOMAIN, OGS_LOG_ERROR);

	// setup the DB IP address
	//db_ip_address = (char*) core_calloc(strlen((char *)argv[2]), sizeof(char));
	//memcpy(db_ip_address, (char *)argv[2], strlen((char *)argv[2]));
	db_sock = db_connect((char *)argv[2], 0);

	start_listener((char *)argv[1]);

	db_disconnect(db_sock);

	// delete the pkbuf pools
	ogs_pkbuf_default_destroy();

	// terminate the Open5GS core library
	ogs_core_terminate();

	return 0;
}
