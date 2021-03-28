#include "initialuemessage.h"

#include "s1ap_handler.h"
#include "downlinknastransport.h"
#include "nas_util.h"

status_t handle_initialuemessage(s1ap_message_t *received_message, S1AP_handler_response_t *response) {
    d_info("Handling S1AP InitialUEMessage");

    S1AP_InitialUEMessage_t *initialUEMessage = &received_message->choice.initiatingMessage->value.choice.InitialUEMessage;

    S1AP_ENB_UE_S1AP_ID_t *enb_ue_id;
    status_t get_enb_eu_id = extract_ENB_UE_ID(initialUEMessage, &enb_ue_id);
    d_assert(get_enb_eu_id == CORE_OK, return CORE_ERROR, "Failed to extract ENB_UE_ID");

    S1AP_MME_UE_S1AP_ID_t mme_ue_id;

    nas_message_t nas_message;
    // note that the mme_ue_id parameter has been set to null because the InitialUEMessage
    // will never contain an mme_ue_id. It is defined above so that it can be determined
    // later. Passing NULL through is OK in this case because the Attach Request is plain
    // with no protection, so will not require DB calls to get security keys
    status_t decode_nas = decode_initialue_nas(initialUEMessage, NULL, &nas_message);
    d_assert(decode_nas == CORE_OK, return CORE_ERROR, "Error decoding InitialUE NAS message");

    pkbuf_t *nas_pkbuf;

    switch (nas_message.emm.h.message_type) {
        case NAS_ATTACH_REQUEST:
            ; // necessary to stop C complaining about labels and declarations
            S1AP_PLMNidentity_t *PLMNidentity = NULL;
            status_t get_PLMN = extract_PLMNidentity(initialUEMessage, &PLMNidentity);
            d_assert(get_PLMN == CORE_OK, return CORE_ERROR, "Failed to extract PLMN identity");

            // nas_handle_attach_request() also fetches the MME_UE_ID from the DB
            status_t nas_handle_attach = nas_handle_attach_request(&nas_message, enb_ue_id, PLMNidentity, &mme_ue_id, &nas_pkbuf);
            d_assert(nas_handle_attach == CORE_OK, return CORE_ERROR, "Failed to handle NAS attach");

            // mark this message as having a response
            response->outcome = HAS_RESPONSE;

            break;
        default:
            d_error("Unknown NAS message type: %d", nas_message.emm.h.message_type);
            return CORE_ERROR;
    }

    status_t get_downlink = generate_downlinknastransport(nas_pkbuf, mme_ue_id, *enb_ue_id, response->response);
    d_assert(get_downlink == CORE_OK, return CORE_ERROR, "Failed to generate DownlinkNASTransport message");


    return CORE_OK;
}

status_t extract_PLMNidentity(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_PLMNidentity_t **PLMNidentity) {
    d_info("Extracting PLMN identity from InitialUEMessage");

    S1AP_InitialUEMessage_IEs_t *TAI_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_TAI, &TAI_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get TAI IE from InitialUEMessage");

    *PLMNidentity = &TAI_IE->value.choice.TAI.pLMNidentity;

    return CORE_OK;
}

status_t extract_ENB_UE_ID(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_ENB_UE_S1AP_ID_t **ENB_UE_ID) {
    d_info("Extracting ENB_UE_S1AP_ID identity from InitialUEMessage");

    S1AP_InitialUEMessage_IEs_t *ENB_UE_ID_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_ENB_UE_S1AP_ID, &ENB_UE_ID_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get ENB_UE_ID IE from InitialUEMessage");

    *ENB_UE_ID = &ENB_UE_ID_IE->value.choice.ENB_UE_S1AP_ID;

    return CORE_OK;
}

status_t get_InitialUE_IE(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR desiredIElabel, S1AP_InitialUEMessage_IEs_t **desiredIE) {
    d_info("Searching for IE in InitialUE message");

    int numIEs = initialUEMessage->protocolIEs.list.count;
    for (int i = 0; i < numIEs; i++) {
        S1AP_InitialUEMessage_IEs_t *theIE = initialUEMessage->protocolIEs.list.array[i];
        if (theIE->value.present == desiredIElabel) {
            *desiredIE = theIE;
            return CORE_OK;
        }
    }

    // if we reach here, then the desired IE has not been found
    return CORE_ERROR;
}

status_t decode_initialue_nas(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *nas_message) {
    d_info("Decoding InitialUE NAS message");

    S1AP_NAS_PDU_t *NAS_PDU = NULL;

    S1AP_InitialUEMessage_IEs_t *NAS_PDU_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_NAS_PDU, &NAS_PDU_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get NAS_PDU IE from InitialUEMessage");

    NAS_PDU = &NAS_PDU_IE->value.choice.NAS_PDU;

    memset(nas_message, 0, sizeof (nas_message_t));

    status_t decode_emm =  decode_nas_emm(NAS_PDU, mme_ue_id, nas_message);
    d_assert(decode_emm == CORE_OK, return CORE_ERROR, "Failed to decode InitialUE NAS EMM");

    return CORE_OK;
}
