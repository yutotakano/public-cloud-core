#define _POSIX_SOURCE
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
#include <unistd.h>
#include <signal.h>

#define ENB_PORT 2233

/* Control plane actions */
#define CP_INIT 0
#define CP_DETACH 1
#define CP_DETACH_SWITCH_OFF 2
#define CP_ATTACH 3
#define CP_MOVE_TO_IDLE 4
#define CP_MOVE_TO_CONNECTED 5
#define CP_HANDOVER 6

ue_data ue;
pid_t traffic_generator;

uint8_t local_ip[4];
uint8_t ue_ip[4];
uint8_t spgw_ip[4];
uint8_t gtp_teid[4];

/* External function (declared as extern by default) */
/* Update controller state with IDLE */
void send_idle_controller(uint32_t ue_id);
/* Update controller state with DETACHED */
void send_ue_detach_controller(uint32_t ue_id);
/* Update controller state with ATTACHED*/
void send_ue_attach_controller(uint32_t ue_id);



/* eNB control plane variables */ 
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

void start_traffic_generator(char * command)
{
	/* Traffic generator is going to be a child process */
	traffic_generator = fork();
    if(traffic_generator == 0)
    {
    	/* Create a new session */
    	/* To ensure system() process kill */
    	setsid();

    	printf("Entering traffic generator...\n");
    	/* Execute command */
    	system(command);
		printf("Exiting traffic generator...\n");
		exit(0);
    }
}

void stop_traffic_generator()
{
	printf("Killing traffic generator process...\n");
    kill(traffic_generator*-1, SIGTERM);
}

int send_ue_context_release()
{
	int sockfd;
	idle_msg * msg;
	int n;
	uint8_t buffer[32];

	/* Socket create */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
    	perror("UE socket");
        return 1;
    }

	/* Connect to eNB */
	if (connect(sockfd, (struct sockaddr *) &enb_addr, sizeof(enb_addr)) != 0) { 
        perror("UE connect");
        return 1; 
    }

    /* Build eNB message (IDLE_CODE + MSIN) */
    bzero(buffer, 32);
    buffer[0] = MOVE_TO_IDLE;
    msg = (idle_msg *)(buffer+1);
    /* MSIN */
	memcpy(msg->msin, ue.msin, 10);

	/* Send UE info to eNB and wait to a response */
	write(sockfd, buffer, sizeof(idle_msg) + 2);

	/* Receive eNB response */
	n = read(sockfd, buffer, sizeof(buffer));
	if(n < 0)
	{
		perror("UE recv");
		close(sockfd);
		return 1;
	}
	if(buffer[0] != (OK_CODE | MOVE_TO_IDLE))
	{
		printError("Error in remote eNB (UEContextRelease)\n");
		close(sockfd);
		return 1;
	}
	printOK("UE moved to Idle (UEContextRelease)\n");
	/* Notify the controller */
	send_idle_controller( (ue.id[0] << 24) | (ue.id[1] << 16) | (ue.id[2] << 8) | ue.id[3] );
	/* Here the UE is in IDLE state */
	close(sockfd);
	return 0;
}

int send_ue_detach(uint8_t switch_off)
{
	int sockfd;
	idle_msg * msg;
	int n;
	uint8_t buffer[32];

	/* Kill the traffic generator child process before send the Detach message to the eNB */
	stop_traffic_generator();

	/* Socket create */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
    	perror("UE socket");
        return 1;
    }

	/* Connect to eNB */
	if (connect(sockfd, (struct sockaddr *) &enb_addr, sizeof(enb_addr)) != 0) { 
        perror("UE connect");
        return 1; 
    }

    /* Build eNB message (IDLE_CODE + MSIN) */
    bzero(buffer, 32);
    /* Detect switch_off case */
    if(switch_off == 0)
    	buffer[0] = UE_DETACH;
    else
    	buffer[0] = UE_SWITCH_OFF_DETACH;
    msg = (idle_msg *)(buffer+1);
    /* MSIN */
	memcpy(msg->msin, ue.msin, 10);

	/* Send UE info to eNB and wait to a response */
	write(sockfd, buffer, sizeof(idle_msg) + 2);

	/* Receive eNB response */
	n = read(sockfd, buffer, sizeof(buffer));
	if(n < 0)
	{
		perror("UE recv");
		close(sockfd);
		return 1;
	}
	if(buffer[0] != (OK_CODE | UE_DETACH) && buffer[0] != (OK_CODE | UE_SWITCH_OFF_DETACH))
	{
		printError("Error in remote eNB (UEDetach)\n");
		close(sockfd);
		return 1;
	}
	printOK("UE Detached (UEDetach)\n");
	/* Notify the controller */
	send_ue_detach_controller( (ue.id[0] << 24) | (ue.id[1] << 16) | (ue.id[2] << 8) | ue.id[3] );
	/* Here the UE is in IDLE state */
	close(sockfd);
	return 0;
}

