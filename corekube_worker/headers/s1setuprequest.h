#ifndef __S1AP_HANDLER_S1SETUPREQUEST_H__
#define __S1AP_HANDLER_S1SETUPREQUEST_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"

S1AP_handle_outcome_t handle_s1setuprequest(s1ap_message_t *received_message, s1ap_message_t *response);

static status_t getPLMNidentity(s1ap_message_t *received_message, S1AP_PLMNidentity_t **PLMNidentity);

#endif /* __S1AP_HANDLER_S1SETUPREQUEST_H__ */