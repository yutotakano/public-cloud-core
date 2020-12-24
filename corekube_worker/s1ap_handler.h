#ifndef __S1AP_HANDLER_H__
#define __S1AP_HANDLER_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"
#include "core/include/core_pkbuf.h"

typedef enum S1AP_handle_outcome {NO_RESPONSE, HAS_RESPONSE} S1AP_handle_outcome_t;

S1AP_handle_outcome_t s1ap_handler_entrypoint(void *incoming, int incoming_len, pkbuf_t **outgoing);

static status_t bytes_to_message(void *payload, int payload_len, s1ap_message_t *message);

static status_t message_to_bytes(s1ap_message_t *message, pkbuf_t **pkbuf);

static S1AP_handle_outcome_t s1ap_message_handler(s1ap_message_t *message, s1ap_message_t *response);

static S1AP_handle_outcome_t s1ap_initiatingMessage_handler(s1ap_message_t *initiatingMessage, s1ap_message_t *response);

#endif /* __S1AP_HANDLER_H__ */