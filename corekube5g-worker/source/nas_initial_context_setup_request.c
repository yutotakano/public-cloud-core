#include "corekube_config.h"

#include "nas_initial_context_setup_request.h"

int ngap_build_initial_context_setup_request(ngap_initial_context_setup_request_params_t * params, ogs_ngap_message_t * response) {
    ogs_info("Building NGAP Initial Context Setup Request");

    // check the required parameters are present
    ogs_assert(params);
    ogs_assert(params->amf_ue_ngap_id);
    ogs_assert(params->ran_ue_ngap_id);
    ogs_assert(params->s_nssai);
    ogs_assert(params->kgnb);

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_InitialContextSetupRequest_t *InitialContextSetupRequest = NULL;

    NGAP_InitialContextSetupRequestIEs_t *ie = NULL;
    NGAP_AMF_UE_NGAP_ID_t *AMF_UE_NGAP_ID = NULL;
    NGAP_RAN_UE_NGAP_ID_t *RAN_UE_NGAP_ID = NULL;
    NGAP_GUAMI_t *GUAMI = NULL;
    NGAP_AllowedNSSAI_t *AllowedNSSAI = NULL;
    NGAP_UESecurityCapabilities_t *UESecurityCapabilities = NULL;
    NGAP_SecurityKey_t *SecurityKey = NULL;
    NGAP_MaskedIMEISV_t *MaskedIMEISV = NULL;
    NGAP_NAS_PDU_t *NAS_PDU = NULL;

    memset(response, 0, sizeof (NGAP_NGAP_PDU_t));
    response->present = NGAP_NGAP_PDU_PR_initiatingMessage;
    response->choice.initiatingMessage = CALLOC(1, sizeof(NGAP_InitiatingMessage_t));

    initiatingMessage = response->choice.initiatingMessage;
    initiatingMessage->procedureCode =
        NGAP_ProcedureCode_id_InitialContextSetup;
    initiatingMessage->criticality = NGAP_Criticality_reject;
    initiatingMessage->value.present =
        NGAP_InitiatingMessage__value_PR_InitialContextSetupRequest;

    InitialContextSetupRequest =
        &initiatingMessage->value.choice.InitialContextSetupRequest;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_InitialContextSetupRequestIEs__value_PR_AMF_UE_NGAP_ID;

    AMF_UE_NGAP_ID = &ie->value.choice.AMF_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_InitialContextSetupRequestIEs__value_PR_RAN_UE_NGAP_ID;

    RAN_UE_NGAP_ID = &ie->value.choice.RAN_UE_NGAP_ID;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_GUAMI;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_InitialContextSetupRequestIEs__value_PR_GUAMI;

    GUAMI = &ie->value.choice.GUAMI;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_AllowedNSSAI;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_InitialContextSetupRequestIEs__value_PR_AllowedNSSAI;

    AllowedNSSAI = &ie->value.choice.AllowedNSSAI;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_InitialContextSetupRequestIEs__value_PR_UESecurityCapabilities;

    UESecurityCapabilities = &ie->value.choice.UESecurityCapabilities;

    ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
    ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_SecurityKey;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present =
        NGAP_InitialContextSetupRequestIEs__value_PR_SecurityKey;

    SecurityKey = &ie->value.choice.SecurityKey;

    asn_uint642INTEGER(AMF_UE_NGAP_ID, params->amf_ue_ngap_id);
    *RAN_UE_NGAP_ID = params->ran_ue_ngap_id;

    ogs_plmn_id_t PLMNid;
    ogs_plmn_id_build(&PLMNid, CoreKube_MCC, CoreKube_MNC, CoreKube_MNClen);
    ogs_asn_buffer_to_OCTET_STRING(&PLMNid, OGS_PLMN_ID_LEN, &GUAMI->pLMNIdentity);
    ogs_ngap_uint8_to_AMFRegionID(CoreKube_AMFSetID, &GUAMI->aMFRegionID);
    ogs_ngap_uint16_to_AMFSetID(CoreKube_AMFSetID, &GUAMI->aMFSetID);
    ogs_ngap_uint8_to_AMFPointer(CoreKube_AMFPointer, &GUAMI->aMFPointer);

    for (int i = 0; i < params->num_of_s_nssai; i++) {
        NGAP_AllowedNSSAI_Item_t *NGAP_AllowedNSSAI_Item = NULL;
        NGAP_S_NSSAI_t *s_NSSAI = NULL;
        NGAP_SST_t *sST = NULL;

        NGAP_AllowedNSSAI_Item = (NGAP_AllowedNSSAI_Item_t *)
                CALLOC(1, sizeof(NGAP_AllowedNSSAI_Item_t));
        s_NSSAI = &NGAP_AllowedNSSAI_Item->s_NSSAI;
        sST = &s_NSSAI->sST;

        ogs_asn_uint8_to_OCTET_STRING(params->s_nssai[i].sst, sST);
        if (params->s_nssai[i].sd.v != OGS_S_NSSAI_NO_SD_VALUE) {
            s_NSSAI->sD = CALLOC(1, sizeof(NGAP_SD_t));
            ogs_asn_uint24_to_OCTET_STRING(params->s_nssai[i].sd, s_NSSAI->sD);
        }

        ASN_SEQUENCE_ADD(&AllowedNSSAI->list, NGAP_AllowedNSSAI_Item);
    }

    UESecurityCapabilities->nRencryptionAlgorithms.size = 2;
    UESecurityCapabilities->nRencryptionAlgorithms.buf =
        CALLOC(UESecurityCapabilities->
                    nRencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRencryptionAlgorithms.buf[0] = (uint8_t)
        (COREKUBE_UE_SECURITY_CAPABILITY_NR_EA << 1);

    UESecurityCapabilities->nRintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf =
        CALLOC(UESecurityCapabilities->
                    nRintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->nRintegrityProtectionAlgorithms.buf[0] = (uint8_t)
        (COREKUBE_UE_SECURITY_CAPABILITY_NR_IA << 1);

    UESecurityCapabilities->eUTRAencryptionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf =
        CALLOC(UESecurityCapabilities->
                    eUTRAencryptionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAencryptionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAencryptionAlgorithms.buf[0] = (uint8_t)
        (COREKUBE_UE_SECURITY_CAPABILITY_EUTRA_EA << 1);

    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.size = 2;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf =
        CALLOC(UESecurityCapabilities->
                    eUTRAintegrityProtectionAlgorithms.size, sizeof(uint8_t));
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.bits_unused = 0;
    UESecurityCapabilities->eUTRAintegrityProtectionAlgorithms.buf[0] = (uint8_t)
        (COREKUBE_UE_SECURITY_CAPABILITY_EUTRA_IA << 1);

    SecurityKey->size = OGS_SHA256_DIGEST_SIZE;
    SecurityKey->buf = CALLOC(SecurityKey->size, sizeof(uint8_t));
    SecurityKey->bits_unused = 0;
    memcpy(SecurityKey->buf, params->kgnb, SecurityKey->size);

    /* TS23.003 6.2.2 Composition of IMEISV
     *
     * The International Mobile station Equipment Identity and
     * Software Version Number (IMEISV) is composed.
     *
     * TAC(8 digits) - SNR(6 digits) - SVN(2 digits)
     * IMEISV(16 digits) ==> 8bytes
     */
    if (params->masked_imeisv) {
        ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
        ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

        ie->id = NGAP_ProtocolIE_ID_id_MaskedIMEISV;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present =
            NGAP_InitialContextSetupRequestIEs__value_PR_MaskedIMEISV;

        MaskedIMEISV = &ie->value.choice.MaskedIMEISV;

        MaskedIMEISV->size = OGS_MAX_IMEISV_LEN;
        MaskedIMEISV->buf = CALLOC(MaskedIMEISV->size, sizeof(uint8_t));
        MaskedIMEISV->bits_unused = 0;
        memcpy(MaskedIMEISV->buf, params->masked_imeisv, MaskedIMEISV->size);
        ogs_free(params->masked_imeisv);
    }

    if (params->nasPdu) {
        ie = CALLOC(1, sizeof(NGAP_InitialContextSetupRequestIEs_t));
        ASN_SEQUENCE_ADD(&InitialContextSetupRequest->protocolIEs, ie);

        ie->id = NGAP_ProtocolIE_ID_id_NAS_PDU;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present =
            NGAP_InitialContextSetupRequestIEs__value_PR_NAS_PDU;

        NAS_PDU = &ie->value.choice.NAS_PDU;

        NAS_PDU->size = params->nasPdu->len;
        NAS_PDU->buf = CALLOC(NAS_PDU->size, sizeof(uint8_t));
        memcpy(NAS_PDU->buf, params->nasPdu->data, NAS_PDU->size);
        ogs_pkbuf_free(params->nasPdu);
    }

    return OGS_OK;
}