#ifndef METRICS_H
#define METRICS_H

#include <stdbool.h>

#define YAGRA_DEFAULT_BUS_PORT 7799
#define YAGRA_MAX_METRIC_NAME_LENGTH 64
#define YAGRA_MAX_METRIC_DESCRIPTION_LENGTH 512

#define YAGRA_LOG(conn, ...)                   \
{                                              \
    if (conn->log != NULL)                     \
        conn->log(__VA_ARGS__);                \
    else                                       \
        printf(__VA_ARGS__);                   \
} 


// Enum describing the type of aggregation to use for a metric when there are
// multiple collections with the same ID. 
typedef enum {
    YAGRA_AGGREGATION_TYPE_KEEP_FIRST,
    YAGRA_AGGREGATION_TYPE_KEEP_LAST,
    // Keep the minimum value of all collections with the same ID
    YAGRA_AGGREGATION_TYPE_MIN,
    // Keep the maximum value of all collections with the same ID
    YAGRA_AGGREGATION_TYPE_MAX,
    // Keep the average value of all collections with the same ID
    YAGRA_AGGREGATION_TYPE_AVERAGE,
    // Keep a sum of all collections with the same ID
    YAGRA_AGGREGATION_TYPE_SUM,
    YAGRA_AGGREGATION_TYPE_NONE
} yagra_batch_aggregation_strategy;

// forward-declare the yagra_metric_t struct
typedef struct yagra_metric yagra_metric_t;

/**
 * @brief A single type of metric description.
 */
typedef struct yagra_metric {
    char name[YAGRA_MAX_METRIC_NAME_LENGTH];
    yagra_batch_aggregation_strategy type;
    yagra_metric_t *next;
} yagra_metric_t;

// forward-declare the yagra_metric_data_t struct
typedef struct yagra_metric_data yagra_metric_data_t;

/**
 * @brief A single metric value collected during a batch.
 */
typedef struct yagra_metric_data {
    // Index into the global metrics list for the name and type of this metric.
    char * metric_name;
    // The current value of this metric.
    uint64_t value;
    // Extra data as necessary for the aggregation of the metric, e.g. a count.
    void * extra_data;
    // Pointer to the next metric in the list.
    yagra_metric_data_t *next;
} yagra_metric_data_t;

/**
 * @brief Main struct to hold the connection data for the metrics server.
 */
typedef struct yagra_conn {
    int sock;
    char *host;
    int port;

    // List of metrics that have been registered with the metrics server. 
    // Just names and types, no values.
    yagra_metric_t *metrics;

    void (*log)(char *, ...);
} yagra_conn_t;

// Struct to hold a batch of metrics to send to the metrics server, such as all
// the metrics observed during the processing of a single message. Contains
// a pointer to connection information.
typedef struct yagra_batch_data {
    yagra_conn_t *conn;
    yagra_metric_data_t *metric_data;
    int num_metrics;
} yagra_batch_data_t;


/**
 * @brief Establish a connection with the metrics server.
 * 
 * @param host The IP address of the metrics server.
 * @param port The port to connect. If 0, will use DEFAULT_METRICS_PORT.
 * @param log_calledback A callback function that will be called witha integer level and string message arguments.
 * @return int The socket file descriptor, or -1 if an error occurred.
 */
yagra_conn_t yagra_init(char * host, int port, void (*log_callback)(char *, ...));

/**
 * @brief Disconnect from the metrics server.
 * 
 * @param sock The socket file descriptor.
 */
void yagra_finalize(yagra_conn_t * conn);

/**
 * @brief Get the current time in microseconds.
 * 
 * @return unsigned long long The current time in microseconds.
 */
unsigned long long get_microtime();

/**
 * @brief Start a new batch of metrics to be sent to the metrics server.
 * 
 * @param data 
 * @return int 
 */
yagra_batch_data_t yagra_init_batch(yagra_conn_t * conn);

/**
 * @brief Send a batch of metrics to the metrics server.
 * 
 * @param data 
 * @return int 
 */
int yagra_send_batch(yagra_batch_data_t * batch);

/**
 * @brief Register a new metric that can be collected. This will send a message
 * to the Yagra bus to register the metric and expose it as a Prometheus metric.
 * 
 * It will also add it to the metric list within the yagra_data_t struct.
 * 
 * @param data Yagra persistent data.
 * @param metric_name The name of the metric.
 * @param metric_description The description of the metric.
 * @param aggregation_type How to aggregate multiple collections with the same ID.
 * @return int Whether the metric was successfully registered.
 */
int yagra_define_metric(yagra_conn_t * conn, char * metric_name, char * metric_description, int aggregation_type);

/**
 * @brief Observe a metric. This will store the value in the metric data store
 * and will be sent to the metrics server when yagra_send_metrics is called.
 * 
 * @param data 
 * @param metric_name 
 * @param value 
 * @return int 
 */
int yagra_observe_metric(yagra_batch_data_t * data, char * metric_name, uint64_t value);

void yagra_print_metrics(yagra_conn_t * conn);

void yagra_print_batch(yagra_batch_data_t * batch);

#endif /* METRICS_H */
