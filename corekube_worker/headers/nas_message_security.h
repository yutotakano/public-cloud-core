#ifndef __S1AP_HANDLER_NAS_MESSAGE_SECURITY_H__
#define __S1AP_HANDLER_NAS_MESSAGE_SECURITY_H__

#include "nas/nas_message.h"
#include "nas_types.h"
#include "s1ap/s1ap_message.h"
#include <libck.h>

#define NAS_SECURITY_BEARER 0
#define NAS_SECURITY_DOWNLINK_DIRECTION 1
#define NAS_SECURITY_UPLINK_DIRECTION 0

#define NAS_SECURITY_MAC_SIZE 4

#define COREKUBE_ENC_ALGORITHM NAS_SECURITY_ALGORITHMS_EEA0
#define COREKUBE_INT_ALGORITHM NAS_SECURITY_ALGORITHMS_128_EIA2

#define EEA_JUST_EEA0 0x80
#define EEA_JUST_EIA2 0x20

// taken from nextepc/src/mme/nas_security.h
typedef struct _nas_security_header_type_t {
    union {
        struct {
        ED5(c_uint8_t integrity_protected:1;,
            c_uint8_t ciphered:1;,
            c_uint8_t new_security_context:1;,
            c_uint8_t service_request:1;,
            c_uint8_t reserved:4;)
        };
        c_uint8_t type;
    };
} __attribute__ ((packed)) nas_security_header_type_t;

status_t nas_security_encode(nas_message_t *message, corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf);

status_t nas_security_decode(S1AP_NAS_PDU_t *nasPdu, corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf_out);

status_t generate_nas_security_header_type(pkbuf_t *nasbuf, nas_security_header_type_t *security_header_type);

status_t get_NAS_decode_security_prerequisites_from_db(S1AP_MME_UE_S1AP_ID_t *mme_ue_id, c_uint8_t *buffer, corekube_db_pulls_t *db_pulls);

void nas_mac_calculate(c_uint8_t algorithm_identity,
        c_uint8_t *knas_int, c_uint32_t count, c_uint8_t bearer, 
        c_uint8_t direction, pkbuf_t *pkbuf, c_uint8_t *mac);

void nas_encrypt(c_uint8_t algorithm_identity,
        c_uint8_t *knas_enc, c_uint32_t count, c_uint8_t bearer, 
        c_uint8_t direction, pkbuf_t *pkbuf);

#endif /* __S1AP_HANDLER_NAS_MESSAGE_SECURITY_H__ */