#include "ngap/ogs-ngap.h"
#include "db_accesses.h"

#include "nas_security.h"

ogs_pkbuf_t *nas_5gs_security_encode(nas_ngap_params_t * params, ogs_nas_5gs_message_t *message) {
    ogs_info("Encoding NAS 5GS message");

    int integrity_protected = 0;
    int ciphered = 0;
    ogs_nas_5gs_security_header_t h;
    ogs_pkbuf_t *new = NULL;

    ogs_assert(message);
    ogs_assert(params);
    ogs_assert(params->nas_security_params);

    switch (message->h.security_header_type) {
    case OGS_NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE:
        return ogs_nas_5gs_plain_encode(message);
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED:
        integrity_protected = 1;
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED:
        integrity_protected = 1;
        ciphered = 1;
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT:
        integrity_protected = 1;
        break;
    case OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHTERD_WITH_NEW_INTEGRITY_CONTEXT:
        integrity_protected = 1;
        ciphered = 1;
        break;
    default:
        ogs_error("Not implemented(securiry header type:0x%x)", 
                message->h.security_header_type);
        return NULL;
    }

    if (COREKUBE_NAS_SECURITY_ENC_ALGORITHM == 0)
        ciphered = 0;
    if (COREKUBE_NAS_SECURITY_INT_ALGORITHM == 0)
        integrity_protected = 0;

    memset(&h, 0, sizeof(h));
    h.security_header_type = message->h.security_header_type;
    h.extended_protocol_discriminator =
        message->h.extended_protocol_discriminator;
    h.sequence_number = (params->nas_security_params->dl_count & 0xff);

    new = ogs_nas_5gs_plain_encode(message);
    if (!new) {
        ogs_error("ogs_nas_5gs_plain_encode() failed");
        return NULL;
    }

    if (ciphered) {
        // the NAS encryption key is required, check it exists
        if (!params->nas_security_params->knas_enc) {
            // security key is missing, fetch it from the DB
            int fetch_keys = nas_security_fetch_keys(params, NAS_COUNTS_JUST_DL);
            ogs_assert(fetch_keys == OGS_OK);
        }

        /* encrypt NAS message */
        ogs_nas_encrypt(COREKUBE_NAS_SECURITY_ENC_ALGORITHM,
            params->nas_security_params->knas_enc, params->nas_security_params->dl_count,
            COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS,
            COREKUBE_NAS_SECURITY_DOWNLINK_DIRECTION, new);
    }

    /* encode sequence number */
    ogs_assert(ogs_pkbuf_push(new, 1));
    *(uint8_t *)(new->data) = h.sequence_number;

    if (integrity_protected) {
        // the NAS integrity key is required, check it exists
        if (!params->nas_security_params->knas_int) {
            // integrity key is missing, fetch it from the DB
            int fetch_keys = nas_security_fetch_keys(params, NAS_COUNTS_JUST_DL);
            ogs_assert(fetch_keys == OGS_OK);
        }

        uint8_t mac[COREKUBE_NAS_SECURITY_MAC_SIZE];

        /* calculate NAS MAC(message authentication code) */
        ogs_nas_mac_calculate(COREKUBE_NAS_SECURITY_INT_ALGORITHM,
            params->nas_security_params->knas_int, params->nas_security_params->dl_count,
            COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS,
            COREKUBE_NAS_SECURITY_DOWNLINK_DIRECTION, new, mac);
        memcpy(&h.message_authentication_code, mac, sizeof(mac));
    }

    /* encode all security header */
    ogs_assert(ogs_pkbuf_push(new, 6));
    memcpy(new->data, &h, sizeof(ogs_nas_5gs_security_header_t));

    return new;
}

