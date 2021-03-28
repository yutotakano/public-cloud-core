#include "ue_initial_context_release_command.h"

// the following function is heavily adapted from 
// s1ap_build_ue_context_release_command() in
// the file nextepc/src/mme/s1ap_build.c
status_t s1ap_build_ue_context_release_command(S1AP_MME_UE_S1AP_ID_t mme_ue_id, S1AP_ENB_UE_S1AP_ID_t enb_ue_id, s1ap_message_t *pdu) {
    d_info("Building S1AP UEInitialContextReleaseCommand");

    S1AP_InitiatingMessage_t *initiatingMessage = NULL;
    S1AP_UEContextReleaseCommand_t *UEContextReleaseCommand = NULL;

    S1AP_UEContextReleaseCommand_IEs_t *ie = NULL;
    S1AP_UE_S1AP_IDs_t *UE_S1AP_IDs = NULL;
    S1AP_Cause_t *Cause = NULL;

    memset(pdu, 0, sizeof (S1AP_S1AP_PDU_t));
    pdu->present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu->choice.initiatingMessage = core_calloc(1, sizeof(S1AP_InitiatingMessage_t));

    initiatingMessage = pdu->choice.initiatingMessage;
    initiatingMessage->procedureCode = S1AP_ProcedureCode_id_UEContextRelease;
    initiatingMessage->criticality = S1AP_Criticality_reject;
    initiatingMessage->value.present = S1AP_InitiatingMessage__value_PR_UEContextReleaseCommand;

    UEContextReleaseCommand = &initiatingMessage->value.choice.UEContextReleaseCommand;

    ie = core_calloc(1, sizeof(S1AP_UEContextReleaseCommand_IEs_t));
    ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);

    ie->id = S1AP_ProtocolIE_ID_id_UE_S1AP_IDs;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UEContextReleaseCommand_IEs__value_PR_UE_S1AP_IDs;

    UE_S1AP_IDs = &ie->value.choice.UE_S1AP_IDs;

    ie = core_calloc(1, sizeof(S1AP_UEContextReleaseCommand_IEs_t));
    ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);

    ie->id = S1AP_ProtocolIE_ID_id_Cause;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_UEContextReleaseCommand_IEs__value_PR_Cause;

    Cause = &ie->value.choice.Cause;

    UE_S1AP_IDs->present = S1AP_UE_S1AP_IDs_PR_uE_S1AP_ID_pair;
    UE_S1AP_IDs->choice.uE_S1AP_ID_pair = core_calloc(1, sizeof(S1AP_UE_S1AP_ID_pair_t));
    UE_S1AP_IDs->choice.uE_S1AP_ID_pair->mME_UE_S1AP_ID = mme_ue_id;
    UE_S1AP_IDs->choice.uE_S1AP_ID_pair->eNB_UE_S1AP_ID = enb_ue_id;

    Cause->present = S1AP_Cause_PR_nas;
    Cause->choice.nas = S1AP_CauseNas_detach;

    return CORE_OK;
}