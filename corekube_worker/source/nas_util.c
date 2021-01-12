#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "nas_util.h"

status_t decode_nas_emm(S1AP_NAS_PDU_t *NAS_PDU, nas_message_t *nas_message) {
    d_info("Decoding NAS-PDU EMM");
    pkbuf_t *pkbuf;

    memset(nas_message, 0, sizeof (nas_message_t));

    pkbuf = pkbuf_alloc(0, MAX_SDU_LEN);
    pkbuf->len = NAS_PDU->size;
    memcpy(pkbuf->payload, NAS_PDU->buf, pkbuf->len);

    status_t nas_decode = nas_emm_decode(nas_message, pkbuf);
    pkbuf_free(pkbuf);
    d_assert(nas_decode == CORE_OK, return CORE_ERROR, "Failed to decode NAS EMM message");

    return CORE_OK;
}