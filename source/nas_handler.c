#include "nas_registration_request.h"
#include "nas_authentication_response.h"
#include "nas_security_mode_complete.h"
#include "nas_ul_nas_transport.h"
#include "nas_pdu_session_establishment_request.h"
#include "nas_deregistration_request_from_ue.h"
#include "nas_security.h"

#include "nas_handler.h"

int nas_handler_entrypoint(NGAP_NAS_PDU_t *nasPdu, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS handler entrypoint");

    ogs_nas_5gs_message_t nasMessage;
    uint8_t messageType;

    if (! params->nas_security_params) {
        // initialise the nas_security_params
        params->nas_security_params = ogs_calloc(1, sizeof(nas_security_params_t));
    }

    int convertToBytes = nas_bytes_to_message(params, nasPdu, &nasMessage, &messageType);
    ogs_assert(convertToBytes == OGS_OK); // Failed to convert NAS message to bytes
    ogs_info("NAS Message converted to bytes");

    int handle_outcome;
    switch (messageType) {
        case OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM:
            handle_outcome = nas_5gmm_handler(&nasMessage.gmm, params, response);
            break;
        case OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GSM:
            handle_outcome = nas_5gsm_handler(&nasMessage.gsm, params, response);
            break;
        default:
            ogs_error("Unknown NAS message type: %d", messageType);
            return OGS_ERROR;
    }

    ogs_assert(handle_outcome == OGS_OK);

    return nas_message_to_bytes(params, response);
}

int nas_security_params_free(nas_security_params_t * params) {
    // free up the previously-allocated memory
    if (!params)
        return OGS_OK;
    if (params->knas_enc)
        ogs_free(params->knas_enc);
    if (params->knas_int)
        ogs_free(params->knas_int);
    if (params->kamf)
        ogs_free(params->kamf);
    ogs_free(params);
    return OGS_OK;
}


int nas_5gmm_handler(ogs_nas_5gmm_message_t *nasMessage, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS 5GMM Handler");

    uint8_t messageType = nasMessage->h.message_type;
    params->nas_message_type = messageType;
    int build_response;
    switch(messageType) {
        case OGS_NAS_5GS_REGISTRATION_REQUEST:
            build_response = nas_handle_registration_request(&nasMessage->registration_request, params, response);
            break;
        case OGS_NAS_5GS_AUTHENTICATION_RESPONSE:
            build_response = nas_handle_authentication_response(&nasMessage->authentication_response, params, response);
            break;
        case OGS_NAS_5GS_SECURITY_MODE_COMPLETE:
            build_response = nas_handle_security_mode_complete(&nasMessage->security_mode_complete, params, response);
            break;
        case OGS_NAS_5GS_REGISTRATION_COMPLETE:
            ogs_info("UE Registration Complete");
            build_response = OGS_OK; // No response to build
            break;
        case OGS_NAS_5GS_UL_NAS_TRANSPORT:
            build_response = nas_handle_ul_nas_transport(&nasMessage->ul_nas_transport, params, response);
            break;
        case OGS_NAS_5GS_DEREGISTRATION_REQUEST:
            build_response = nas_handle_deregistration_request_from_ue(&nasMessage->deregistration_request_from_ue, params, response);
            break;
        default:
            ogs_error("Unknown NAS 5GMM message type: %d", messageType);
            return OGS_ERROR;
    }

    ogs_assert(build_response == OGS_OK);

    return OGS_OK;
}


int nas_5gsm_handler(ogs_nas_5gsm_message_t *nasMessage, nas_ngap_params_t *params, message_handler_response_t *response) {
    ogs_info("NAS 5GSM Handler");

    uint8_t messageType = nasMessage->h.message_type;
    params->nas_message_type = messageType;
    int build_response;
    switch(messageType) {
        case OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_REQUEST:
            build_response = nas_handle_pdu_session_establishment_request(&nasMessage->pdu_session_establishment_request, params, response);
            break;
        default:
            ogs_error("Unknown NAS 5GSM message type: %d", messageType);
            return OGS_ERROR;
    }

    ogs_assert(build_response == OGS_OK);

    return OGS_OK;
}


int nas_bytes_to_message(nas_ngap_params_t * params, NGAP_NAS_PDU_t *nasPdu, ogs_nas_5gs_message_t *message, uint8_t *messageType) {
    ogs_info("NAS bytes to message");

    ogs_nas_5gmm_header_t *h = NULL;
    ogs_pkbuf_t *nasbuf = NULL;

    ogs_assert(params);
    ogs_assert(nasPdu);

    /* The Packet Buffer(pkbuf_t) for NAS message MUST make a HEADROOM. 
     * When calculating AES_CMAC, we need to use the headroom of the packet. */
    nasbuf = ogs_pkbuf_alloc(NULL, OGS_NAS_HEADROOM+nasPdu->size);
    ogs_assert(nasbuf);
    ogs_pkbuf_reserve(nasbuf, OGS_NAS_HEADROOM);
    ogs_pkbuf_put_data(nasbuf, nasPdu->buf, nasPdu->size);

    int security_decode = security_decode_gsm(params, nasbuf);
    ogs_assert(security_decode == OGS_OK);

    h = (ogs_nas_5gmm_header_t *)nasbuf->data;
    ogs_assert(h);
    *messageType = h->extended_protocol_discriminator;
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

int security_decode_gsm(nas_ngap_params_t * params, ogs_pkbuf_t *nasbuf) {
    ogs_info("Security Decoding GSM Message");

    ogs_nas_5gs_security_header_t * sh = (ogs_nas_5gs_security_header_t *)nasbuf->data;
    ogs_assert(sh);

    ogs_nas_security_header_type_t security_header_type;

    if (sh->extended_protocol_discriminator == OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM) {
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

        int security_decode = nas_5gs_security_decode(params, security_header_type, nasbuf);
        ogs_assert(security_decode == OGS_OK);
    }

    return OGS_OK;
}

int nas_message_to_bytes(nas_ngap_params_t * nas_params, message_handler_response_t * response) {
    ogs_info("NAS Message to bytes");

    // currently the DL / UL counts for NAS encoding do not
    // support more than one NAS message at a time
    ogs_assert(response->num_responses <= 1);

    ogs_nas_5gs_message_t * nasMessage;
    ogs_pkbuf_t * pkbuf;

    for (int i = 0; i < response->num_responses; i++) {
        ogs_info("NAS message to bytes for message %d of %d", i+i, response->num_responses);
        nasMessage = response->responses[i];
        pkbuf = nas_5gs_security_encode(nas_params, nasMessage);
        ogs_assert(pkbuf);
        ogs_free(nasMessage);
        response->responses[i] = pkbuf;
    }

    return OGS_OK;
}