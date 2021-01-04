#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libconfig.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "log.h"
#include "user.h"
#include "crypto.h"
#include "hashmap.h"
#include "api.h"

#define OK 0
#define ERROR -1
#define CONFIG_FILE_PATH "db.conf"
#define LISTEN_QUEUE_SIZE 128
#define BUFFER_LEN 1024

/* DB structure
* HashMap1: <IMSI><UserInfo>
* HashMap2: <TMSI><UserInfo>
*/

void dump_mem(uint8_t * value, int len)
{
	int i;

	for(i = 0; i < len; i++) {
		if(i % 17 == 0)
			printf("\n");
		printf("%.2x ", value[i]);
	}
	printf("\n");
}

HashMap * imsi_map;
HashMap * tmsi_map;

/* Global socket */
int sock;

const uint8_t ascii_map[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //  !"#$%&'
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ()*+,-./
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
	0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
	0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // PQRSTUVW
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // XYZ[\]^_
	0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // `abcdefg
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // hijklmno
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // pqrstuvw
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // xyz{|}~.
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ........
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ........
};

void key_to_hex(uint8_t * key_hex, char * key_str)
{
	uint8_t  pos, err = 0;
	uint8_t  idx0;
	uint8_t  idx1;

	if(key_str[0] == '0' && key_str[1] == 'x')
		err += 2;

	for (pos = 0; ((pos < 32) && (pos < strlen(key_str))); pos += 2) {
		idx0 = (uint8_t)key_str[pos+err+0];
		idx1 = (uint8_t)key_str[pos+err+1];
		key_hex[(pos)/2] = (uint8_t)(ascii_map[idx0] << 4) | ascii_map[idx1];
	}

	return;
}

enum colums{IMSI_COL, KEY_COL, OP_OPC_COL, OPC_COL};

void process_line(char * line, ssize_t line_len)
{
	char * token;
	int column = 0, opc_flag;
	const char delim[2] = ",";
	UserInfo * new_user;
	uint8_t op_key[16];

	/* Check comment line or empty line */
	if(line[0] == '#' || line_len <= 1)
		return;

	/* Allocate memory for a new user */
	new_user = new_user_info();
	if(new_user == NULL) {
#ifdef DEBUG
		printError("Unable to allocate memory for a new user.\n");
#endif
		return;
	}

	/* Get the first token */
	token = strtok(line, delim);

	/* Iterate over the tokens/columns */
	while( token != NULL ) {
		switch(column) {
			case IMSI_COL:
				set_user_imsi(new_user, token);
				break;
			case KEY_COL:
				key_to_hex(get_user_key(new_user), token);
				break;
			case OP_OPC_COL:
				if(!strcmp(token, "opc"))
					opc_flag = 0;
				else if(!strcmp(token, "op"))
					opc_flag = 1;
				else {
#ifdef DEBUG
					printError("Invalid operator's code type.\n");
#endif
					free_user_info((void *)new_user);
					return;
				}
				break;
			case OPC_COL:
				/* Convert the OP to OPC if necessary */
				if(opc_flag == 1) {
					key_to_hex(op_key, token);
					generate_opc(get_user_key(new_user), op_key, get_user_opc(new_user));
				}
				else
					key_to_hex(get_user_opc(new_user), token);
				break;
			default:
				break;
		}
		column++; /* Increase column index */
		token = strtok(NULL, delim); /* Get next token */
	}

	/* Complete user information */
	complete_user_info(new_user);

	/* Add UserInfo structure to both hashmap structures */
	if(hashmap_add(imsi_map, (uint8_t *) new_user, user_info_size()) == ERROR) {
#ifdef DEBUG
		printWarning("Unable to add user (%s) to IMSI HasMap.\n", get_user_imsi(new_user));
#endif
		free_user_info((void *)new_user);
		return;
	}
	if(hashmap_add(tmsi_map, (uint8_t *) new_user, user_info_size()) == ERROR) {
#ifdef DEBUG
		printWarning("Unable to add user (0x%.8x) to TMSI HasMap.\n", hash_tmsi_user_info(new_user));
#endif
		free_user_info((void *)new_user);
		return;
	}

#ifdef DEBUG
	show_user_info(new_user);
#endif

	free_user_info((void *)new_user);

	return;
}

