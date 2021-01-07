#include "initialuemessage.h"
#include "s1ap_handler.h"
#include "nas_authentication_request.h"
#include "downlinknastransport.h"

#include <libck.h>

S1AP_handle_outcome_t handle_initialuemessage(s1ap_message_t *received_message, s1ap_message_t *response) {

    S1AP_InitialUEMessage_t *initialUEMessage = &received_message->choice.initiatingMessage->value.choice.InitialUEMessage;

    S1AP_ENB_UE_S1AP_ID_t *enb_ue_id;
    status_t get_enb_eu_id = extract_ENB_UE_ID(initialUEMessage, &enb_ue_id);
    d_assert(get_enb_eu_id == CORE_OK, return CORE_ERROR, "Failed to extract ENB_UE_ID");

    nas_message_t nas_attach_message;
    status_t decode_attach = decode_attach_request(initialUEMessage, &nas_attach_message);
    d_assert(decode_attach == CORE_OK, return CORE_ERROR, "Error decoding NAS attach request");

    status_t net_capabilities = check_network_capabilities(&nas_attach_message);
    d_assert(net_capabilities == CORE_OK, return CORE_ERROR, "UE does not support MME network capabilities");

    nas_attach_request_t *attach_request = &nas_attach_message.emm.attach_request;

    nas_mobile_identity_imsi_t * imsi;
    status_t get_imsi = extract_imsi(attach_request, &imsi);
    d_assert(get_imsi == CORE_OK, return CORE_ERROR, "Failed to extract IMSI");

    S1AP_PLMNidentity_t *PLMNidentity;
    status_t get_PLMN = extract_PLMNidentity(initialUEMessage, &PLMNidentity);
    d_assert(get_PLMN == CORE_OK, return CORE_ERROR, "Failed to extract PLMN identity");

    corekube_db_pulls_t db_pulls;
    c_uint8_t buffer[1024];
    status_t db_prerequisites = get_initialue_prerequisites_from_db(imsi, buffer, &db_pulls);
    d_assert(db_prerequisites == CORE_OK, return CORE_ERROR, "Failed to get InitialUE prerequisite values from DB");

    nas_authentication_vector_t auth_vec;
    status_t get_auth_vec = get_authentication_vector(imsi, PLMNidentity, &db_pulls, &auth_vec);
    d_assert(get_auth_vec == CORE_OK, return CORE_ERROR, "Failed to derive authentication vector");

    pkbuf_t *nas_pkbuf;
    status_t get_nas_auth_req = generate_nas_authentication_request(auth_vec.rand, auth_vec.autn, &nas_pkbuf);
    d_assert(get_nas_auth_req == CORE_OK, return CORE_ERROR, "Failed to generate NAS authentication request");

    // the DB returns the MME_UE_ID as a 2-byte array, however the S1AP expects
    // it as a S1AP_MME_UE_S1AP_ID_t (unsigned long) - this conversion should work
    S1AP_MME_UE_S1AP_ID_t mme_ue_id = 0;
    mme_ue_id += db_pulls.mme_ue_s1ap_id[0] << 24;
    mme_ue_id += db_pulls.mme_ue_s1ap_id[1] << 16;
    mme_ue_id += db_pulls.mme_ue_s1ap_id[2] << 8;
    mme_ue_id += db_pulls.mme_ue_s1ap_id[3];

    status_t get_downlink = generate_downlinknastransport(nas_pkbuf, mme_ue_id, *enb_ue_id, response);
    d_assert(get_downlink == CORE_OK, return CORE_ERROR, "Failed to generate DownlinkNASTransport message");

    status_t save_to_db = save_initialue_info_in_db(imsi, &auth_vec, enb_ue_id);
    d_assert(save_to_db == CORE_OK, return CORE_ERROR, "Failed to save InitialUE values into DB");

    return HAS_RESPONSE;
}

status_t extract_PLMNidentity(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_PLMNidentity_t **PLMNidentity) {

    S1AP_InitialUEMessage_IEs_t *TAI_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_TAI, &TAI_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get TAI IE from InitialUEMessage");

    *PLMNidentity = &TAI_IE->value.choice.TAI.pLMNidentity;

    return CORE_OK;
}

status_t extract_ENB_UE_ID(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_ENB_UE_S1AP_ID_t **ENB_UE_ID) {

    S1AP_InitialUEMessage_IEs_t *ENB_UE_ID_IE;
    status_t get_ie = get_InitialUE_IE(initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR_ENB_UE_S1AP_ID, &ENB_UE_ID_IE);
    d_assert(get_ie == CORE_OK, return CORE_ERROR, "Failed to get ENB_UE_ID IE from InitialUEMessage");

    *ENB_UE_ID = &ENB_UE_ID_IE->value.choice.ENB_UE_S1AP_ID;

    return CORE_OK;
}

status_t get_InitialUE_IE(S1AP_InitialUEMessage_t *initialUEMessage, S1AP_InitialUEMessage_IEs__value_PR desiredIElabel, S1AP_InitialUEMessage_IEs_t **desiredIE) {
    int numIEs = initialUEMessage->protocolIEs.list.count;
    for (int i = 0; i < numIEs; i++) {
        S1AP_InitialUEMessage_IEs_t *theIE = initialUEMessage->protocolIEs.list.array[i];
        if (theIE->value.present == desiredIElabel) {
            *desiredIE = theIE;
            return CORE_OK;
        }
    }

    // if we reach here, then the desired IE has not been found
    return CORE_ERROR;
}

status_t get_initialue_prerequisites_from_db(nas_mobile_identity_imsi_t *imsi, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls) {

    c_uint8_t raw_imsi[15];
    status_t get_raw_imsi = extract_raw_imsi(imsi, raw_imsi);
    d_assert(get_raw_imsi == CORE_OK, return CORE_ERROR, "Could not get raw IMSI");

    int sock = db_connect("127.0.0.1", 0);
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

status_t save_initialue_info_in_db(nas_mobile_identity_imsi_t * imsi, nas_authentication_vector_t *auth_vec, S1AP_ENB_UE_S1AP_ID_t *enb_ue_id) {

    c_uint8_t raw_imsi[15];
    status_t get_raw_imsi = extract_raw_imsi(imsi, raw_imsi);
    d_assert(get_raw_imsi == CORE_OK, return CORE_ERROR, "Could not get raw IMSI");

    int sock = db_connect("127.0.0.1", 0);
    uint8_t buf[1024];
    int n;

    n = push_items(buf, IMSI, (uint8_t *)raw_imsi, 4,
        AUTH_RES, auth_vec->res,
        ENC_KEY, auth_vec->knas_enc,
        INT_KEY, auth_vec->knas_int,
        ENB_UE_S1AP_ID, enb_ue_id);
    n = pull_items(buf, n, 0);
    send_request(sock, buf, n);

    db_disconnect(sock);

    return CORE_OK;
}