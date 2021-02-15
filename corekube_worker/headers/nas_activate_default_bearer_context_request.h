#ifndef __S1AP_HANDLER_NAS_ACTIVATE_DEFAULT_BEARER_CONTEXT_REQUEST_H__
#define __S1AP_HANDLER_NAS_ACTIVATE_DEFAULT_BEARER_CONTEXT_REQUEST_H__

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"
#include <libck.h>

status_t esm_build_activate_default_bearer_context_request(pkbuf_t **pkbuf);

status_t get_default_pdn(pdn_t **pdn_out);

status_t get_default_pco(nas_protocol_configuration_options_t *default_pco);



#endif /* __S1AP_HANDLER_NAS_ACTIVATE_DEFAULT_BEARER_CONTEXT_REQUEST_H__ */