int read_db_file(const char * db_file)
{
	FILE * file;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	file = fopen(db_file, "r");
	if(file == NULL) {
		printError("Unable to open file at: %s\n", db_file);
		return ERROR;
	}

	while ((read = getline(&line, &len, file)) != -1) {
		process_line(line, read);
	}

	/* Free line after the last call to getline to avoid memory leaks */
	free(line);
	fclose(file);

	return OK;
}

int init_db(const char * db_file, int hashmap_size)
{
	/* Initialize hashmap structures */
	/* IMSI HashMap */
	imsi_map = init_hashmap(hashmap_size, hash_imsi_user_info);
	if(imsi_map == NULL) {
		printError("Error creating IMSI HashMap.\n");
		return ERROR;

	}
	/* TMSI HashMap */
	tmsi_map = init_hashmap(hashmap_size, hash_tmsi_user_info);
	if(tmsi_map == NULL) {
		printError("Error creating TMSI HashMap.\n");
		free_hashmap(imsi_map, free_user_info);
		return ERROR;

	}

	/* Read DB from file */
	if(read_db_file(db_file) == ERROR) {
		printError("Error reading DB file.\n");
		/* Destroy HashMaps */
		free_hashmap(imsi_map, free_user_info);
		free_hashmap(tmsi_map, free_user_info);
		return ERROR;
	}

	return OK;
}

int configure_network(const char * ip_address, int port)
{
	struct sockaddr_in db_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		printError("%s\n", strerror(errno));
		return ERROR;
	}
	/* Disable Nagle's algorithm */
	int nodelay = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0) {
		printError("setsockopt no_delay (%s)\n", strerror(errno));
		close(sock);
		return ERROR;
	}

	/* Setup DB address */
	db_addr.sin_family = AF_INET;
	db_addr.sin_addr.s_addr = inet_addr(ip_address);
	db_addr.sin_port = htons(port);
	memset(&(db_addr.sin_zero), '\0', 8);

	/* Binding socket */
	if(bind(sock, (struct sockaddr *) &db_addr, sizeof(struct sockaddr)) == -1)
	{
		printError("socket bind (%s)\n", strerror(errno));
		close(sock);
		return ERROR;
	}

	/* Setup listen */
	if(listen(sock, LISTEN_QUEUE_SIZE) == -1)
	{
		printError("socket listen (%s)\n", strerror(errno));
		close(sock);
		return ERROR;
	}


	return OK;
}