int send_ue_attach()
{
	int sockfd;
	idle_msg * msg;
	int n;
	uint8_t buffer[256];
	init_response_msg * res;

	/* Socket create */
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) {
    	perror("UE socket");
        return 1;
    }

	/* Connect to eNB */
	if (connect(sockfd, (struct sockaddr *) &enb_addr, sizeof(enb_addr)) != 0) { 
        perror("UE connect");
        return 1; 
    }

    /* Build eNB message (IDLE_CODE + MSIN) */
    bzero(buffer, 256);
    buffer[0] = UE_ATTACH;
    msg = (idle_msg *)(buffer+1);
    /* MSIN */
	memcpy(msg->msin, ue.msin, 10);

	/* Send UE info to eNB and wait to a response */
	write(sockfd, buffer, sizeof(idle_msg) + 2);

	/* Receive eNB response */
	n = read(sockfd, buffer, sizeof(buffer));
	if(n < 0)
	{
		perror("UE recv");
		close(sockfd);
		return 1;
	}
	if(buffer[0] != (OK_CODE | UE_ATTACH))
	{
		printError("Error in remote eNB (UEAttach)\n");
		close(sockfd);
		return 1;
	}
	printOK("UE Attached (UEAttach)\n");

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
	printf("Received new information from eNB\n");
	printf("New GTP-TEID: 0x%x\n", teid);
	printf("New UE IP: %d.%d.%d.%d\n", ue_ip[0], ue_ip[1], ue_ip[2], ue_ip[3]);
	printf("New SPGW IP: %d.%d.%d.%d\n", spgw_ip[0], spgw_ip[1], spgw_ip[2], spgw_ip[3]);

	/* Update data plane */
	if(update_data_plane(ue_ip, spgw_ip, teid) < 0)
	{
		printError("Error updating data plane parameters\n");
		close(sockfd);
		return 1;
	}

	/* Run the traffic generator process */
	start_traffic_generator(ue.command);

	/* Notify the controller */
	send_ue_attach_controller( (ue.id[0] << 24) | (ue.id[1] << 16) | (ue.id[2] << 8) | ue.id[3] );
	/* Here the UE is in IDLE state */
	close(sockfd);
	return 0;
}

void do_control_plane_action(uint8_t action)
{
	switch(action)
	{
		case CP_INIT:
			/* Do nothing */
			return;
		case CP_DETACH:
			send_ue_detach(0);
			return;
		case CP_DETACH_SWITCH_OFF:
			send_ue_detach(1);
			return;
		case CP_ATTACH:
			send_ue_attach();
			return;
		case CP_MOVE_TO_IDLE:
			send_ue_context_release();
			return;
		case CP_MOVE_TO_CONNECTED:
			/* TODO: Not implemented */
			return;
		case CP_HANDOVER:
			/* TODO: Not implemented */
			return;
	}
}

void * control_plane(void * args)
{
	int i;
	uint32_t cp_sleep;

	/* Create traffic generator process */
	start_traffic_generator(ue.command);
	
	/* Infinite loop */
	while(1)
	{
		/* control plane behaviour */
		for(i = 0; i < ue.control_plane_len; i += 4)
		{
			/* Do the action */
			do_control_plane_action(ue.control_plane[i]);
			/* Sleep */
			cp_sleep = (ue.control_plane[i+1] << 16) | (ue.control_plane[i+2] << 8) | (ue.control_plane[i+3]);
			if(cp_sleep == 16777215)
			{
				printInfo("Control plane: INF sleep detected\n");
				return NULL;
			}
			printInfo("Sleeping %d seconds\n", cp_sleep);
			sleep(cp_sleep);
		}
	}
}

int ue_emulator_start(ue_data * data)
{
	int sockfd;
	uint8_t buffer[1024];
	init_msg * msg;
	init_response_msg * res;
	int n;
	int i;
	int ret;
	pthread_t control_plane_thread;

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
	printf("Control plane behaviour (%d): ", data->control_plane_len);
	for(i = 0; i < data->control_plane_len; i++)
		printf("%.2x ", data->control_plane[i]);
	printf("\n");

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
		printError("Error in remote eNB\n");
		close(sockfd);
		return 1;
	}

	close(sockfd);

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

    

    /* TESTING */
    //printf("Sleeping for 5 seconds\n");
    //sleep(10);
    //printf("UE_DETACH\n");
    //send_ue_detach(0);
    //sleep(10);
    //printf("UE_ATTACH\n");
    //send_ue_attach();
    //sleep(10);
    //printf("UE_DETACH with Switch Off flag\n");
    //send_ue_detach(1);
    //sleep(10);
    //printf("UE_ATTACH\n");
    //send_ue_attach();


    /* Create data plane thread */
    if (pthread_create(&control_plane_thread, NULL, control_plane, 0) != 0)
    {
        perror("pthread_create control_plane_thread");
        return 1;
    }
    /*

	while True:
		sleep X
		move to idle
		sleep Y
		move to connected

    */



	return 0;
}
