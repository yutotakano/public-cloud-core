#ifndef __COREKUBE_NAS_SECURITY_H__
#define __COREKUBE_NAS_SECURITY_H__

#include <libck.h>
#include "ogs-crypt.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "corekube_config.h"
#include "nas_ngap_params.h"
#include "nas_security_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

#define NAS_COUNTS_BOTH_UL_DL 0
#define NAS_COUNTS_JUST_UL 1
#define NAS_COUNTS_JUST_DL 2

ogs_pkbuf_t *nas_5gs_security_encode(nas_ngap_params_t * params, ogs_nas_5gs_message_t *message);

int nas_5gs_security_decode(nas_ngap_params_t * params, ogs_nas_security_header_type_t security_header_type, ogs_pkbuf_t *pkbuf);

int nas_5gs_generate_keys(ogs_nas_5gs_mobile_identity_t * mob_ident, uint8_t * opc, uint8_t * key, uint8_t * rand, uint8_t * sqn, uint8_t * autn, uint8_t * kamf);

int nas_security_fetch_keys(nas_ngap_params_t * params, int includeCounts);

int nas_security_store_keys_in_params(corekube_db_pulls_t * db_pulls, nas_security_params_t * nas_security_params);

#endif /* __COREKUBE_NAS_SECURITY_H__ */