void analyze_request(uint8_t * request, int request_len, uint8_t * response, int * response_len) {
	UserInfo * user;
	int offset = 0, res_offset = 0; 
	uint8_t tmp_nas;

	/* Get user info based on the client's ID*/
	switch(request[0]) {
		case IMSI:
			user = (UserInfo *) hashmap_get(imsi_map, hash_imsi((char *)(request+1)));
			break;
		case MSIN:
			user = (UserInfo *) hashmap_get(imsi_map, hash_msin((char *)(request+1)));
			break;
		case TMSI:
			user = (UserInfo *) hashmap_get(tmsi_map, hash_tmsi(request+1));
			break;
		default:
			response[0] = 0;
			*response_len = 1;
			return;
	}
	offset += 17;

#ifdef DEBUG
	printInfo("Requested User Info: \n");
	show_user_info(user);
#endif

	/* Update user info based on the PUSH items */
	while(request[offset] != 0) {
		switch(request[offset]) {
			case ENB_UE_S1AP_ID:
				memcpy(get_user_enb_ue_id(user), request+offset+1, ENB_UE_S1AP_ID_LEN);
				break;
			case UE_TEID:
				memcpy(get_user_ue_teid(user), request+offset+1, TEID_LEN);
				break;
			case SPGW_IP:
				memcpy(get_user_spgw_ip(user), request+offset+1, IP_LEN);
				break;
			case ENB_IP:
				memcpy(get_user_enb_ip(user), request+offset+1, IP_LEN);
				break;
			case PDN_IP:
				memcpy(get_user_pdn_ipv4(user), request+offset+1, IP_LEN);
				break;
			case EPC_NAS_SEQUENCE_NUMBER:
				set_user_epc_nas_sequence_number(user, request[offset+1]);
				break;
			case UE_NAS_SEQUENCE_NUMBER:
				set_user_ue_nas_sequence_number(user, request[offset+1]);
				break;
			case AUTH_RES:
				memcpy(get_user_auth_res(user), request+offset+1, AUTH_RES_LEN);
				break;
			case ENC_KEY:
				memcpy(get_user_enc_key(user), request+offset+1, KEY_LEN);
				break;
			case INT_KEY:
				memcpy(get_user_int_key(user), request+offset+1, KEY_LEN);
				break;
			default:
				/* Undefined Item */
				break;
		}
		offset += 17;
	}

	/* Skip the 0 byte used to separate PUSH items from PULL items */
	offset++;

	/* Copy the fields specified in the PULL item list */
	while(offset < request_len) {
		switch(request[offset]) {
			case IMSI:
				response[res_offset] = IMSI;
				memcpy(response+res_offset+1, (uint8_t *)get_user_imsi(user), IMSI_LEN);
				response[res_offset+16] = 0;
				break;
			case MSIN:
				response[res_offset] = MSIN;
				memcpy(response+res_offset+1, (uint8_t *)get_user_msin(user), MSIN_LEN);
				break;
			case TMSI:
				response[res_offset] = TMSI;
				memcpy(response+res_offset+1, get_user_tmsi(user), TMSI_LEN);
				break;
			case ENB_UE_S1AP_ID:
				response[res_offset] = ENB_UE_S1AP_ID;
				memcpy(response+res_offset+1, get_user_enb_ue_id(user), ENB_UE_S1AP_ID_LEN);
				break;
			case UE_TEID:
				response[res_offset] = UE_TEID;
				memcpy(response+res_offset+1, get_user_ue_teid(user), TEID_LEN);
				break;
			case SPGW_IP:
				response[res_offset] = SPGW_IP;
				memcpy(response+res_offset+1, get_user_spgw_ip(user), SPGW_IP);
				break;
			case ENB_IP:
				response[res_offset] = ENB_IP;
				memcpy(response+res_offset+1, get_user_enb_ip(user), ENB_IP);
				break;
			case PDN_IP:
				response[res_offset] = PDN_IP;
				memcpy(response+res_offset+1, get_user_pdn_ipv4(user), PDN_IP);
				break;
			case EPC_NAS_SEQUENCE_NUMBER:
				/* Copy and increase EPC NAS Sequence number */
				response[res_offset] = EPC_NAS_SEQUENCE_NUMBER;
				tmp_nas = get_user_epc_nas_sequence_number(user);
				response[res_offset+1] = tmp_nas;
				set_user_epc_nas_sequence_number(user, tmp_nas+1);
				break;
			case UE_NAS_SEQUENCE_NUMBER:
				/* Copy and increase UE NAS Sequence number */
				response[res_offset] = UE_NAS_SEQUENCE_NUMBER;
				tmp_nas = get_user_ue_nas_sequence_number(user);
				response[res_offset+1] = tmp_nas;
				set_user_ue_nas_sequence_number(user, tmp_nas+1);
				break;
			case KEY:
				response[res_offset] = KEY;
				memcpy(response+res_offset+1, get_user_key(user), KEY_LEN);
				break;
			case OPC:
				response[res_offset] = OPC;
				memcpy(response+res_offset+1, get_user_opc(user), KEY_LEN);
				break;
			case RAND:
				response[res_offset] = RAND;
				memcpy(response+res_offset+1, get_user_rand(user), RAND_LEN);
				break;
			case RAND_UPDATE:
				/* Copy and generate a new RAND array */
				response[res_offset] = RAND_UPDATE;
				memcpy(response+res_offset+1, get_user_rand(user), RAND_LEN);
				generate_rand(user);
				break;
			case MME_UE_S1AP_ID:
				response[res_offset] = MME_UE_S1AP_ID;
				memcpy(response+res_offset+1, get_user_mme_ue_id(user), MME_UE_S1AP_ID_LEN);
				break;
			case EPC_TEID:
				response[res_offset] = EPC_TEID;
				memcpy(response+res_offset+1, get_user_epc_teid(user), TEID_LEN);
				break;
			case AUTH_RES:
				response[res_offset] = AUTH_RES;
				memcpy(response+res_offset+1, get_user_auth_res(user), AUTH_RES_LEN);
				break;
			case ENC_KEY:
				response[res_offset] = ENC_KEY;
				memcpy(response+res_offset+1, get_user_enc_key(user), KEY_LEN);
				break;
			case INT_KEY:
				response[res_offset] = INT_KEY;
				memcpy(response+res_offset+1, get_user_int_key(user), KEY_LEN);
				break;
		}
		res_offset += 17;
		offset++;
	}
	*response_len = res_offset;
	return;
}

