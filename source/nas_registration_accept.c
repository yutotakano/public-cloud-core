#include "corekube_config.h"

#include "nas_registration_accept.h"

int nas_build_registration_accept(uint32_t m_tmsi, ogs_nas_5gs_message_t *message)
{
    ogs_info("Building NAS Registration Accept");

    ogs_assert(message);

    ogs_nas_5gs_registration_accept_t *registration_accept =
        &message->gmm.registration_accept;
    ogs_nas_5gs_registration_result_t *registration_result =
        &registration_accept->registration_result;
    ogs_nas_5gs_mobile_identity_t *mobile_identity =
        &registration_accept->guti;
    ogs_nas_5gs_mobile_identity_guti_t * mobile_identity_guti;
    mobile_identity_guti = ogs_malloc(sizeof(ogs_nas_5gs_mobile_identity_guti_t));

    ogs_nas_nssai_t *allowed_nssai = &registration_accept->allowed_nssai;
    ogs_nas_5gs_network_feature_support_t *network_feature_support =
        &registration_accept->network_feature_support;
    ogs_nas_gprs_timer_3_t *t3512_value = &registration_accept->t3512_value;

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->h.security_header_type =
        OGS_NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED;
    message->h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;

    message->gmm.h.extended_protocol_discriminator =
        OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GMM;
    message->gmm.h.message_type = OGS_NAS_5GS_REGISTRATION_ACCEPT;

    /* Registration Result */
    registration_result->length = 1;
    registration_result->value = COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS;

    /* Set GUTI */
    registration_accept->presencemask |=
        OGS_NAS_5GS_REGISTRATION_ACCEPT_5G_GUTI_PRESENT;

    ogs_nas_5gs_guti_t nas_guti;

    // set the AMF ID
    nas_guti.amf_id.pointer = CoreKube_AMFPointer;
    nas_guti.amf_id.region = CoreKube_AMFRegionID;
    nas_guti.amf_id.set1 = CoreKube_AMFSetID;

    // set the PLMN id
    ogs_plmn_id_t PLMNid;
    ogs_plmn_id_build(&PLMNid, CoreKube_MCC, CoreKube_MNC, CoreKube_MNClen);
    ogs_nas_from_plmn_id(&nas_guti.nas_plmn_id, &PLMNid);

    // set the TMSI
    nas_guti.m_tmsi = m_tmsi;

    ogs_nas_5gs_nas_guti_to_mobility_identity_guti(&nas_guti, mobile_identity_guti);
    mobile_identity->length = sizeof(ogs_nas_5gs_mobile_identity_guti_t);
    mobile_identity->buffer = mobile_identity_guti;

    /* Set TAI List */
    registration_accept->presencemask |= OGS_NAS_5GS_REGISTRATION_ACCEPT_TAI_LIST_PRESENT;
    ogs_5gs_tai0_list_t list0;
    ogs_5gs_tai2_list_t list2;
    bzero(&list0, sizeof(ogs_5gs_tai0_list_t));
    bzero(&list2, sizeof(ogs_5gs_tai2_list_t));
    list0.tai[0].num = 1;
    list0.tai[0].plmn_id = PLMNid;
    list0.tai[0].type = OGS_TAI0_TYPE;
    list0.tai[0].tac[0].v = COREKUBE_DEFAULT_TAC;

    ogs_nas_5gs_tai_list_build(&registration_accept->tai_list, &list0, &list2);

    /* Set Allowed NSSAI */
    allowed_nssai->length = 2;
    OGS_HEX(COREKUBE_DEFAULT_sNSSAI_VALUE, strlen(COREKUBE_DEFAULT_sNSSAI_VALUE), allowed_nssai->buffer);

    registration_accept->presencemask |=
        OGS_NAS_5GS_REGISTRATION_ACCEPT_ALLOWED_NSSAI_PRESENT;

    /* 5GS network feature support */
    registration_accept->presencemask |=
        OGS_NAS_5GS_REGISTRATION_ACCEPT_5GS_NETWORK_FEATURE_SUPPORT_PRESENT;
    network_feature_support->length = 2;
    network_feature_support->ims_vops_3gpp = 1;

    /* Set T3512 */
    registration_accept->presencemask |= OGS_NAS_5GS_REGISTRATION_ACCEPT_T3512_VALUE_PRESENT;
    t3512_value->length = 1;
    t3512_value->unit = OGS_NAS_GRPS_TIMER_3_UNIT_MULTIPLES_OF_1_HH;
    t3512_value->value = 9;

    return OGS_OK;
}