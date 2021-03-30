#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_attach.h"
#include "nas_util.h"

#include "core/include/core_lib.h"

#include "hss/hss_auc.h"
#include "hss/milenage.h"

#include "nas_security_mode_command.h"
#include "nas_authentication_request.h"

#include "s1ap_conv.h"

#include <libck.h>

// external reference to variable in the listener
extern char* db_ip_address;

status_t nas_handle_attach_request(nas_message_t *nas_attach_message, S1AP_ENB_UE_S1AP_ID_t *enb_ue_id, S1AP_PLMNidentity_t *PLMNidentity, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, pkbuf_t **nas_pkbuf) {
    d_info("Handling NAS Attach Request");

    status_t net_capabilities = check_network_capabilities(nas_attach_message);
    d_assert(net_capabilities == CORE_OK, return CORE_ERROR, "UE does not support MME network capabilities");

    nas_attach_request_t *attach_request = &(nas_attach_message->emm.attach_request);

    nas_mobile_identity_imsi_t * imsi;
    status_t get_imsi = extract_imsi(attach_request, &imsi);
    d_assert(get_imsi == CORE_OK, return CORE_ERROR, "Failed to extract IMSI");

    corekube_db_pulls_t db_pulls;
    c_uint8_t buffer[1024];
    status_t db_prerequisites = get_attach_request_prerequisites_from_db(imsi, buffer, &db_pulls);
    d_assert(db_prerequisites == CORE_OK, return CORE_ERROR, "Failed to get attach request prerequisite values from DB");

    nas_authentication_vector_t auth_vec;
    status_t get_auth_vec = get_authentication_vector(imsi, PLMNidentity, &db_pulls, &auth_vec);
    d_assert(get_auth_vec == CORE_OK, return CORE_ERROR, "Failed to derive authentication vector");

    status_t get_nas_auth_req = generate_nas_authentication_request(auth_vec.rand, auth_vec.autn, nas_pkbuf);
    d_assert(get_nas_auth_req == CORE_OK, return CORE_ERROR, "Failed to generate NAS authentication request");

    status_t save_to_db = save_attach_request_info_in_db(imsi, &auth_vec, enb_ue_id);
    d_assert(save_to_db == CORE_OK, return CORE_ERROR, "Failed to save attach request values into DB");

    // the DB returns the MME_UE_ID as a 2-byte array, however the S1AP expects
    // it as a S1AP_MME_UE_S1AP_ID_t (unsigned long) - this conversion should work
    *mme_ue_id = array_to_int(db_pulls.mme_ue_s1ap_id);

    return CORE_OK;
}

status_t get_attach_request_prerequisites_from_db(nas_mobile_identity_imsi_t *imsi, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls) {
    d_info("Fetching attach request prerequisites from DB");

    c_uint8_t raw_imsi[15];
    status_t get_raw_imsi = extract_raw_imsi(imsi, raw_imsi);
    d_assert(get_raw_imsi == CORE_OK, return CORE_ERROR, "Could not get raw IMSI");

    int sock = db_connect(db_ip_address, 0);
    int n;

    const int NUM_PULL_ITEMS = 5;
    n = push_items(buffer, IMSI, (uint8_t *)raw_imsi, 0);
    n = pull_items(buffer, n, NUM_PULL_ITEMS,
        KEY, OPC, RAND, EPC_NAS_SEQUENCE_NUMBER, MME_UE_S1AP_ID);
    send_request(sock, buffer, n);
    n = recv_response(sock, buffer, 1024);
    db_disconnect(sock);

    d_assert(n == 17 * NUM_PULL_ITEMS, return CORE_ERROR, "Failed to extract values from DB");

    extract_db_values(buffer, n, db_pulls);

    return CORE_OK;
}

status_t save_attach_request_info_in_db(nas_mobile_identity_imsi_t * imsi, nas_authentication_vector_t *auth_vec, S1AP_ENB_UE_S1AP_ID_t *enb_ue_id) {
    d_info("Saving attach request values into DB");

    c_uint8_t raw_imsi[15];
    status_t get_raw_imsi = extract_raw_imsi(imsi, raw_imsi);
    d_assert(get_raw_imsi == CORE_OK, return CORE_ERROR, "Could not get raw IMSI");

    OCTET_STRING_t raw_enb_ue_id;
    s1ap_uint32_to_OCTET_STRING(*enb_ue_id, &raw_enb_ue_id);

    int sock = db_connect(db_ip_address, 0);
    uint8_t buf[1024];
    int n;

    n = push_items(buf, IMSI, (uint8_t *)raw_imsi, 6,
        AUTH_RES, auth_vec->res,
        ENC_KEY, auth_vec->knas_enc,
        INT_KEY, auth_vec->knas_int,
        ENB_UE_S1AP_ID, raw_enb_ue_id.buf,
        KASME_1, auth_vec->kasme,
        KASME_2, (auth_vec->kasme)+16);
    n = pull_items(buf, n, 0);
    send_request(sock, buf, n);

    // don't forget to free the raw_enb_ue_id
    core_free(raw_enb_ue_id.buf);

    db_disconnect(sock);

    return CORE_OK;
}

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
