#ifndef __COREKUBE_NAS_REGISTRATION_REQUEST_H__
#define __COREKUBE_NAS_REGISTRATION_REQUEST_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"
#include "nas_handler.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handle_registration_request(ogs_nas_5gs_registration_request_t *message, nas_ngap_params_t *params, message_handler_response_t * response);

int get_sample_auth_response(ogs_nas_5gs_message_t *message);

#endif /* __COREKUBE_NAS_REGISTRATION_REQUEST_H__ */