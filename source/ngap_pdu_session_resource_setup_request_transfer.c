#include "corekube_config.h"

#include "ngap_pdu_session_resource_setup_request_transfer.h"

ogs_pkbuf_t * nas_build_ngap_pdu_session_resource_setup_request_transfer()
{
    ogs_info("Building NGAP PDU Session Resource Setup Request Transfer message");

    ogs_ip_t upf_n3_ip;

    NGAP_PDUSessionResourceSetupRequestTransfer_t message;

    NGAP_PDUSessionResourceSetupRequestTransferIEs_t *ie = NULL;
    NGAP_PDUSessionAggregateMaximumBitRate_t *PDUSessionAggregateMaximumBitRate;
    NGAP_UPTransportLayerInformation_t *UPTransportLayerInformation = NULL;
    NGAP_GTPTunnel_t *gTPTunnel = NULL;
    NGAP_PDUSessionType_t *PDUSessionType = NULL;
    NGAP_QosFlowSetupRequestList_t *QosFlowSetupRequestList = NULL;
    NGAP_QosFlowSetupRequestItem_t *QosFlowSetupRequestItem = NULL;
    NGAP_QosFlowIdentifier_t *qosFlowIdentifier = NULL;
    NGAP_QosFlowLevelQosParameters_t *qosFlowLevelQosParameters = NULL;
    NGAP_QosCharacteristics_t *qosCharacteristics = NULL;
    NGAP_NonDynamic5QIDescriptor_t *nonDynamic5QI = NULL;
    NGAP_AllocationAndRetentionPriority_t *allocationAndRetentionPriority;

    memset(&message, 0, sizeof(NGAP_PDUSessionResourceSetupRequestTransfer_t));

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestTransferIEs_t));
    ogs_assert(ie);
    ASN_SEQUENCE_ADD(&message.protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestTransferIEs__value_PR_PDUSessionAggregateMaximumBitRate;

    PDUSessionAggregateMaximumBitRate =
        &ie->value.choice.PDUSessionAggregateMaximumBitRate;

    asn_uint642INTEGER(&PDUSessionAggregateMaximumBitRate->
        pDUSessionAggregateMaximumBitRateUL, COREKUBE_AMBR_UPLINK_BPS);
    asn_uint642INTEGER(&PDUSessionAggregateMaximumBitRate->
        pDUSessionAggregateMaximumBitRateDL, COREKUBE_AMBR_DOWNLINK_BPS);

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestTransferIEs_t));
    ogs_assert(ie);
    ASN_SEQUENCE_ADD(&message.protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestTransferIEs__value_PR_UPTransportLayerInformation;

    UPTransportLayerInformation = &ie->value.choice.UPTransportLayerInformation;

    gTPTunnel = CALLOC(1, sizeof(struct NGAP_GTPTunnel));
    ogs_assert(gTPTunnel);
    UPTransportLayerInformation->present =
        NGAP_UPTransportLayerInformation_PR_gTPTunnel;
    UPTransportLayerInformation->choice.gTPTunnel = gTPTunnel;

    upf_n3_ip.ipv4 = 1;
    OGS_HEX(COREKUBE_DEFAULT_TRANSPORT_LAYER_ADDRESS, strlen(COREKUBE_DEFAULT_TRANSPORT_LAYER_ADDRESS), &upf_n3_ip.addr);
    ogs_asn_ip_to_BIT_STRING(&upf_n3_ip, &gTPTunnel->transportLayerAddress);
    ogs_asn_uint32_to_OCTET_STRING(COREKUBE_DEFAULT_TEID, &gTPTunnel->gTP_TEID);

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestTransferIEs_t));
    ogs_assert(ie);
    ASN_SEQUENCE_ADD(&message.protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionType;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestTransferIEs__value_PR_PDUSessionType;

    PDUSessionType = &ie->value.choice.PDUSessionType;

    *PDUSessionType = OGS_PDU_SESSION_TYPE_IPV4;
    switch (COREKUBE_DEFAULT_SESSION_TYPE) {
    case OGS_PDU_SESSION_TYPE_IPV4 :
        *PDUSessionType = NGAP_PDUSessionType_ipv4;
        break;
    case OGS_PDU_SESSION_TYPE_IPV6 :
        *PDUSessionType = NGAP_PDUSessionType_ipv6;
        break;
    case OGS_PDU_SESSION_TYPE_IPV4V6 :
        *PDUSessionType = NGAP_PDUSessionType_ipv4v6;
        break;
    default:
        ogs_fatal("Unknown PDU Session Type [%d]", COREKUBE_DEFAULT_SESSION_TYPE);
        ogs_assert_if_reached();
    }

    ie = CALLOC(1, sizeof(NGAP_PDUSessionResourceSetupRequestTransferIEs_t));
    ogs_assert(ie);
    ASN_SEQUENCE_ADD(&message.protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_QosFlowSetupRequestList;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceSetupRequestTransferIEs__value_PR_QosFlowSetupRequestList;

    QosFlowSetupRequestList = &ie->value.choice.QosFlowSetupRequestList;

    QosFlowSetupRequestItem =
        CALLOC(1, sizeof(struct NGAP_QosFlowSetupRequestItem));
    ogs_assert(QosFlowSetupRequestItem);
    ASN_SEQUENCE_ADD(&QosFlowSetupRequestList->list,
        QosFlowSetupRequestItem);

    qosFlowIdentifier = &QosFlowSetupRequestItem->qosFlowIdentifier;
    qosFlowLevelQosParameters =
        &QosFlowSetupRequestItem->qosFlowLevelQosParameters;

    allocationAndRetentionPriority =
        &qosFlowLevelQosParameters->allocationAndRetentionPriority;
    qosCharacteristics = &qosFlowLevelQosParameters->qosCharacteristics;
    nonDynamic5QI = CALLOC(1, sizeof(struct NGAP_NonDynamic5QIDescriptor));
    ogs_assert(nonDynamic5QI);
    qosCharacteristics->choice.nonDynamic5QI = nonDynamic5QI;
    qosCharacteristics->present = NGAP_QosCharacteristics_PR_nonDynamic5QI;

    *qosFlowIdentifier = COREKUBE_QOS_FLOW_IDENTIFIER;

    nonDynamic5QI->fiveQI = COREKUBE_QOS_5QI;

    allocationAndRetentionPriority->priorityLevelARP = COREKUBE_QOS_ARP_PRIORITY;

    return ogs_asn_encode(
            &asn_DEF_NGAP_PDUSessionResourceSetupRequestTransfer, &message);
}