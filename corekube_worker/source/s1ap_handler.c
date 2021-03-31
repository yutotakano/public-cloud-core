/*************************************************************************** 

    Copyright (C) 2019 NextEPC Inc. All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***************************************************************************/

#include "s1ap_handler.h"

#include "s1setuprequest.h"
#include "initialuemessage.h"
#include "uplinknastransport.h"

#include "core/include/3gpp_types.h"

status_t s1ap_handler_entrypoint(void *incoming, int incoming_len, S1AP_handler_response_t *response) {
    d_info("Reached S1AP Handler entrypoint");

    s1ap_message_t incoming_s1ap;
    s1ap_message_t outgoing_s1ap;
    s1ap_message_t outgoing_s1ap_2;

    // Decode the incoming message
    status_t b_to_m = bytes_to_message(incoming, incoming_len, &incoming_s1ap);
    d_assert(b_to_m == CORE_OK, return CORE_ERROR, "Failed to decode incoming S1AP message");

    // Handle the decoded message
    response->response = &outgoing_s1ap;
    response->response = &outgoing_s1ap_2;
    status_t message_handle = s1ap_message_handler(&incoming_s1ap, response);
    d_assert(message_handle == CORE_OK, return CORE_ERROR, "Failed to handle S1AP message");
    
    // Encode the outgoing message, if one exists
    if (response->outcome == HAS_RESPONSE || response->outcome == DUAL_RESPONSE) {
        status_t m_to_b = message_to_bytes(response->response, (pkbuf_t **) &response->response);
        d_assert(m_to_b == CORE_OK, return CORE_ERROR, "Failed to encode outgoing S1AP message");
    }

    // Encode the second (optional) outgoing message, if one exists
    if (response->outcome == DUAL_RESPONSE) {
        status_t m_to_b = message_to_bytes(response->response2, (pkbuf_t **) &response->response2);
        d_assert(m_to_b == CORE_OK, return CORE_ERROR, "Failed to encode second outgoing S1AP message");
    }

    // Free up memory
    status_t free_incoming = s1ap_free_pdu(&incoming_s1ap);
    d_assert(free_incoming == CORE_OK, return CORE_ERROR, "Failed to free memory from incoming message");
    // the S1AP response is freed in message_to_bytes() above

    return CORE_OK;
}

status_t bytes_to_message(void *payload, int payload_len, s1ap_message_t *message)
{
    d_info("Converting received bytes to S1AP message");

    pkbuf_t *pkbuf;

    pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
    pkbuf->len = payload_len;
    memcpy(pkbuf->payload, payload, pkbuf->len);

    status_t decode_result = s1ap_decode_pdu(message, pkbuf);
    pkbuf_free(pkbuf);
    d_assert(decode_result == CORE_OK, return CORE_ERROR, "Failed to decode bytes");

    return CORE_OK;
}

status_t message_to_bytes(s1ap_message_t *message, pkbuf_t **out)
{
    d_info("Converting response S1AP message to raw bytes to send");

    pkbuf_t *pkbuf;
    status_t encode_result = s1ap_encode_pdu(&pkbuf, message);
    d_assert(encode_result == CORE_OK, return CORE_ERROR, "Failed to encode bytes");

    status_t free_pdu = s1ap_free_pdu(message);
    d_assert(free_pdu == CORE_OK, return CORE_ERROR, "Failed to free S1AP message");
    *out = pkbuf;

    return CORE_OK;
}

status_t s1ap_message_handler(s1ap_message_t *message, S1AP_handler_response_t *response) {
    d_info("Handling S1AP message");

    int s1ap_print = asn_fprint(stdout, &asn_DEF_S1AP_S1AP_PDU, message);
    d_assert(s1ap_print == 0, return CORE_ERROR, "Failed to print S1AP message");

    // with a single exception (S1SetupResponse),
    // the SCTP stream ID should be 1
    response->sctpStreamID = 1;

    switch (message->present) {
        case S1AP_S1AP_PDU_PR_initiatingMessage:
            return s1ap_initiatingMessage_handler(message, response);
        case S1AP_S1AP_PDU_PR_successfulOutcome:
            return s1ap_successfulOutcome_handler(message, response);
        case S1AP_S1AP_PDU_PR_unsuccessfulOutcome:
        case S1AP_S1AP_PDU_PR_NOTHING:
        default:
            response->outcome = NO_RESPONSE;
            return CORE_ERROR;
    }
}

status_t s1ap_initiatingMessage_handler(s1ap_message_t *initiatingMessage, S1AP_handler_response_t *response) {
    d_info("Handling S1AP message of type InitiatingMessage");

    switch (initiatingMessage->choice.initiatingMessage->value.present) {
        case S1AP_InitiatingMessage__value_PR_S1SetupRequest:
            return handle_s1setuprequest(initiatingMessage, response);
        case S1AP_InitiatingMessage__value_PR_InitialUEMessage:
            return handle_initialuemessage(initiatingMessage, response);
        case S1AP_InitiatingMessage__value_PR_UplinkNASTransport:
            return handle_uplinknastransport(initiatingMessage, response);
        case S1AP_InitiatingMessage__value_PR_UECapabilityInfoIndication:
            d_info("Received UECapabilityInfoIndication message");
            d_info("Nothing to handle, no response to return");
            response->outcome = NO_RESPONSE;
            return CORE_OK;
        default:
            response->outcome = NO_RESPONSE;
            return CORE_ERROR;
    }
}

status_t s1ap_successfulOutcome_handler(s1ap_message_t *s1ap_message, S1AP_handler_response_t *response) {
    d_info("Handling S1AP message of type InitiatingMessage");

    S1AP_SuccessfulOutcome_t *successfulOutcome = s1ap_message->choice.successfulOutcome;

    // all successful outcomes have no response
    response->outcome = NO_RESPONSE;

    switch (successfulOutcome->value.present) {
        case S1AP_SuccessfulOutcome__value_PR_InitialContextSetupResponse:
            d_info("Received InitialContextSetupResponse");
            d_info("Nothing to handle, no response to return");
            return CORE_OK;
        case S1AP_SuccessfulOutcome__value_PR_UEContextReleaseComplete:
            d_info("Received UEContextReleaseComplete");
            d_info("Nothing to handle, no response to return");
            return CORE_OK;
        default:
            d_error("Unknown S1AP SuccessfulOutcome type: %d", successfulOutcome->value.present);
            return CORE_ERROR;
    }
}