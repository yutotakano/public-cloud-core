#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <arpa/inet.h>
#include "nas_activate_default_bearer_context_request.h"
#include "core/include/core_lib.h"
#include "nas_message_security.h"

// the following function is heavily adapted from 
// esm_build_activate_default_bearer_context_request()
// in the file nextepc/src/mme/esm_build.c
status_t esm_build_activate_default_bearer_context_request(pkbuf_t **pkbuf)
{
    d_info("Building ESM Activate Default Bearer Context Request");

    nas_message_t message;
    nas_activate_default_eps_bearer_context_request_t 
        *activate_default_eps_bearer_context_request = 
            &message.esm.activate_default_eps_bearer_context_request;
    nas_eps_quality_of_service_t *eps_qos =
        &activate_default_eps_bearer_context_request->eps_qos;
    nas_access_point_name_t *access_point_name =
        &activate_default_eps_bearer_context_request->access_point_name;
    nas_pdn_address_t *pdn_address = 
        &activate_default_eps_bearer_context_request->pdn_address;
    nas_apn_aggregate_maximum_bit_rate_t *apn_ambr =
        &activate_default_eps_bearer_context_request->apn_ambr;
    nas_protocol_configuration_options_t *protocol_configuration_options =
        &activate_default_eps_bearer_context_request->protocol_configuration_options;

    pdn_t *pdn = NULL;

    status_t get_pdn =  get_default_pdn(&pdn);
    d_assert(get_pdn == CORE_OK, return CORE_ERROR, "Failed to get default PDN");

    memset(&message, 0, sizeof(message));
    message.esm.h.eps_bearer_identity = 5; // TODO: fixed value may need to be derived
    message.esm.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_ESM;
    message.esm.h.procedure_transaction_identity = 1; // TODO: fixed value may need to be derived
    message.esm.h.message_type = 
        NAS_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST;

    eps_qos_build(eps_qos, pdn->qos.qci,
            pdn->qos.mbr.downlink, pdn->qos.mbr.uplink,
            pdn->qos.gbr.downlink, pdn->qos.gbr.uplink);

    access_point_name->length = strlen(pdn->apn);
    core_cpystrn(access_point_name->apn, pdn->apn,
            c_min(access_point_name->length, MAX_APN_LEN) + 1);

    pdn_address->pdn_type = pdn->paa.pdn_type;
    if (pdn_address->pdn_type == GTP_PDN_TYPE_IPV4)
    {
        pdn_address->addr = pdn->paa.addr;
        pdn_address->length = NAS_PDN_ADDRESS_IPV4_LEN;
    }
    else if (pdn_address->pdn_type == GTP_PDN_TYPE_IPV6)
    {
        memcpy(pdn_address->addr6, pdn->paa.addr6+(IPV6_LEN>>1), IPV6_LEN>>1);
        pdn_address->length = NAS_PDN_ADDRESS_IPV6_LEN;
    }
    else if (pdn_address->pdn_type == GTP_PDN_TYPE_IPV4V6)
    {
        pdn_address->both.addr = pdn->paa.both.addr;
        memcpy(pdn_address->both.addr6,
                pdn->paa.both.addr6+(IPV6_LEN>>1), IPV6_LEN>>1);
        pdn_address->length = NAS_PDN_ADDRESS_IPV4V6_LEN;
    }
    else
        d_assert(0, return CORE_ERROR,
                "Invalid PDN_TYPE(%d)", pdn->paa.pdn_type);

    activate_default_eps_bearer_context_request->presencemask |=
        NAS_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_APN_AMBR_PRESENT;
    apn_ambr->length = 4;
    apn_ambr->dl_apn_ambr = 0xfe; // 8640 Kbps
    apn_ambr->dl_apn_ambr_extended = 0xde; // 200 Mbps
    apn_ambr->ul_apn_ambr = 0xfe; // 8640 Kbps
    apn_ambr->ul_apn_ambr_extended = 0x9e; // 100 Mbps

    activate_default_eps_bearer_context_request->presencemask |=
            NAS_ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_PROTOCOL_CONFIGURATION_OPTIONS_PRESENT;
    status_t get_pco = get_default_pco(protocol_configuration_options);
    d_assert(get_pco == CORE_OK, return CORE_ERROR, "Failed to get PCO");

    status_t plain_encode = nas_plain_encode(pkbuf, &message);
    d_assert(plain_encode == CORE_OK, return CORE_ERROR,
        "Failed to plain encode NAS activate default bearer context request message");

    return CORE_OK;
}


status_t get_default_pdn(pdn_t **pdn_out) {
    d_info("Getting Default PDN");

    pdn_t *pdn;

    qos_t *qos;
    paa_t *paa;
    ip_t *pgw_ip;

    pdn = core_calloc(1, sizeof(pdn_t));
    memset(pdn, 0, sizeof(pdn_t));

    // set the AP name and type
    c_int8_t *apn_name = "corekube ipv4";
    strcpy(pdn->apn, apn_name);
    pdn->pdn_type = HSS_PDN_TYPE_IPV4;

    // define the default QOS
    qos = &pdn->qos;
    qos->qci = PDN_QCI_9;
    qos->arp.priority_level = 1;
    qos->arp.pre_emption_capability = 0; // shall-not-trigger-pre-emption
    qos->arp.pre_emption_vulnerability = 0; // not-pre-emptable
    qos->mbr.uplink = 0b11111110; // 8640 Kbps
    qos->mbr.downlink = 0b11111110; // 8640 Kbps

    // do not define the defaut AMBR bitrates
    // they are defined manually in
    // esm_build_activate_default_bearer_context_request()

    // define the default PAA
    paa = &pdn->paa;
    paa->pdn_type = GTP_PDN_TYPE_IPV4;
    paa->addr = inet_addr("192.168.56.106");

    // define the default pgw ip
    pgw_ip = &pdn->pgw_ip;
    pgw_ip->ipv4 = 1;
    pgw_ip->len = IPV4_LEN;
    pgw_ip->addr = inet_addr("8.8.8.8");

    // set the output
    *pdn_out = pdn;

    return CORE_OK;
}

status_t get_default_pco(nas_protocol_configuration_options_t *default_pco) {
    d_info("Getting the default Protocol Configuration Options");

    // the following is an encoded pco taken from the OAI messages
    // it sets the primary DNS to 8.8.8.8 and the secondary DNS to 8.8.4.4
    char default_pco_hex[] = "8080211003000010810608080808830608080404000d0408080808";

    // store the hex pco in the correct struct
    memset(default_pco, 0, sizeof(nas_protocol_configuration_options_t));
    CORE_HEX(default_pco_hex, strlen(default_pco_hex), default_pco->buffer);
    default_pco->length = strlen(default_pco_hex) / 2;

    return CORE_OK;

}