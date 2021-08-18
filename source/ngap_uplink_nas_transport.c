#include "nas_ngap_params.h"
#include "nas_handler.h"
#include "ngap_downlink_nas_transport.h"
#include "nas_initial_context_setup_request.h"
#include "db_accesses.h"
#include "corekube_config.h"

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
    bzero(&nas_params, sizeof(nas_ngap_params_t));
    nas_params.ran_ue_ngap_id = RAN_UE_NGAP_ID;
    nas_params.amf_ue_ngap_id = &amf_ue_ngap_id;

    int handle_nas = nas_handler_entrypoint(NAS_PDU, &nas_params, response);
    ogs_assert(handle_nas == OGS_OK);

    // expect a single NAS response
    ogs_assert(response->num_responses == 1);

    // check for the special case of the NAS Security Mode Complete message,
    // which needs a NGAP response of type Initial Context Setup Request,
    // rather than the Downlink NAS Transport
    if (nas_params.nas_message_type == OGS_NAS_5GS_SECURITY_MODE_COMPLETE) {
        // the NAS handler should have derived the masked IMEISV
        ogs_assert(nas_params.masked_imeisv);

        // prepare the parameters for the response (an Initial Context Setup Request)
        ngap_initial_context_setup_request_params_t initial_context_setup_request_params;
        bzero(&initial_context_setup_request_params, sizeof(ngap_initial_context_setup_request_params_t));
        initial_context_setup_request_params.amf_ue_ngap_id = amf_ue_ngap_id;
        initial_context_setup_request_params.ran_ue_ngap_id = *RAN_UE_NGAP_ID;
        initial_context_setup_request_params.nasPdu = response->responses[0];
        initial_context_setup_request_params.num_of_s_nssai = 1;
        ogs_nas_s_nssai_ie_t nssai;
        nssai.sst = CoreKube_NSSAI_sST;
        nssai.sd.v = OGS_S_NSSAI_NO_SD_VALUE;
        initial_context_setup_request_params.s_nssai[0] = nssai;
        initial_context_setup_request_params.masked_imeisv = nas_params.masked_imeisv;

        // format the AMF_UE_NGAP_ID as a uint8_t buffer, to be passed to the DB
        OCTET_STRING_t amf_id_buf;
        ogs_asn_uint32_to_OCTET_STRING((uint32_t) amf_ue_ngap_id, &amf_id_buf);

        // generate the kgnb
        initial_context_setup_request_params.kgnb = ogs_malloc(32 * sizeof(uint8_t));
        ogs_kdf_kgnb_and_kn3iwf(nas_params.nas_security_params->kamf, nas_params.nas_security_params->ul_count, COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS, initial_context_setup_request_params.kgnb);

        // build the NGAP response
        response->num_responses = 1;
        response->responses[0] = ogs_calloc(1, sizeof(ogs_ngap_message_t));
        int build_response = ngap_build_initial_context_setup_request(&initial_context_setup_request_params, response->responses[0]);
        ogs_assert(build_response == OGS_OK);
    }
    else {
        // prepare the parameters for the response (a Downlink NAS Transport)
        ngap_downlink_nas_transport_params_t downlink_nas_params;
        bzero(&downlink_nas_params, sizeof(ngap_downlink_nas_transport_params_t));
        downlink_nas_params.nasPdu = response->responses[0];
        downlink_nas_params.amf_ue_ngap_id = *nas_params.amf_ue_ngap_id;
        downlink_nas_params.ran_ue_ngap_id = *RAN_UE_NGAP_ID;

        // build the NGAP response
        response->num_responses = 1;
        response->responses[0] = ogs_calloc(1, sizeof(ogs_ngap_message_t));
        int build_response = ngap_build_downlink_nas_transport(&downlink_nas_params, response->responses[0]);
        ogs_assert(build_response == OGS_OK);
    }

    // free up the NAS security parameters
    nas_security_params_free(nas_params.nas_security_params);
    
    return OGS_OK;
}