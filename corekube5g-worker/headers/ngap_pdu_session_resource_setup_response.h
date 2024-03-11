#ifndef __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_H__
#define __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_H__

#include "ngap/ogs-ngap.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int ngap_handle_pdu_session_resource_setup_response(ogs_ngap_message_t *message, message_handler_response_t *response);

#endif /* __COREKUBE_NGAP_PDU_SESSION_RESOURCE_SETUP_RESPONSE_H__ */
