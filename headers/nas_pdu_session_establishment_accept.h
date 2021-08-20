#ifndef __COREKUBE_NAS_AUTHENTICATION_REQUEST_H__
#define __COREKUBE_NAS_AUTHENTICATION_REQUEST_H__

#include "core/ogs-core.h"
#include "nas/5gs/ogs-nas-5gs.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_build_pdu_session_establishment_accept(ogs_nas_5gs_message_t *message);

#endif /* __COREKUBE_NAS_AUTHENTICATION_REQUEST_H__ */