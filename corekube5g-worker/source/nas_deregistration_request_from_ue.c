#include "corekube_config.h"
#include "nas_deregistration_accept.h"

#include "nas_deregistration_request_from_ue.h"

int nas_handle_deregistration_request_from_ue(ogs_nas_5gs_deregistration_request_from_ue_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS De-registration Request from UE");

    // only 3GPP access type is supported
    ogs_assert(message->de_registration_type.access_type == COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS);

    // re-registration is not supported
    ogs_assert(message->de_registration_type.re_registration_required == 0);

    // only send a Deregistraion Accept if not a switch-off
    if (!message->de_registration_type.switch_off) {
        response->num_responses = 1;
        response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));
        return nas_build_deregistration_accept(response->responses[0]);
    }
    else {
        response->num_responses = 0;
        return OGS_OK;
    }
}