#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_authentication_response.h"
#include "nas_security_mode_command.h"
#include "downlinknastransport.h"
#include "s1ap_conv.h"

#include <libck.h>

// external reference to variable in the listener
extern char* db_ip_address;

status_t nas_handle_authentication_response(nas_message_t *nas_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, pkbuf_t **nas_pkbuf) {
    d_info("Handling NAS Authentication Message");

    d_assert(nas_message->emm.h.message_type == NAS_AUTHENTICATION_RESPONSE, return CORE_ERROR, "Decoded NAS message is not authentication response");
    nas_authentication_response_t auth_response = nas_message->emm.authentication_response;

    corekube_db_pulls_t db_pulls;
    c_uint8_t buffer[1024];
    status_t db_prerequisites = get_nas_authentication_response_prerequisites_from_db(mme_ue_id, buffer, &db_pulls);
    d_assert(db_prerequisites == CORE_OK, return CORE_ERROR, "Failed to get UplinkNASTransport prerequisite values from DB");

    status_t auth_res_check = nas_authentication_response_check(&auth_response, db_pulls.auth_res);
    // TODO: simply returning an error when XRES != RES is not acceptable
    // in the long-term: we need to return an authentication failure message
    d_assert(auth_res_check == CORE_OK, return CORE_ERROR, "MME XRES and UE RES do not match");

    status_t security_mode_cmd = nas_build_security_mode_command(&db_pulls, nas_pkbuf);
    d_assert(security_mode_cmd == CORE_OK, return CORE_ERROR, "Failed to build NAS Security Mode Command");

    return CORE_OK;
}

status_t get_nas_authentication_response_prerequisites_from_db(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls) {
    d_info("Fetching NAS Authentication Response prerequisites from DB");

    OCTET_STRING_t raw_mme_ue_id;
    s1ap_uint32_to_OCTET_STRING(*mme_ue_id, &raw_mme_ue_id);

    int sock = db_connect(db_ip_address, 0);
    int n;

    // reset the NAS sequence numbers to zero, because the Security Mode Command
    // (sent as a response) requires a new security context
    c_uint8_t reset_sqn = 0;
    n = push_items(buffer, MME_UE_S1AP_ID, (uint8_t *)raw_mme_ue_id.buf, 2,
        UE_NAS_SEQUENCE_NUMBER, &reset_sqn, EPC_NAS_SEQUENCE_NUMBER, &reset_sqn);
    core_free(raw_mme_ue_id.buf);

    const int NUM_PULL_ITEMS = 4;
    n = pull_items(buffer, n, NUM_PULL_ITEMS,
        AUTH_RES, INT_KEY, ENC_KEY, EPC_NAS_SEQUENCE_NUMBER);
    send_request(sock, buffer, n);
    n = recv_response(sock, buffer, 1024);
    db_disconnect(sock);

    d_assert(n == 17 * NUM_PULL_ITEMS, return CORE_ERROR, "Failed to extract values from DB");

    extract_db_values(buffer, n, db_pulls);

    return CORE_OK;
}

status_t nas_authentication_response_check(nas_authentication_response_t *auth_response, c_uint8_t *mme_res) {
    d_info("Checking NAS Authentication Response");

    c_uint8_t *ue_res = auth_response->authentication_response_parameter.res;
    c_uint8_t ue_res_len = auth_response->authentication_response_parameter.length;
    // CoreKube always expects the length of RES to be 8
    d_assert(ue_res_len == 8, return CORE_ERROR, "UE RES len not 8");

    int res_compare = memcmp(ue_res, mme_res, 8);
    d_assert(res_compare == 0, return CORE_ERROR, "MME XRES and UE RES do not match");

    return CORE_OK;
}