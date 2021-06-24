#ifndef __COREKUBE_NGAP_HANDLER_H__
#define __COREKUBE_NGAP_HANDLER_H__

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

// the maximum number of responses that any message can possibly have
#define MAX_NUM_RESPONSES 1

typedef struct message_handler_response {
    int num_responses;
    // the response can either be a pointer to an ngap_message_t
    // or a pointer to a pkbuf_t, so use void* to allow it to
    // take on both types
    void **responses;
    // the SCTP stream ID differs depending on whether
    // this is a S1Setup message or a UE message
    uint8_t sctpStreamID;
} message_handler_response_t;

int ngap_handler_entrypoint(void *incoming, int incoming_len, message_handler_response_t *response);

int bytes_to_message(void *payload, int payload_len, ogs_ngap_message_t *message);

int message_to_bytes(ogs_ngap_message_t *message, ogs_pkbuf_t **out);

int ngap_message_handler(ogs_ngap_message_t *message, message_handler_response_t *response);

int ngap_initiatingMessage_handler(ogs_ngap_message_t *initiatingMessage, message_handler_response_t *response);

int ngap_successfulOutcome_handler(ogs_ngap_message_t *ngap_message, message_handler_response_t *response);

#endif /* __COREKUBE_NGAP_HANDLER_H__ */