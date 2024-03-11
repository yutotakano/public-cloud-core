#include "ngap_ng_setup_response.h"

#include "corekube_config.h"

int ngap_build_ng_setup_response(ogs_ngap_message_t * response) {
    ogs_info("Building NG Setup Response");

    NGAP_SuccessfulOutcome_t *successfulOutcome = NULL;
    NGAP_NGSetupResponse_t *NGSetupResponse = NULL;

    NGAP_NGSetupResponseIEs_t *ie = NULL;
    NGAP_AMFName_t *AMFName = NULL;
    NGAP_ServedGUAMIList_t *ServedGUAMIList = NULL;
    NGAP_RelativeAMFCapacity_t *RelativeAMFCapacity = NULL;
    NGAP_PLMNSupportList_t *PLMNSupportList = NULL;

    ogs_debug("NGSetupResponse");

    memset(response, 0, sizeof (NGAP_NGAP_PDU_t));
    response->present = NGAP_NGAP_PDU_PR_successfulOutcome;
    response->choice.successfulOutcome =
        CALLOC(1, sizeof(NGAP_SuccessfulOutcome_t));

    successfulOutcome = response->choice.successfulOutcome;
    successfulOutcome->procedureCode = NGAP_ProcedureCode_id_NGSetup;
    successfulOutcome->criticality = NGAP_Criticality_reject;
    successfulOutcome->value.present =
        NGAP_SuccessfulOutcome__value_PR_NGSetupResponse;

    NGSetupResponse = &successfulOutcome->value.choice.NGSetupResponse;

    ie = CALLOC(1, sizeof(NGAP_NGSetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_AMFName;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_NGSetupResponseIEs__value_PR_AMFName;

    AMFName = &ie->value.choice.AMFName;

    ie = CALLOC(1, sizeof(NGAP_NGSetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_ServedGUAMIList;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_NGSetupResponseIEs__value_PR_ServedGUAMIList;

    ServedGUAMIList = &ie->value.choice.ServedGUAMIList;

    ie = CALLOC(1, sizeof(NGAP_NGSetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_RelativeAMFCapacity;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_NGSetupResponseIEs__value_PR_RelativeAMFCapacity;

    RelativeAMFCapacity = &ie->value.choice.RelativeAMFCapacity;

    ie = CALLOC(1, sizeof(NGAP_NGSetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&NGSetupResponse->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_PLMNSupportList;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_NGSetupResponseIEs__value_PR_PLMNSupportList;

    PLMNSupportList = &ie->value.choice.PLMNSupportList;

    const char amf_name[] = CoreKube_AMFName;
    ogs_asn_buffer_to_OCTET_STRING((void *) amf_name, strlen(amf_name), AMFName);

    NGAP_ServedGUAMIItem_t *ServedGUAMIItem = NULL;
    NGAP_GUAMI_t *gUAMI = NULL;
    NGAP_PLMNIdentity_t *pLMNIdentity = NULL;
    NGAP_AMFRegionID_t *aMFRegionID = NULL;
    NGAP_AMFSetID_t *aMFSetID = NULL;
    NGAP_AMFPointer_t *aMFPointer = NULL;

    ServedGUAMIItem = (NGAP_ServedGUAMIItem_t *)
            CALLOC(1, sizeof(NGAP_ServedGUAMIItem_t));
    gUAMI = &ServedGUAMIItem->gUAMI;
    pLMNIdentity = &gUAMI->pLMNIdentity;
    aMFRegionID = &gUAMI->aMFRegionID;
    aMFSetID = &gUAMI->aMFSetID;
    aMFPointer = &gUAMI->aMFPointer;

    ogs_plmn_id_t PLMNid;
    ogs_plmn_id_build(&PLMNid, CoreKube_MCC, CoreKube_MNC, CoreKube_MNClen);
    ogs_asn_buffer_to_OCTET_STRING(&PLMNid, OGS_PLMN_ID_LEN, pLMNIdentity);
    ogs_ngap_uint8_to_AMFRegionID(CoreKube_AMFRegionID, aMFRegionID);
    ogs_ngap_uint16_to_AMFSetID(CoreKube_AMFSetID, aMFSetID);
    ogs_ngap_uint8_to_AMFPointer(CoreKube_AMFPointer, aMFPointer);

    ASN_SEQUENCE_ADD(&ServedGUAMIList->list, ServedGUAMIItem);

    *RelativeAMFCapacity = CoreKube_RelativeCapacity;

    NGAP_PLMNSupportItem_t *NGAP_PLMNSupportItem = NULL;
    NGAP_SliceSupportList_t *sliceSupportList = NULL;

    NGAP_PLMNSupportItem = (NGAP_PLMNSupportItem_t *)
            CALLOC(1, sizeof(NGAP_PLMNSupportItem_t));
    pLMNIdentity = &NGAP_PLMNSupportItem->pLMNIdentity;
    sliceSupportList = &NGAP_PLMNSupportItem->sliceSupportList;

    ogs_plmn_id_build(&PLMNid, CoreKube_MCC, CoreKube_MNC, CoreKube_MNClen);
    ogs_asn_buffer_to_OCTET_STRING(&PLMNid, OGS_PLMN_ID_LEN, pLMNIdentity);

    NGAP_SliceSupportItem_t *NGAP_SliceSupportItem = NULL;
    NGAP_S_NSSAI_t *s_NSSAI = NULL;
    NGAP_SST_t *sST = NULL;

    NGAP_SliceSupportItem = (NGAP_SliceSupportItem_t *)
            CALLOC(1, sizeof(NGAP_SliceSupportItem_t));
    s_NSSAI = &NGAP_SliceSupportItem->s_NSSAI;
    sST = &s_NSSAI->sST;

    ogs_asn_uint8_to_OCTET_STRING(CoreKube_NSSAI_sST, sST);

    ASN_SEQUENCE_ADD(&sliceSupportList->list, NGAP_SliceSupportItem);

    ASN_SEQUENCE_ADD(&PLMNSupportList->list, NGAP_PLMNSupportItem);

    return OGS_OK;
}