int nas_5gs_security_decode(nas_ngap_params_t * params, ogs_nas_security_header_type_t security_header_type, ogs_pkbuf_t *pkbuf)
{
    ogs_info("Decoding NAS 5GS message");

    // NAS GSM messages are not security encoded, and will fail if decoding is attempted
    ogs_nas_5gsm_header_t * h = (ogs_nas_5gsm_header_t *)pkbuf->data;
    if (h->extended_protocol_discriminator == OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GSM) {
        ogs_info("Found Session Management (5GSM) Message");
        return OGS_OK;
    }

    ogs_assert(pkbuf);
    ogs_assert(pkbuf->data);

    if (COREKUBE_NAS_SECURITY_ENC_ALGORITHM == 0)
        security_header_type.ciphered = 0;
    if (COREKUBE_NAS_SECURITY_INT_ALGORITHM == 0)
        security_header_type.integrity_protected = 0;

    if (security_header_type.ciphered || 
        security_header_type.integrity_protected) {
        ogs_nas_5gs_security_header_t *h = NULL;

        /* NAS Security Header */
        ogs_assert(ogs_pkbuf_push(pkbuf, 7));
        h = (ogs_nas_5gs_security_header_t *)pkbuf->data;

        /* NAS Security Header.Sequence_Number */
        ogs_assert(ogs_pkbuf_pull(pkbuf, 6));

        if (security_header_type.integrity_protected) {
            uint8_t mac[COREKUBE_NAS_SECURITY_MAC_SIZE];
            uint32_t mac32;
            uint32_t original_mac = h->message_authentication_code;

            // the NAS integrity key is required, check it exists
            if (!params->nas_security_params->knas_int) {
                // integrity key is missing, fetch it from the DB
                int fetch_keys = nas_security_fetch_keys(params, NAS_COUNTS_BOTH_UL_DL);
                ogs_assert(fetch_keys == OGS_OK);
            }

            /* calculate NAS MAC(message authentication code) */
            ogs_nas_mac_calculate(COREKUBE_NAS_SECURITY_INT_ALGORITHM,
                params->nas_security_params->knas_int, params->nas_security_params->ul_count,
                COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS,
                COREKUBE_NAS_SECURITY_UPLINK_DIRECTION, pkbuf, mac);
            h->message_authentication_code = original_mac;

            memcpy(&mac32, mac, COREKUBE_NAS_SECURITY_MAC_SIZE);
            if (h->message_authentication_code != mac32) {
                ogs_warn("NAS MAC verification failed(0x%x != 0x%x)",
                    be32toh(h->message_authentication_code), be32toh(mac32));
            }
        }

        /* NAS EMM Header or ESM Header */
        ogs_assert(ogs_pkbuf_pull(pkbuf, 1));

        if (security_header_type.ciphered) {

            // the NAS security key is required, check it exists
            if (!params->nas_security_params->knas_enc) {
                // security key is missing, fetch it from the DB
                int fetch_keys = nas_security_fetch_keys(params, NAS_COUNTS_BOTH_UL_DL);
                ogs_assert(fetch_keys == OGS_OK);
            }

            /* decrypt NAS message */
            ogs_nas_encrypt(COREKUBE_NAS_SECURITY_ENC_ALGORITHM,
                params->nas_security_params->knas_enc, params->nas_security_params->ul_count,
                COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS,
                COREKUBE_NAS_SECURITY_UPLINK_DIRECTION, pkbuf);
        }
    }

    return OGS_OK;
}


int nas_5gs_generate_keys(ogs_nas_5gs_mobile_identity_t * mob_ident, uint8_t * opc, uint8_t * key, uint8_t * rand, uint8_t * autn, uint8_t * kamf) {
    ogs_info("Generating NAS Security keys");

    // generate the authentication parameters from RAND, Key and OPC
    uint8_t amf[OGS_AMF_LEN] = CoreKube_AMF_Field;
    uint8_t sqn[OGS_SQN_LEN] = CoreKube_SQN_Value;
    uint8_t ik[16];
    uint8_t ck[16];
    uint8_t ak[6];
    uint8_t res[OGS_MAX_RES_LEN];
    size_t res_len = OGS_MAX_RES_LEN;
    milenage_generate(opc, amf, key, sqn, rand, autn, ik, ck, ak, res, &res_len);
    ogs_assert(res_len != 0); // Failed to generate security parameters

    // generate the PLMN identity
    ogs_plmn_id_t PLMNid;
    ogs_plmn_id_build(&PLMNid, CoreKube_MCC, CoreKube_MNC, CoreKube_MNClen);

    // generate the serving network identity
    char * serving_network_name = ogs_serving_network_name_from_plmn_id(&PLMNid);

    // generate the kausf
    uint8_t kausf[OGS_KEY_LEN*2];
    ogs_kdf_kausf(ck, ik, serving_network_name, autn, kausf);

    // generate the kseaf
    uint8_t kseaf[OGS_SHA256_DIGEST_SIZE];
    ogs_kdf_kseaf(serving_network_name, kausf, kseaf);

    // generate the kamf
    char * suci = ogs_nas_5gs_suci_from_mobile_identity(mob_ident);
    char * supi = ogs_supi_from_suci(suci);
    uint8_t abba_value[CoreKube_ABBA_Length];
    OGS_HEX(CoreKube_ABBA_Value, strlen(CoreKube_ABBA_Value), abba_value);
    ogs_kdf_kamf(supi, (uint8_t *) abba_value, CoreKube_ABBA_Length, kseaf, kamf);

    return OGS_OK;
}

