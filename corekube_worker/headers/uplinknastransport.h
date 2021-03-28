#ifndef __S1AP_HANDLER_UPLINKNASTRANSPORT_H__
#define __S1AP_HANDLER_UPLINKNASTRANSPORT_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"

#include "nas_attach.h"

status_t handle_uplinknastransport(s1ap_message_t *received_message, S1AP_handler_response_t *response);

status_t decode_uplinknastransport_nas(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *auth_response);

status_t UplinkNASTransport_extract_MME_UE_ID(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_MME_UE_S1AP_ID_t **MME_UE_ID);

status_t UplinkNASTransport_extract_ENB_UE_ID(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_ENB_UE_S1AP_ID_t **ENB_UE_ID);

status_t get_uplinkNASTransport_IE(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_UplinkNASTransport_IEs__value_PR desiredIElabel, S1AP_UplinkNASTransport_IEs_t **desiredIE);

#endif /* __S1AP_HANDLER_UPLINKNASTRANSPORT_H__ */