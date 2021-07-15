#include "nas_registration_request.h"

#include "nas_handler.h"

int nas_handler_entrypoint(NGAP_NAS_PDU_t *nasPdu, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS handler entrypoint");

    ogs_nas_5gs_message_t nasMessage;
    uint8_t messageType;
    int convertToBytes = nas_bytes_to_message(nasPdu, &nasMessage, &messageType);
    ogs_assert(convertToBytes == OGS_OK); // Failed to convert NAS message to bytes

    ogs_info("NAS Message converted to bytes");
    switch (messageType) {
        case OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM:
            return nas_5gmm_handler(&nasMessage.gmm, params, response);
        case OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GSM:
            return nas_5gsm_handler(&nasMessage.gsm, params, response);
        default:
            ogs_error("Unknown NAS message type: %d", messageType);
            return OGS_ERROR;
    }

    return OGS_OK;
}


int nas_5gmm_handler(ogs_nas_5gmm_message_t *nasMessage, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS 5GMM Handler");

    uint8_t messageType = nasMessage->h.message_type;
    int build_response;
    switch(messageType) {
        case OGS_NAS_5GS_REGISTRATION_REQUEST:
            build_response = nas_handle_registration_request(&nasMessage->registration_request, params, response);
            break;
        default:
            ogs_error("Unknown NAS 5GMM message type: %d", messageType);
            return OGS_ERROR;
    }

    ogs_assert(build_response == OGS_OK);
    return nas_message_to_bytes(response);
}


int nas_5gsm_handler(ogs_nas_5gsm_message_t *nasMessage, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS 5GSM Handler");

    uint8_t messageType = nasMessage->h.message_type;
    switch(messageType) {
        default:
            ogs_error("Unknown NAS 5GSM message type: %d", messageType);
            return OGS_ERROR;
    }

    return OGS_OK;
}


int nas_bytes_to_message(NGAP_NAS_PDU_t *nasPdu, ogs_nas_5gs_message_t *message, uint8_t *messageType) {
    ogs_info("NAS bytes to message");

    ogs_nas_5gs_security_header_t *sh = NULL;
    ogs_nas_security_header_type_t security_header_type;

    ogs_nas_5gmm_header_t *h = NULL;
    ogs_pkbuf_t *nasbuf = NULL;

    ogs_assert(nasPdu);

    /* The Packet Buffer(pkbuf_t) for NAS message MUST make a HEADROOM. 
     * When calculating AES_CMAC, we need to use the headroom of the packet. */
    nasbuf = ogs_pkbuf_alloc(NULL, OGS_NAS_HEADROOM+nasPdu->size);
    ogs_assert(nasbuf);
    ogs_pkbuf_reserve(nasbuf, OGS_NAS_HEADROOM);
    ogs_pkbuf_put_data(nasbuf, nasPdu->buf, nasPdu->size);

    sh = (ogs_nas_5gs_security_header_t *)nasbuf->data;
    ogs_assert(sh);

    memset(&security_header_type, 0, sizeof(ogs_nas_security_header_type_t));
    switch(sh->security_header_type) {
    case OGS_NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE:
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED:
        security_header_type.integrity_protected = 1;
        ogs_pkbuf_pull(nasbuf, 7);
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED:
        security_header_type.integrity_protected = 1;
        security_header_type.ciphered = 1;
        ogs_pkbuf_pull(nasbuf, 7);
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT:
        security_header_type.integrity_protected = 1;
        security_header_type.new_security_context = 1;
        ogs_pkbuf_pull(nasbuf, 7);
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHTERD_WITH_NEW_INTEGRITY_CONTEXT:
        security_header_type.integrity_protected = 1;
        security_header_type.ciphered = 1;
        security_header_type.new_security_context = 1;
        ogs_pkbuf_pull(nasbuf, 7);
        break;
    default:
        ogs_error("Not implemented(security header type:0x%x)",
                sh->security_header_type);
        return OGS_ERROR;
    }

    /*if (ran_ue->amf_ue) {
        if (nas_5gs_security_decode(ran_ue->amf_ue,
                security_header_type, nasbuf) != OGS_OK) {
            ogs_error("nas_eps_security_decode failed()");
	        return OGS_ERROR;
        }
    }*/

    h = (ogs_nas_5gmm_header_t *)nasbuf->data;
    ogs_assert(h);
    *messageType = h->extended_protocol_discriminator;
    ogs_info("MESSAGE TYPE: %d", *messageType);
    if (*messageType == OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM) {
        ogs_info("5GMM NAS Message");
        int decode = ogs_nas_5gmm_decode(message, nasbuf);
        ogs_assert(decode == OGS_OK); // Failed to decode NAS 5GMM message
    } else if (*messageType == OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GSM) {
        ogs_info("5GSM NAS Message");
        int decode = ogs_nas_5gsm_decode(message, nasbuf);
        ogs_assert(decode == OGS_OK); // Failed to decode NAS 5GSM message
    } else {
        ogs_error("Unknown NAS Protocol discriminator 0x%02x", *messageType);
        ogs_pkbuf_free(nasbuf);
        return OGS_ERROR;
    }

    return OGS_OK;
}

int nas_message_to_bytes(message_handler_response_t * response) {
    ogs_info("NAS Message to bytes");

    ogs_nas_5gs_message_t * nasMessage;
    ogs_pkbuf_t * pkbuf;

    for (int i = 0; i < response->num_responses; i++) {
        ogs_info("NAS message to bytes for message %d of %d", i+i, response->num_responses);
        nasMessage = response->responses[i];
        pkbuf = ogs_nas_5gs_plain_encode(nasMessage);
        ogs_assert(pkbuf);
        ogs_free(nasMessage);
        response->responses[i] = pkbuf;
    }

    return OGS_OK;
}