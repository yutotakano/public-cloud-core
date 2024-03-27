#include "corekube_config.h"
#include "db_accesses.h"
#include "yagra.h"

#include "ngap_pdu_session_resource_setup_response_transfer.h"


int ngap_handle_pdu_session_resource_setup_response_transfer(uint64_t amf_ue_ngap_id, ogs_pkbuf_t *pkbuf, message_handler_response_t *response) {
    ogs_info("Handling NGAP PDU Session Resource Setup Response Transfer message");

    uint32_t gnb_n3_teid;
    ogs_ip_t gnb_n3_ip;

    NGAP_PDUSessionResourceSetupResponseTransfer_t message;

    NGAP_QosFlowPerTNLInformation_t *dLQosFlowPerTNLInformation = NULL;

    NGAP_UPTransportLayerInformation_t *uPTransportLayerInformation = NULL;
    NGAP_GTPTunnel_t *gTPTunnel = NULL;

    ogs_assert(pkbuf);

    ogs_debug("PDUSessionResourceSetupResponseTransfer");

    int pkbuf_decode = ogs_asn_decode(
            &asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer,
            &message,
            sizeof(message),
            pkbuf
    );
    ogs_assert(pkbuf_decode == OGS_OK);

    dLQosFlowPerTNLInformation = &message.dLQosFlowPerTNLInformation;
    uPTransportLayerInformation = &dLQosFlowPerTNLInformation->uPTransportLayerInformation;

    ogs_assert(uPTransportLayerInformation->present == NGAP_UPTransportLayerInformation_PR_gTPTunnel);

    gTPTunnel = uPTransportLayerInformation->choice.gTPTunnel;
    ogs_assert(gTPTunnel);

    ogs_asn_BIT_STRING_to_ip(&gTPTunnel->transportLayerAddress, &gnb_n3_ip);
    ogs_assert(gnb_n3_ip.ipv4 == 1); // Ensure IPv4, not IPv6 or IPv4/6
    ogs_asn_OCTET_STRING_to_uint32(&gTPTunnel->gTP_TEID, &gnb_n3_teid);

    // Convert the IP and TEID to parameters ready to be stored in DB
    pdu_session_resource_setup_response_transfer_params_t params;
    params.amf_ue_ngap_id = amf_ue_ngap_id;
    OCTET_STRING_t ip_addr_buf;
    ogs_asn_uint32_to_OCTET_STRING(gnb_n3_ip.addr, &ip_addr_buf);
    params.enb_ip = ip_addr_buf.buf;
    OCTET_STRING_t teid_buf;
    ogs_asn_uint32_to_OCTET_STRING(gnb_n3_teid, &teid_buf);
    params.ue_teid = teid_buf.buf;

    // Store the IP and TEID in the DB
    int store_result = nas_store_ngap_pdu_session_resource_setup_response_transfer_fetch_prerequisites(&params, response);
    ogs_assert(store_result == OGS_OK);

    // Free the structs used for the DB access
    ogs_free(ip_addr_buf.buf);
    ogs_free(teid_buf.buf);

    ogs_asn_free(&asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, &message);

    return OGS_OK;
}


int nas_store_ngap_pdu_session_resource_setup_response_transfer_fetch_prerequisites(pdu_session_resource_setup_response_transfer_params_t * params, message_handler_response_t *response) {
    ogs_info("Storing RAN-side UE_TEID and ENB_IP into database");

    ogs_assert(params);

    // convert the AMF_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t amf_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING( (uint32_t) params->amf_ue_ngap_id, &amf_ue_ngap_id_buf);

    // store the UE_TEID and ENB_IP into the database
    unsigned long long start_time = get_microtime();
    corekube_db_pulls_t dbp;
    int db = db_access(&dbp, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 2, 1, UE_TEID, params->ue_teid, ENB_IP, params->enb_ip, PDN_IP);
    ogs_assert(db == OGS_OK);
    unsigned long long end_time = get_microtime();
    yagra_observe_metric(response->batch, "db_access_latency", (int)(end_time - start_time));
    
    ogs_free(dbp.head);

    // free the buffer used for the DB access
    ogs_free(amf_ue_ngap_id_buf.buf);

    return OGS_OK;
}
