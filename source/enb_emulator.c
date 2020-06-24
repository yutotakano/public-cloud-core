#include "enb_emulator.h"
#include "enb.h"
#include "sctp.h"
#include "s1ap.h"
#include "log.h"
#include "ue.h"
#include "message.h"
#include "map.h"

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
int sockfd;
map_void_t map;

void dump(uint8_t * pointer, int len)
{
	int i;
	for(i = 0; i < len; i++)
	{
		if(i % 16 == 0)
			printf("\n");
		else if(i % 8 == 0)
			printf("  ");
		printf("%.2x ", pointer[i]);
	}
	printf("\n");
}

void enb_copy_id_to_buffer(uint8_t * buffer)
{
	uint32_t id = get_enb_id(enb);
	buffer[0] = (id >> 24) & 0xFF;
	buffer[1] = (id >> 16) & 0xFF;
	buffer[2] = (id >> 8) & 0xFF;
	buffer[3] = id & 0xFF;
}

void enb_copy_port_to_buffer(uint8_t * buffer)
{
	struct sockaddr_in sin;
	uint16_t port;
	socklen_t len = sizeof(sin);
	if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
	{
		perror("getsockname");
		port = 0;
	}
	port = ntohs(sin.sin_port);
	printf("eNB internal port: %d\n", port);
	buffer[0] = (port >> 8) & 0xFF;
	buffer[1] = port & 0xFF;
}

uint32_t id_to_uint32(uint8_t * id)
{
	uint32_t ret;
	ret = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];
	return ret;
}

int analyze_ue_msg(uint8_t * buffer, int len, uint8_t * response, int * response_len, uint8_t * ue_ip)
{
	init_msg * initmsg;
	idle_msg * idlemsg;
	init_response_msg * res;
	UE * ue = NULL;
	uint8_t switch_off;

	printInfo("Analyzing UE message...\n");

	/* UE connecting to the EPC */
	if(buffer[0] == INIT_CODE)
	{
		printInfo("UE INIT message received\n");
		initmsg = (init_msg *)(buffer+1);
		/* Create UE */
		ue = init_UE((char *)initmsg->mcc, (char *)initmsg->mnc, (char *)initmsg->msin, initmsg->key, initmsg->op_key, initmsg->ue_ip);
		printUE(ue);

		/* Add to map structure */
		/* NOTE: Because the MAP implementation, the key has to be the UE msin string */
		map_add(&map, (char *)initmsg->msin, (void *)ue, get_ue_size());

		/* Free the original object and use the copy stored in the MAP structure */
		free_UE(ue);
		ue = (UE *)map_get(&map, (char *)initmsg->msin);

		/* Attach UE 1 */
    	if(procedure_Attach_Default_EPS_Bearer(enb, ue, get_ue_ip(ue)))
    	{
    		printf("Attach and Setup default bearer error\n");
    		free_UE(ue);
    		/* Generate UE Error response */
    		response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
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
	else if(buffer[0] == MOVE_TO_IDLE)
	{
		printInfo("UE MOVE_TO_IDLE message received\n");

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)map_get(&map, (char *)idlemsg->msin);

		/* Move UE to idle */
		if(procedure_UE_Context_Release(enb, ue))
		{
			printError("Move to Idle (UEContextRelease) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;
	}
	else if(buffer[0] == UE_DETACH || buffer[0] == UE_SWITCH_OFF_DETACH)
	{
		printInfo("UE UE_DETACH message received\n");

		switch_off = buffer[0] & UE_SWITCH_OFF_DETACH;

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)map_get(&map, (char *)idlemsg->msin);

		/* Detach UE */
		if(procedure_UE_Detach(enb, ue, switch_off))
		{
			printError("Move to Detached (UEDetach) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;
	}
	else if(buffer[0] == UE_ATTACH)
	{
		printInfo("UE UE_ATTACH message received\n");

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)map_get(&map, (char *)idlemsg->msin);

		/* Detach UE */
		if(procedure_Attach_Default_EPS_Bearer(enb, ue, get_ue_ip(ue)))
		{
			printError("Move to Attached (UEAttach) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;

		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		/* init_response_msg struct reused */
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
	/* TODO: Implement othe kind of messages */
	return 0;
}

void * enb_emulator_thread(void * args)
{
	/* Open TCP socket to listen to UEs */
	int client, n, response_len = 0;
	struct sockaddr_in client_addr;
	uint8_t buffer[1024];
	uint8_t response[1024];
	socklen_t addrlen;

	addrlen = sizeof(client_addr);

	/* Init UE map */
	map_init(&map);

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
		printf("\n\n\n");
		printInfo("New UE connected\n");
		/* Receive message */
		n = recv(client, (void *) buffer, 1024, 0);
		if(n <= 0)
		{
			perror("eNB recv");
			close(client);
			continue;
		}

		/* Analyze message and generate the response*/
		if(analyze_ue_msg(buffer, n, response, &response_len, (uint8_t *) &client_addr.sin_addr.s_addr))
		{
			printError("S1AP error\n");
		}
		/* Send response to client */
		write(client, response, response_len);
		/* Close socket */
		close(client);
	}
}

int enb_emulator_start(enb_data * data)
{
	struct sockaddr_in enb_addr;

	/* Start emulator thread */
	printf("Real value: 0x%.8x\n", id_to_uint32(data->id));
	printf("MCC: %s\n", data->mcc);
	printf("MNC: %s\n", data->mnc);
	printf("EPC IP: %d.%d.%d.%d\n", data->epc_ip[0], data->epc_ip[1], data->epc_ip[2], data->epc_ip[3]);

	/* Create eNB object */
	enb = init_eNB(id_to_uint32(data->id), (char *)data->mcc, (char *)data->mnc, data->enb_ip);
	if(enb == NULL)
		return 1;

	/***************/
    /* Connect eNB */
    /***************/
    /* Open SCTP connection */
    if(sctp_connect_enb_to_mme(enb, data->epc_ip) == 1)
    {
    	printError("SCTP Setup error\n");
    	return 1;
    }
    /* Connect eNB to MME */
    if(procedure_S1_Setup(enb) == 1)
    {
    	printError("eNB S1 Setup problems\n");
    	return 1;
    }
    printOK("eNB successfully connected\n");

    /* Open eNB port. In this port the eNB will receive UEs connections */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("eNB socket");
		return 1;
	}

	/* Setup eNB address */
	enb_addr.sin_family = AF_INET;
	memcpy(&enb_addr.sin_addr.s_addr, data->enb_ip, 4);
	/* Bind to 0 to bind to a random port */
	enb_addr.sin_port = htons(0);
	memset(&(enb_addr.sin_zero), '\0', 8);

	/* Binding process */
	if(bind(sockfd, (struct sockaddr *) &enb_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("eNB bind");
		return 1;
	}

	/* Listen for UE's */
	if(listen(sockfd, CLIENT_NUMBER) == -1)
	{
		perror("eNB listen");
		return 1;
	}

	/* Initialize eNB thread */
	/* Create distributor thread */
    if (pthread_create(&enb_thread, NULL, enb_emulator_thread, 0) != 0)
    {
        perror("pthread_create enb_emulator_thread");
        return 1;
    }

    /* Return back an wait for Controller messages */
	return 0;
}
