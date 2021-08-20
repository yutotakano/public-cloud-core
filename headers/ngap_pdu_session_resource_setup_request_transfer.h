#ifndef __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_TRANSFER_H__
#define __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_TRANSFER_H__

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"
#include "nas/5gs/ogs-nas-5gs.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

ogs_pkbuf_t * nas_build_ngap_pdu_session_resource_setup_request_transfer();

#endif /* __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_REQUEST_TRANSFER_H__ */