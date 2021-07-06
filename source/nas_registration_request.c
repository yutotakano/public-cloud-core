#include "nas_authentication_request.h"

#include "nas_registration_request.h"

int nas_handle_registration_request(ogs_nas_5gs_registration_request_t *message, message_handler_response_t * response) {
    ogs_info("Handling NAS Registration Request");

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));
    return get_sample_auth_response(response->responses[0]);
}

int get_sample_auth_response(ogs_nas_5gs_message_t *message) {
    ogs_info("Getting Sample NAS Authentication Response");

    ogs_nas_5gs_authentication_request_t params;
    params.ngksi.tsc = 0;
    params.ngksi.value = 0;
    params.abba.length = 2;
    OGS_HEX("0000", 2, params.abba.value);
    OGS_HEX("1bbbb54850826ea309d2fdf7d8ae14a8", 16, params.authentication_parameter_rand.rand);
    OGS_HEX("2f23985de1c68000f51c70401ba30eec", 16, params.authentication_parameter_autn.autn);

    return nas_build_authentication_request(&params, message);
}