#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_attach.h"

#include "core/include/core_lib.h"

#include "hss/hss_auc.h"
#include "hss/milenage.h"

#include "nas_security_mode_command.h"

#include "initialuemessage.h"

status_t get_authentication_vector(nas_mobile_identity_imsi_t *imsi, S1AP_PLMNidentity_t *PLMNidentity, corekube_db_pulls_t *db_pulls, nas_authentication_vector_t *auth_vec) {
    d_info("Fetching authentication vector");

    c_uint8_t raw_imsi[15];

    status_t extract_imsi = extract_raw_imsi(imsi, raw_imsi);
    d_assert(extract_imsi == CORE_OK, return CORE_ERROR, "Failed to extract raw IMSI");

    d_assert(PLMNidentity->size == 3, return CORE_ERROR, "PLMN identity not of size 3");

    status_t auth_generate = generate_authentication_vector(
        db_pulls->key,
        db_pulls->opc,
        db_pulls->rand,
        db_pulls->epc_nas_sequence_number,
        PLMNidentity->buf,
        auth_vec);
    d_assert(auth_generate == CORE_OK, return CORE_ERROR, "Failed to generate authentication vector");

    return CORE_OK;
}

status_t extract_raw_imsi(nas_mobile_identity_imsi_t *imsi, c_uint8_t *raw_imsi) {
    d_info("Extracting raw IMSI from NAS message");

    d_assert(imsi->type == NAS_MOBILE_IDENTITY_IMSI, return CORE_ERROR, "Not a true IMSI to extract from");

    raw_imsi[0] = imsi->digit1;
    raw_imsi[1] = imsi->digit2;
    raw_imsi[2] = imsi->digit3;
    raw_imsi[3] = imsi->digit4;
    raw_imsi[4] = imsi->digit5;
    raw_imsi[5] = imsi->digit6;
    raw_imsi[6] = imsi->digit7;
    raw_imsi[7] = imsi->digit8;
    raw_imsi[8] = imsi->digit9;
    raw_imsi[9] = imsi->digit10;
    raw_imsi[10] = imsi->digit11;
    raw_imsi[11] = imsi->digit12;
    raw_imsi[12] = imsi->digit13;
    raw_imsi[13] = imsi->digit14;
    raw_imsi[14] = imsi->digit15;

    for (int i=0; i<15; i++) {
        raw_imsi[i] += 48;
    }

    return CORE_OK;
}

status_t generate_authentication_vector(c_uint8_t *k, c_uint8_t *opc, c_uint8_t *rand, c_uint8_t *sqn, c_uint8_t *plmn_id, nas_authentication_vector_t *auth_vec) {
    d_info("Generating authentication vector");
    
    c_uint8_t amf[] = { 0x80, 0x00 };

    // generate the autn and associated values
    c_uint8_t res[8];
    c_uint8_t ck[16];
    c_uint8_t ik[16];
    c_uint8_t ak[6];
    c_uint8_t autn[16];
    size_t res_len = MAX_RES_LEN;
    milenage_generate(opc, amf, k, sqn, rand, autn, ik, ck, ak, res, &res_len);
    d_assert(res_len != 0, return CORE_ERROR, "Failed in milenage generation");
    // CoreKube always expects the RES length to be 8
    d_assert(res_len == 8, return CORE_ERROR, "RES length not 8");

    // generate the kasme
    c_uint8_t kasme[32];
    hss_auc_kasme(ck, ik, plmn_id, sqn, ak, kasme);

    // generate ENC_KEY and INT_KEY now, and save them in the DB
    // they will be used later in the attach procedure, during the
    // security mode command
    c_uint8_t knas_int[SHA256_DIGEST_SIZE/2]; 
    c_uint8_t knas_enc[SHA256_DIGEST_SIZE/2];
    status_t nas_key_gen = generate_nas_keys(kasme, knas_int, knas_enc);
    d_assert(nas_key_gen == CORE_OK, return CORE_ERROR, "Failed to generate NAS keys");

    memcpy(auth_vec->rand, rand, RAND_LEN);
    memcpy(auth_vec->autn, autn, AUTN_LEN);
    memcpy(auth_vec->res, res, res_len);
    memcpy(auth_vec->knas_int, knas_int, SHA256_DIGEST_SIZE/2);
    memcpy(auth_vec->knas_enc, knas_enc, SHA256_DIGEST_SIZE/2);
    memcpy(auth_vec->kasme, kasme, SHA256_DIGEST_SIZE);

    return CORE_OK;
}

status_t check_network_capabilities(nas_message_t *nas_message) {
    d_info("Checking UE network capabilities matched expected");

    nas_ue_network_capability_t *network_capability;

    network_capability = &nas_message->emm.attach_request.ue_network_capability;

    // CoreKube only supports EEA0 and 128-EIA2
    d_assert(network_capability->eea0 == 1, return CORE_ERROR, "UE does not support EEA0");
    d_assert(network_capability->eia2 == 1, return CORE_ERROR, "UE does not support 128-EIA2");

    return CORE_OK;
}

status_t extract_imsi(nas_attach_request_t *attach_request, nas_mobile_identity_imsi_t **imsi) {
    d_info("Extracting IMSI from NAS message");

    *imsi = &attach_request->eps_mobile_identity.imsi;

    d_assert((*imsi)->type == NAS_MOBILE_IDENTITY_IMSI, return CORE_ERROR, "Only IMSI is supported");

    return CORE_OK;
}

status_t decode_attach_request(S1AP_InitialUEMessage_t *initialUEMessage, nas_message_t *attach_request) {
    d_info("Decodeing NAS attach request");

    S1AP_NAS_PDU_t *NAS_PDU = NULL;

    S1AP_InitialUEMessage_IEs_t *NAS_PDU_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_NAS_PDU, &NAS_PDU_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get NAS_PDU IE from InitialUEMessage");

    NAS_PDU = &NAS_PDU_IE->value.choice.NAS_PDU;

    pkbuf_t *pkbuf;

    memset(attach_request, 0, sizeof (nas_message_t));

    pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
    pkbuf->len = NAS_PDU->size;
    memcpy(pkbuf->payload, NAS_PDU->buf, pkbuf->len);

    status_t nas_decode = nas_emm_decode(attach_request, pkbuf);
    d_assert(nas_decode == CORE_OK, return CORE_ERROR, "Failed to decode NAS EMM message");

    pkbuf_free(pkbuf);

    d_assert(attach_request->emm.h.message_type == NAS_ATTACH_REQUEST, return CORE_ERROR, "Decoded NAS message is not an attach request");

    return CORE_OK;
}