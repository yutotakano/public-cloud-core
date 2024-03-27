#ifndef __COREKUBE_NAS_PDU_SESSION_ESTABLISHMENT_REQUEST_H__
#define __COREKUBE_NAS_PDU_SESSION_ESTABLISHMENT_REQUEST_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"
#include "nas_ngap_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handle_pdu_session_establishment_request(ogs_nas_5gs_pdu_session_establishment_request_t *message, nas_ngap_params_t *params, message_handler_response_t * response);

int nas_pdu_session_establishment_fetch_pdu_ip(uint32_t amf_ue_ngap_id, uint32_t *pdn_ip, yagra_batch_data_t *yagra_batch);

#endif /* __COREKUBE_NAS_PDU_SESSION_ESTABLISHMENT_REQUEST_H__ */
