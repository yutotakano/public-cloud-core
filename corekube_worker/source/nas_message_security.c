#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_message_security.h"

#include "s1ap_conv.h"
#include "core_aes_cmac.h"

// external reference to variable in the listener
extern char* db_ip_address;

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

    if (new_security_context)
        // TODO: functionality needed in DB to implement this
        d_info("New security context");

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
        // a poor hack: the sequence number is a 6-byte number, but in
        // practice it will never get higher than 4-bytes, so in this
        // context we can treat it like a 4-byte number
        nas_dl_count = array_to_int(db_pulls->epc_nas_sequence_number+2);
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
            d_info("Int key:");
            d_print_hex(db_pulls->int_key, 16);
            d_info("NAS DL count: %d, raw:", nas_dl_count);
            nas_mac_calculate(COREKUBE_INT_ALGORITHM, db_pulls->int_key, nas_dl_count, NAS_SECURITY_BEARER, NAS_SECURITY_DOWNLINK_DIRECTION, new, mac);
            d_info("Calculated MAC:");
            d_print_hex(mac, 4);
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
// nas_security_decode() in nextepc/src/mme/nas_security.c
status_t nas_security_decode(S1AP_NAS_PDU_t *nasPdu, corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf_out) {
    d_info("Decoding NAS Security message");

    nas_security_header_type_t security_header_type;
    pkbuf_t *pkbuf = NULL;

    d_assert(nasPdu, return CORE_ERROR, "Null param");

    /* The Packet Buffer(pkbuf_t) for NAS message MUST make a HEADROOM. 
     * When calculating AES_CMAC, we need to use the headroom of the packet. */
    pkbuf = pkbuf_alloc(NAS_HEADROOM, nasPdu->size);
    d_assert(pkbuf, return CORE_ERROR, "Null param");

    *pkbuf_out = pkbuf;

    memcpy(pkbuf->payload, nasPdu->buf, nasPdu->size);

    generate_nas_security_header_type(pkbuf, &security_header_type);

    d_assert(pkbuf, return CORE_ERROR, "Null param");
    d_assert(pkbuf->payload, return CORE_ERROR, "Null param");

    c_uint32_t nas_ul_count;
    // a poor hack: the sequence number is a 6-byte number, but in
    // practice it will never get higher than 4-bytes, so in this
    // context we can treat it like a 4-byte number
    nas_ul_count = array_to_int(db_pulls->ue_nas_sequence_number+2);

    if (security_header_type.service_request)
    {
#define SHORT_MAC_SIZE 2
        
        c_uint16_t original_pkbuf_len = pkbuf->len;
        c_uint8_t original_mac[SHORT_MAC_SIZE];
        c_uint8_t mac[NAS_SECURITY_MAC_SIZE];

        if (COREKUBE_INT_ALGORITHM == 0)
        {
            d_warn("integrity algorithm is not defined");
            return CORE_ERROR;
        }

        // TODO: unsure about this sequence number stuff
        /*
        c_uint8_t estimated_sequence_number;
        c_uint8_t sequence_number_high_3bit;
        nas_ksi_and_sequence_number_t *ksi_and_sequence_number = pkbuf->payload + 1;
        d_assert(ksi_and_sequence_number, return CORE_ERROR, "Null param");
        estimated_sequence_number = 
            ksi_and_sequence_number->sequence_number;

        sequence_number_high_3bit = mme_ue->ul_count.sqn & 0xe0;
        if ((mme_ue->ul_count.sqn & 0x1f) > estimated_sequence_number)
        {
            sequence_number_high_3bit += 0x20;
        }
        estimated_sequence_number += sequence_number_high_3bit;

        if (mme_ue->ul_count.sqn > estimated_sequence_number)
            mme_ue->ul_count.overflow++;
        mme_ue->ul_count.sqn = estimated_sequence_number;
        */
        // END TODO

        pkbuf->len = 2;
        memcpy(original_mac, pkbuf->payload + 2, SHORT_MAC_SIZE);

        nas_mac_calculate(COREKUBE_INT_ALGORITHM,
            db_pulls->int_key, nas_ul_count, NAS_SECURITY_BEARER,
            NAS_SECURITY_UPLINK_DIRECTION, pkbuf, mac);

        pkbuf->len = original_pkbuf_len;
        memcpy(pkbuf->payload + 2, original_mac, SHORT_MAC_SIZE);

        if (memcmp(mac + 2, pkbuf->payload + 2, 2) != 0)
        {
            d_warn("NAS MAC verification failed");
        }

        return CORE_OK;
    }

    // TODO: does the DB take care of all this for us?
    /*
    if (!mme_ue->security_context_available)
    {
        security_header_type.integrity_protected = 0;
        security_header_type.new_security_context = 0;
        security_header_type.ciphered = 0;
    }

    if (security_header_type.new_security_context)
    {
        mme_ue->ul_count.i32 = 0;
    }
    */
    // END TODO

    if (COREKUBE_ENC_ALGORITHM == 0)
        security_header_type.ciphered = 0;
    if (COREKUBE_INT_ALGORITHM == 0)
        security_header_type.integrity_protected = 0;

    if (security_header_type.ciphered || 
        security_header_type.integrity_protected)
    {
        nas_security_header_t *h = NULL;

        /* NAS Security Header */
        d_assert(CORE_OK == pkbuf_header(pkbuf, 6),
            return CORE_ERROR, "pkbuf_header error");
        h = pkbuf->payload;

        /* NAS Security Header.Sequence_Number */
        d_assert(CORE_OK == pkbuf_header(pkbuf, -5),
            return CORE_ERROR, "pkbuf_header error");

        // TODO: calculating counts
        /* calculate ul_count */
        /*
        if (mme_ue->ul_count.sqn > h->sequence_number)
            mme_ue->ul_count.overflow++;
        mme_ue->ul_count.sqn = h->sequence_number;
        */
        // END TODO

        if (security_header_type.integrity_protected)
        {
            c_uint8_t mac[NAS_SECURITY_MAC_SIZE];
            c_uint32_t mac32;
            c_uint32_t original_mac = h->message_authentication_code;

            /* calculate NAS MAC(message authentication code) */
            nas_mac_calculate(COREKUBE_INT_ALGORITHM,
                db_pulls->int_key, nas_ul_count, NAS_SECURITY_BEARER, 
                NAS_SECURITY_UPLINK_DIRECTION, pkbuf, mac);
            h->message_authentication_code = original_mac;

            memcpy(&mac32, mac, NAS_SECURITY_MAC_SIZE);
            if (h->message_authentication_code != mac32)
            {
                d_warn("NAS MAC verification failed(0x%x != 0x%x)",
                        ntohl(h->message_authentication_code), ntohl(mac32));
            }
        }

        /* NAS EMM Header or ESM Header */
        d_assert(CORE_OK == pkbuf_header(pkbuf, -1),
            return CORE_ERROR, "pkbuf_header error");

        if (security_header_type.ciphered)
        {
            /* decrypt NAS message */
            nas_encrypt(COREKUBE_ENC_ALGORITHM,
                db_pulls->enc_key, nas_ul_count, NAS_SECURITY_BEARER,
                NAS_SECURITY_UPLINK_DIRECTION, pkbuf);
        }
    }

    return CORE_OK;
}

