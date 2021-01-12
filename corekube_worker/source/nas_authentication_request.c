#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_authentication_request.h"

// heavily based upon the emm_build_authentication_request()
// function in nextepc/src/mme/emm_build.c
status_t generate_nas_authentication_request(c_uint8_t *rand, c_uint8_t *autn, pkbuf_t **pkbuf) {
    d_info("Generating NAS authentication request");

    nas_message_t nas_message;
    nas_authentication_request_t *auth_req = &nas_message.emm.authentication_request;

    memset(&nas_message, 0, sizeof(nas_message));
    nas_message.emm.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;
    nas_message.emm.h.message_type = NAS_AUTHENTICATION_REQUEST;

    memcpy(auth_req->authentication_parameter_rand.rand, rand, RAND_LEN);
    memcpy(auth_req->authentication_parameter_autn.autn, autn, AUTN_LEN);
    auth_req->authentication_parameter_autn.length = AUTN_LEN;

    status_t nas_encode_status = nas_plain_encode(pkbuf, &nas_message);
    d_assert(nas_encode_status == CORE_OK, , "Failed to encode NAS message");

    return CORE_OK;
} 