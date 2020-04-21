#include "enb_emulator.h"
#include "enb.h"
#include "sctp.h"
#include "s1ap.h"
#include "log.h"
#include "ue.h"
#include "message.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h> 

#define CLIENT_NUMBER 1024
#define ENB_PORT 2233

/* GLOBAL VARIABLES */
eNB * enb;
const char * enb_ip = "192.168.56.101";
pthread_t enb_thread;

void enb_copy_id_to_buffer(uint8_t * buffer)
{
	uint32_t id = get_enb_id(enb);
	buffer[0] = (id >> 24) & 0xFF;
	buffer[1] = (id >> 16) & 0xFF;
	buffer[2] = (id >> 8) & 0xFF;
	buffer[3] = id & 0xFF;
}

uint32_t id_to_uint32(uint8_t * id)
{
	uint32_t ret;
	ret = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];
	return ret;
}

int analyze_ue_msg(uint8_t * buffer, int len, uint8_t * response, int * response_len)
{
	init_msg * msg;
	init_response_msg * res;
	UE * ue;
	/* UE connecting to the EPC */
	if(buffer[0] == INIT_CODE)
	{
		msg = (init_msg *)(buffer+1);
		/* Implement lists to store all connected UEs (TODO) */
		/* Check the UE does not exist */
		/* Create UE */
		ue = init_UE((char *)msg->mcc, (char *)msg->mnc, (char *)msg->msin, msg->key, msg->op_key);

		/* Attach UE 1 */
    	if(procedure_Attach_Default_EPS_Bearer(enb, ue))
    	{
    		printf("Attach and Setup default bearer error\n");
    		free_UE(ue);
    		return 1;
    	}

		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		res = (init_response_msg *) (response+1);
		uint32_t teid = get_gtp_teid(ue);
		res->teid[0] = (teid >> 24) & 0xFF;
		res->teid[1] = (teid >> 16) & 0xFF;
		res->teid[2] = (teid >> 8) & 0xFF;
		res->teid[3] = teid & 0xFF;
		memcpy(res->ue_ip, get_pdn_ip(ue), 4);
		memcpy(res->spgw_ip, get_spgw_ip(ue), 4);
		*response_len = sizeof(init_response_msg) + 1;
	}
	return 0;
}

void * enb_emulator_thread(void * args)
{
	/* Open TCP socket to listen to UEs */
	int sockfd, client, n, response_len = 0;
	struct sockaddr_in enb_addr, client_addr;
	uint8_t buffer[1024];
	uint8_t response[1024];
	socklen_t addrlen;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("eNB socket");
		return NULL;
	}

	/* Setup eNB address */
	enb_addr.sin_family = AF_INET;
	enb_addr.sin_addr.s_addr = inet_addr(enb_ip);
	enb_addr.sin_port = htons(ENB_PORT);
	memset(&(enb_addr.sin_zero), '\0', 8);

	/* Binding process */
	if(bind(sockfd, (struct sockaddr *) &enb_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("eNB bind");
		return NULL;
	}

	/*  */
	if(listen(sockfd, CLIENT_NUMBER) == -1)
	{
		perror("eNB listen");
		return NULL;
	}

	addrlen = sizeof(client);

	/* Wait for UE messages */
	while(1)
	{
		/* Accept one client */
		client = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
		if(client == -1)
		{
			perror("accept");
			continue;
		}
		/* Receive message */
		n = recv(client, (void *) buffer, 1024, 0);
		if(n <= 0)
		{
			perror("eNB recv");
			close(client);
			continue;
		}
		/* Analyze message and generate the response*/
		if(analyze_ue_msg(buffer, n, response, &response_len))
		{
			perror("S1AP error");
			close(client);
			continue;
		}
		/* Send response to client */
		write(client, response, response_len);
		/* Close socket */
		close(client);
	}
}

int enb_emulator_start(enb_data * data)
{
	/* Start emulator thread */
	printf("Real value: 0x%.8x\n", id_to_uint32(data->id));
	printf("MCC: %s\n", data->mcc);
	printf("MNC: %s\n", data->mnc);
	printf("EPC IP: %d.%d.%d.%d\n", data->epc_ip[0], data->epc_ip[1], data->epc_ip[2], data->epc_ip[3]);

	/* Create eNB object */
	enb = init_eNB(id_to_uint32(data->id), (char *)data->mcc, (char *)data->mnc, enb_ip);
	if(enb == NULL)
		return 1;

	/***************/
    /* Connect eNB */
    /***************/
    /* Open SCTP connection */
    if(sctp_connect_enb_to_mme(enb, data->epc_ip) == 1)
    {
    	printError("SCTP Setup error\n");
    	return -1;
    }
    /* Connect eNB to MME */
    if(procedure_S1_Setup(enb) == 1)
    {
    	printError("eNB S1 Setup problems\n");
    	return -1;
    }
    printOK("eNB successfully connected\n");

	/* Initialize eNB thread */
	/* Create distributor thread */
    if (pthread_create(&enb_thread, NULL, enb_emulator_thread, 0) != 0)
    {
        perror("pthread_create enb_emulator_thread");
        return -1;
    }

    /* Return back an wait for Controller messages */
	return 0;
}