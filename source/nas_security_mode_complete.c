#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"
#include "nas_security_mode_command.h"
#include "nas_registration_accept.h"

#include "nas_security_mode_complete.h"

int nas_handle_security_mode_complete(ogs_nas_5gs_security_mode_complete_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS Security Mode Complete");

    ogs_nas_5gs_mobile_identity_t *imeisv = NULL;
    ogs_nas_mobile_identity_imeisv_t *mobile_identity_imeisv = NULL;

    char imeisv_bcd[17];
    int masked_imeisv_len;

    ogs_assert(message);

    /*
     * TS33.501
     * Ch 6.4.6. Protection of initial NAS message
     *
     * UE should send NAS Container in Security mode complete message.
     * Otherwise, Open5GS will send Registration reject message.
     *
     * Step 4: The UE shall send the NAS Security Mode Complete message
     * to the network in response to a NAS Security Mode Command message.
     * The NAS Security Mode Complete message shall be ciphered and
     * integrity protected. Furthermore the NAS Security Mode Complete message
     * shall include the complete initial NAS message in a NAS Container
     * if either requested by the AMF or the UE sent the initial NAS message
     * unprotected. The AMF shall use the complete initial NAS message
     * that is in the NAS container as the message to respond to.
     */
    if ((message->presencemask &
        OGS_NAS_5GS_SECURITY_MODE_COMPLETE_NAS_MESSAGE_CONTAINER_PRESENT)
            == 0) {
        ogs_error("No NAS Message Container in Security mode complete message");
        return OGS_ERROR;
    }

    if (message->presencemask &
        OGS_NAS_5GS_SECURITY_MODE_COMPLETE_IMEISV_PRESENT) {

        imeisv = &message->imeisv;
        ogs_assert(imeisv);
        mobile_identity_imeisv =
            (ogs_nas_mobile_identity_imeisv_t *)imeisv->buffer;
        ogs_assert(mobile_identity_imeisv);

        switch (mobile_identity_imeisv->type) {
        case OGS_NAS_5GS_MOBILE_IDENTITY_IMEISV:
            /* TS23.003 6.2.2 Composition of IMEISV
             *
             * The International Mobile station Equipment Identity and
             * Software Version Number (IMEISV) is composed.
             *
             * TAC(8 digits) - SNR(6 digits) - SVN(2 digits)
             * IMEISV(16 digits) ==> 8bytes
             */
            if (imeisv->length == sizeof(ogs_nas_mobile_identity_imeisv_t)) {
                params->masked_imeisv = ogs_calloc(8, sizeof(uint8_t));
                ogs_nas_imeisv_to_bcd(mobile_identity_imeisv, imeisv->length, imeisv_bcd);
                ogs_nas_imeisv_bcd_to_buffer(imeisv_bcd, params->masked_imeisv, &masked_imeisv_len);
                params->masked_imeisv[5] = 0xff;
                params->masked_imeisv[6] = 0xff;
            } else {
                ogs_error("Unknown IMEISV Length [%d]", imeisv->length);
                ogs_log_hexdump(OGS_LOG_ERROR, imeisv->buffer, imeisv->length);
            }
            break;
        default:
            ogs_error("Invalid IMEISV Type [%d]", mobile_identity_imeisv->type);
            break;

        }
    }

    if (message->presencemask &
        OGS_NAS_5GS_SECURITY_MODE_COMPLETE_NAS_MESSAGE_CONTAINER_PRESENT) {
        ogs_info("The Security Mode Complete messages contains a NAS Registration Request");
    }

    // format the AMF_UE_NGAP_ID as a uint8_t buffer, to be passed to the DB
    OCTET_STRING_t amf_id_buf;
    ogs_asn_uint32_to_OCTET_STRING((uint32_t) *params->amf_ue_ngap_id, &amf_id_buf);

    // fetch the TMSI (for NAS Registration Request)
    corekube_db_pulls_t db_pulls;
    int db = db_access(&db_pulls, MME_UE_S1AP_ID, (uint8_t *) amf_id_buf.buf, 0, 1, TMSI);
    ogs_assert(db == OGS_OK);

    // free the DB access buffer
    ogs_free(amf_id_buf.buf);

    // convert the TMSI to a uint32_t
    uint32_t tmsi;
    OCTET_STRING_t tmsi_buf;
    tmsi_buf.size = 4;
    tmsi_buf.buf = db_pulls.tmsi;
    ogs_asn_OCTET_STRING_to_uint32(&tmsi_buf, &tmsi);

    // Free the pulled data from the DV
    ogs_free(db_pulls.head);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_registration_accept(tmsi, response->responses[0]);
}