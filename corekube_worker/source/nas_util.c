#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_util.h"
#include "nas_message_security.h"

status_t decode_nas_emm(S1AP_NAS_PDU_t *NAS_PDU, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *nas_message) {
    d_info("Decoding NAS-PDU EMM");

    pkbuf_t *pkbuf = NULL;

    memset(nas_message, 0, sizeof (nas_message_t));

    // determine if this message is plain or security-protected
    pkbuf_t *pkbuf2 = pkbuf_alloc(0, MAX_SDU_LEN);
    pkbuf2->len = NAS_PDU->size;
    memcpy(pkbuf2->payload, NAS_PDU->buf, pkbuf2->len);
    nas_emm_header_t header;
    c_uint16_t size = sizeof(nas_emm_header_t);
    d_assert(pkbuf_header(pkbuf2, -size) == CORE_OK, 
            return CORE_ERROR, "pkbuf_header error");
    memcpy(&header, pkbuf2->payload - size, size);
    pkbuf_free(pkbuf2);

    d_info("NAS EMM header type is: %d", header.security_header_type);
    d_info("Checking equality to NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE: %d", header.security_header_type == NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE);

    if (header.security_header_type == NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE) {
        d_info("Handling plain NAS message");
        pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
        pkbuf->len = NAS_PDU->size;
        memcpy(pkbuf->payload, NAS_PDU->buf, pkbuf->len);
    }
    else {
        d_info("Handling security-protected NAS Message");

        // TODO: doing a separate fetch from the DB just for decoding the NAS
        // security message is a very poor use of resources - rework this!
        c_uint8_t buffer[1024];
        corekube_db_pulls_t db_pulls;
        status_t db_fetch = get_NAS_decode_security_prerequisites_from_db(mme_ue_id, buffer, &db_pulls);
        d_assert(db_fetch == CORE_OK, return CORE_ERROR, "Failed to fetch NAS decode security prerequisites from DB");

        status_t security_decode = nas_security_decode(NAS_PDU, &db_pulls, &pkbuf);
        d_assert(security_decode == CORE_OK, return CORE_ERROR, "Security decode of NAS-PDU failed");
        d_assert(pkbuf, return CORE_ERROR, "somehow pkbuf is null");
    }

    status_t nas_decode = nas_emm_decode(nas_message, pkbuf);
    pkbuf_free(pkbuf);
    d_assert(nas_decode == CORE_OK, return CORE_ERROR, "Failed to decode NAS EMM message");

    return CORE_OK;
}