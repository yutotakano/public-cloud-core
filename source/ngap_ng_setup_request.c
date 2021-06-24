#include "ngap_ng_setup_request.h"

#include "ngap_ng_setup_response.h"

int ngap_handle_ng_setup_request(ogs_ngap_message_t *message, message_handler_response_t *response) {
    ogs_info("Handling NG Setup Request");

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_NGSetupRequest_t *NGSetupRequest = NULL;

    NGAP_NGSetupRequestIEs_t *ie = NULL;
    NGAP_GlobalRANNodeID_t *GlobalRANNodeID = NULL;
    NGAP_GlobalGNB_ID_t *globalGNB_ID = NULL;
    NGAP_SupportedTAList_t *SupportedTAList = NULL;
    NGAP_PagingDRX_t *PagingDRX = NULL;

    uint32_t gnb_id;

    ogs_assert(message);
    initiatingMessage = message->choice.initiatingMessage;
    ogs_assert(initiatingMessage);
    NGSetupRequest = &initiatingMessage->value.choice.NGSetupRequest;
    ogs_assert(NGSetupRequest);

    ogs_debug("NGSetupRequest");

    for (int i = 0; i < NGSetupRequest->protocolIEs.list.count; i++) {
        ie = NGSetupRequest->protocolIEs.list.array[i];
        switch (ie->id) {
        case NGAP_ProtocolIE_ID_id_GlobalRANNodeID:
            GlobalRANNodeID = &ie->value.choice.GlobalRANNodeID;
            break;
        case NGAP_ProtocolIE_ID_id_SupportedTAList:
            SupportedTAList = &ie->value.choice.SupportedTAList;
            break;
        case NGAP_ProtocolIE_ID_id_DefaultPagingDRX:
            PagingDRX = &ie->value.choice.PagingDRX;
            break;
        default:
            break;
        }
    }

    if (!GlobalRANNodeID) {
        ogs_error("No GlobalRANNodeID");
        return OGS_ERROR;
    }

    globalGNB_ID = GlobalRANNodeID->choice.globalGNB_ID;
    if (!globalGNB_ID) {
        ogs_error("No globalGNB_ID");
        return OGS_ERROR;
    }

    if (!SupportedTAList) {
        ogs_error("No SupportedTAList");
        return OGS_ERROR;
    }

    ogs_ngap_GNB_ID_to_uint32(&globalGNB_ID->gNB_ID, &gnb_id);

    if (PagingDRX)
        ogs_debug("    PagingDRX[%ld]", *PagingDRX);

    // non-UE signalling has a stream ID of 0
    response->sctpStreamID = 0;

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_ngap_message_t));
    int build_response = ngap_build_ng_setup_response(response->responses[0]);
    ogs_assert(build_response == OGS_OK);
    
    return OGS_OK;
}