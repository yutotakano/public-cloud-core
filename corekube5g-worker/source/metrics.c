#include <stdarg.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include "core/ogs-core.h"
#include "metrics.h"

int metrics_connect(char * host, int port)
{
	int flag = 1;
	struct sockaddr_in metrics_addr;
	int sock;
	int res;

	/* Configure Metrics connection values */
	metrics_addr.sin_family = AF_INET;
	metrics_addr.sin_addr.s_addr = inet_addr(host);
	if(port == 0)
		metrics_addr.sin_port = htons(DEFAULT_METRICS_PORT);
	else
		metrics_addr.sin_port = htons(port);
	memset(&(metrics_addr.sin_zero), '\0', 8);

	/* Create socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		return -1;
  }

	/* Disable Nagle's algorithm */
	res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if(res < 0) {
		ogs_error("Error disabling Nagle's algorithm\n");
		close(sock);
		return -1;
	}

	/* Connect with Metrics server */
	if(connect(sock, (struct sockaddr *)&metrics_addr, sizeof(struct sockaddr)) == -1) {
		ogs_error("Unable to connect with %s:%d. Error Code: %i\n", host, port == 0 ? DEFAULT_METRICS_PORT : port, errno);
		close(sock);
		return -1;
	}
	return sock;
}

void metrics_disconnect(int sock)
{
	close(sock);
}

unsigned long long get_microtime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long)(tv.tv_sec) * 1000000 + (unsigned long long)(tv.tv_usec);
}

int metrics_send(int sock, worker_metrics_t *stats)
{
  int res;
  char buffer[256];
  int buffer_len = 0;
  ogs_debug("Creating metrics byte buffer\n");

  buffer_len += sprintf(buffer + buffer_len, "start_time:%llu|", stats->start_time);
	buffer_len += sprintf(buffer + buffer_len, "ue_id:%ld|", stats->ue_id);
	buffer_len += sprintf(buffer + buffer_len, "message_type:%d|", stats->message_type);
	buffer_len += sprintf(buffer + buffer_len, "invalid:%d|", stats->invalid);
	buffer_len += sprintf(buffer + buffer_len, "latency:%d|", stats->latency);
	buffer_len += sprintf(buffer + buffer_len, "handle_latency:%d|", stats->handle_latency);
	buffer_len += sprintf(buffer + buffer_len, "decode_latency:%d|", stats->decode_latency);
	buffer_len += sprintf(buffer + buffer_len, "encode_latency:%d|", stats->encode_latency);
	buffer_len += sprintf(buffer + buffer_len, "send_latency:%d|", stats->send_latency);
	buffer_len += sprintf(buffer + buffer_len, "end_time:%llu\n", stats->end_time);

	ogs_trace("Sending metrics data: %s", buffer);
  res = send(sock, buffer, buffer_len, 0);
  if (res < 0) {
    ogs_error("Error sending metrics data\n");
    return -1;
  }

  return 0;
}
