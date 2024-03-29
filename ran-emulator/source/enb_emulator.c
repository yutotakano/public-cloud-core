#include "enb_emulator.h"
#include "enb.h"
#include "sctp.h"
#include "s1ap.h"
#include "ngap.h"
#include "log.h"
#include "ue.h"
#include "message.h"
#include "list.h"

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

#define CLIENT_NUMBER 256
#define ENB_PORT 2233
#define X2_ENB_PORT 2234
#define X2_OK 0
#define X2_ERROR 1

/* GLOBAL VARIABLES */
eNB * enb;
pthread_t enb_thread;
int sockfd;
List * list;

void send_enb_behaviour(uint32_t ue_id);
void send_enb_behaviour_error(uint32_t ue_id);

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

uint32_t id_to_uint32(uint8_t * id)
{
	uint32_t ret;
	ret = (id[0] << 24) | (id[1] << 16) | (id[2] << 8) | id[3];
	return ret;
}

int analyze_ue_msg(int client, uint8_t * buffer, int len, uint8_t * response, int * response_len, uint8_t * ue_ip)
{
	init_msg * initmsg = NULL;
	idle_msg * idlemsg = NULL;
	ho_msg * homsg;
	init_response_msg * res;
	idle_response_msg * idle_res;
	UE * ue = NULL;
	uint8_t switch_off;
	int target_sockfd, n;
	struct sockaddr_in target_enb;
	uint8_t buffer_ho[1024];
	int s1_sockfd;
	uint8_t sync_resp;

	printf("\n\n\n");
	printInfo("Analyzing new request...\n");

	/* UE connecting to the EPC */
	if(buffer[0] == INIT_CODE)
	{
		printInfo("UE INIT message received\n");
		initmsg = (init_msg *)(buffer+1);
		/* Security checkings */
		/* Add 0 to the end of every string */
		initmsg->mcc[3] = 0;
		initmsg->mnc[2] = 0;
		initmsg->msin[10] = 0;
		/* Create UE */
		ue = init_UE((char *)initmsg->mcc, (char *)initmsg->mnc, (char *)initmsg->msin, initmsg->key, initmsg->op_key, initmsg->ue_ip);

		printInfo("Creating new UE: %s-%s-%s\n", (char *)initmsg->mcc, (char *)initmsg->mnc, (char *)initmsg->msin);

		/* Add to list structure */
		if(addElem(list, (void *)ue))
		{
			printError("Error adding UE (%s)\n", (char *)initmsg->msin);
			free_UE(ue);
			/* Generate UE Error response */
            response[0] = buffer[0]; /* Without OK_CODE */
            *response_len = 1;
            return 1;
		}
		#ifdef _5G
		/* Attach UE */
		if(procedure_Registration_Request(enb, ue))
		{
			printError("Move to Attached (UERegistrationRequest) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		#else
		/* Attach UE 1 */
	    if(procedure_Attach_Default_EPS_Bearer(enb, ue))
    	{
    		printError("Attach and Setup default bearer error\n");
    		free_UE(ue);
    		/* Generate UE Error response */
    		response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
    	}
    	#endif

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
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);

		#ifdef _5G
		printWarning("5G MOVE_TO_IDLE event not implemented.\n");
		#else
		/* Move UE to idle */
		if(procedure_UE_Context_Release(enb, ue))
		{
			printError("Move to Idle (UEContextRelease) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		#endif
		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;
	}
	else if(buffer[0] == UE_DETACH || buffer[0] == UE_SWITCH_OFF_DETACH)
	{
		printInfo("UE UE_DETACH message received\n");

		switch_off = buffer[0] & UE_SWITCH_OFF_DETACH;

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);
		#ifdef _5G
		if(procedure_Deregistration_Request(enb, ue, switch_off))
		{
			printError("Move to Detached (UEDeregister) error\n");
			response[0] = 0;
			*response_len = 1;
			return 1;
		}
		#else
		/* Detach UE */
		if(procedure_UE_Detach(enb, ue, switch_off))
		{
			printError("Move to Detached (UEDetach) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		#endif
		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;
	}
	else if(buffer[0] == UE_ATTACH)
	{
		printInfo("UE UE_ATTACH message received\n");

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);
		#ifdef _5G
		/* Attach UE */
		if(procedure_Registration_Request(enb, ue))
		{
			printError("Move to Attached (UERegistrationRequest) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		#else
		/* Attach UE */
		if(procedure_Attach_Default_EPS_Bearer(enb, ue))
		{
			printError("Move to Attached (UEAttach) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
    		*response_len = 1;
    		return 1;
		}
		#endif
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
	else if(buffer[0] == MOVE_TO_CONNECTED)
	{
		printInfo("UE MOVE_TO_CONNECTED message received\n");

		idlemsg = (idle_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);

		#ifdef _5G
		printWarning("5G MOVE_TO_CONNECTED event not implemented.\n");
		#else
		/* Detach UE */
		if(procedure_UE_Service_Request(enb, ue, get_ue_ip(ue)))
		{
			printError("Move to Attached (UEServiceRequest) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
	    		*response_len = 1;
    			return 1;
		}
		#endif

		/* Setting up Response */
		response[0] = OK_CODE | buffer[0];
		/* idle_response_msg struct reused */
		idle_res = (idle_response_msg *) (response+1);
		uint32_t teid = get_gtp_teid(ue);
		idle_res->teid[0] = (teid >> 24) & 0xFF;
		idle_res->teid[1] = (teid >> 16) & 0xFF;
		idle_res->teid[2] = (teid >> 8) & 0xFF;
		idle_res->teid[3] = teid & 0xFF;
		memcpy(idle_res->spgw_ip, get_spgw_ip(ue), 4);
		*response_len = sizeof(idle_response_msg) + 1;
	}

	else if(buffer[0] == HO_SETUP)
	{
		printInfo("UE HO_SETUP message received\n");

		homsg = (ho_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);
		if(ue == NULL)
		{
			printError("Wrong UE MSIN descriptor in X2 Handover-Setup\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
    		return 1;
		}

		/* Open a UDP socket to connect with Target-eNB */
	    if ( (target_sockfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
	        perror("socket creation failed in X2 HO-Setup");
	        response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
	        return 1;
	    }

	    /* Configure Target-eNB address */
	    target_enb.sin_family = AF_INET;
	    target_enb.sin_port = htons(X2_ENB_PORT);
	    memcpy(&target_enb.sin_addr.s_addr, homsg->target_enb_ip, 4);
	    memset(&(target_enb.sin_zero), '\0', 8);

	    /* Store UE in the buffer */
	    memcpy(buffer_ho, (uint8_t *)ue, get_ue_size());

	    /* Send UE information to Target-eNB */
	    sendto(target_sockfd, buffer_ho, get_ue_size(), 0, (const struct sockaddr *) &target_enb, sizeof(target_enb));

	    /* Receive Target-eNB response */
		n = read(target_sockfd, buffer_ho, 1024);
		/* Close socket */
		close(target_sockfd);
		if(n < 0)
		{
			perror("Recv from Target-eNB");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
			return 1;
		}
		/* Analyze Target-eNB response */
		if(buffer[0] == X2_ERROR)
		{
			/* Error case */
			printError("X2 Handover-Setup in Target-eNB\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
			return 1;
		}

		/* Delete UE information in the list structure */
		free_UE(getElem(list, (void *)homsg->msin));

		printOK("X2 Handover-Setup done\n");

		/* Generate HO_OK message */
		response[0] = X2_OK;
    	*response_len = 1;
	}
	else if(buffer[0] == HO_REQUEST)
	{
		printInfo("UE HO_REQUEST message received\n");

		homsg = (ho_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);

		if(ue == NULL)
		{
			printError("Wrong UE MSIN descriptor in Handover-Request\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
    		return 1;
		}

		/* Call S1AP procedure and check return */
		if(procedure_UE_X2_handover(enb, ue))
		{
			printError("X2 Handover-Request error\n");
                        /* Generate UE Error response */
                        response[0] = X2_ERROR; /* X2 Error message */
                        *response_len = 1;
                        return 1;
		}

		printOK("X2 Handover-Request done\n");

		/* Generate HO_OK message */
		response[0] = X2_OK;
    	*response_len = 1;
	}
	else if(buffer[0] == UE_TRANFSER)
	{
		/* This procedure is identical to HO_SETUP */
		/* Response messages reused from HO procedures */
		printInfo("UE UE_TRANFSER message received\n");

		homsg = (ho_msg *)(buffer + 1);
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);
		if(ue == NULL)
		{
			printError("Wrong UE MSIN descriptor in UE Transfer\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
    		return 1;
		}

		/* Open a UDP socket to connect with Target-eNB */
	    if ( (target_sockfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
	        perror("socket creation failed in UE Transfer");
	        response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
	        return 1;
	    }

	    /* Configure Target-eNB address */
	    target_enb.sin_family = AF_INET;
	    target_enb.sin_port = htons(X2_ENB_PORT);
	    memcpy(&target_enb.sin_addr.s_addr, homsg->target_enb_ip, 4);
	    memset(&(target_enb.sin_zero), '\0', 8);

	    /* Store UE in the buffer */
	    memcpy(buffer_ho, (uint8_t *)ue, get_ue_size());

	    /* Send UE information to Target-eNB */
	    sendto(target_sockfd, buffer_ho, get_ue_size(), 0, (const struct sockaddr *) &target_enb, sizeof(target_enb));

	    /* Receive Target-eNB response */
		n = read(target_sockfd, buffer_ho, 1024);
		/* Close socket */
		close(target_sockfd);
		if(n < 0)
		{
			perror("Recv from Target-eNB");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
			return 1;
		}
		/* Analyze Target-eNB response */
		if(buffer[0] == X2_ERROR)
		{
			/* Error case */
			printError("UE Transfer in Target-eNB\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
			return 1;
		}

		/* Delete UE information in the list structure */
		free_UE(getElem(list, (void *)homsg->msin));

		printOK("UE Transfer done\n");

		/* Generate HO_OK message */
		response[0] = X2_OK;
    	*response_len = 1;
	}
	else if(buffer[0] == UE_S1_HANDOVER)
	{
		printInfo("UE UE_S1_HANDOVER message received\n");

		/* Get UE IMSI and Target-eNB IP */
		homsg = (ho_msg *)(buffer + 1);

		/* Get UE */
		ue = (UE *)peekElem(list, (void *)idlemsg->msin);
		if(ue == NULL)
		{
			printError("Wrong UE MSIN descriptor in S1 Handover-Request\n");
			response[0] = X2_ERROR; /* X2 Error message */
    		*response_len = 1;
    		return 1;
		}

		printInfo("Synchronizing with Target-eNB (%d.%d.%d.%d) ...\n", homsg->target_enb_ip[0], homsg->target_enb_ip[1], homsg->target_enb_ip[2], homsg->target_enb_ip[3]);

		/* Configure Target-eNB address */
	    target_enb.sin_family = AF_INET;
	    target_enb.sin_port = htons(ENB_PORT);
	    memcpy(&target_enb.sin_addr.s_addr, homsg->target_enb_ip, 4);
	    memset(&(target_enb.sin_zero), '\0', 8);

		/* To sync with Target-eNB, the UE channel is going to be used */
		/* Socket create */
	    s1_sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	    if (s1_sockfd == -1) {
	    	perror("UE socket");
	        return 1;
	    }

		/* Connect to eNB */
		if (connect(s1_sockfd, (struct sockaddr *) &target_enb, sizeof(target_enb)) != 0) { 
	        perror("eNB S1 connect");
	        close(s1_sockfd);
	        return 1; 
	    }
	    /* Sync message: Sync flag + UE */
	    buffer_ho[0] = S1_SYNC;
	    memcpy(buffer_ho+1, (uint8_t *)ue, get_ue_size());
	    /* Send Sync msg to Target-eNB and wait until sync ack msg (During this period, Source-eNB is going to be unavailable to attend other UEs) */
	    /* TODO: Can be solve by the use of a lock over the SCTP socket adn multithreading */
	    write(s1_sockfd, buffer_ho, get_ue_size() + 1);
	    /* Receive Target-eNB response */
		n = read(s1_sockfd, buffer_ho, 1);
		if(n < 0)
		{
			perror("eNB S1 recv (Sync)");
			close(s1_sockfd);
			return 1;
		}
		if(buffer_ho[0] != (OK_CODE | S1_SYNC))
		{
			printError("Error in Target-eNB (Sync)\n");
			close(s1_sockfd);
			return 1;
		}

		printInfo("Starting S1 Handover as Source-eNB...\n");

		/* Start S1AP S1 source-enb procedure */
		if(procedure_S1_Handover_Source_eNB(enb, ue))
		{
			printError("S1 Handover (Source-eNB) error\n");
			/* Generate UE Error response */
			response[0] = buffer[0]; /* Without OK_CODE */
	    	*response_len = 1;
	    	close(s1_sockfd);
    		return 1;
		}

		printOK("S1 Handover (Source-eNB)\n");

		/* Verify that Target-eNB has complete S1AP S1 Handover correctly */
		/* Receive eNB response */
		n = read(s1_sockfd, buffer_ho, 1);
		if(n < 0)
		{
			perror("eNB S1 recv (Sync 2)");
			close(s1_sockfd);
			/* Generate error message for the UE */
			response[0] = buffer[0]; /* Without OK_CODE */
	    	*response_len = 1;
			return 1;
		}
		if(buffer_ho[0] != S1_SYNC2)
		{
			printError("Error in Target-eNB (Sync 2)\n");
			close(s1_sockfd);
			/* Generate error message for the UE */
			response[0] = buffer[0]; /* Without OK_CODE */
	    	*response_len = 1;
			return 1;
		}

		printOK("S1 Handover completed successfully\n");

		/* Remove UE from eNB list structure */
		free_UE(getElem(list, (void *)homsg->msin));

		/* Generate OK response */
		response[0] = OK_CODE | buffer[0];
		*response_len = 1;
	}
	else if(buffer[0] == S1_SYNC)
	{
		printInfo("UE S1_SYNC message received\n");

		/* Get the UE from the message */
		ue = (UE *) buffer;

		sync_resp = OK_CODE | S1_SYNC;
		printInfo("Sending Sync message to Source-eNB...\n");
		/* Send SYNC_OK to the Source-eNB */
		write(client, &sync_resp, 1);

		/* Start S1AP S1 target-enb procedure */
		if(procedure_S1_Handover_Target_eNB(enb, ue))
		{
			printError("S1 Handover (Target-eNB) error\n");
			/* Generate UE Error response */
			response[0] = S1_ERROR; /* Without OK_CODE */
	    	*response_len = 1;
    		return 1;
		}

		printOK("S1 Handover (Target-eNB)\n");

		/* Add UE to Target-eNB list structure */
		addElem(list, (void *)ue);


		/* Send SYNC_2_OK to the Source-eNB */
		printInfo("Sending Sync 2 message to Source-eNB...\n");

		response[0] = S1_SYNC2;
    	*response_len = 1;
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

	/* Init UE list */
	list = initList(ue_compare);

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
		if(analyze_ue_msg(client, buffer, n, response, &response_len, (uint8_t *) &client_addr.sin_addr.s_addr))
		{
			printError("S1AP error\n");
		}
		/* Send response to client */
		write(client, response, response_len);
		/* Close socket */
		close(client);
	}
}

void * x2_thread(void * args)
{
	int x2sock;
	struct sockaddr_in enb_addr;
	struct sockaddr_in source_enb;
	uint8_t buffer[1024];
	int len, n;
	UE * recv_ue;
	uint8_t resp;
	/* Open a socket (global to be accessed from other functions)*/
    if ( (x2sock =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("X2 socket creation failed");
        printError("Exiting X2 thread...\n");
        return NULL;
    }
	/* Setup eNB address */
	enb_addr.sin_family = AF_INET;
	memcpy(&enb_addr.sin_addr.s_addr, get_enb_ip(enb), 4);
	enb_addr.sin_port = htons(X2_ENB_PORT);
	//LOCAL enb_addr.sin_port = htons(0);
	memset(&(enb_addr.sin_zero), '\0', 8);
	/* Bind it to a specific port */

	/* Binding process */
	if(bind(x2sock, (struct sockaddr *) &enb_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("X2 eNB bind");
		printError("Exiting X2 thread...\n");
		close(x2sock);
		return NULL;
	}

	/* Listen for UE transfer requests from other eNBs */
	while(1)
	{
		/* Receive UE */
	    bzero(buffer, 1024);

	    /* Receive eNB IP */
	    n = recvfrom(x2sock, (char *)buffer, 1024, 0, (struct sockaddr *) &source_enb, (uint32_t *)&len);
	    if(n < 0)
	    {
	        perror("X2 eNB recv");
			printError("Exiting X2 thread...\n");
			close(x2sock);
	        return NULL;
	    }
	    recv_ue = (UE *)buffer;
	    /* Add UE to list structure */
		if(addElem(list, (void *)recv_ue))
		{
			/* Error adding the UE to the list structure */
			resp = X2_ERROR;

		}
		else
		{
			/* UE added to the list structure successfully */
			resp = X2_OK;
		}
		/* Send response to Source-eNB */
		sendto(x2sock, &resp, 1, 0, (const struct sockaddr *) &source_enb, sizeof(source_enb));
	}

	close(x2sock);
	return NULL;
}

int enb_emulator_start(enb_data * data)
{
	struct sockaddr_in enb_addr;
	pthread_t x2_t;

	/* Start emulator thread */
	printf("Real value: 0x%.8x\n", id_to_uint32(data->id));
	printf("MCC: %s\n", data->mcc);
	printf("MNC: %s\n", data->mnc);
	printf("EPC IP: %d.%d.%d.%d\n", data->epc_ip[0], data->epc_ip[1], data->epc_ip[2], data->epc_ip[3]);

	/* Create eNB object */
	enb = init_eNB(id_to_uint32(data->id), (char *)data->mcc, (char *)data->mnc, data->enb_ip);
	if(enb == NULL)
	{
		send_enb_behaviour_error(id_to_uint32(data->id));
		return 1;
	}

	/***************/
    /* Connect eNB */
    /***************/
    /* Open SCTP connection */
    if(sctp_connect_enb_to_mme(enb, data->epc_ip) == 1)
    {
    	printError("SCTP Setup error\n");
    	send_enb_behaviour_error(get_enb_id(enb));
    	return 1;
    }

    /* Check whether this version is 4G or 5G */
    #ifdef _5G
    if(procedure_NG_Setup(enb) == 1)
    {
		printError("gNB NG Setup problems\n");
		send_enb_behaviour_error(get_enb_id(enb));
		return 1;
    }
    #else
    /* Connect eNB to MME */
    if(procedure_S1_Setup(enb) == 1)
    {
    	printError("eNB S1 Setup problems\n");
    	send_enb_behaviour_error(get_enb_id(enb));
    	return 1;
    }
    #endif
    printOK("eNB successfully connected\n");

    /* Open eNB port. In this port the eNB will receive UEs connections */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		perror("eNB socket");
		send_enb_behaviour_error(get_enb_id(enb));
		return 1;
	}

	/* Setup eNB address */
	enb_addr.sin_family = AF_INET;
	memcpy(&enb_addr.sin_addr.s_addr, data->enb_ip, 4);
	enb_addr.sin_port = htons(ENB_PORT);
	//LOCAL enb_addr.sin_port = htons(0);
	memset(&(enb_addr.sin_zero), '\0', 8);

	printInfo("Binding eNB to %d.%d.%d.%d:%d\n", data->enb_ip[0], data->enb_ip[1], data->enb_ip[2], data->enb_ip[3], ENB_PORT);

	/* Binding process */
	if(bind(sockfd, (struct sockaddr *) &enb_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("eNB bind");
		send_enb_behaviour_error(get_enb_id(enb));
		return 1;
	}

	/* Listen for UE's */
	if(listen(sockfd, CLIENT_NUMBER) == -1)
	{
		perror("eNB listen");
		send_enb_behaviour_error(get_enb_id(enb));
		return 1;
	}

	/* Initialize eNB thread */
	/* Create distributor thread */
    if (pthread_create(&enb_thread, NULL, enb_emulator_thread, 0) != 0)
    {
        perror("pthread_create enb_emulator_thread");
        send_enb_behaviour_error(get_enb_id(enb));
        return 1;
    }

    /* Start X2 interface thread */
	if (pthread_create(&x2_t, NULL, x2_thread, 0) != 0)
    {
        perror("pthread_create x2_thread");
        send_enb_behaviour_error(get_enb_id(enb));
        return 1;
    }

    send_enb_behaviour(get_enb_id(enb));

    /* Return back an wait for Controller messages */
	return 0;
}