int nas_security_fetch_keys(nas_ngap_params_t * params, int includeCounts) {
    ogs_info("Fetching NAS security keys from database");

    ogs_assert(params);
    ogs_assert(params->amf_ue_ngap_id);

    // convert the AMF_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t amf_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING( (uint32_t) *params->amf_ue_ngap_id, &amf_ue_ngap_id_buf);

    // check to see if both the UL / DL counts should be fetched, or just one.
    // normally both are required (to decode an incoming NAS message and then
    // encode the response) but sometimes just a response needs to be sent, or
    // just an incoming message needs to be received.
    // fetch the KAMF (stored as KASME1 and KASME2) and DL / UL counts from the DB
    corekube_db_pulls_t *db_pulls;
    int db;
    if (includeCounts == NAS_COUNTS_BOTH_UL_DL)
        db = db_access(&db_pulls, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 0, 4, KASME_1, KASME_2, EPC_NAS_SEQUENCE_NUMBER, UE_NAS_SEQUENCE_NUMBER);
    else if (includeCounts == NAS_COUNTS_JUST_DL)
        db = db_access(&db_pulls, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 0, 3, KASME_1, KASME_2, EPC_NAS_SEQUENCE_NUMBER);
    else if (includeCounts == NAS_COUNTS_JUST_UL)
        db = db_access(&db_pulls, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 0, 3, KASME_1, KASME_2, UE_NAS_SEQUENCE_NUMBER);
    else {
        ogs_error("Unknown includeCounts param: %d", includeCounts);
        return OGS_ERROR;
    }
    ogs_assert(db == OGS_OK);

    // store the KNAS_INT, KNAS_ENC and DL count in the NAS params
    int store_keys = nas_security_store_keys_in_params(db_pulls, params->nas_security_params);
    ogs_assert(store_keys == OGS_OK);

    // free the structures pulled from the DB
    ogs_free(db_pulls->head);

    return OGS_OK;
}

int nas_security_store_keys_in_params(corekube_db_pulls_t * db_pulls, nas_security_params_t * nas_security_params) {
    ogs_info("Storing NAS security keys from DB pulls struct into nas_security_params struct");

    // check the input parameters exist
    ogs_assert(db_pulls);
    ogs_assert(nas_security_params);

    // check all the required values from the DB are present
    ogs_assert(db_pulls->kasme1);
    ogs_assert(db_pulls->kasme2);

    // store the Kamf in the security params struct
    nas_security_params->kamf = ogs_malloc(32 * sizeof(uint8_t));
    memcpy(nas_security_params->kamf, db_pulls->kasme1, 16);
    memcpy(nas_security_params->kamf+16, db_pulls->kasme2, 16);

    nas_security_params->knas_int = ogs_malloc(16 * sizeof(uint8_t));
    nas_security_params->knas_enc = ogs_calloc(16, sizeof(uint8_t));

    // generate knas_int
    ogs_kdf_nas_5gs(OGS_KDF_NAS_INT_ALG, COREKUBE_NAS_SECURITY_INT_ALGORITHM, nas_security_params->kamf, nas_security_params->knas_int);

    // generate knas_enc
    ogs_kdf_nas_5gs(OGS_KDF_NAS_ENC_ALG, COREKUBE_NAS_SECURITY_ENC_ALGORITHM, nas_security_params->kamf, nas_security_params->knas_enc);

    // copy the DL count if required
    if (db_pulls->epc_nas_sequence_number) {
        OCTET_STRING_t dl_count_buf;
        dl_count_buf.buf = db_pulls->epc_nas_sequence_number+2; // TODO: bodge from when counts were stored as 6 bytes
        dl_count_buf.size = 4;
        ogs_asn_OCTET_STRING_to_uint32(&dl_count_buf, &nas_security_params->dl_count);
        ogs_info("Fetched DL count: %d", nas_security_params->dl_count);
    }

    // copy the UL count if required
    if (db_pulls->ue_nas_sequence_number) {
        OCTET_STRING_t ul_count_buf;
        ul_count_buf.buf = db_pulls->ue_nas_sequence_number+2; // TODO: bodge from when counts were stored as 6 bytes
        ul_count_buf.size = 4;
        ogs_asn_OCTET_STRING_to_uint32(&ul_count_buf, &nas_security_params->ul_count);
        ogs_info("Fetched UL count: %d", nas_security_params->ul_count);
    }

    return OGS_OK;
}