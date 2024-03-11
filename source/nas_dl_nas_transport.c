#include "corekube_config.h"

#include "nas_dl_nas_transport.h"

int nas_build_dl_nas_transport(nas_dl_nas_transport_params_t * params, ogs_nas_5gs_message_t *message)
{
    ogs_info("Building NAS DL NAS Transport message");

    ogs_assert(message);

    ogs_nas_5gs_dl_nas_transport_t *dl_nas_transport =
        &message->gmm.dl_nas_transport;

    ogs_nas_pdu_session_identity_2_t *pdu_session_id = NULL;
    ogs_nas_5gmm_cause_t *gmm_cause = NULL;
    ogs_nas_gprs_timer_3_t *back_off_timer_value = NULL;

    ogs_assert(params);

    pdu_session_id = &dl_nas_transport->pdu_session_id;
    ogs_assert(pdu_session_id);
    gmm_cause = &dl_nas_transport->gmm_cause;
    ogs_assert(gmm_cause);
    back_off_timer_value = &dl_nas_transport->back_off_timer_value;
    ogs_assert(back_off_timer_value);

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->h.security_header_type =
        OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED;
    message->h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;

    message->gmm.h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;
    message->gmm.h.message_type = OGS_NAS_5GS_DL_NAS_TRANSPORT;

    dl_nas_transport->payload_container_type.value = params->payload_container_type;
    dl_nas_transport->payload_container.length = params->payload_container.length;
    dl_nas_transport->payload_container.buffer = params->payload_container.buffer;

    dl_nas_transport->presencemask |=
        OGS_NAS_5GS_DL_NAS_TRANSPORT_PDU_SESSION_ID_PRESENT;
    *pdu_session_id = COREKUBE_PDN_SESSION_IDENTITY;

    if (params->gmm_cause) {
        dl_nas_transport->presencemask |=
            OGS_NAS_5GS_DL_NAS_TRANSPORT_5GMM_CAUSE_PRESENT;
        *gmm_cause = *params->gmm_cause;
    }

    if (params->back_off_timer_value && *params->back_off_timer_value >= 2) {
        dl_nas_transport->presencemask |=
            OGS_NAS_5GS_DL_NAS_TRANSPORT_BACK_OFF_TIMER_VALUE_PRESENT;
        back_off_timer_value->length = 1;
        back_off_timer_value->unit =
            OGS_NAS_GRPS_TIMER_3_UNIT_MULTIPLES_OF_2_SS;
        back_off_timer_value->value = *params->back_off_timer_value / 2;
    }

    return OGS_OK;
}