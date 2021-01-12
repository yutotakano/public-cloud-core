#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_message_security.h"

#include "s1ap_conv.h"
#include "core_aes_cmac.h"

// the following function was adapted from
// nas_security_encode() in nextepc/src/mme/nas_security.h
status_t nas_security_encode(nas_message_t *message, corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf) {
    d_info("Encoding NAS messaging securely");

    int integrity_protected = 0;
    int new_security_context = 0;
    int ciphered = 0;

    switch(message->h.security_header_type) {
        case NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE:
            return nas_plain_encode(pkbuf, message);
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED:
            integrity_protected = 1;
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED:
            integrity_protected = 1;
            ciphered = 1;
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT:
            integrity_protected = 1;
            new_security_context = 1;
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHTERD_WITH_NEW_INTEGRITY_CONTEXT:
            integrity_protected = 1;
            new_security_context = 1;
            ciphered = 1;
            break;
        default:
            d_error("Not implemented(securiry header type:0x%x)", message->h.security_header_type);
            return CORE_ERROR;
    }

    if (COREKUBE_ENC_ALGORITHM == NAS_SECURITY_ALGORITHMS_EEA0)
        ciphered = 0;
    if (COREKUBE_INT_ALGORITHM == NAS_SECURITY_ALGORITHMS_EIA0)
        integrity_protected = 0;

    if (ciphered || integrity_protected) {
        nas_security_header_t h;
        pkbuf_t *new = NULL;

        memset(&h, 0, sizeof(h));
        h.security_header_type = message->h.security_header_type;
        h.protocol_discriminator = message->h.protocol_discriminator;
        c_uint32_t nas_dl_count;
        nas_dl_count = array_to_int(db_pulls->epc_nas_sequence_number);
        h.sequence_number = (nas_dl_count & 0xff);

        status_t plain_encode = nas_plain_encode(&new, message);
        d_assert(plain_encode == CORE_OK, return CORE_ERROR, "NAS plain encoding error");

        if (ciphered) {
            // encrypt NAS message
            nas_encrypt(COREKUBE_ENC_ALGORITHM, db_pulls->enc_key, nas_dl_count, NAS_SECURITY_BEARER, NAS_SECURITY_DOWNLINK_DIRECTION, new);
        }

        // encode sequence number
        status_t pkbuf_head = pkbuf_header(new, 1);
        d_assert(pkbuf_head == CORE_OK, pkbuf_free(new);return CORE_ERROR, "pkbuf_header error");
        *(c_uint8_t *)(new->payload) = h.sequence_number;

        if (integrity_protected) {
            c_uint8_t mac[NAS_SECURITY_MAC_SIZE];

            // calculate NAS MAC(message authentication code)
            nas_mac_calculate(COREKUBE_INT_ALGORITHM, db_pulls->int_key, nas_dl_count, NAS_SECURITY_BEARER, NAS_SECURITY_DOWNLINK_DIRECTION, new, mac);
            memcpy(&h.message_authentication_code, mac, sizeof(mac));
        }

        // encode all security header 
        pkbuf_head = pkbuf_header(new, 5);
        d_assert(pkbuf_head == CORE_OK, pkbuf_free(new);return CORE_ERROR, "pkbuf_header error");
        memcpy(new->payload, &h, sizeof(nas_security_header_t));

        *pkbuf = new;

        // TODO: store in the DB that this UE has an MME security
        // context that is available to be used when decoding messages
    }

    return CORE_OK;
}

// the following function was adapted from
// nas_mac_calculate() in nextepc/src/mme/nas_security.h
void nas_mac_calculate(c_uint8_t algorithm_identity,
        c_uint8_t *knas_int, c_uint32_t count, c_uint8_t bearer, 
        c_uint8_t direction, pkbuf_t *pkbuf, c_uint8_t *mac)
{
    d_info("Calculating NAS MAC");

    d_assert(knas_int, return, "Null param");
    d_assert(bearer >= 0 && bearer <= 0x1f, return, "Invalid param");
    d_assert(direction == 0 || direction == 1, return, "Invalid param");
    d_assert(pkbuf, return, "Null param");
    d_assert(pkbuf->payload, return, "Null param");
    d_assert(pkbuf->len, return, "Null param");
    d_assert(mac, return, "Null param");

    switch(algorithm_identity)
    {
        case NAS_SECURITY_ALGORITHMS_128_EIA2:
        {
            count = htonl(count);
            c_uint8_t *ivec = NULL;;
            c_uint8_t cmac[16];

            pkbuf_header(pkbuf, 8);

            ivec = pkbuf->payload;
            memset(ivec, 0, 8);
            memcpy(ivec + 0, &count, sizeof(count));
            ivec[4] = (bearer << 3) | (direction << 2);

            aes_cmac_calculate(cmac, knas_int, pkbuf->payload, pkbuf->len);
            memcpy(mac, cmac, 4);

            pkbuf_header(pkbuf, -8);

            break;
        }
        case NAS_SECURITY_ALGORITHMS_EIA0:
        {
            d_error("Invalid identity : NAS_SECURITY_ALGORITHMS_EIA0");
            break;
        }
        default:
        {
            d_assert(0, return, "Unknown algorithm identity(%d)", 
                    algorithm_identity);
            break;
        }
    }
}

// the following function was adapted from
// nas_encrypt() in nextepc/src/mme/nas_security.h
void nas_encrypt(c_uint8_t algorithm_identity,
        c_uint8_t *knas_enc, c_uint32_t count, c_uint8_t bearer, 
        c_uint8_t direction, pkbuf_t *pkbuf)
{
    d_info("Encrypting NAS message");

    d_assert(knas_enc, return, "Null param");
    d_assert(bearer >= 0 && bearer <= 0x1f, return, "Invalid param");
    d_assert(direction == 0 || direction == 1, return, "Invalid param");
    d_assert(pkbuf, return, "Null param");
    d_assert(pkbuf->payload, return, "Null param");
    d_assert(pkbuf->len, return, "Null param");

    switch(algorithm_identity)
    {
        case NAS_SECURITY_ALGORITHMS_EEA0:
        {
            d_error("Invalid identity : NAS_SECURITY_ALGORITHMS_EEA0");
            break;
        }
        default:
        {
            d_assert(0, return, "Unknown algorithm identity(%d)", 
                    algorithm_identity);
            break;
        }
    }
}