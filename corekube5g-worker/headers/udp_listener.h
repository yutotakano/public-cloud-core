#ifndef __COREKUBE_UDP_LISTENER_H__
#define __COREKUBE_UDP_LISTENER_H__

#define MME_LISTEN_PORT 5566
#define BUFFER_LEN 1024

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

#include "yagra.h"

int configure_udp_socket(char *mme_ip_address);

typedef struct process_message_args
{
	int sock_udp;
	socklen_t from_len;
	struct sockaddr_in *client_addr;
	uint8_t *buffer;
	int num_bytes_received;
	yagra_conn_t *metrics_conn;
} process_message_args_t;

void *process_message(void *raw_args);

void start_listener(char *mme_ip_address, yagra_conn_t *metrics_con, int use_threads);

int main(int argc, char const *argv[]);

#endif /* __COREKUBE_UDP_LISTENER_H__ */
