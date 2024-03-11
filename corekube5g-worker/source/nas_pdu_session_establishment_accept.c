#include "corekube_config.h"

#include "nas_pdu_session_establishment_accept.h"

int nas_build_pdu_session_establishment_accept(uint32_t pdu_ip, ogs_nas_5gs_message_t *message)
{
    ogs_info("Building NAS GSM PDU Session Establishment Accept message");

    ogs_assert(message);

    int num_of_param, rv;

    ogs_nas_5gs_pdu_session_establishment_accept_t *
        pdu_session_establishment_accept =
            &message->gsm.pdu_session_establishment_accept;

    ogs_nas_pdu_session_type_t *selected_pdu_session_type = NULL;
    ogs_nas_qos_rules_t *authorized_qos_rules = NULL;
    ogs_nas_session_ambr_t *session_ambr = NULL;
    ogs_nas_5gsm_cause_t *gsm_cause = NULL;
    ogs_nas_pdu_address_t *pdu_address = NULL;
    ogs_nas_s_nssai_t *nas_s_nssai = NULL;
    ogs_nas_qos_flow_descriptions_t *authorized_qos_flow_descriptions = NULL;
    ogs_nas_extended_protocol_configuration_options_t
        *extended_protocol_configuration_options = NULL;
    ogs_nas_dnn_t *dnn = NULL;

    ogs_nas_qos_rule_t qos_rule[OGS_NAS_MAX_NUM_OF_QOS_RULE];
    ogs_nas_qos_flow_description_t
        qos_flow_description[OGS_NAS_MAX_NUM_OF_QOS_FLOW_DESCRIPTION];

    uint8_t * pco_buf;
    pco_buf = ogs_malloc(OGS_MAX_PCO_LEN * sizeof(uint8_t)); // TODO: not freed
    int16_t pco_len;

    selected_pdu_session_type = &pdu_session_establishment_accept->
        selected_pdu_session_type;
    ogs_assert(selected_pdu_session_type);
    authorized_qos_rules = &pdu_session_establishment_accept->
        authorized_qos_rules;
    ogs_assert(authorized_qos_rules);
    session_ambr = &pdu_session_establishment_accept->session_ambr;
    ogs_assert(session_ambr);
    pdu_address = &pdu_session_establishment_accept->pdu_address;
    ogs_assert(pdu_address);
    gsm_cause = &pdu_session_establishment_accept->gsm_cause;
    ogs_assert(gsm_cause);
    nas_s_nssai = &pdu_session_establishment_accept->s_nssai;
    ogs_assert(nas_s_nssai);
    authorized_qos_flow_descriptions =
        &pdu_session_establishment_accept->authorized_qos_flow_descriptions;
    ogs_assert(authorized_qos_flow_descriptions);
    extended_protocol_configuration_options =
        &pdu_session_establishment_accept->
            extended_protocol_configuration_options;
    ogs_assert(extended_protocol_configuration_options);
    dnn = &pdu_session_establishment_accept->dnn;
    ogs_assert(dnn);

    memset(message, 0, sizeof(ogs_nas_5gs_message_t));
    message->gsm.h.extended_protocol_discriminator =
            OGS_NAS_EXTENDED_PROTOCOL_DISCRIMINATOR_5GSM;
    message->gsm.h.pdu_session_identity = COREKUBE_PDN_SESSION_IDENTITY;
    message->gsm.h.procedure_transaction_identity = COREKUBE_PDN_PROCEDURE_TRANSACTION_IDENTITY;
    message->gsm.h.message_type = OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT;

    selected_pdu_session_type->type = OGS_NAS_SSC_MODE_1;
    selected_pdu_session_type->value = OGS_NAS_SSC_MODE_1;

    /*
     * TS23.501
     * 5.7.1.3 QoS Rules
     *
     * A default QoS rule is required to be sent to the UE for every PDU
     * Session establishment and it is associated with a QoS Flow. For IP type
     * PDU Session or Ethernet type PDU Session, the default QoS rule is
     * the only QoS rule of a PDU Session which may contain a Packet Filter
     * Set that allows all UL packets, and in this case, the highest
     * precedence value shall be used for the QoS rule.
     *
     * As long as the default QoS rule does not contain a Packet Filter Set or
     * contains a Packet Filter Set that allows all UL packets, Reflective QoS
     * should not be applied for the QoS Flow which the default QoS rule is
     * associated with and the RQA should not be sent for this QoS Flow.
     */
    memset(qos_rule, 0, sizeof(qos_rule));
    qos_rule[0].identifier = COREKUBE_QOS_RULE_IDENTIFIER;
    qos_rule[0].code = OGS_NAS_QOS_CODE_CREATE_NEW_QOS_RULE;
    qos_rule[0].DQR_bit = 1;
    qos_rule[0].num_of_packet_filter = 1;

    qos_rule[0].pf[0].direction = OGS_NAS_QOS_DIRECTION_BIDIRECTIONAL;
    qos_rule[0].pf[0].identifier = 1;
    qos_rule[0].pf[0].content.length = 1;
    qos_rule[0].pf[0].content.num_of_component = 1;
    qos_rule[0].pf[0].content.component[0].type = OGS_PACKET_FILTER_MATCH_ALL;

    /*
     * TS23.501
     * 5.7.1.9 Precedence Value
     *
     * The QoS rule precedence value and the PDR precedence value determine
     * the order in which a QoS rule or a PDR, respectively, shall be evaluated.
     * The evaluation of the QoS rules or PDRs is performed in increasing order
     * of their precedence value.
     */
    qos_rule[0].precedence = 255; /* lowest precedence */
    qos_rule[0].flow.segregation = 0;
    qos_rule[0].flow.identifier = COREKUBE_QOS_FLOW_IDENTIFIER;

    rv = ogs_nas_build_qos_rules(authorized_qos_rules, qos_rule, 1);
    ogs_assert(rv == OGS_OK);
    ogs_assert(authorized_qos_rules->length);

    /* Session-AMBR */
    session_ambr->length = 6;
    session_ambr->downlink.unit = OGS_NAS_BR_UNIT_1G;
    session_ambr->downlink.value = COREKUBE_AMBR_DOWNLINK_GBPS;
    session_ambr->uplink.unit = OGS_NAS_BR_UNIT_1G;
    session_ambr->uplink.value = COREKUBE_AMBR_UPLINK_GBPS;

    /* PDU Address */
    pdu_session_establishment_accept->presencemask |=
        OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT_PDU_ADDRESS_PRESENT;
    pdu_address->pdn_type = OGS_NAS_EPS_PDN_TYPE_IPV4;

    if (pdu_address->pdn_type == OGS_PDU_SESSION_TYPE_IPV4) {
        pdu_address->addr = pdu_ip;
        pdu_address->length = OGS_NAS_PDU_ADDRESS_IPV4_LEN;
    } else {
        ogs_error("Unexpected PDN Type %u", pdu_address->pdn_type);
        return OGS_ERROR;
    }

    /* S-NSSAI */
    pdu_session_establishment_accept->presencemask |=
        OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT_S_NSSAI_PRESENT;
    ogs_s_nssai_t default_nssai;
    ogs_s_nssai_t mapped_plmn;
    default_nssai.sst = CoreKube_NSSAI_sST;
    default_nssai.sd.v = OGS_S_NSSAI_NO_SD_VALUE;
    ogs_nas_build_s_nssai2(nas_s_nssai, &default_nssai, &mapped_plmn);

    /* QoS flow descriptions */
    memset(&qos_flow_description, 0, sizeof(qos_flow_description));
    qos_flow_description[0].identifier = COREKUBE_QOS_FLOW_IDENTIFIER;
    qos_flow_description[0].code = OGS_NAS_CREATE_NEW_QOS_FLOW_DESCRIPTION;
    qos_flow_description[0].E_bit = 1;

    num_of_param = 0;

    qos_flow_description[0].param[num_of_param].identifier =
        OGS_NAX_QOS_FLOW_PARAMETER_ID_5QI;
    qos_flow_description[0].param[num_of_param].len =
        sizeof(qos_flow_description[0].param[num_of_param].qos_index);
    qos_flow_description[0].param[num_of_param].qos_index = COREKUBE_QOS_5QI;
    num_of_param++;

    qos_flow_description[0].num_of_parameter = num_of_param;

    pdu_session_establishment_accept->presencemask |=
        OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT_AUTHORIZED_QOS_FLOW_DESCRIPTIONS_PRESENT;
    rv = ogs_nas_build_qos_flow_descriptions(
            authorized_qos_flow_descriptions, qos_flow_description, 1);
    ogs_assert(rv == OGS_OK);
    ogs_assert(authorized_qos_flow_descriptions->length);

    /* Extended protocol configuration options */
    ogs_pco_t pco;
    pco.ext = 1;
    pco.configuration_protocol = 0;
    pco.num_of_id = 2;
    // Primary DNS Server
    pco.ids[0].id = OGS_PCO_ID_DNS_SERVER_IPV4_ADDRESS_REQUEST;
    pco.ids[0].len = OGS_IPV4_LEN;
    pco.ids[0].data = ogs_malloc(OGS_IPV4_LEN * sizeof(uint8_t));
    OGS_HEX(COREKUBE_DEFAULT_DNS_PRIMARY, 2 * OGS_IPV4_LEN, pco.ids[0].data);
    // Primary DNS Server
    pco.ids[1].id = OGS_PCO_ID_DNS_SERVER_IPV4_ADDRESS_REQUEST;
    pco.ids[1].len = OGS_IPV4_LEN;
    pco.ids[1].data = ogs_malloc(OGS_IPV4_LEN * sizeof(uint8_t));
    OGS_HEX(COREKUBE_DEFAULT_DNS_SECONDARY, 2 * OGS_IPV4_LEN, pco.ids[1].data);
    // Convert to a buffer
    pco_len = ogs_pco_build(pco_buf, OGS_MAX_PCO_LEN, &pco);
    ogs_assert(pco_len > 0);
    pdu_session_establishment_accept->presencemask |=
        OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT_EXTENDED_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    extended_protocol_configuration_options->buffer = pco_buf;
    extended_protocol_configuration_options->length = pco_len;
    // Free created (now uneeded) structures
    ogs_free(pco.ids[0].data);
    ogs_free(pco.ids[1].data);

    /* DNN */
    pdu_session_establishment_accept->presencemask |=
        OGS_NAS_5GS_PDU_SESSION_ESTABLISHMENT_ACCEPT_DNN_PRESENT;
    dnn->length = strlen(COREKUBE_DNN_SESSION_NAME);
    ogs_cpystrn(dnn->value, COREKUBE_DNN_SESSION_NAME, ogs_min(dnn->length, OGS_MAX_DNN_LEN) + 1);

    // TODO: these are meant to be freed AFTER the conversion to raw NAS bytes
    // not sure how to handle this...
    ogs_free(authorized_qos_rules->buffer);
    ogs_free(authorized_qos_flow_descriptions->buffer);

    return OGS_OK;
}