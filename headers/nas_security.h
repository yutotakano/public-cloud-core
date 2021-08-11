#ifndef __COREKUBE_NAS_SECURITY_H__
#define __COREKUBE_NAS_SECURITY_H__

#include <libck.h>
#include "ogs-crypt.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "corekube_config.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

typedef struct nas_security_params {
    uint32_t ul_count;
    uint32_t dl_count;
    uint8_t * knas_int;
    uint8_t * knas_enc;
} nas_security_params_t;

ogs_pkbuf_t *nas_5gs_security_encode(nas_security_params_t * nas_security_params, ogs_nas_5gs_message_t *message);

int nas_5gs_generate_keys(ogs_nas_5gs_mobile_identity_t * mob_ident, uint8_t * opc, uint8_t * key, uint8_t * rand, uint8_t * autn, uint8_t * kamf);

int nas_security_store_keys_in_params(corekube_db_pulls_t * db_pulls, nas_security_params_t * nas_security_params);

#endif /* __COREKUBE_NAS_SECURITY_H__ */

