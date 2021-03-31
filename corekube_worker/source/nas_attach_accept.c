#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_attach_accept.h"

#include "core/include/core_lib.h"
#include "nas_message_security.h"
#include "s1ap_conv.h"

// the following function is heavily adapted from 
// s1ap_build_initial_context_setup_request() in
// the file nextepc/src/mme/esm_build.c
status_t emm_build_attach_accept(pkbuf_t **emmbuf, pkbuf_t *esmbuf, corekube_db_pulls_t *db_pulls)
{
    d_info("Building EMM Attach Accept message");

    nas_message_t message;
    nas_attach_accept_t *attach_accept = &message.emm.attach_accept;
    nas_eps_attach_result_t *eps_attach_result = 
        &attach_accept->eps_attach_result;
    nas_gprs_timer_t *t3412_value = &attach_accept->t3412_value;
    nas_eps_mobile_identity_t *guti = &attach_accept->guti;

    memset(&message, 0, sizeof(message));
    message.h.security_header_type = 
       NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED;
    message.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;

    message.emm.h.protocol_discriminator = NAS_PROTOCOL_DISCRIMINATOR_EMM;
    message.emm.h.message_type = NAS_ATTACH_ACCEPT;

    /* Set T3412 */
    eps_attach_result->result = NAS_ATTACH_TYPE_EPS_ATTACH;
    t3412_value->unit = NAS_GRPS_TIMER_UNIT_MULTIPLES_OF_DECI_HH;
    t3412_value->value = 9;

    tai0_list_t list0;
    // set to mcc=208 (France), mnc=93 (Unknown)
    plmn_id_build(&list0.tai[0].plmn_id, 208, 93, 2); // TODO: fixed values may need to be derived
    list0.tai[0].num = 1;
    list0.tai[1].tac[0] = 1; // TODO: fixed values may need to be derived

    tai2_list_t list2;
    list2.num = 0;

    // TODO: this isn't working 100%, the TAC is set to zero
    // regardless of it's actual value
    nas_tai_list_build(&attach_accept->tai_list, &list0, &list2);

    // TODO: temporary fix to the above issue
    memset(&attach_accept->tai_list, 0, sizeof(nas_tracking_area_identity_list_t));
    attach_accept->tai_list.length = 6;
    char tai_list[] = "2002f8390001";
    CORE_HEX(tai_list, strlen(tai_list), &attach_accept->tai_list.buffer);

    attach_accept->esm_message_container.buffer = esmbuf->payload;
    attach_accept->esm_message_container.length = esmbuf->len;

    attach_accept->presencemask |= NAS_ATTACH_ACCEPT_GUTI_PRESENT;
    guti->length = sizeof(nas_eps_mobile_identity_guti_t);
    guti->guti.odd_even = NAS_EPS_MOBILE_IDENTITY_EVEN;
    guti->guti.type = NAS_EPS_MOBILE_IDENTITY_GUTI;
    CORE_HEX("02f839", 6, &guti->guti.plmn_id); // TODO: fixed values may need to be derived
    guti->guti.mme_gid = 0x04; // TODO: fixed values may need to be derived
    guti->guti.mme_code = 0x01; // TODO: fixed values may need to be derived

    d_assert(db_pulls->tmsi != NULL, return CORE_ERROR, "DB TMSI is NULL");
    guti->guti.m_tmsi = array_to_int(db_pulls->tmsi);

    status_t securtiy_encode = nas_security_encode(&message, db_pulls, emmbuf);
    d_assert(securtiy_encode == CORE_OK && *emmbuf, return CORE_ERROR, "nas_security_encode error");
    pkbuf_free(esmbuf);

    return CORE_OK;
}