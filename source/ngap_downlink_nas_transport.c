#include "ngap_downlink_nas_transport.h"

int ngap_build_downlink_nas_transport(ngap_downlink_nas_transport_params_t * params, ogs_ngap_message_t * response) {
    ogs_info("Building NGAP Downlink NAS Transport");

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_DownlinkNASTransport_t *DownlinkNASTransport = NULL;

    NGAP_DownlinkNASTransport_IEs_t *ie = NULL;
    NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    NGAP_RAN_UE_NGAP_ID_t *RAN_UE_NGAP_ID = NULL;
    NGAP_NAS_PDU_t *NAS_PDU = NULL;
    NGAP_UEAggregateMaximumBitRate_t *UEAggregateMaximumBitRate = NULL;
    NGAP_AllowedNSSAI_t *AllowedNSSAI = NULL;

    // check the required parameters are present
    ogs_assert(params);
    ogs_assert(params->amf_ue_ngap_id);
    ogs_assert(params->ran_ue_ngap_id);
    ogs_assert(params->nasPdu);

    memset(response, 0, sizeof (NGAP_NGAP_PDU_t));
    response->present = NGAP_NGAP_PDU_PR_initiatingMessage;
    response->choice.initiatingMessage = CALLOC(1, sizeof(NGAP_InitiatingMessage_t));

    initiatingMessage = response->choice.initiatingMessage;
    initiatingMessage->procedureCode =
        NGAP_ProcedureCode_id_DownlinkNASTransport;
    initiatingMessage->criticality = NGAP_Criticality_ignore;
    initiatingMessage->value.present =
        NGAP_InitiatingMessage__value_PR_DownlinkNASTransport;

    DownlinkNASTransport =
        &initiatingMessage->value.choice.DownlinkNASTransport;

    ie = CALLOC(1, sizeof(NGAP_DownlinkNASTransport_IEs_t));
    ASN_SEQUENCE_ADD(&DownlinkNASTransport->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_DownlinkNASTransport_IEs__value_PR_AMF_UE_NGAP_ID;

    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_DownlinkNASTransport_IEs_t));
    ASN_SEQUENCE_ADD(&DownlinkNASTransport->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_DownlinkNASTransport_IEs__value_PR_RAN_UE_NGAP_ID;

    RAN_UE_NGAP_ID = &ie->value.choice.RAN_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_DownlinkNASTransport_IEs_t));
    ASN_SEQUENCE_ADD(&DownlinkNASTransport->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_NAS_PDU;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_DownlinkNASTransport_IEs__value_PR_NAS_PDU;

    NAS_PDU = &ie->value.choice.NAS_PDU;

    asn_uint642INTEGER(AMF_UE_NGAP_ID, params->amf_ue_ngap_id);
    *RAN_UE_NGAP_ID = params->ran_ue_ngap_id;

    NAS_PDU->size = params->nasPdu->len;
    NAS_PDU->buf = CALLOC(NAS_PDU->size, sizeof(uint8_t));
    memcpy(NAS_PDU->buf, params->nasPdu->data, NAS_PDU->size);
    ogs_pkbuf_free(params->nasPdu);

    /*
     * TS 38.413
     * 8.6.2 Downlink NAS Transport
     * 8.6.2.1. Successful Operation
     *
     * The UE Aggregate Maximum Bit Rate IE should be sent to the NG-RAN node
     * if the AMF has not sent it previously
     */
    if (params->ambr) {
        ogs_assert(params->ambr->downlink && params->ambr->uplink);

        ie = CALLOC(1, sizeof(NGAP_DownlinkNASTransport_IEs_t));
        ASN_SEQUENCE_ADD(&DownlinkNASTransport->protocolIEs, ie);

        ie->id = NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_DownlinkNASTransport_IEs__value_PR_UEAggregateMaximumBitRate;

        UEAggregateMaximumBitRate = &ie->value.choice.UEAggregateMaximumBitRate;

        asn_uint642INTEGER(
                &UEAggregateMaximumBitRate->uEAggregateMaximumBitRateUL,
                params->ambr->uplink);
        asn_uint642INTEGER(
                &UEAggregateMaximumBitRate->uEAggregateMaximumBitRateDL,
                params->ambr->downlink);
    }

    if (params->num_of_s_nssai > 0) {
        int i;
        ogs_assert(params->s_nssai);

        ie = CALLOC(1, sizeof(NGAP_DownlinkNASTransport_IEs_t));
        ASN_SEQUENCE_ADD(&DownlinkNASTransport->protocolIEs, ie);

        ie->id = NGAP_ProtocolIE_ID_id_AllowedNSSAI;
        ie->criticality = NGAP_Criticality_reject;
        ie->value.present =
            NGAP_DownlinkNASTransport_IEs__value_PR_AllowedNSSAI;

        AllowedNSSAI = &ie->value.choice.AllowedNSSAI;

        ogs_assert(params->num_of_s_nssai);
        for (i = 0; i < params->num_of_s_nssai; i++) {
            NGAP_AllowedNSSAI_Item_t *NGAP_AllowedNSSAI_Item = NULL;
            NGAP_S_NSSAI_t *s_NSSAI = NULL;
            NGAP_SST_t *sST = NULL;

            NGAP_AllowedNSSAI_Item = (NGAP_AllowedNSSAI_Item_t *)
                    CALLOC(1, sizeof(NGAP_AllowedNSSAI_Item_t));
            s_NSSAI = &NGAP_AllowedNSSAI_Item->s_NSSAI;
            sST = &s_NSSAI->sST;

            ogs_asn_uint8_to_OCTET_STRING(
                params->s_nssai[i].sst, sST);
            if (params->s_nssai[i].sd.v !=
                    OGS_S_NSSAI_NO_SD_VALUE) {
                s_NSSAI->sD = CALLOC(1, sizeof(NGAP_SD_t));
                ogs_asn_uint24_to_OCTET_STRING(
                    params->s_nssai[i].sd, s_NSSAI->sD);
            }

            ASN_SEQUENCE_ADD(&AllowedNSSAI->list, NGAP_AllowedNSSAI_Item);
        }
    }

    return OGS_OK;
}