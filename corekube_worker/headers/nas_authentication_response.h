#ifndef __S1AP_HANDLER_NAS_AUTHENTICATION_RESPONSE_H__
#define __S1AP_HANDLER_NAS_AUTHENTICATION_RESPONSE_H__

#include <libck.h>

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"

status_t nas_handle_authentication_response(nas_message_t *nas_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, pkbuf_t **nas_pkbuf);

status_t get_nas_authentication_response_prerequisites_from_db(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls);

status_t nas_authentication_response_check(nas_authentication_response_t *auth_response, c_uint8_t *mme_res);

#endif /* __S1AP_HANDLER_NAS_AUTHENTICATION_RESPONSE_H__ */