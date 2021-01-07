#ifndef __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__
#define __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"

#define KDF_NAS_ENC_ALG 0x01
#define KDF_NAS_INT_ALG 0x02

status_t generate_nas_keys(c_uint8_t *kasme, c_uint8_t *knas_int, c_uint8_t *knas_enc);

void kdf_nas(c_uint8_t algorithm_type_distinguishers,
    c_uint8_t algorithm_identity, c_uint8_t *kasme, c_uint8_t *knas);

#endif /* __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__ */