#include "corekube_config.h"

#include "nas_authentication_request.h"

int nas_build_security_mode_command(ogs_nas_5gs_message_t *message)
{
    ogs_info("Building NAS Security Mode Command");

    ogs_assert(message);

    ogs_nas_5gs_security_mode_command_t *security_mode_command =
        &message->gmm.security_mode_command;
    ogs_nas_security_algorithms_t *selected_nas_security_algorithms =
        &security_mode_command->selected_nas_security_algorithms;
    ogs_nas_key_set_identifier_t *ngksi = &security_mode_command->ngksi;
    ogs_nas_ue_security_capability_t *replayed_ue_security_capabilities =
        &security_mode_command->replayed_ue_security_capabilities;
    ogs_nas_imeisv_request_t *imeisv_request =
        &security_mode_command->imeisv_request;
    ogs_nas_additional_5g_security_information_t
        *additional_security_information =
            &security_mode_command->additional_security_information;

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->h.security_header_type =
        OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT;
    message->h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;

    message->gmm.h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;
    message->gmm.h.message_type = OGS_NAS_5GS_SECURITY_MODE_COMMAND;

    selected_nas_security_algorithms->type_of_integrity_protection_algorithm =
        COREKUBE_NAS_SECURITY_INT_ALGORITHM;
    selected_nas_security_algorithms->type_of_ciphering_algorithm =
        COREKUBE_NAS_SECURITY_ENC_ALGORITHM;

    ngksi->tsc = CoreKube_NGKSI_TSC;
    ngksi->value = CoreKube_NGKSI_Value;

    replayed_ue_security_capabilities->nr_ea = 0xF0; // COREKUBE_NAS_SECURITY_5GEA_SUPPORT;
    replayed_ue_security_capabilities->nr_ia = 0xF0; // COREKUBE_NAS_SECURITY_5GIA_SUPPORT;
    replayed_ue_security_capabilities->eutra_ea = 0xF0; // COREKUBE_NAS_SECURITY_EEA_SUPPORT;
    replayed_ue_security_capabilities->eutra_ia = 0xF0; // COREKUBE_NAS_SECURITY_EIA_SUPPORT;

    replayed_ue_security_capabilities->length =
        sizeof(replayed_ue_security_capabilities->nr_ea) +
        sizeof(replayed_ue_security_capabilities->nr_ia);
    if (replayed_ue_security_capabilities->eutra_ea ||
        replayed_ue_security_capabilities->eutra_ia)
        replayed_ue_security_capabilities->length =
            sizeof(replayed_ue_security_capabilities->nr_ea) +
            sizeof(replayed_ue_security_capabilities->nr_ia) +
            sizeof(replayed_ue_security_capabilities->eutra_ea) +
            sizeof(replayed_ue_security_capabilities->eutra_ia);

    security_mode_command->presencemask |=
        OGS_NAS_5GS_SECURITY_MODE_COMMAND_IMEISV_REQUEST_PRESENT;
    imeisv_request->type = OGS_NAS_IMEISV_TYPE;
    imeisv_request->value = OGS_NAS_IMEISV_REQUESTED;

    security_mode_command->presencemask |= OGS_NAS_5GS_SECURITY_MODE_COMMAND_ADDITIONAL_5G_SECURITY_INFORMATION_PRESENT;
    additional_security_information->length = 1;
    additional_security_information->
        retransmission_of_initial_nas_message_request = 1;

    if (COREKUBE_NAS_SECURITY_INT_ALGORITHM == OGS_NAS_SECURITY_ALGORITHMS_EIA0) {
        ogs_error("Encrypt[0x%x] can be skipped with NEA0, "
            "but Integrity[0x%x] cannot be bypassed with NIA0",
            COREKUBE_NAS_SECURITY_ENC_ALGORITHM, COREKUBE_NAS_SECURITY_INT_ALGORITHM);
        return OGS_ERROR;
    }

    return OGS_OK;
}