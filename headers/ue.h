#ifndef __ue__
#define __ue__

#define UE_PLMN_LENGTH 3
#define UE_MSIN_LENGTH 5
#define UE_CAPABILITIES_LENGTH 3
#define EPS_MOBILITY_IDENTITY_FLAGS 0x09
#define UE_DEFAULT_CAPABILITIES 0x8020
#define KEY_LENGTH 16

typedef struct _UE UE;

UE * init_UE(char * mcc, char * mnc, char * msin, uint16_t capabilities, uint32_t ue_s1ap_id, uint8_t * key, uint8_t * op_key);
void free_UE(UE * ue);
uint8_t * get_ue_plmn(UE * ue);
uint8_t * get_ue_msin(UE * ue);
uint8_t * get_ue_capabilities(UE * ue);
uint32_t get_ue_s1ap_id(UE * ue);
uint8_t * get_mme_s1ap_id(UE * ue, uint8_t * len);
void set_mme_s1ap_id(UE * ue, uint8_t * mme_s1ap_id, uint8_t len);
uint8_t * get_ue_key(UE * ue);
uint8_t * get_ue_op_key(UE * ue);
void set_nas_session_security_algorithms(UE * ue, uint8_t algs);
uint8_t get_nas_session_enc_alg(UE * ue);
uint8_t get_nas_session_int_alg(UE * ue);
void printUE(UE * ue);

#endif
