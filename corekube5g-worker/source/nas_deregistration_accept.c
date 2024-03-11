#include "nas_deregistration_accept.h"

int nas_build_deregistration_accept(ogs_nas_5gs_message_t *message)
{
    ogs_info("Building NAS De-registration Accept");

    ogs_assert(message);

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->h.security_header_type =
        OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED;
    message->h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;

    message->gmm.h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;
    message->gmm.h.message_type = OGS_NAS_5GS_DEREGISTRATION_ACCEPT;

    return OGS_OK;
}