#ifndef __S1AP_HANDLER_NAS_DETACH_REQUEST_H__
#define __S1AP_HANDLER_NAS_DETACH_REQUEST_H__

#include "nas_message.h"
#include "s1ap/s1ap_message.h"

#include <libck.h>

status_t nas_handle_detach_request(nas_message_t *nas_detach_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, pkbuf_t **nas_pkbuf);

status_t detach_request_fetch_state(nas_eps_mobile_identity_t *mobile_identity, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls);

#endif /* __S1AP_HANDLER_NAS_DETACH_REQUEST_H__ */