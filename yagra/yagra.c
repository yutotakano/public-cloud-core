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
#include <sys/time.h>
#include <assert.h>

#include "yagra.h"

yagra_conn_t yagra_init(char * host, int port)
{
	int flag = 1;
	struct sockaddr_in metrics_addr;
	int sock;
	int res;

	yagra_conn_t conn;
	conn.sock = -1;
	conn.host = NULL;
	conn.port = 0;

	/* Configure Metrics connection values */
	metrics_addr.sin_family = AF_INET;
	metrics_addr.sin_addr.s_addr = inet_addr(host);
	if(port == 0)
		metrics_addr.sin_port = htons(YAGRA_DEFAULT_BUS_PORT);
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
		printf("Error disabling Nagle's algorithm\n");
		close(sock);
		return conn;
	}

	/* Connect with Metrics server */
	if(connect(sock, (struct sockaddr *)&metrics_addr, sizeof(struct sockaddr)) == -1) {
		printf("Unable to connect with %s:%d. Error Code: %i\n", host, port == 0 ? YAGRA_DEFAULT_BUS_PORT : port, errno);
		close(sock);
		return conn;
	}

	conn.sock	= sock;
	conn.host	= host;
	conn.port	= port == 0 ? YAGRA_DEFAULT_BUS_PORT : port;

	conn.metrics = NULL;
	return conn;
}

void yagra_finalize(yagra_conn_t * conn)
{
	close(conn->sock);
}

unsigned long long get_microtime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long)(tv.tv_sec) * 1000000 + (unsigned long long)(tv.tv_usec);
}

int yagra_define_metric(yagra_conn_t * data, char * metric_name, char * metric_description, int aggregation_type)
{
	char buffer[512];
	int buffer_len = 0;
	int send_response_code = 0;

	printf("Creating register byte buffer for %s, %s\n", metric_name, metric_description);

	int header_len = 4;

	buffer_len += sprintf(buffer + header_len + buffer_len, "name:%s|", metric_name);
	buffer_len += sprintf(buffer + header_len + buffer_len, "description:%s|", metric_description);
	printf("Buffer: %s\n", buffer);

	// Sanity check length of buffer
	if(header_len + buffer_len > 512) {
		printf("Send buffer overflow, %d\n", buffer_len);
		return -1;
	}

	// First two bytes are magic number to identify the start of the header
	buffer[0] = 0x99;
	buffer[1] = 0x98;

	// Third and fourth bytes are the length of the following header bytes.
	// We use two bytes since name length + description length will be over 256
	buffer[2] = (buffer_len >> 0) & 0xFF;
	buffer[3] = (buffer_len >> 8) & 0xFF;

	send_response_code = send(data->sock, buffer, header_len + buffer_len, 0);

	if(send_response_code < 0) {
		printf("Error sending register data, error code %d: errno %i\n", send_response_code, errno);
		return -1;
	}

	printf("Register data sent\n");

	// Finally, add to the list of metrics by first creating the metric data
	yagra_metric_t *metric = malloc(sizeof(yagra_metric_t));
	// Safe string copy
	strncpy(metric->name, metric_name, YAGRA_MAX_METRIC_NAME_LENGTH);
	// Make sure we terminate with NUL
	metric->name[YAGRA_MAX_METRIC_NAME_LENGTH - 1] = '\0';

	metric->type = aggregation_type;

	// Traverse to the end of the list
	yagra_metric_t *last_metric = data->metrics;
	if (last_metric == NULL) {
		data->metrics = metric;
		return 0;
	}

	while (last_metric->next != NULL) {
		last_metric = last_metric->next;
	}

	// Add the new metric to the end of the list
	last_metric->next = metric;

	return 0;
}

yagra_batch_data_t yagra_init_batch(yagra_conn_t * conn)
{
	yagra_batch_data_t batch;
	batch.metric_data = NULL;
	batch.num_metrics = 0;

	return batch;
}

