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

int metrics_send(int sock, worker_stats_t *stats)
{
  int res;
  char buffer[1024];
  int buffer_len = 0;
  ogs_debug("Sending metrics data\n");

  buffer_len += sprintf(buffer, "num_imsi_pulls:%d\n", stats->num_imsi_pulls);

  res = send(sock, buffer, buffer_len, 0);
  if (res < 0) {
    ogs_error("Error sending metrics data\n");
    return -1;
  }

  return 0;
}
