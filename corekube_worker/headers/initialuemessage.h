#ifndef __S1AP_HANDLER_INITIALUEMESSAGE_H__
#define __S1AP_HANDLER_INITIALUEMESSAGE_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"

#include "nas_attach.h"

#include "nas/nas_message.h"

status_t handle_initialuemessage(s1ap_message_t *received_message, S1AP_handler_response_t *response);

status_t extract_PLMNidentity(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_PLMNidentity_t **PLMNidentity);

status_t extract_ENB_UE_ID(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_ENB_UE_S1AP_ID_t **ENB_UE_ID);

status_t get_InitialUE_IE(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR desiredIElabel, S1AP_InitialUEMessage_IEs_t **desiredIE);

status_t decode_initialue_nas(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *nas_message);

#endif /* __S1AP_HANDLER_INITIALUEMESSAGE_H__ */