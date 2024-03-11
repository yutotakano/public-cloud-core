#ifndef __COREKUBE_NGAP_UE_CONTEXT_RELEASE_COMMAND_H__
#define __COREKUBE_NGAP_UE_CONTEXT_RELEASE_COMMAND_H__

#include "ngap/ogs-ngap.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct ngap_ue_context_release_command_params {
    uint64_t amf_ue_ngap_id;
    uint64_t * ran_ue_ngap_id;
    uint8_t group;
    uint8_t cause;
} ngap_ue_context_release_command_params_t;

int ngap_build_ue_context_release_command(ngap_ue_context_release_command_params_t * params, ogs_ngap_message_t * response);

#endif /* __COREKUBE_NGAP_UE_CONTEXT_RELEASE_COMMAND_H__ */