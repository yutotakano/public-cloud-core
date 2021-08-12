#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"
#include "nas_security_mode_command.h"
#include "nas_security.h"

#include "nas_authentication_response.h"

int nas_handle_authentication_response(ogs_nas_5gs_authentication_response_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS Authentication Response");

    // check the required parameters are present
    ogs_assert(params->amf_ue_ngap_id);

    // convert the AMF_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t amf_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING( (uint32_t) *params->amf_ue_ngap_id, &amf_ue_ngap_id_buf);

    // fetch the KAMF (stored as KASME1 and KASME2) and DL count from the DB
    // (required to encode the Security Mode Command response)
    corekube_db_pulls_t *db_pulls;
    int db = db_access(&db_pulls, MME_UE_S1AP_ID, amf_ue_ngap_id_buf.buf, 0, 3, KASME_1, KASME_2, EPC_NAS_SEQUENCE_NUMBER);
    ogs_assert(db == OGS_OK);

    // store the KNAS_INT, KNAS_ENC and DL count in the NAS params
    int store_keys = nas_security_store_keys_in_params(db_pulls, params->nas_security_params);
    ogs_assert(store_keys == OGS_OK);

    // free the structures pulled from the DB
    ogs_free(db_pulls->head);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_security_mode_command(response->responses[0]);
}