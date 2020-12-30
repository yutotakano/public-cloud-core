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
#include "core/include/3gpp_types.h"

S1AP_handle_outcome_t s1ap_handler_entrypoint(void *incoming, int incoming_len, pkbuf_t **outgoing) {
    s1ap_message_t incoming_s1ap;
    s1ap_message_t outgoing_s1ap;
    memset(&incoming_s1ap, 0, sizeof (s1ap_message_t));
    memset(&outgoing_s1ap, 0, sizeof (s1ap_message_t));

    // Decode the incoming message
    bytes_to_message(incoming, incoming_len, &incoming_s1ap);

    // Handle the decoded message
    S1AP_handle_outcome_t outcome = s1ap_message_handler(&incoming_s1ap, &outgoing_s1ap);
    
    // Encode the outgoing message, if one exists
    if (outcome == HAS_RESPONSE)
        message_to_bytes(&outgoing_s1ap, outgoing);

    // Free up memory
    s1ap_free_pdu(&incoming_s1ap);
    s1ap_free_pdu(&outgoing_s1ap);

    // Return the outcome
    return outcome;
}

static status_t bytes_to_message(void *payload, int payload_len, s1ap_message_t *message)
{
    pkbuf_t *pkbuf;

    pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
    pkbuf->len = payload_len;
    memcpy(pkbuf->payload, payload, pkbuf->len);

    status_t decode_result = s1ap_decode_pdu(message, pkbuf);

    pkbuf_free(pkbuf);

    if (decode_result != CORE_OK)
    {
        d_error("failed to decode bytes");
        return CORE_ERROR;
    }
    return CORE_OK;
}

static status_t message_to_bytes(s1ap_message_t *message, pkbuf_t **pkbuf)
{
    status_t encode_result = s1ap_encode_pdu(pkbuf, message);

    if (encode_result != CORE_OK)
    {
        d_error("failed to encode bytes");
        return CORE_ERROR;
    }
    return CORE_OK;
}

static S1AP_handle_outcome_t s1ap_message_handler(s1ap_message_t *message, s1ap_message_t *response) {
    switch (message->present) {
        case S1AP_S1AP_PDU_PR_initiatingMessage:
            return s1ap_initiatingMessage_handler(message, response);
        case S1AP_S1AP_PDU_PR_successfulOutcome:
            return NO_RESPONSE;
        case S1AP_S1AP_PDU_PR_unsuccessfulOutcome:
            return NO_RESPONSE;
        case S1AP_S1AP_PDU_PR_NOTHING:
        default:
            return NO_RESPONSE;
    }
}

static S1AP_handle_outcome_t s1ap_initiatingMessage_handler(s1ap_message_t *initiatingMessage, s1ap_message_t *response) {
    switch (initiatingMessage->choice.initiatingMessage->value.present) {
        case S1AP_InitiatingMessage__value_PR_S1SetupRequest:
            return handle_s1setuprequest(initiatingMessage, response);
        default:
            return NO_RESPONSE;
    }
}