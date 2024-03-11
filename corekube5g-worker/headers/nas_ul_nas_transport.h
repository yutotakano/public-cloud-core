#ifndef __COREKUBE_NAS_UL_NAS_TRANSPORT_H__
#define __COREKUBE_NAS_UL_NAS_TRANSPORT_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"
#include "nas_ngap_params.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handle_ul_nas_transport(ogs_nas_5gs_ul_nas_transport_t *message, nas_ngap_params_t *params, message_handler_response_t * response);

#endif /* __COREKUBE_NAS_UL_NAS_TRANSPORT_H__ */