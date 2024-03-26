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

#include "udp_listener.h"
#include "yagra.h"

int __corekube_log_domain;
int db_sock;
yagra_conn_t metrics_conn;
pthread_mutex_t db_sock_mutex = PTHREAD_MUTEX_INITIALIZER;

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"
#include "nas/common/ogs-nas-common.h"
#include "ngap_handler.h"

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

void *process_message(void *raw_args) {
	process_message_args_t *args = (process_message_args_t *) raw_args;

	// free the dynamically-allocated buffer as soon as possible
	// since it takes up a relatively large amount of the limited
	// amount of available dynamically-allocatable memory (1KB of 7MB)
	uint8_t buffer[args->num_bytes_received];
	memcpy(buffer, args->buffer, args->num_bytes_received);
	ogs_free(args->buffer);

	ogs_info("New SCTP message received.");
	if (ogs_log_get_domain_level(__corekube_log_domain) >= OGS_LOG_TRACE)
		ogs_log_hexdump(OGS_LOG_INFO, buffer, args->num_bytes_received);

	message_handler_response_t response;
	yagra_batch_data_t batch = yagra_init_batch(args->metrics_conn);

	// initialise the default response values
	response.num_responses = 0;
	response.responses = ogs_malloc(sizeof(void *) * MAX_NUM_RESPONSES);
	response.batch = &batch;
	unsigned long long start_time = get_microtime();

	int outcome = ngap_handler_entrypoint(buffer+4, (args->num_bytes_received)-4, &response);
	ogs_assert(outcome == OGS_OK); // Failed to handle the message

	unsigned long long send_start = get_microtime();
	yagra_observe_metric(&batch, "num_responses", response.num_responses);
	if (response.num_responses == 0)
		ogs_info("Finished handling NO_RESPONSE message");

	args->client_addr->sin_port = htons(32566);

	// send the responses, if there are any
	for (int i = 0; i < response.num_responses; i++) {
		ogs_pkbuf_t *responseBuffer = response.responses[i];

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

	unsigned long long send_end = get_microtime();
	yagra_observe_metric(&batch, "send_latency", (int)(send_end - send_start));
	yagra_observe_metric(&batch, "latency", (int)(send_end - start_time));

	ogs_info("Finished handling message, sending metrics data");
	// If there is an IP to send metrics to, send them to args->metrics_addr
	int error = yagra_send_batch(&batch);
	if (error != 0) {
		ogs_warn("Error sending metrics data, error code %d", error);
	}
	ogs_info("Finished sending metrics data");

	// free the dynamically-allocated structures
	ogs_free(args->client_addr);
	ogs_free(args);
	ogs_free(response.responses);

	ogs_info("Finished processing message");

	return NULL;
}


void start_listener(char * mme_ip_address, yagra_conn_t * metrics_conn, int use_threads)
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

	// setup the thread attributes so the thread terminates when complete
	// this requires seting the detach state to DETACHED, rather than the
	// default value of JOINABLE
	pthread_attr_t thread_attr;
	int attr_init = pthread_attr_init(&thread_attr);
	ogs_assert(attr_init == 0);
	int detach_state = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	ogs_assert(detach_state == 0);

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
		args->metrics_conn = metrics_conn;

		if (use_threads) {
			pthread_t thread;
			int thread_create = pthread_create(&thread, &thread_attr, process_message, (void *) args);
			ogs_assert(thread_create == 0); // Failed to create thread
		}
		else {
			process_message((void *) args);
		}
	}

	ogs_assert(n != -1); // An UDP error occured

	/* Close the socket when done */
	close(sock_udp);

}


int main(int argc, char const *argv[])
{
	if(argc < 3 || argc > 6) {
		printf("RUN: ./corekube_udp_listener <WORKER_IP_ADDRESS> <DB_IP_ADDRESS> [LOG_LEVEL=4] [THREADS=1] [METRICS_IP_ADDRESS]\n");
		return 1;
	}

	// turn off printf buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// in production, turn off info logs
	int default_log_level = OGS_LOG_INFO;
	if (argc >= 4 && atoi(argv[3]) >= OGS_LOG_NONE && atoi(argv[3]) <= OGS_LOG_FULL)
		default_log_level = atoi(argv[3]);

	// determine whether threading should be used
	int use_threads = 1;
	if (argc >= 5 && ! atoi(argv[4]))
		use_threads = 0;

	// initialise the Open5GS core library
	ogs_core_initialize();
	ogs_core()->log.level = default_log_level;

	// initialise the logs for the libraries being used
	ogs_log_install_domain(&__ogs_ngap_domain, "ngap", default_log_level);
	ogs_log_install_domain(&__ogs_nas_domain, "nas", default_log_level);

	// initialise the logs for corekube
	ogs_log_install_domain(&__corekube_log_domain, "ck", default_log_level);
	ogs_log_set_domain_level(OGS_LOG_DOMAIN, default_log_level);

	// create the default pkbuf size config
	ogs_pkbuf_config_t config;
	ogs_pkbuf_default_init(&config);

	// initialise the pkbuf pools with the above config
	ogs_pkbuf_default_create(&config);

	// connecto to the metrics server
	metrics_conn = yagra_init((char *)argv[5], 0);
	ogs_assert(metrics_conn.sock != -1);
	ogs_info("Metrics socket configured correctly.\n");

	// setup the DB IP address
	//db_ip_address = (char*) core_calloc(strlen((char *)argv[2]), sizeof(char));
	//memcpy(db_ip_address, (char *)argv[2], strlen((char *)argv[2]));
	db_sock = db_connect((char *)argv[2], 0);
	ogs_assert(db_sock != -1);
	ogs_info("DB socket configured correctly.\n");

	start_listener((char *)argv[1], &metrics_conn, use_threads);

	db_disconnect(db_sock);
	yagra_finalize(&metrics_conn);

	// delete the pkbuf pools
	ogs_pkbuf_default_destroy();

	// terminate the Open5GS core library
	ogs_core_terminate();

	return 0;
}
