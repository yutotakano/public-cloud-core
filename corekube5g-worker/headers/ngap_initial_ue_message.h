#ifndef __COREKUBE_NGAP_INITIAL_UE_MESSAGE_H__
#define __COREKUBE_NGAP_INITIAL_UE_MESSAGE_H__

#include "ngap/ogs-ngap.h"
#include "ngap_handler.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int ngap_handle_initial_ue_message(ogs_ngap_message_t *message, message_handler_response_t *response);

#endif /* __COREKUBE_NGAP_INITIAL_UE_MESSAGE_H__ */