#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

#define IMSI_LEN 16
#define KEY_LEN 16

struct _UserInfo
{
	char imsi[IMSI_LEN];
	uint8_t key[KEY_LEN];
	uint8_t opc[KEY_LEN];
};

UserInfo * new_user_info()
{
	return (UserInfo *)malloc(sizeof(UserInfo));
}

void free_user_info(UserInfo * user)
{
	free(user);
}

void set_user_imsi(UserInfo * user, char  * imsi)
{
	memcpy(user->imsi, imsi, IMSI_LEN);
	user->imsi[15] = 0;
	return;
}

uint8_t * get_user_key(UserInfo * user)
{
	return user->key;
}

uint8_t * get_user_opc(UserInfo * user)
{
	return user->opc;
}

void show_user_info(UserInfo * user)
{
	int i;
	printf("IMSI: %s\n", user->imsi);
	printf("KEY: ");
	for(i = 0; i < 16; i++) printf("%.2x ", user->key[i]);
	printf("\n");
	printf("OPC: ");
	for(i = 0; i < 16; i++) printf("%.2x ", user->opc[i]);
	printf("\n");
}