int yagra_observe_metric(yagra_batch_data_t * data, char * metric_name, int value)
{
	// Find the metric in the global list
	yagra_metric_t *metric = data->conn->metrics;
	int metric_index = 0;
	while(metric != NULL) {
		printf("Comparing %s to %s\n", metric->name, metric_name);
		if(strcmp(metric->name, metric_name) == 0) {
			break;
		}
		metric = metric->next;
		metric_index++;
	}

	if(metric == NULL) {
		printf("Could not find metric %s\n", metric_name);
		return -1;
	}

	// Check if the metric is already in the batch
	yagra_metric_data_t *existing_metric_data = data->metric_data;
	while(existing_metric_data != NULL) {
		if(existing_metric_data->metric_index == metric_index) {
			break;
		}
		existing_metric_data = existing_metric_data->next;
	}

	if(existing_metric_data != NULL) {
		yagra_batch_aggregation_strategy type = metric->type;
		// Update the existing metric data using the aggregation type
		// TODO: use the aggregation type
		existing_metric_data->value = value + existing_metric_data->value;
		return 0;
	}

	// Add the metric data to the batch
	yagra_metric_data_t *metric_data = malloc(sizeof(yagra_metric_data_t));
	metric_data->metric_index = metric_index;
	metric_data->value = value;

	// Traverse to the end of the list
	yagra_metric_data_t *last_metric_data = data->metric_data;
	if (last_metric_data == NULL) {
		data->metric_data = metric_data;
		data->num_metrics++;
		return 0;
	}

	while (last_metric_data->next != NULL) {
		last_metric_data = last_metric_data->next;
	}

	// Add the new metric data to the end of the list
	last_metric_data->next = metric_data;

	// Increase the counter
	data->num_metrics++;

	return 0;

}

int yagra_send_batch(yagra_batch_data_t *batch)
{
  char buffer[512];
  int buffer_len = 0;
	
	int send_response_code = 0;

	// Stop early if there are no metrics to send
	if(batch->num_metrics == 0) {
		return 0;
	}

  printf("Creating metrics byte buffer\n");

	// Calculate the maximum length of the buffer, assuming 64 for length of
	// each metric value when converted to string (TODO: make this accurate)
	unsigned long max_len = (YAGRA_MAX_METRIC_NAME_LENGTH + 64) * batch->num_metrics;

	// Determine amount of bytes required to represent max_len in binary
	int body_length_bytes = 0;
	unsigned long temp = max_len;
	while(temp > 0) {
		temp >>= 8;
		body_length_bytes++;
	}

	// Make sure we can represent the length of the max_len in a single byte
	assert(body_length_bytes < 256);

	// Header:
	// 0x99 0x99 <length of following length> <body length, lsb> <body length, msb>

	int header_len = 2 + 1 + body_length_bytes;

	for (int i = 0; i < batch->num_metrics; i++) {
		yagra_metric_data_t *metric = &batch->metric_data[i];
		buffer_len += sprintf(buffer + header_len + buffer_len, "amf_%s:%d|", batch->conn->metrics[metric->metric_index].name, metric->value);
	}
  // buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_start_time:%llu|", stats->start_time);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_ue_id:%ld|", stats->ue_id);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_ngap_type:%d|", stats->ngap_message_type);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_type:%d|", stats->nas_message_type);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_invalid:%d|", stats->invalid);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_latency:%d|", stats->latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_decode_latency:%d|", stats->decode_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_handle_latency:%d|", stats->handle_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_decode_latency:%d|", stats->nas_decode_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_handle_latency:%d|", stats->nas_handle_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_nas_encode_latency:%d|", stats->nas_encode_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_build_latency:%d|", stats->response_build_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_encode_latency:%d|", stats->encode_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_send_latency:%d|", stats->send_latency);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_responses:%d|", stats->num_responses);
	// buffer_len += sprintf(buffer + header_len + buffer_len, "amf_message_end_time:%llu\n", stats->end_time);

	// Sanity check length of buffer
	if(header_len + buffer_len > 512) {
		printf("Send buffer overflow, %d\n", buffer_len);
		return -1;
	}

	// First two bytes are magic number to identify the start of the header
	buffer[0] = 0x99;
	buffer[1] = 0x99;

	// Third byte is the length of following header bytes
	buffer[2] = body_length_bytes;

	for (int i = 0; i < body_length_bytes; i++) {
		buffer[3 + i] = (max_len >> (i * 8)) & 0xFF;
	}

	send_response_code = send(batch->conn->sock, buffer, header_len + buffer_len, 0);

	if(send_response_code < 0) {
		printf("Error sending metrics data, error code %d: errno %i\n", send_response_code, errno);
		return -1;
	}

  return 0;
}

void yagra_print_metrics(yagra_conn_t * conn)
{
	printf("Defined Metrics: ");
	yagra_metric_t *metric = conn->metrics;
	while(metric != NULL) {
		printf("%s, ", metric->name);
		metric = metric->next;
	}
	printf("\n");
}

void yagra_print_batch(yagra_batch_data_t * batch)
{
	printf("Batch Metrics (%d items): ", batch->num_metrics);
	yagra_metric_data_t *metric = batch->metric_data;
	while(metric != NULL) {
		printf("Metric: %d, ", metric->metric_index);
		printf("Value: %d\n", metric->value);
		metric = metric->next;
	}
}
