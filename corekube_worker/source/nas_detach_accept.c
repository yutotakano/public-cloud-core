#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "core/include/core_lib.h"

#include "nas_detach_accept.h"

#include "s1ap_conv.h"
#include "nas_message_security.h"

// this function is based upon emm_build_detach_accept()
// from nextepc/src/mme/emm_build.c
status_t nas_build_detach_accept(nas_eps_mobile_identity_t *mobile_identity, corekube_db_pulls_t *db_pulls, pkbuf_t **emmbuf)
{
    d_info("Building NAS Detach Accept");

    nas_message_t message;

    memset(&message, 0, sizeof(message));
    message.h.security_header_type =  NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED;
    message.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;

    message.emm.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;
    message.emm.h.message_type = NAS_DETACH_ACCEPT;

    status_t nas_encode = nas_security_encode(&message, db_pulls, emmbuf);
    d_assert(nas_encode == CORE_OK && emmbuf, return CORE_ERROR, "emm build error");

    return CORE_OK;
}
