#ifndef __S1AP_HANDLER_S1SETUPRESPONSE_H__
#define __S1AP_HANDLER_S1SETUPRESPONSE_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"

void s1ap_build_setup_resp(s1ap_message_t *response, S1AP_PLMNidentity_t *derivedPLMNidentity);

#endif /* __S1AP_HANDLER_S1SETUPRESPONSE_H__ */