#ifndef __COREKUBE_NGAP_DOWNLINK_NAS_TRANSPORT_H__
#define __COREKUBE_NGAP_DOWNLINK_NAS_TRANSPORT_H__

#include "ngap/ogs-ngap.h"
#include "nas/5gs/ogs-nas-5gs.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct ngap_downlink_nas_transport_params {
    uint64_t amf_ue_ngap_id;
    uint64_t ran_ue_ngap_id;
    ogs_pkbuf_t * nasPdu;
    ogs_bitrate_t * ambr;
    int num_of_s_nssai;
    ogs_nas_s_nssai_ie_t s_nssai[OGS_MAX_NUM_OF_SLICE];
} ngap_downlink_nas_transport_params_t;

int ngap_build_downlink_nas_transport(ngap_downlink_nas_transport_params_t * params, ogs_ngap_message_t * response);

#endif /* __COREKUBE_NGAP_DOWNLINK_NAS_TRANSPORT_H__ */