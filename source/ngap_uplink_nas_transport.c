#include "nas_handler.h"
#include "ngap_downlink_nas_transport.h"

#include "ngap_uplink_nas_transport.h"

int ngap_handle_uplink_nas_transport(ogs_ngap_message_t *message, message_handler_response_t *response) {
    ogs_info("Handling Uplink NAS Transport message");

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_UplinkNASTransport_t *UplinkNASTransport = NULL;

    NGAP_UplinkNASTransport_IEs_t *ie = NULL;
    NGAP_RAN_UE_NGAP_ID_t *RAN_UE_NGAP_ID = NULL;
    NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    NGAP_NAS_PDU_t *NAS_PDU = NULL;
    NGAP_UserLocationInformation_t *UserLocationInformation = NULL;

    ogs_assert(message);
    initiatingMessage = message->choice.initiatingMessage;
    ogs_assert(initiatingMessage);
    UplinkNASTransport = &initiatingMessage->value.choice.UplinkNASTransport;
    ogs_assert(UplinkNASTransport);

    for (int i = 0; i < UplinkNASTransport->protocolIEs.list.count; i++) {
        ie = UplinkNASTransport->protocolIEs.list.array[i];
        switch (ie->id) {
        case NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID:
            RAN_UE_NGAP_ID = &ie->value.choice.RAN_UE_NGAP_ID;
            break;
        case NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID:
            AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;
            break;
        case NGAP_ProtocolIE_ID_id_NAS_PDU:
            NAS_PDU = &ie->value.choice.NAS_PDU;
            break;
        case NGAP_ProtocolIE_ID_id_UserLocationInformation:
            UserLocationInformation = &ie->value.choice.UserLocationInformation;
            break;
        default:
            break;
        }
    }

    ogs_assert(RAN_UE_NGAP_ID); // Failed to find RAN_UE_NGAP_ID element
    ogs_assert(AMF_UE_NGAP_ID); // Failed to find AMF_UE_NGAP_ID element
    ogs_assert(UserLocationInformation); // Failed to find UserLocationInformation element
    ogs_assert(UserLocationInformation->present == NGAP_UserLocationInformation_PR_userLocationInformationNR); // Only userLocationInformationNR is implemented
    ogs_assert(NAS_PDU); // Failed to find NAS_PDU element

    // convert the AMF_UE_NGAP_ID to an integer from a byte array
    uint64_t amf_ue_ngap_id;
    asn_INTEGER2ulong(AMF_UE_NGAP_ID, (unsigned long *) &amf_ue_ngap_id);

    // setup the parameters required by the NAS handler
    nas_ngap_params_t nas_params;
    nas_params.ran_ue_ngap_id = RAN_UE_NGAP_ID;
    nas_params.amf_ue_ngap_id = &amf_ue_ngap_id;

    int handle_nas = nas_handler_entrypoint(NAS_PDU, &nas_params, response);
    ogs_assert(handle_nas == OGS_OK);
    // expect a single NAS response (Authentication Request)
    ogs_assert(response->num_responses == 1);
    // also expect the AMF_UE_NGAP_ID to have been retreived
    ogs_assert(nas_params.amf_ue_ngap_id);

    // prepare the parameters for the response (a Downlink NAS Transport)
    ngap_downlink_nas_transport_params_t response_params;
    response_params.nasPdu = response->responses[0];
    response_params.amf_ue_ngap_id = *nas_params.amf_ue_ngap_id;
    response_params.ran_ue_ngap_id = *RAN_UE_NGAP_ID;
    response_params.ambr = NULL;
    response_params.num_of_s_nssai = 0;

    // build the NGAP response
    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_ngap_message_t));
    int build_response = ngap_build_downlink_nas_transport(&response_params, response->responses[0]);
    ogs_assert(build_response == OGS_OK);
    
    return OGS_OK;
}