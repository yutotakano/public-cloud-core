#ifndef __ue__
#define __ue__

#define UE_PLMN_LENGTH 3
#define UE_MSIN_LENGTH 5
#define UE_CAPABILITIES_LENGTH 3
#define EPS_MOBILITY_IDENTITY_FLAGS 0x09
#define KEY_LENGTH 16
#define IP_LEN 4
#define GUTI_LEN 10
#define AUTH_RES_LENGTH 8
#define GUAMI_LEN 6

typedef struct _UE UE;
typedef struct _Auth_Challenge
{
	uint8_t RAND[16];
	uint8_t AUTN[16];
	uint8_t CK[16];
	uint8_t IK[16];
	uint8_t AK[16];
	uint8_t RES[AUTH_RES_LENGTH];
	uint8_t KASME[32];
	uint8_t NAS_KEY_ENC[16];
	uint8_t NAS_KEY_INT[16];
} Auth_Challenge;

UE * init_UE(char * mcc, char * mnc, char * msin, uint8_t * key, uint8_t * op_key, uint8_t * ue_ip);
void free_UE(UE * ue);
uint8_t * get_ue_mcc(UE * ue);
uint8_t * get_ue_mnc(UE * ue);
uint8_t * get_ue_plmn(UE * ue);
uint8_t * get_ue_msin(UE * ue);
uint8_t * get_ue_capabilities(UE * ue);
uint32_t get_ue_s1ap_id(UE * ue);
uint8_t * get_mme_s1ap_id(UE * ue, uint8_t * len);
void set_mme_s1ap_id(UE * ue, uint8_t * mme_s1ap_id, uint8_t len);
void reset_mme_s1ap_id_len(UE * ue);
uint8_t * get_ue_key(UE * ue);
uint8_t * get_ue_op_key(UE * ue);
void set_nas_session_security_algorithms(UE * ue, uint8_t algs);
uint8_t get_nas_session_enc_alg(UE * ue);
uint8_t get_nas_session_int_alg(UE * ue);
void set_spgw_ip(UE * ue, uint8_t * ip);
uint8_t * get_spgw_ip(UE * ue);
void set_gtp_teid(UE * ue, uint32_t teid);
uint32_t get_gtp_teid(UE * ue);
void set_apn_name(UE * ue, uint8_t * name, uint8_t name_len);
char * get_apn_name(UE * ue);
void set_pdn_ip(UE * ue, uint8_t * ip);
uint8_t * get_pdn_ip(UE * ue);
void set_guti(UE * ue, uint8_t * guti);
uint8_t * get_guti(UE * ue);
char * get_tun_name(UE * ue);
void set_data_plane_socket(UE * ue, int sockfd);
int get_data_plane_socket(UE * ue);
void set_tun_device(UE * ue, int tun);
int get_tun_device(UE * ue);
int get_ue_id(UE * ue);
int get_ue_size();
uint8_t * get_ue_ip(UE * ue);
uint8_t get_nas_sequence_number(UE * ue);
void reset_nas_sequence_number(UE * ue);
Auth_Challenge * get_auth_challenge(UE * ue);
uint8_t get_mme_code(UE * ue);
uint8_t * get_m_tmsi(UE * ue);
char * get_msin_string(UE * ue);
void set_security_capabilities(UE * ue, uint8_t * sec_cap);
uint8_t * get_security_capabilities(UE * ue);
int ue_compare(void * data, void * key);
void set_guami(UE * ue, uint8_t * guami);
uint8_t * get_guami(UE * ue);

void printUE(UE * ue);

#endif
