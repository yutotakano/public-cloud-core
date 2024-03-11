#include "nas_authentication_request.h"

int nas_build_authentication_request(ogs_nas_5gs_authentication_request_t *params, ogs_nas_5gs_message_t *message) {
    ogs_info("Building NAS Authentication Request");

    ogs_assert(params);

    ogs_nas_5gs_authentication_request_t *authentication_request = &message->gmm.authentication_request;

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->gmm.h.extended_protocol_discriminator =
            OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;
    message->gmm.h.message_type = OGS_NAS_5GS_AUTHENTICATION_REQUEST;

    authentication_request->ngksi.tsc = params->ngksi.tsc;
    authentication_request->ngksi.value = params->ngksi.value;
    authentication_request->abba.length = params->abba.length;
    memcpy(authentication_request->abba.value, params->abba.value, params->abba.length);

    authentication_request->presencemask |=
    OGS_NAS_5GS_AUTHENTICATION_REQUEST_AUTHENTICATION_PARAMETER_RAND_PRESENT;
    authentication_request->presencemask |=
    OGS_NAS_5GS_AUTHENTICATION_REQUEST_AUTHENTICATION_PARAMETER_AUTN_PRESENT;

    memcpy(authentication_request->authentication_parameter_rand.rand,
            params->authentication_parameter_rand.rand, OGS_RAND_LEN);
    memcpy(authentication_request->authentication_parameter_autn.autn,
            params->authentication_parameter_autn.autn, OGS_AUTN_LEN);
    authentication_request->authentication_parameter_autn.length =
            OGS_AUTN_LEN;

    return OGS_OK;
}