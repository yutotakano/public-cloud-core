#include "db_accesses.h"
#include "ngap/ogs-ngap.h"
#include "nas_pdu_session_establishment_accept.h"

#include "nas_pdu_session_establishment_request.h"

int nas_handle_pdu_session_establishment_request(ogs_nas_5gs_pdu_session_establishment_request_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS PDU Session Establishment Request");

    ogs_assert(message->pdu_session_type.value == OGS_PDU_SESSION_TYPE_IPV4);

    uint32_t pdu_ip;
    ogs_assert(params);
    ogs_assert(params->amf_ue_ngap_id);
    int fetch_pdu_ip = nas_pdu_session_establishment_fetch_pdu_ip(*params->amf_ue_ngap_id, &pdu_ip);
    ogs_assert(fetch_pdu_ip == OGS_OK);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_pdu_session_establishment_accept(pdu_ip, response->responses[0]);
}

int nas_pdu_session_establishment_fetch_pdu_ip(uint32_t amf_ue_ngap_id, uint32_t *pdn_ip) {
    ogs_info("Fetching the PDN IP address from the DB");

    // convert the AMF_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t amf_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING( (uint32_t) amf_ue_ngap_id, &amf_ue_ngap_id_buf);

    // fetch the PDN IP from the database
    unsigned long long start_time = get_microtime();
    corekube_db_pulls_t db_pulls;
    int db = db_access(&db_pulls, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 0, 1, PDN_IP);
    ogs_assert(db == OGS_OK);
    unsigned long long end_time = get_microtime();
    yagra_observe_metric(response->batch, "db_access_latency", (int)(end_time - start_time));

    // store the fetched values in the return structure
    OCTET_STRING_t pdn_ip_str;
    pdn_ip_str.buf = db_pulls.pdn_ip;
    pdn_ip_str.size = IP_LEN;
    ogs_asn_OCTET_STRING_to_uint32(&pdn_ip_str, pdn_ip);

    // free the buffer used for the DB access
    ogs_free(amf_ue_ngap_id_buf.buf);
    // free the structures pulled from the DB
    ogs_free(db_pulls.head);

    return OGS_OK;
}
