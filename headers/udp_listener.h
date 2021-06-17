#define MME_LISTEN_PORT 5566
#define BUFFER_LEN 1024

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

// TODO: these should be defined in a separate header
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

int s1ap_handler_entrypoint(void *incoming, int incoming_len, S1AP_handler_response_t *response);
// END TODO

int configure_udp_socket(char * mme_ip_address);

typedef struct process_message_args {
	int sock_udp;
	socklen_t from_len;
	struct sockaddr_in *client_addr;
	uint8_t *buffer;
	int num_bytes_received;
} process_message_args_t;

void *process_message(void *raw_args);

void start_listener(char * mme_ip_address);

int main(int argc, char const *argv[]);
