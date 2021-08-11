#include "ngap_ng_setup_request.h"
#include "ngap_initial_ue_message.h"
#include "ngap_uplink_nas_transport.h"

#include "ngap_handler.h"

int ngap_handler_entrypoint(void *incoming, int incoming_len, message_handler_response_t *response) {
    ogs_info("Reached NGAP Handler entrypoint");

    ogs_ngap_message_t incoming_ngap;

    // Decode the incoming message
    int b_to_m = bytes_to_message(incoming, incoming_len, &incoming_ngap);
    ogs_assert(b_to_m == OGS_OK); // Failed to decode incoming NGAP message

    // Handle the decoded message
    int message_handle = ngap_message_handler(&incoming_ngap, response);
    ogs_assert(message_handle == OGS_OK); // Failed to handle NGAP message

    // handle the outgoing messages, if there are any
    for (int i = 0; i < response->num_responses; i++) {
        int m_to_b = message_to_bytes(response->responses[i], (ogs_pkbuf_t **) &response->responses[i]);
        ogs_assert(m_to_b == OGS_OK); // Failed to encode outgoing NGAP message
    }

    // Free up memory
    ogs_ngap_free(&incoming_ngap);
    // the NGAP response is freed in message_to_bytes() above

    return OGS_OK;
}

int bytes_to_message(void *payload, int payload_len, ogs_ngap_message_t *message)
{
    ogs_info("Converting received bytes to NGAP message");

    ogs_pkbuf_t *pkbuf;

    pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
    pkbuf->len = payload_len;
    memcpy(pkbuf->data, payload, pkbuf->len);

    int decode_result = ogs_ngap_decode(message, pkbuf);
    ogs_pkbuf_free(pkbuf);
    ogs_assert(decode_result == OGS_OK); // Failed to decode bytes

    return OGS_OK;
}

int message_to_bytes(ogs_ngap_message_t *message, ogs_pkbuf_t **out)
{
    ogs_info("Converting response NGAP message to raw bytes to send");

    ogs_pkbuf_t *pkbuf = ogs_ngap_encode(message);

    ogs_ngap_free(message);
    *out = pkbuf;

    return OGS_OK;
}

int ngap_message_handler(ogs_ngap_message_t *message, message_handler_response_t *response) {
    ogs_info("Handling NGAP message");

    if (ogs_log_get_domain_level(OGS_LOG_DOMAIN) >= OGS_LOG_INFO) {
        int ngap_print = asn_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, message);
        ogs_assert(ngap_print == 0); // Failed to print NGSP message
    }

    // all UE-signalling messaged (the majority) should have
    // an SCTP stream ID >0 (in practice this may need to be
    // set as the RAN-UE-NGAP-ID, see
    // https://github.com/free5gc/free5gc/issues/88)
    response->sctpStreamID = 1;

    switch (message->present) {
        case NGAP_NGAP_PDU_PR_initiatingMessage:
            return ngap_initiatingMessage_handler(message, response);
        case NGAP_NGAP_PDU_PR_successfulOutcome:
            return ngap_successfulOutcome_handler(message, response);
        case NGAP_NGAP_PDU_PR_unsuccessfulOutcome:
        case NGAP_NGAP_PDU_PR_NOTHING:
        default:
            response->num_responses = 0;
            return OGS_ERROR;
    }
}

int ngap_initiatingMessage_handler(ogs_ngap_message_t *initiatingMessage, message_handler_response_t *response) {
    ogs_info("Handling NGAP message of type InitiatingMessage");

    NGAP_InitiatingMessage__value_PR messageType = initiatingMessage->choice.initiatingMessage->value.present;
    switch (messageType) {
        case NGAP_InitiatingMessage__value_PR_NGSetupRequest:
            return ngap_handle_ng_setup_request(initiatingMessage, response);
            break;
        case NGAP_InitiatingMessage__value_PR_InitialUEMessage:
            return ngap_handle_initial_ue_message(initiatingMessage, response);
        case NGAP_InitiatingMessage__value_PR_UplinkNASTransport:
            return ngap_handle_uplink_nas_transport(initiatingMessage, response);
        default:
            ogs_info("Unknown InitiatingMessage type: %d", messageType);
            response->num_responses = 0;
            return OGS_ERROR;
    }
}

int ngap_successfulOutcome_handler(ogs_ngap_message_t *ngap_message, message_handler_response_t *response) {
    ogs_info("Handling NGAP message of type InitiatingMessage");

    NGAP_SuccessfulOutcome_t *successfulOutcome = ngap_message->choice.successfulOutcome;

    // all successful outcomes have no response
    response->num_responses = 0;

    switch (successfulOutcome->value.present) {
        default:
            ogs_error("Unknown NGAP SuccessfulOutcome type: %d", successfulOutcome->value.present);
            return OGS_ERROR;
    }
}