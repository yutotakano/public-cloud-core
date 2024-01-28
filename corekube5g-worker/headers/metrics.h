#ifndef METRICS_H
#define METRICS_H

#define DEFAULT_METRICS_PORT 7799

// Struct that is passed around to all message handlers to fill in with stats.
// This will be sent to a UDP endpoint at the very end of the message handling.
typedef struct worker_metrics {
    unsigned long long start_time;
    uint64_t ran_ue_ngap_id;
    uint64_t amf_ue_ngap_id;
    int message_type;
    int invalid;
    int latency;
    int handle_latency;
    int decode_latency;
    int encode_latency;
    int send_latency;
    int num_responses;
    unsigned long long end_time;
} worker_metrics_t;

/**
 * @brief Establish a connection with the metrics server.
 * 
 * @param host The IP address of the metrics server.
 * @param port The port to connect. If 0, will use DEFAULT_METRICS_PORT.
 * @return int The socket file descriptor, or -1 if an error occurred.
 */
int metrics_connect(char * host, int port);

/**
 * @brief Disconnect from the metrics server.
 * 
 * @param sock The socket file descriptor.
 */
void metrics_disconnect(int sock);

/**
 * @brief Get the current time in microseconds.
 * 
 * @return unsigned long long The current time in microseconds.
 */
unsigned long long get_microtime();

/**
 * @brief Send collected stats to the metrics server.
 * 
 * @param sock The socket file descriptor.
 * @param stats The stats to send.
 * @return int 0 if successful, -1 if an error occurred.
 */
int metrics_send(int sock, worker_metrics_t *stats);

#endif /* METRICS_H */
