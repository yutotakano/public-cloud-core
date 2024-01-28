#ifndef METRICS_H
#define METRICS_H

#define DEFAULT_METRICS_PORT 8000

// Struct that is passed around to all message handlers to fill in with stats.
// This will be sent to a UDP endpoint at the very end of the message handling.
typedef struct worker_stats {
    int num_imsi_pulls;
} worker_stats_t;

int metrics_connect(char * host, int port);
void metrics_disconnect(int sock);
int metrics_send(int sock, worker_stats_t *stats);

#endif /* METRICS_H */
