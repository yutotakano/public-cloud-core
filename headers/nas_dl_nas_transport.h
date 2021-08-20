#ifndef __COREKUBE_NAS_DL_NAS_TRANSPORT_H__
#define __COREKUBE_NAS_DL_NAS_TRANSPORT_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct nas_dl_nas_transport_params {
    // Required
    uint8_t payload_container_type;
    ogs_nas_payload_container_t payload_container;
    // Optional
    uint8_t * gmm_cause;
    uint8_t * back_off_timer_value;
} nas_dl_nas_transport_params_t;

int nas_build_dl_nas_transport(nas_dl_nas_transport_params_t * params, ogs_nas_5gs_message_t *message);

#endif /* __COREKUBE_NAS_DL_NAS_TRANSPORT_H__ */