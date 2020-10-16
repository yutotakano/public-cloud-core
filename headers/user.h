#ifndef __USER__
#define __USER__

typedef struct _UserInfo UserInfo;

UserInfo * new_user_info();
void free_user_info(UserInfo * user);
void set_user_imsi(UserInfo * user, char  * imsi);
uint8_t * get_user_key(UserInfo * user);
uint8_t * get_user_opc(UserInfo * user);
void show_user_info(UserInfo * user);

#endif