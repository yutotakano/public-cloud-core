#ifndef __COREKUBE_NAS_HANDLER_H__
#define __COREKUBE_NAS_HANDLER_H__

#include "core/ogs-core.h"
#include "ngap/ogs-ngap.h"
#include "nas/5gs/ogs-nas-5gs.h"
#include "ngap_handler.h"

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

int nas_handler_entrypoint(NGAP_NAS_PDU_t *nasPdu, message_handler_response_t *response);

int nas_bytes_to_message(NGAP_NAS_PDU_t *nasPdu, ogs_nas_5gs_message_t *message, uint8_t * messageType);

int nas_5gmm_handler(ogs_nas_5gmm_message_t *nasMessage, message_handler_response_t *response);

int nas_5gsm_handler(ogs_nas_5gsm_message_t *nasMessage, message_handler_response_t *response);

int nas_message_to_bytes(message_handler_response_t * response);

#endif /* __COREKUBE_NAS_HANDLER_H__ */