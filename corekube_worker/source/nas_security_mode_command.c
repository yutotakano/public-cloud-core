#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_security_mode_command.h"

#include "core_sha2_hmac.h"
#include "nas_types.h" // TODO: at some point this will probably be in the header, if so, remove

status_t generate_nas_keys(c_uint8_t *kasme, c_uint8_t *knas_int, c_uint8_t *knas_enc) {
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