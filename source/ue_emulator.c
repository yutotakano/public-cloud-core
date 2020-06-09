#include "ue_emulator.h"
#include "message.h"
#include "log.h"
#include "data_plane.h"

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

#define ENB_PORT 2233

ue_data ue;
pthread_t ue_thread;

uint8_t local_ip[4];
uint8_t ue_ip[4];
uint8_t spgw_ip[4];
uint8_t gtp_teid[4];

/* eNB control plane variables */
int sockfd; 
struct sockaddr_in enb_addr;

void dKey(uint8_t * pointer, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        printf("%.2x ", pointer[i]);
    }
    printf("\n");
}


void ue_copy_id_to_buffer(uint8_t * buffer)
{
	buffer[0] = ue.id[0];
	buffer[1] = ue.id[1];
	buffer[2] = ue.id[2];
	buffer[3] = ue.id[3];
}

void * run_command(void * args)
{
	system(ue.command);
	return NULL;
}

int ue_emulator_start(ue_data * data)
{
	uint8_t buffer[1024];
	init_msg * msg;
	init_response_msg * res;
	int n;
	int ret;

    /* Print UE data */
	printf("ID: %d\n", (data->id[0] << 24) | (data->id[1] << 16) | (data->id[2] << 8) | data->id[3] );
	printf("MCC: %s\n", data->mcc);
	printf("MNC: %s\n", data->mnc);
	printf("MSIN: %s\n", data->msin);
	printf("Key: ");
	dKey(data->key, 16);
	printf("OpKey: ");
	dKey(data->op_key, 16);
	printf("Command: %s\n", data->command);
	printf("eNB IP: %d.%d.%d.%d\n", data->enb_ip[0], data->enb_ip[1], data->enb_ip[2], data->enb_ip[3]);
	printf("eNB port: %d\n", data->enb_port);
	printf("Local IP: %d.%d.%d.%d\n", data->local_ip[0], data->local_ip[1], data->local_ip[2], data->local_ip[3]);
	printf("UE IP: %d.%d.%d.%d\n", data->ue_ip[0], data->ue_ip[1], data->ue_ip[2], data->ue_ip[3]);
	printf("Data Plane Port: %d\n", data->spgw_port);

	/* Save UE information */
	memcpy(&ue, data, sizeof(ue_data));

	/* Socket create */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
    	perror("UE socket");
        return 1;
    }   
  
    /* Configure eNB address */
    bzero(&enb_addr, sizeof(enb_addr));
    enb_addr.sin_family = AF_INET;
    memcpy(&enb_addr.sin_addr.s_addr, data->enb_ip, 4);
    enb_addr.sin_port = htons(data->enb_port);
    /* TODO: set eNB port */

	/* Connect to eNB */
	if (connect(sockfd, (struct sockaddr *) &enb_addr, sizeof(enb_addr)) != 0) { 
        perror("UE connect");
        return 1; 
    }

	/* Filling buffer with the UE information */
	buffer[0] = INIT_CODE;
	msg = (init_msg *)(buffer+1);
	bzero(msg, sizeof(msg));
	/* UE ID */
	memcpy(msg->id, ue.id, 4);
	/* MCC */
	memcpy(msg->mcc, ue.mcc, 3);
	/* MNC */
	memcpy(msg->mnc, ue.mnc, 2);
	/* MSIN */
	memcpy(msg->msin, ue.msin, 10);
	/* Key */
	memcpy(msg->key, ue.key, 16);
	/* OpKey */
	memcpy(msg->op_key, ue.op_key, 16);
	/* UE IP */
	memcpy(msg->ue_ip, ue.ue_ip, 4);

	/* Send UE info to eNB and wait to a response */
	write(sockfd, buffer, sizeof(init_msg) + 1);

	/* Receive eNB response */
	n = read(sockfd, buffer, sizeof(buffer));
	if(n < 0)
	{
		perror("UE recv");
		close(sockfd);
		return 1;
	}
	if(buffer[0] != (OK_CODE | INIT_CODE))
	{
		perror("Error in remote eNB");
		close(sockfd);
		return 1;
	}

	/* Analyze response */
	res = (init_response_msg *) (buffer+1);
	/* Gert TEID */
	memcpy(gtp_teid, res->teid, 4);
	/* Get UE IP */
	memcpy(ue_ip, res->ue_ip, 4);
	/* Get SPGW_IP */
	memcpy(spgw_ip, res->spgw_ip, 4);

	/* Print received parameters */
	uint32_t teid = (gtp_teid[0] << 24) | (gtp_teid[1] << 16) | (gtp_teid[2] << 8) | gtp_teid[3];
	printf("Received information from eNB\n");
	printf("GTP-TEID: 0x%x\n", teid);
	printf("UE IP: %d.%d.%d.%d\n", ue_ip[0], ue_ip[1], ue_ip[2], ue_ip[3]);
	printf("SPGW IP: %d.%d.%d.%d\n", spgw_ip[0], spgw_ip[1], spgw_ip[2], spgw_ip[3]);

	/* Create data-plane */
	if(data->spgw_port == 2152)
		ret = start_data_plane(ue.local_ip, ue.msin, ue_ip, spgw_ip, teid, data->spgw_port);
	else
		ret = start_data_plane(ue.local_ip, ue.msin, ue_ip, ue.ue_ip, teid, data->spgw_port);
	if(ret == 1)
	{
		printError("start_data_plane error\n");
		return 1;
	}


	/* Create traffic generator thread */
	if (pthread_create(&ue_thread, NULL, run_command, 0) != 0)
    {
        perror("pthread_create run_command");
        return 1;
    }
	return 0;
}