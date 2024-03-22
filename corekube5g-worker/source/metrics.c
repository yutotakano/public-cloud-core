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
#include <errno.h>

#include "core/ogs-core.h"
#include "metrics.h"

metrics_conn_t metrics_connect(char * host, int port)
{
	int flag = 1;
	struct sockaddr_in metrics_addr;
	int sock;
	int res;

	metrics_conn_t conn;
	conn.sock = -1;
	conn.host = NULL;
	conn.port = 0;

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
		return conn;
  }

	/* Disable Nagle's algorithm */
	res = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if(res < 0) {
		ogs_error("Error disabling Nagle's algorithm\n");
		close(sock);
		return conn;
	}

	/* Connect with Metrics server */
	if(connect(sock, (struct sockaddr *)&metrics_addr, sizeof(struct sockaddr)) == -1) {
		ogs_error("Unable to connect with %s:%d. Error Code: %i\n", host, port == 0 ? DEFAULT_METRICS_PORT : port, errno);
		close(sock);
		return conn;
	}

	conn.sock	= sock;
	conn.host	= host;
	conn.port	= port == 0 ? DEFAULT_METRICS_PORT : port;
	return conn;
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

int metrics_send(metrics_conn_t * conn, worker_metrics_t *stats)
{
  char buffer[512];
  int buffer_len = 0;
	
	int send_response_code = 0;

  ogs_debug("Creating metrics byte buffer\n");

	int header_len = 3;
  buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_start_time:%llu|", stats->start_time);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_ue_id:%ld|", stats->ue_id);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_ngap_type:%d|", stats->ngap_message_type);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_type:%d|", stats->nas_message_type);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_invalid:%d|", stats->invalid);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_latency:%d|", stats->latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_decode_latency:%d|", stats->decode_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_handle_latency:%d|", stats->handle_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_handle_latency", stats->nas_handle_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_build_latency:%d|", stats->response_build_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_encode_latency:%d|", stats->encode_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_send_latency:%d|", stats->send_latency);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_responses:%d|", stats->num_responses);
	buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_end_time:%llu\n", stats->end_time);

	// Sanity check length of buffer
	if(header_len + buffer_len > 512) {
		ogs_error("Send buffer overflow, %d\n", buffer_len);
		return -1;
	}

	// First byte is the length of following header bytes
	buffer[0] = 0x02;
	// We currently hardcode the message length to be less than 2^16 bytes long
	// using two bytes for the length
	// TODO: dynamically calculate header_len based on some maximum character
	// limit imposed on the metric descriptions * number of metrics
	buffer[1] = buffer_len & 0xFF;
	buffer[2] = (buffer_len >> 8) & 0xFF;

	send_response_code = send(conn->sock, buffer, header_len + buffer_len, 0);

	if(send_response_code < 0) {
		ogs_error("Error sending metrics data, error code %d: errno %i\n", send_response_code, errno);
		return -1;
	}

  return 0;
}
