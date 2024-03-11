#ifndef __crypto__
#define __crypto__

#include <stdio.h>
#include <stdint.h>

void f2345( uint8_t * k, uint8_t * rand, uint8_t * res, uint8_t * ck, uint8_t * ik, uint8_t * ak, uint8_t * op_c);
/* 4G Derivation functions */
void generate_kasme(uint8_t * ck, uint8_t * ik, uint8_t * ak, uint8_t * sqn, uint8_t * plmn, uint8_t plmn_len, uint8_t * k_asme);
void generate_nas_enc_keys(uint8_t * k_asme, uint8_t * nas_key_enc, uint8_t alg_identity);
void generate_nas_int_keys(uint8_t * k_asme, uint8_t * nas_key_int, uint8_t alg_identity);
void nas_integrity_eia2(uint8_t * key, uint8_t * message, uint8_t message_length, uint32_t count, uint8_t bearer, uint8_t out[4]);
/* 5G Derivation functions */
void generate_res_5g(uint8_t * res5g, uint8_t * ck, uint8_t * ik, uint8_t * rand, uint8_t * res, uint8_t * mcc, uint8_t * mnc);
void generate_kausf(uint8_t * kausf, uint8_t * ck, uint8_t * ik, uint8_t * autn, uint8_t * mcc, uint8_t * mnc);
void generate_kseaf(uint8_t * kseaf, uint8_t * kausf, uint8_t * mcc, uint8_t * mnc);
void generate_kamf(uint8_t * kamf, char * supi, uint8_t * kseaf);
void generate_5g_nas_enc_key(uint8_t * enc_key, uint8_t * kamf, uint8_t alg_identity);
void generate_5g_nas_int_key(uint8_t * int_key, uint8_t * kamf, uint8_t alg_identity);

#endif