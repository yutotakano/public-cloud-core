#ifndef __COREKUBE_NGAP_UPLINK_NAS_TRANSPORT_H__
#define __COREKUBE_NGAP_UPLINK_NAS_TRANSPORT_H__

#include "ngap/ogs-ngap.h"
#include "ngap_handler.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int ngap_handle_uplink_nas_transport(ogs_ngap_message_t *message, message_handler_response_t *response);

#endif /* __COREKUBE_NGAP_UPLINK_NAS_TRANSPORT_H__ */