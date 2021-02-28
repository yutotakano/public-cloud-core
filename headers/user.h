#ifndef __USER__
#define __USER__

#define IMSI_LEN 16
#define MSIN_LEN 11
#define KEY_LEN 16
#define ENB_UE_S1AP_ID_LEN 4
#define MME_UE_S1AP_ID_LEN 4
#define IP_LEN 4
#define TMSI_LEN 4
#define TEID_LEN 4
#define RAND_LEN 16
#define AUTH_RES_LEN 8

typedef struct _UserInfo UserInfo;

UserInfo * new_user_info();
void free_user_info(void * user);
void set_user_imsi(UserInfo * user, char  * imsi);
char * get_user_imsi(UserInfo * user);
char * get_user_msin(UserInfo * user);
uint8_t * get_user_key(UserInfo * user);
uint8_t * get_user_opc(UserInfo * user);
uint8_t * get_user_rand(UserInfo * user);
uint8_t * get_user_enb_ue_id(UserInfo * user);
uint8_t * get_user_mme_ue_id(UserInfo * user);
uint8_t * get_user_tmsi(UserInfo * user);
uint8_t * get_user_ue_teid(UserInfo * user);
uint8_t * get_user_epc_teid(UserInfo * user);
uint8_t * get_user_spgw_ip(UserInfo * user);
uint8_t * get_user_enb_ip(UserInfo * user);
uint8_t * get_user_pdn_ipv4(UserInfo * user);
uint8_t get_user_epc_nas_sequence_number(UserInfo * user);
void set_user_epc_nas_sequence_number(UserInfo * user, uint8_t value);
uint8_t get_user_ue_nas_sequence_number(UserInfo * user);
void set_user_ue_nas_sequence_number(UserInfo * user, uint8_t value);
uint8_t * get_user_auth_res(UserInfo * user);
uint8_t * get_user_enc_key(UserInfo * user);
uint8_t * get_user_int_key(UserInfo * user);
uint8_t * get_user_kasme1_key(UserInfo * user);
uint8_t * get_user_kasme2_key(UserInfo * user);
void generate_rand(UserInfo * user);
void complete_user_info(UserInfo * user);
void show_user_info(UserInfo * user);
uint32_t hash_imsi(char * imsi);
uint32_t hash_imsi_user_info(void * user);
uint32_t hash_msin(char * imsi);
uint32_t hash_msin_user_info(void * user);
uint32_t hash_tmsi(uint8_t * tmsi);
uint32_t hash_tmsi_user_info(void * user);
uint32_t hash_mme_ue_s1ap_id(uint8_t * id);
size_t user_info_size();

#endif