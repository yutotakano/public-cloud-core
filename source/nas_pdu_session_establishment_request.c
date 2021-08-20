#include "nas_pdu_session_establishment_accept.h"

#include "nas_pdu_session_establishment_request.h"

int nas_handle_pdu_session_establishment_request(ogs_nas_5gs_pdu_session_establishment_request_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS PDU Session Establishment Request");

    ogs_assert(message->pdu_session_type.value == OGS_PDU_SESSION_TYPE_IPV4);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_pdu_session_establishment_accept(response->responses[0]);
}