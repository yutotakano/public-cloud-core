#ifndef __COREKUBE_NAS_AUTHENTICATION_FAILURE_H__
#define __COREKUBE_NAS_AUTHENTICATION_FAILURE_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"
#include "nas_ngap_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handle_authentication_failure(ogs_nas_5gs_authentication_failure_t *message, nas_ngap_params_t *params, message_handler_response_t * response);

#endif /* __COREKUBE_NAS_AUTHENTICATION_FAILURE_H__ */