#ifndef __COREKUBE_NAS_SECURITY_MODE_COMPLETE_H__
#define __COREKUBE_NAS_SECURITY_MODE_COMPLETE_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"
#include "nas_ngap_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handle_security_mode_complete(ogs_nas_5gs_security_mode_complete_t *message, nas_ngap_params_t *params, message_handler_response_t * response);

#endif /* __COREKUBE_NAS_SECURITY_MODE_COMPLETE_H__ */