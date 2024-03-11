#ifndef __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_H__
#define __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_H__

#include "ngap/ogs-ngap.h"
#include "nas/5gs/ogs-nas-5gs.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct ngap_pdu_session_resource_setup_request_params_t {
    uint64_t amf_ue_ngap_id;
    uint64_t ran_ue_ngap_id;
    ogs_pkbuf_t * nasPdu;
} ngap_pdu_session_resource_setup_request_params_t;

int ngap_build_pdu_session_resource_setup_request(ngap_pdu_session_resource_setup_request_params_t * params, ogs_ngap_message_t * response);

#endif /* __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_H__ */