
#include "uplinknastransport.h"

#include "nas_util.h"
#include "s1ap_conv.h"
#include "nas_authentication_response.h"
#include "nas_message_security.h" //TODO - included while testing, make sure it is actually needed when commiting final version

status_t handle_uplinknastransport(s1ap_message_t *received_message, S1AP_handler_response_t *response) {
    d_info("Handling UplinkNASTransport");
    S1AP_UplinkNASTransport_t *uplinkNASTransport = &received_message->choice.initiatingMessage->value.choice.UplinkNASTransport;

    S1AP_MME_UE_S1AP_ID_t *mme_ue_id;
    UplinkNASTransport_extract_MME_UE_ID(uplinkNASTransport, &mme_ue_id);

    nas_message_t nas_message;
    status_t decode_nas = decode_uplinknastransport_nas(uplinkNASTransport, mme_ue_id, &nas_message);
    d_assert(decode_nas == CORE_OK, return CORE_ERROR, "Failed to decode NAS authentication response");

    switch (nas_message.emm.h.message_type) {
        case NAS_AUTHENTICATION_RESPONSE:
            return nas_handle_authentication_response(&nas_message, mme_ue_id, response);
        default:
            d_error("Unknown NAS message type");
            return CORE_ERROR;
    }
}

status_t decode_uplinknastransport_nas(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *auth_response) {
    d_info("Decoding NAS-PDU in UplinkNASTransport message");
    S1AP_NAS_PDU_t *NAS_PDU = NULL;

    S1AP_UplinkNASTransport_IEs_t *NAS_PDU_IE;
    status_t get_ie = get_uplinkNASTransport_IE(uplinkNASTransport, S1AP_UplinkNASTransport_IEs__value_PR_NAS_PDU, &NAS_PDU_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get NAS_PDU IE from UplinkNASTransport");
    NAS_PDU = &NAS_PDU_IE->value.choice.NAS_PDU;

    // TODO: doing a separate fetch from the DB just for decoding the NAS
    // security message is a very poor use of resources - rework this!
    c_uint8_t buffer[1024];
    corekube_db_pulls_t db_pulls;
    status_t db_fetch = get_NAS_decode_security_prerequisites_from_db(mme_ue_id, buffer, &db_pulls);
    d_assert(db_fetch == CORE_OK, return CORE_ERROR, "Failed to fetch NAS decode security prerequisites from DB");

    status_t nas_decode = decode_nas_emm(NAS_PDU, &db_pulls, auth_response);
    d_assert(nas_decode == CORE_OK, return CORE_ERROR, "Failed to decode NAS authentication response");

    return CORE_OK;
}

status_t UplinkNASTransport_extract_MME_UE_ID(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_MME_UE_S1AP_ID_t **MME_UE_ID) {
    d_info("Extracting MME_UE_S1AP_ID from UplinkNASTransport message");

    S1AP_UplinkNASTransport_IEs_t *MME_UE_ID_IE;
    status_t get_ie = get_uplinkNASTransport_IE(uplinkNASTransport, S1AP_UplinkNASTransport_IEs__value_PR_MME_UE_S1AP_ID, &MME_UE_ID_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get MME_UE_ID IE from UplinkNASTransport");

    *MME_UE_ID = &MME_UE_ID_IE->value.choice.MME_UE_S1AP_ID;

    return CORE_OK;
}

status_t get_uplinkNASTransport_IE(S1AP_UplinkNASTransport_t *uplinkNASTransport, S1AP_UplinkNASTransport_IEs__value_PR desiredIElabel, S1AP_UplinkNASTransport_IEs_t **desiredIE) {
    d_info("Searching for IE in UplinkNASTransport message");
    int numIEs = uplinkNASTransport->protocolIEs.list.count;
    for (int i = 0; i < numIEs; i++) {
        S1AP_UplinkNASTransport_IEs_t *theIE = uplinkNASTransport->protocolIEs.list.array[i];
        if (theIE->value.present == desiredIElabel) {
            *desiredIE = theIE;
            return CORE_OK;
        }
    }

    // if we reach here, then the desired IE has not been found
    return CORE_ERROR;
}