#ifndef __crypto__
#define __crypto__

#include <stdio.h>
#include <stdint.h>

void f2345( uint8_t * k, uint8_t * rand, uint8_t * res, uint8_t * ck, uint8_t * ik, uint8_t * ak, uint8_t * op_c);
void generate_kasme(uint8_t * ck, uint8_t * ik, uint8_t * ak, uint8_t * sqn, uint8_t * plmn, uint8_t plmn_len, uint8_t * k_asme);
void generate_nas_enc_keys(uint8_t * k_asme, uint8_t * nas_key_enc, uint8_t alg_identity);
void generate_nas_int_keys(uint8_t * k_asme, uint8_t * nas_key_int, uint8_t alg_identity);
void nas_integrity_eia2(uint8_t * key, uint8_t * message, uint8_t message_length, uint32_t count, uint8_t out[4]);

#endif