// the following function was adapted from
// s1ap_send_to_nas() in nextepc/src/mme/s1ap_path.c
status_t generate_nas_security_header_type(pkbuf_t *nasbuf, nas_security_header_type_t *security_header_type) {
    d_info("Generating NAS Security Header Type");

    nas_security_header_t *sh = NULL;
    sh = nasbuf->payload;
    d_assert(sh, return CORE_ERROR, "Null param");

    memset(security_header_type, 0, sizeof(nas_security_header_type_t));
    switch(sh->security_header_type)
    {
        case NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE:
            break;
        case NAS_SECURITY_HEADER_FOR_SERVICE_REQUEST_MESSAGE:
            security_header_type->service_request = 1;
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED:
            security_header_type->integrity_protected = 1;
            d_assert(pkbuf_header(nasbuf, -6) == CORE_OK, return CORE_ERROR, "pkbuf_header error");
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED:
            security_header_type->integrity_protected = 1;
            security_header_type->ciphered = 1;
            d_assert(pkbuf_header(nasbuf, -6) == CORE_OK, return CORE_ERROR, "pkbuf_header error");
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT:
            security_header_type->integrity_protected = 1;
            security_header_type->new_security_context = 1;
            d_assert(pkbuf_header(nasbuf, -6) == CORE_OK, return CORE_ERROR, "pkbuf_header error");
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHTERD_WITH_NEW_INTEGRITY_CONTEXT:
            security_header_type->integrity_protected = 1;
            security_header_type->ciphered = 1;
            security_header_type->new_security_context = 1;
            d_assert(pkbuf_header(nasbuf, -6) == CORE_OK, return CORE_ERROR, "pkbuf_header error");
            break;
        default:
            d_error("Not implemented(securiry header type:0x%x)", sh->security_header_type);
            return CORE_ERROR;
    }

    return CORE_OK;
}

status_t get_NAS_decode_security_prerequisites_from_db(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls) {
    d_info("Fetching NAS Decode Security prerequisites from DB");

    OCTET_STRING_t raw_mme_ue_id;
    s1ap_uint32_to_OCTET_STRING(*mme_ue_id, &raw_mme_ue_id);

    int sock = db_connect(db_ip_address, 0);
    int n;

    const int NUM_PULL_ITEMS = 3;
    n = push_items(buffer, MME_UE_S1AP_ID, (uint8_t *)raw_mme_ue_id.buf, 0);
    core_free(raw_mme_ue_id.buf);
    n = pull_items(buffer, n, NUM_PULL_ITEMS,
        INT_KEY, ENC_KEY, UE_NAS_SEQUENCE_NUMBER);
    send_request(sock, buffer, n);
    n = recv_response(sock, buffer, 1024);
    db_disconnect(sock);

    d_assert(n == 17 * NUM_PULL_ITEMS, return CORE_ERROR, "Failed to extract values from DB");

    extract_db_values(buffer, n, db_pulls);

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