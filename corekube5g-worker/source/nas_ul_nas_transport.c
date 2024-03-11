#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"
#include "nas_security.h"
#include "nas_handler.h"
#include "nas_dl_nas_transport.h"

#include "nas_ul_nas_transport.h"

int nas_handle_ul_nas_transport(ogs_nas_5gs_ul_nas_transport_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS UL NAS Transport Message");

    ogs_nas_payload_container_type_t *payload_container_type = NULL;
    ogs_nas_payload_container_t *payload_container = NULL;
    ogs_nas_pdu_session_identity_2_t *pdu_session_id = NULL;

    payload_container_type = &message->payload_container_type;
    ogs_assert(payload_container_type);
    payload_container = &message->payload_container;
    ogs_assert(payload_container);

    ogs_assert(payload_container_type->value);
    ogs_assert(payload_container->length);
    ogs_assert(payload_container->buffer);

    ogs_assert(message->presencemask & OGS_NAS_5GS_UL_NAS_TRANSPORT_PDU_SESSION_ID_PRESENT);

    pdu_session_id = &message->pdu_session_id;
    ogs_assert(*pdu_session_id != OGS_NAS_PDU_SESSION_IDENTITY_UNASSIGNED);

    switch (payload_container_type->value) {
    case OGS_NAS_PAYLOAD_CONTAINER_N1_SM_INFORMATION:
        ;
        // Send the contained NAS message on to the NAS handler
        // Note that the NAS handler expects a OCTET_STRING input
        OCTET_STRING_t nasBuffer;
        nasBuffer.buf = payload_container->buffer;
        nasBuffer.size = (uint8_t) payload_container->length;
        int handle_inner_nas = nas_handler_entrypoint(&nasBuffer, params, response);
        ogs_assert(handle_inner_nas == OGS_OK);

        // Expect a single response
        ogs_assert(response->num_responses == 1);
        break;
    default:
        ogs_error("Unknown Payload container type: %d", payload_container_type->value);
        return OGS_ERROR;
    }

    ogs_pkbuf_t * nasPkbuf = (ogs_pkbuf_t *) response->responses[0];
    uint8_t * nasBuf = ogs_malloc(nasPkbuf->len * sizeof(uint8_t));
    memcpy(nasBuf, nasPkbuf->data, nasPkbuf->len);

    nas_dl_nas_transport_params_t nas_params;
    bzero(&nas_params, sizeof(nas_dl_nas_transport_params_t));
    nas_params.payload_container_type = OGS_NAS_PAYLOAD_CONTAINER_N1_SM_INFORMATION;
    nas_params.payload_container.length = nasPkbuf->len;
    nas_params.payload_container.buffer = nasBuf;

    ogs_pkbuf_free(nasPkbuf);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_dl_nas_transport(&nas_params, response->responses[0]);
}