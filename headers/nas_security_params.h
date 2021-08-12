#ifndef __COREKUBE_NAS_SECURITY_PARAMS_H__
#define __COREKUBE_NAS_SECURITY_PARAMS_H__

#include "core/ogs-core.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct nas_security_params {
    uint32_t ul_count;
    uint32_t dl_count;
    uint8_t * knas_int;
    uint8_t * knas_enc;
} nas_security_params_t;

#endif /* __COREKUBE_NAS_SECURITY_PARAMS_H__ */