#include "nas_ngap_params.h"
#include "nas_handler.h"
#include "ngap_pdu_session_resource_setup_request_transfer.h"
#include "ngap_pdu_session_resource_setup_response_transfer.h"
#include "yagra.h"
#include "ngap_pdu_session_resource_setup_response.h"

int ngap_handle_pdu_session_resource_setup_response(ogs_ngap_message_t *message, message_handler_response_t *response) {
    ogs_info("Handling NGAP PDU Session Resource Setup Response message");

    uint64_t amf_ue_ngap_id;

    NGAP_SuccessfulOutcome_t *successfulOutcome = NULL;
    NGAP_PDUSessionResourceSetupResponse_t *PDUSessionResourceSetupResponse;

    NGAP_PDUSessionResourceSetupResponseIEs_t *ie = NULL;
    NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    NGAP_PDUSessionResourceSetupListSURes_t *PDUSessionList = NULL;
    NGAP_PDUSessionResourceSetupItemSURes_t *PDUSessionItem = NULL;
    OCTET_STRING_t *transfer = NULL;

    ogs_assert(message);
    successfulOutcome = message->choice.successfulOutcome;
    ogs_assert(successfulOutcome);
    PDUSessionResourceSetupResponse =
        &successfulOutcome->value.choice.PDUSessionResourceSetupResponse;
    ogs_assert(PDUSessionResourceSetupResponse);

    ogs_debug("PDUSessionResourceSetupResponse");

    for (int i = 0; i < PDUSessionResourceSetupResponse->protocolIEs.list.count;
            i++) {
        ie = PDUSessionResourceSetupResponse->protocolIEs.list.array[i];
        switch (ie->id) {
        case NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
            AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
            break;
        case NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSURes:
            PDUSessionList =
                &ie->value.choice.PDUSessionResourceSetupListSURes;
            break;
        default:
            break;
        }
    }

    ogs_assert(AMF_UE_NGAP_ID);

    int convert_amf_id = asn_INTEGER2ulong(AMF_UE_NGAP_ID, (unsigned long *)&amf_ue_ngap_id) != 0;
    ogs_assert(convert_amf_id == OGS_OK);

    ogs_assert(PDUSessionList);

    for (int i = 0; i < PDUSessionList->list.count; i++) {
        PDUSessionItem = (NGAP_PDUSessionResourceSetupItemSURes_t *) PDUSessionList->list.array[i];

        ogs_assert(PDUSessionItem);

        transfer = &PDUSessionItem->pDUSessionResourceSetupResponseTransfer;
        ogs_assert(transfer);

        ogs_assert(PDUSessionItem->pDUSessionID != OGS_NAS_PDU_SESSION_IDENTITY_UNASSIGNED);

        ogs_pkbuf_t * transfer_pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
        ogs_pkbuf_put_data(transfer_pkbuf, transfer->buf, transfer->size);
    
        int handle_transfer = ngap_handle_pdu_session_resource_setup_response_transfer(amf_ue_ngap_id, transfer_pkbuf);
        ogs_assert(handle_transfer == OGS_OK);

        ogs_pkbuf_free(transfer_pkbuf);
    }

    // There is no response to this message
    response->num_responses=0;
    
    return OGS_OK;
}
