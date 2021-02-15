#ifndef __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__
#define __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__

#include <libck.h>

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"

#define KDF_NAS_ENC_ALG 0x01
#define KDF_NAS_INT_ALG 0x02

status_t nas_build_security_mode_command(corekube_db_pulls_t *db_pulls, pkbuf_t **pkbuf);

status_t nas_build_cleartext_security_mode_command(nas_message_t *message);

status_t generate_nas_keys(c_uint8_t *kasme, c_uint8_t *knas_int, c_uint8_t *knas_enc);

void kdf_nas(c_uint8_t algorithm_type_distinguishers,
    c_uint8_t algorithm_identity, c_uint8_t *kasme, c_uint8_t *knas);

void kdf_enb(c_uint8_t *kasme, c_uint32_t ul_count, c_uint8_t *kenb);

#endif /* __S1AP_HANDLER_NAS_SECURITY_MODE_COMMAND_H__ */