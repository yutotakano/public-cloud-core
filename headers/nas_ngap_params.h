#ifndef __COREKUBE_NAS_NGAP_PARAMS_H__
#define __COREKUBE_NAS_NGAP_PARAMS_H__

#include "core/ogs-core.h"
#include "nas_security_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

// used to allow transferring of values from the NGAP message
// to the NAS message, and vice-versa
typedef struct nas_ngap_params {
    uint64_t * ran_ue_ngap_id;
    uint64_t * amf_ue_ngap_id;
    nas_security_params_t * nas_security_params;
} nas_ngap_params_t;

#endif /* __COREKUBE_NAS_NGAP_PARAMS_H__ */