void * attend_request(void * args)
{
	int client, * client_ref;
	uint8_t request[BUFFER_LEN], response[BUFFER_LEN];
	int request_len, response_len;

	client_ref = (int *) args;
	client = *client_ref;

	while(1) {
		/* Read client's request */
		request_len = recv(client, request, BUFFER_LEN, 0);
		if(request_len == 0) {
			close(client);
			return NULL;
		}
#ifdef DEBUG
		printInfo("Analyzing request...\n");
		printf("REQUEST:\n");
		dump_mem(request, request_len);
#endif
		/* Analyze request and generate the response message */
		analyze_request(request, request_len, response, &response_len);
#ifdef DEBUG
		printInfo("Sending answer to client...\n");
		printf("RESPONSE:\n");
		dump_mem(response, response_len);
#endif
		/* Send response to client and close connection */
		send(client, response, response_len, 0);
	}

	return NULL;
}

void main_worker()
{
	pthread_t worker;
	int client;
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	addrlen = sizeof(client_addr);

	while(1) {
		/* Accept client */
		client = accept(sock, (struct sockaddr *)&client_addr, &addrlen);
		if(client == -1)
		{
			printError("socket accept (%s)\n", strerror(errno));
			continue;
		}
#ifdef DEBUG
		printInfo("New client at %s\n", inet_ntoa(client_addr.sin_addr));
#endif
		/* Create a thread toa ttend client's request */
		if (pthread_create(&worker, NULL, attend_request, &client) != 0) {
			printError("Error creating worker thread (%s)\n", strerror(errno));
			close(client);
		}
	}

	return;
}

int main(int argc, char const *argv[])
{
	config_t cfg;
	const char * db_file_path = NULL;
	const char * db_ip_address = NULL;
	int hashmap_size, db_port;

	config_init(&cfg);
	if (!config_read_file(&cfg, CONFIG_FILE_PATH)) {
		printError("Unable to open config file (%s:%d - %s).\n",
			config_error_file(&cfg),
			config_error_line(&cfg),
			config_error_text(&cfg));
		config_destroy(&cfg);
		return ERROR;
	}

	/* Retrieve information from the config file */
	/* Get config file path */
	if (!config_lookup_string(&cfg, "db.users_db", &db_file_path)) {
		printError("Config file error: db.users_db not defined.\n");
		config_destroy(&cfg);
		return ERROR;
	}
	/* Get hashmap_size */
	if (!config_lookup_int(&cfg, "db.hashmap_size", &hashmap_size)) {
		printError("Config file error: db.hashmap_size not defined.\n");
		config_destroy(&cfg);
		return ERROR;
	}
	/* Get db_ip_address */
	if (!config_lookup_string(&cfg, "network.db_ip_address", &db_ip_address)) {
		printError("Config file error: network.db_ip_address not defined.\n");
		config_destroy(&cfg);
		return ERROR;
	}
	/* Get db_port */
	if (!config_lookup_int(&cfg, "network.db_port", &db_port)) {
		printError("Config file error: network.db_port not defined.\n");
		config_destroy(&cfg);
		return ERROR;
	}

	printOK("Config file successfully readed.\n");
#ifdef DEBUG
	printf("DB file: %s\n", db_file_path);
	printf("Hashmap Table size: %d\n", hashmap_size);
	printf("Network address: %s:%d\n", db_ip_address, db_port);
#endif

	printInfo("Starting CoreKubeDB...\n");
	if(init_db(db_file_path, hashmap_size) == ERROR)
	{
		printError("Error initializing DB.\n");
		config_destroy(&cfg);
		return ERROR;
	}
	printOK("Database initialized.\n");

	printInfo("Configuring network...\n");
	if(configure_network(db_ip_address, db_port) == ERROR)
	{
		printError("Error configuring network.\n");
		config_destroy(&cfg);
		free_hashmap(imsi_map, free_user_info);
		free_hashmap(tmsi_map, free_user_info);
		return ERROR;
	}
	printOK("Network configured.\n");

	/* Initialize workers pool */
	printOK("CoreKubeDB running.\n");
	main_worker();

	free_hashmap(imsi_map, free_user_info);
	free_hashmap(tmsi_map, free_user_info);
	return OK;
}