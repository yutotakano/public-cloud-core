#include "nas_authentication_request.h"
#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"

#include "nas_registration_request.h"

int nas_handle_registration_request(ogs_nas_5gs_registration_request_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS Registration Request");

    // check the required parameters are present
    ogs_assert(params->ran_ue_ngap_id);

    // convert the RAN_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t ran_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING(*params->ran_ue_ngap_id, &ran_ue_ngap_id_buf);

    // fetch the IMSI from the NAS message
    char imsi[OGS_MAX_IMSI_BCD_LEN];
    ogs_nas_5gs_mobile_identity_t mob_ident = message->mobile_identity;
    ogs_nas_5gs_imsi_to_bcd(&mob_ident, imsi);

    // store the RAN_UE_NGAP_ID in the DB,
    // whilst also fetching the AMF_UE_NGAP_ID, RAND, KEY and OPC
    corekube_db_pulls_t *db_pulls;
    int db = db_access(&db_pulls, IMSI, (uint8_t *) imsi, 1, 4, ENB_UE_S1AP_ID, ran_ue_ngap_id_buf.buf, MME_UE_S1AP_ID, RAND, KEY, OPC);
    ogs_assert(db == OGS_OK);

    // store the AMF_UE_NGAP_ID in the NAS params, so it can be used
    // by the calling NGAP message handler
    OCTET_STRING_t amf_ue_ngap_id_buf;
    amf_ue_ngap_id_buf.buf = db_pulls->mme_ue_s1ap_id;
    amf_ue_ngap_id_buf.size = 4;
    params->amf_ue_ngap_id = ogs_malloc(sizeof(uint32_t));
    ogs_asn_OCTET_STRING_to_uint32(&amf_ue_ngap_id_buf, params->amf_ue_ngap_id);

    // generate the authentication parameters from RAND, Key and OPC
    uint8_t amf[OGS_AMF_LEN] = CoreKube_AMF_Field;
    uint8_t sqn[OGS_SQN_LEN] = CoreKube_SQN_Value;
    uint8_t autn[OGS_AUTN_LEN];
    uint8_t ik[16];
    uint8_t ck[16];
    uint8_t ak[6];
    uint8_t res[OGS_MAX_RES_LEN];
    size_t res_len = OGS_MAX_RES_LEN;

    milenage_generate(db_pulls->opc, amf, db_pulls->key, sqn, db_pulls->rand, autn, ik, ck, ak, res, &res_len);
    ogs_assert(res_len != 0); // Failed to generate security parameters

    ogs_nas_5gs_authentication_request_t auth_request_params;
    auth_request_params.ngksi.tsc = CoreKube_NGKSI_TSC;
    auth_request_params.ngksi.value = CoreKube_NGKSI_Value;
    auth_request_params.abba.length = CoreKube_ABBA_Length;
    OGS_HEX(CoreKube_ABBA_Value, strlen(CoreKube_ABBA_Value), auth_request_params.abba.value);

    memcpy(auth_request_params.authentication_parameter_autn.autn, autn, OGS_AUTN_LEN);
    memcpy(auth_request_params.authentication_parameter_rand.rand, db_pulls->rand, OGS_RAND_LEN);

    // free the structures pulled from the DB
    ogs_free(db_pulls->head);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_authentication_request(&auth_request_params, response->responses[0]);
}