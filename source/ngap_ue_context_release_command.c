#include "ngap_ue_context_release_command.h"

#include "corekube_config.h"

int ngap_build_ue_context_release_command(ngap_ue_context_release_command_params_t * params, ogs_ngap_message_t * response) {
    ogs_info("Building NGAP UE Context Release Command");

    NGAP_InitiatingMessage_t *initiatingMessage = NULL;
    NGAP_UEContextReleaseCommand_t *UEContextReleaseCommand = NULL;

    NGAP_UEContextReleaseCommand_IEs_t *ie = NULL;
    NGAP_UE_NGAP_IDs_t *UE_NGAP_IDs = NULL;
    NGAP_Cause_t *Cause = NULL;

    memset(response, 0, sizeof (NGAP_NGAP_PDU_t));
    response->present = NGAP_NGAP_PDU_PR_initiatingMessage;
    response->choice.initiatingMessage = CALLOC(1, sizeof(NGAP_InitiatingMessage_t));

    initiatingMessage = response->choice.initiatingMessage;
    initiatingMessage->procedureCode = NGAP_ProcedureCode_id_UEContextRelease;
    initiatingMessage->criticality = NGAP_Criticality_reject;
    initiatingMessage->value.present =
        NGAP_InitiatingMessage__value_PR_UEContextReleaseCommand;

    UEContextReleaseCommand =
        &initiatingMessage->value.choice.UEContextReleaseCommand;

    ie = CALLOC(1, sizeof(NGAP_UEContextReleaseCommand_IEs_t));
    ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_UE_NGAP_IDs;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UEContextReleaseCommand_IEs__value_PR_UE_NGAP_IDs;

    UE_NGAP_IDs = &ie->value.choice.UE_NGAP_IDs;

    ie = CALLOC(1, sizeof(NGAP_UEContextReleaseCommand_IEs_t));
    ASN_SEQUENCE_ADD(&UEContextReleaseCommand->protocolIEs, ie);

    ie->id = NGAP_ProtocolIE_ID_id_Cause;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_UEContextReleaseCommand_IEs__value_PR_Cause;

    Cause = &ie->value.choice.Cause;

    if (! params->ran_ue_ngap_id) {
        UE_NGAP_IDs->present = NGAP_UE_NGAP_IDs_PR_aMF_UE_NGAP_ID;
        asn_uint642INTEGER(
                &UE_NGAP_IDs->choice.aMF_UE_NGAP_ID, params->amf_ue_ngap_id);
    } else {
        UE_NGAP_IDs->present = NGAP_UE_NGAP_IDs_PR_uE_NGAP_ID_pair;
        UE_NGAP_IDs->choice.uE_NGAP_ID_pair = 
            CALLOC(1, sizeof(NGAP_UE_NGAP_ID_pair_t));
        asn_uint642INTEGER(
            &UE_NGAP_IDs->choice.uE_NGAP_ID_pair->aMF_UE_NGAP_ID,
            params->amf_ue_ngap_id);
        UE_NGAP_IDs->choice.uE_NGAP_ID_pair->rAN_UE_NGAP_ID =
            *params->ran_ue_ngap_id;
    }

    Cause->present = params->group;
    Cause->choice.radioNetwork = params->cause;

    return OGS_OK;
}