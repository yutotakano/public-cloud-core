#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_security_mode_command.h"

#include "nas_message_security.h"
#include "core_sha2_hmac.h"
#include "nas_types.h" // TODO: at some point this will probably be in the header, if so, remove

status_t nas_build_security_mode_command(corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf) {
    d_info("Building NAS Security Mode Command");

    nas_message_t cleartext_security_mode;
    status_t build_cleartext = nas_build_cleartext_security_mode_command(&cleartext_security_mode);
    d_assert(build_cleartext == CORE_OK, return CORE_ERROR, "Failed to build cleartext NAS security mode command");

    status_t secure_message = nas_security_encode(&cleartext_security_mode, db_pulls, pkbuf);
    d_assert(secure_message == CORE_OK, return CORE_ERROR, "Failed to secure NAS message");    

    return CORE_OK;
}

// this function is adapted from emm_build_security_mode_command()
// in nextepc/src/mme/emm_build.c
status_t nas_build_cleartext_security_mode_command(nas_message_t *message) {
    d_info("Building cleartext NAS Security Mode Command");

    nas_security_mode_command_t *security_mode_command = 
        &message->emm.security_mode_command;
    nas_security_algorithms_t *selected_nas_security_algorithms =
        &security_mode_command->selected_nas_security_algorithms;
    nas_key_set_identifier_t *nas_key_set_identifier =
        &security_mode_command->nas_key_set_identifier;
    nas_ue_security_capability_t *replayed_ue_security_capabilities = 
        &security_mode_command->replayed_ue_security_capabilities;

    memset(message, 0, sizeof(*message));
    message->h.security_header_type = 
       NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT;
    message->h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;

    message->emm.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;
    message->emm.h.message_type = NAS_SECURITY_MODE_COMMAND;

    selected_nas_security_algorithms->type_of_integrity_protection_algorithm = COREKUBE_INT_ALGORITHM;
    selected_nas_security_algorithms->type_of_ciphering_algorithm = COREKUBE_ENC_ALGORITHM;

    nas_key_set_identifier->tsc = 0;
    nas_key_set_identifier->nas_key_set_identifier = 0;

    replayed_ue_security_capabilities->eea = EEA_JUST_EEA0;
    replayed_ue_security_capabilities->eia = EEA_JUST_EIA2;
    replayed_ue_security_capabilities->length = 2;

    return CORE_OK;
}

status_t generate_nas_keys(c_uint8_t *kasme, c_uint8_t *knas_int, c_uint8_t *knas_enc) {
    d_info("Generating NAS keys");

    // although this is part of the security mode command,
    // this function is actually called during the handling
    // of the NAS attach request command, because that is when
    // the Kasme key (used to generate NAS keys) is generated

    // the two keys are then stored in the DB during the handling
    // of the NAS attach request command until they are needed
    // during the handling of the security mode command

    // CoreKube only supports EEA0 and 128-EIA2
    // UE support for these algorithms has already been verified in
    // the check_network_capabilities() function
    kdf_nas(KDF_NAS_INT_ALG, NAS_SECURITY_ALGORITHMS_128_EIA2, kasme, knas_int);
    kdf_nas(KDF_NAS_ENC_ALG, NAS_SECURITY_ALGORITHMS_EEA0, kasme, knas_enc);

    return CORE_OK;
}

// taken from mme_kdf_nas in nextepc/src/mme/mme_kdf.c
void kdf_nas(c_uint8_t algorithm_type_distinguishers,
    c_uint8_t algorithm_identity, c_uint8_t *kasme, c_uint8_t *knas)
{
    d_info("Generating NAS KDF");

    c_uint8_t s[7];
    c_uint8_t out[32];

    s[0] = 0x15; /* FC Value */

    s[1] = algorithm_type_distinguishers;
    s[2] = 0x00;
    s[3] = 0x01;

    s[4] = algorithm_identity;
    s[5] = 0x00;
    s[6] = 0x01;

    hmac_sha256(kasme, 32, s, 7, out, 32);
    memcpy(knas, out+16, 16);
}

// taken from mme_kdf_enb in nextepc/src/mme/mme_kdf.c
void kdf_enb(c_uint8_t *kasme, c_uint32_t ul_count, c_uint8_t *kenb)
{
    d_info("Generating ENB KDF");

    c_uint8_t s[7];

    s[0] = 0x11; /* FC Value */

    ul_count = htonl(ul_count);
    memcpy(s+1, &ul_count, 4);

    s[5] = 0x00;
    s[6] = 0x04;

    hmac_sha256(kasme, 32, s, 7, kenb, 32);
}