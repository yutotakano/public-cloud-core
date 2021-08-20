#include "corekube_config.h"
#include "ngap_pdu_session_resource_setup_request_transfer.h"

#include "ngap_pdu_session_resource_setup_request.h"

int ngap_build_pdu_session_resource_setup_request(ngap_pdu_session_resource_setup_request_params_t * params, ogs_ngap_message_t * response) {
    ogs_info("Building NGAP PDU Session Resource Setup Request");

    // check the required parameters are present
    ogs_assert(params);
    ogs_assert(params->amf_ue_ngap_id);
    ogs_assert(params->ran_ue_ngap_id);

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_PDUSessionResourceSetupRequest_t *PDUSessionResourceSetupRequest;

    NGAP_PDUSessionResourceSetupRequestIEs_t *ie = NULL;
    NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    NGAP_RAN_UE_NGAP_ID_t *RAN_UE_NGAP_ID = NULL;

    NGAP_PDUSessionResourceSetupListSUReq_t *PDUSessionList = NULL;
    NGAP_PDUSessionResourceSetupItemSUReq_t *PDUSessionItem = NULL;
    NGAP_NAS_PDU_t *pDUSessionNAS_PDU = NULL;
    NGAP_S_NSSAI_t *s_NSSAI = NULL;
    NGAP_SST_t *sST = NULL;
    OCTET_STRING_t *transfer = NULL;

    NGAP_UEAggregateMaximumBitRate_t *UEAggregateMaximumBitRate = NULL;

    ogs_debug("PDUSessionResourceSetupRequest(Session)");

    memset(response, 0, sizeof (NGAP_NGAP_PDU_t));
    response->present = NGAP_NGAP_PDU_PR_initiatingMessage;
    response->choice.initiatingMessage = CALLOC(1, sizeof(NGAP_InitiatingMessage_t));

    initiatingMessage = response->choice.initiatingMessage;
    initiatingMessage->procedureCode =
        NGAP_ProcedureCode_id_PDUSessionResourceSetup;
    initiatingMessage->criticality = NGAP_Criticality_reject;
    initiatingMessage->value.present =
        NGAP_InitiatingMessage__value_PR_PDUSessionResourceSetupRequest;

    PDUSessionResourceSetupRequest =
        &initiatingMessage->value.choice.PDUSessionResourceSetupRequest;

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&PDUSessionResourceSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_PDUSessionResourceSetupRequestIEs__value_PR_AMF_UE_NGAP_ID;

    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&PDUSessionResourceSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_PDUSessionResourceSetupRequestIEs__value_PR_RAN_UE_NGAP_ID;

    RAN_UE_NGAP_ID = &ie->value.choice.RAN_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&PDUSessionResourceSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSUReq;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestIEs__value_PR_PDUSessionResourceSetupListSUReq;

    PDUSessionList = &ie->value.choice.PDUSessionResourceSetupListSUReq;

    asn_uint642INTEGER(AMF_UE_NGAP_ID, params->amf_ue_ngap_id);
    *RAN_UE_NGAP_ID = params->ran_ue_ngap_id;

    PDUSessionItem =
        CALLOC(1, sizeof(struct NGAP_PDUSessionResourceSetupItemSUReq));
    ASN_SEQUENCE_ADD(&PDUSessionList->list, PDUSessionItem);

    PDUSessionItem->pDUSessionID = COREKUBE_PDN_SESSION_IDENTITY;

    if (params->nasPdu) {
        PDUSessionItem->pDUSessionNAS_PDU =
            pDUSessionNAS_PDU = CALLOC(1, sizeof(NGAP_NAS_PDU_t));
        pDUSessionNAS_PDU->size = params->nasPdu->len;
        pDUSessionNAS_PDU->buf =
            CALLOC(pDUSessionNAS_PDU->size, sizeof(uint8_t));
        memcpy(pDUSessionNAS_PDU->buf, params->nasPdu->data, pDUSessionNAS_PDU->size);
        ogs_pkbuf_free(params->nasPdu);
    }

    s_NSSAI = &PDUSessionItem->s_NSSAI;
    sST = &s_NSSAI->sST;
    ogs_asn_uint8_to_OCTET_STRING(CoreKube_NSSAI_sST, sST);

    ogs_pkbuf_t * transferBuf = NULL;
    transferBuf = nas_build_ngap_pdu_session_resource_setup_request_transfer();
    ogs_assert(transferBuf);

    transfer = &PDUSessionItem->pDUSessionResourceSetupRequestTransfer;
    transfer->size = transferBuf->len;
    transfer->buf = CALLOC(transfer->size, sizeof(uint8_t));
    memcpy(transfer->buf, transferBuf->data, transfer->size);
    ogs_pkbuf_free(transferBuf);

    /*
     * TS 38.413
     * 8.2.1. PDU Session Resource Setup
     * 8.2.1.2. Successful Operation
     *
     * The UE Aggregate Maximum Bit Rate IE should be sent to the NG-RAN node
     * if the AMF has not sent it previously.
     */
    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&PDUSessionResourceSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestIEs__value_PR_UEAggregateMaximumBitRate;

    UEAggregateMaximumBitRate = &ie->value.choice.UEAggregateMaximumBitRate;

    asn_uint642INTEGER(
            &UEAggregateMaximumBitRate->uEAggregateMaximumBitRateUL,
            COREKUBE_AMBR_DOWNLINK_BPS);
    asn_uint642INTEGER(
            &UEAggregateMaximumBitRate->uEAggregateMaximumBitRateDL,
            COREKUBE_AMBR_UPLINK_BPS);

    return OGS_OK;
}