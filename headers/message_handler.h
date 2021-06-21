extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

#include "core/ogs-core.h"

typedef enum message_handle_outcome {
    NO_RESPONSE,
    HAS_RESPONSE,
    DUAL_RESPONSE
} message_handle_outcome_t;

typedef struct message_handler_response {
    message_handle_outcome_t outcome;
    // the SCTP stream ID differs depending on whether
    // this is a S1Setup message or a UE message
    uint8_t sctpStreamID;
    // the response can either be a pointer to an ngap_message_t
    // or a pointer to a pkbuf_t, so use void* to allow it to
    // take on both types
    void * response;
    // an (optional) second response, for cases where a single
    // incoming messages must be responded with two outgoing messages
    void * response2;
} message_handler_response_t;

int message_handler_entrypoint(void *incoming, int incoming_len, message_handler_response_t *response);