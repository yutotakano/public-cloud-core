#ifndef __S1AP_HANDLER_INITIAL_CONTEXT_SETUP_REQUEST_H__
#define __S1AP_HANDLER_INITIAL_CONTEXT_SETUP_REQUEST_H__

#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"
#include <libck.h>

status_t nas_send_attach_accept(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, S1AP_handler_response_t *response);

status_t attach_accept_fetch_state(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls);

status_t s1ap_build_initial_context_setup_request(pkbuf_t *emmbuf, corekube_db_pulls_t *db_pulls, s1ap_message_t *pdu);


#endif /* __S1AP_HANDLER_INITIAL_CONTEXT_SETUP_REQUEST_H__ */