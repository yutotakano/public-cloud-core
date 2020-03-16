#ifndef __crypto__
#define __crypto__

#include <stdio.h>
#include <stdint.h>

void f1( uint8_t * k, uint8_t * rand, uint8_t * sqn, uint8_t * amf, uint8_t * mac_a, uint8_t * op_c);
void f2345( uint8_t * k, uint8_t * rand, uint8_t * res, uint8_t * ck, uint8_t * ik, uint8_t * ak, uint8_t * op_c);
void naive_milenage( uint8_t * k, uint8_t * rand, uint8_t * op_c, uint8_t * res);

#endif