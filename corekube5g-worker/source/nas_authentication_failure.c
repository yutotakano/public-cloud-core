#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"
#include "nas_security.h"
#include "nas_authentication_request.h"

#include "nas_authentication_failure.h"

int nas_handle_authentication_failure(ogs_nas_5gs_authentication_failure_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS Authentication Failure");

    // ensure the failure is due to a sync failure
    ogs_assert(message->gmm_cause == OGS_5GMM_CAUSE_SYNCH_FAILURE);

    // fetch the auts parameter from the message
    // auts = SQN_xor_AK (48-bit), AMF (16-bit), MAC (64-bit)
    uint8_t * auts = message->authentication_failure_parameter.auts;

    // convert the AMF_UE_NGAP_ID to a buffer, suitable for the DB
    OCTET_STRING_t amf_ue_ngap_id_buf;
    ogs_asn_uint32_to_OCTET_STRING( (uint32_t) *(params->amf_ue_ngap_id), &amf_ue_ngap_id_buf);

    // fetch the RAND, KEY, OPC, and MOB_IDENT (stored in KNH_1 - TODO!)
    unsigned long long start_time = get_microtime();
    corekube_db_pulls_t db_pulls;
    int db = db_access(&db_pulls, MME_UE_S1AP_ID, (uint8_t *) amf_ue_ngap_id_buf.buf, 0, 4, RAND, KEY, OPC, KNH_1);
    ogs_assert(db == OGS_OK);
    unsigned long long end_time = get_microtime();
    yagra_observe_metric(response->batch, "db_access_latency", (int)(end_time - start_time));

    // generate the new SQN from RAND, Key and OPC
    uint8_t new_sqn[OGS_SQN_LEN];
    uint8_t mac_s[OGS_MAC_S_LEN];
    ogs_auc_sqn(db_pulls.opc, db_pulls.key, db_pulls.rand, auts, new_sqn, mac_s);

    // increment and save the incremented SQN
    uint64_t sqn = ogs_buffer_to_uint64(new_sqn, OGS_SQN_LEN);
    sqn = (sqn + 32 + 1) & OGS_MAX_SQN;
    ogs_uint64_to_buffer(sqn, OGS_SQN_LEN, new_sqn);

    ogs_info("Calculated new SQN:");
    if (ogs_log_get_domain_level(__corekube_log_domain) >= OGS_LOG_INFO)
      ogs_log_hexdump(OGS_LOG_INFO, new_sqn, 6);

    // Retreive the mobile identity from the DB
    // TODO: fix when we have better DB
    ogs_nas_5gs_mobile_identity_t mob_ident;
    memcpy(&mob_ident.length, db_pulls.knh1, 2);
    uint8_t mob_ident_buffer[mob_ident.length];
    memcpy(mob_ident_buffer, db_pulls.knh1+2, mob_ident.length);
    mob_ident.buffer = mob_ident_buffer;

    // generate the authentication and security parameters
    uint8_t autn[OGS_AUTN_LEN];
    uint8_t kamf[OGS_SHA256_DIGEST_SIZE];
    int key_gen = nas_5gs_generate_keys(&mob_ident, db_pulls.opc, db_pulls.key, db_pulls.rand, new_sqn, autn, kamf);
    ogs_assert(key_gen == OGS_OK);

    // store the KAMF in the DB, and set the NAS UL / DL counts to zero
    uint8_t zero_nas_count[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    start_time = get_microtime();
    // TODO: pulling the SPGW_IP is for debug purposes only
    corekube_db_pulls_t db_pulls2;
    int storeKamf = db_access(&db_pulls2, MME_UE_S1AP_ID, (uint8_t *) amf_ue_ngap_id_buf.buf, 4, 1, KASME_1, kamf, KASME_2, kamf+16, EPC_NAS_SEQUENCE_NUMBER, zero_nas_count, UE_NAS_SEQUENCE_NUMBER, zero_nas_count, SPGW_IP);
    ogs_assert(storeKamf == OGS_OK);
    end_time = get_microtime();
    yagra_observe_metric(response->batch, "db_access_latency", (int)(end_time - start_time));

    ogs_nas_5gs_authentication_request_t auth_request_params;
    auth_request_params.ngksi.tsc = CoreKube_NGKSI_TSC;
    auth_request_params.ngksi.value = CoreKube_NGKSI_Value;
    auth_request_params.abba.length = CoreKube_ABBA_Length;
    OGS_HEX(CoreKube_ABBA_Value, strlen(CoreKube_ABBA_Value), auth_request_params.abba.value);

    memcpy(auth_request_params.authentication_parameter_autn.autn, autn, OGS_AUTN_LEN);
    memcpy(auth_request_params.authentication_parameter_rand.rand, db_pulls.rand, OGS_RAND_LEN);

    // free the structures pulled from the DB
    ogs_free(db_pulls.head);
    ogs_free(db_pulls2.head);

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_authentication_request(&auth_request_params, response->responses[0]);
}
