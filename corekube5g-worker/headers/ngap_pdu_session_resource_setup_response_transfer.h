#ifndef __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_TRANSFER_H__
#define __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_TRANSFER_H__

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct pdu_session_resource_setup_response_transfer_params {
    uint64_t amf_ue_ngap_id;
    uint8_t *ue_teid;
    uint8_t *enb_ip;
} pdu_session_resource_setup_response_transfer_params_t;

int ngap_handle_pdu_session_resource_setup_response_transfer(uint64_t amf_ue_ngap_id, ogs_pkbuf_t *pkbuf, message_handler_response_t *response);

int nas_store_ngap_pdu_session_resource_setup_response_transfer_fetch_prerequisites(pdu_session_resource_setup_response_transfer_params_t * params, message_handler_response_t *response);

#endif /* __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_TRANSFER_H__ */
