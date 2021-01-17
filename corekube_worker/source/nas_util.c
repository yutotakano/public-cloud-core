#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_util.h"
#include "nas_message_security.h"

status_t decode_nas_emm(S1AP_NAS_PDU_t *NAS_PDU, corekube_db_pulls_t *db_pulls, nas_message_t *nas_message) {
    d_info("Decoding NAS-PDU EMM");

    pkbuf_t *pkbuf = NULL;

    memset(nas_message, 0, sizeof (nas_message_t));

    if (db_pulls != NULL) {
        d_info("db_pulls not null");
        status_t security_decode = nas_security_decode(NAS_PDU, db_pulls, &pkbuf);
        d_assert(security_decode == CORE_OK, return CORE_ERROR, "Security decode of NAS-PDU failed");
        d_assert(pkbuf, return CORE_ERROR, "somehow pkbuf is null");
    }
    else {
        d_info("db_pulls null");
        pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
        pkbuf->len = NAS_PDU->size;
        memcpy(pkbuf->payload, NAS_PDU->buf, pkbuf->len);
    }

    status_t nas_decode = nas_emm_decode(nas_message, pkbuf);
    pkbuf_free(pkbuf);
    d_assert(nas_decode == CORE_OK, return CORE_ERROR, "Failed to decode NAS EMM message");

    return CORE_OK;
}