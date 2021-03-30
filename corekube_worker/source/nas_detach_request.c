#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "core/include/core_lib.h"

#include "nas_detach_request.h"

#include "nas_detach_accept.h"
#include "s1ap_conv.h"

// external reference to variable in the listener
extern char* db_ip_address;

status_t nas_handle_detach_request(nas_message_t *nas_detach_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, pkbuf_t **nas_pkbuf) {
    d_info("Handling NAS Detach Request");

    nas_detach_request_from_ue_t detach_request = nas_detach_message->emm.detach_request_from_ue;

    // verify the core can handle this type of detach request
    c_uint8_t detach_type = detach_request.detach_type.detach_type;
    d_assert(detach_type == NAS_DETACH_TYPE_FROM_UE_EPS_DETACH, return CORE_ERROR, "Unknown detach type %d, only NAS_DETACH_TYPE_FROM_UE_EPS_DETACH is supported", detach_type);
    c_uint8_t switch_off = detach_request.detach_type.switch_off;
    d_assert(switch_off == 0, return CORE_ERROR, "Core cannot handle detach request with switch_off=1");

    // get the identity of the detaching UE
    nas_eps_mobile_identity_t *mobile_identity = &detach_request.eps_mobile_identity;

    // fetch the state required for processing the detach request
    // TODO: this would be a suitable point to mark the UE as detached,
    // once the DB has this functionality
    corekube_db_pulls_t db_pulls;
    c_uint8_t buffer[1024];
    status_t fetch_state = detach_request_fetch_state(mobile_identity, buffer, &db_pulls);
    d_assert(fetch_state == CORE_OK, return CORE_ERROR, "Failed to fetch detach request state from DB");

    // save the MME_UE_ID so it can be accessed by the callee
    if (mme_ue_id != NULL)
        *mme_ue_id = array_to_int(db_pulls.mme_ue_s1ap_id);

    // build the detach accept
    status_t build_accept = nas_build_detach_accept(mobile_identity, &db_pulls, nas_pkbuf);
    d_assert(build_accept == CORE_OK, return CORE_ERROR, "Failed to build NAS detach accept");

    return CORE_OK;
}

status_t detach_request_fetch_state(nas_eps_mobile_identity_t *mobile_identity, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls) {
    d_info("Fetching Detach Accept state from DB");

    int sock = db_connect(db_ip_address, 0);
    int n;

    // determine the identifier in the detach request
    c_uint8_t identity_type = mobile_identity->imsi.type;

    // use the identity given as the key for the DB
    switch (identity_type) {
        case NAS_MOBILE_IDENTITY_GUTI:
            ; // necessary to stop C complaining about labels and declarations
            c_uint32_t tmsi = mobile_identity->guti.m_tmsi;
            OCTET_STRING_t raw_tmsi;
            s1ap_uint32_to_OCTET_STRING(tmsi, &raw_tmsi);
            n = push_items(buffer, MME_UE_S1AP_ID, (uint8_t *)raw_tmsi.buf, 0);
            core_free(raw_tmsi.buf);
            break;
        default:
            d_error("Unknown EPS mobile identity type %d", identity_type);
            db_disconnect(sock);
            return CORE_ERROR;
    }

    const int NUM_PULL_ITEMS = 7;
    n = pull_items(buffer, n, NUM_PULL_ITEMS,
        MME_UE_S1AP_ID, ENB_UE_S1AP_ID, INT_KEY, ENC_KEY, EPC_NAS_SEQUENCE_NUMBER, KASME_1, KASME_2);
    send_request(sock, buffer, n);
    n = recv_response(sock, buffer, 1024);
    db_disconnect(sock);

    d_assert(n == 17 * NUM_PULL_ITEMS, return CORE_ERROR, "Failed to extract values from DB");

    extract_db_values(buffer, n, db_pulls);

    return CORE_OK;
}