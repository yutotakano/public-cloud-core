#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"

#include "s1ap_handler.h"
#include "core/include/core_general.h"

#define MME_LISTEN_PORT 5566
#define BUFFER_LEN 1024


void dumpMessage(uint8_t * message, int len)
{
	int i;
	printf("(%d)\n", len);
	for(i = 0; i < len; i++)
	{
		if( i % 16 == 0)
			printf("\n");
		printf("%.2x ", message[i]);
	}
	printf("\n");
}

int configure_udp_socket(char * mme_ip_address)
{
	int sock_udp;
	struct sockaddr_in listener_addr, client_addr;

	/****************************/
	/* Initialise socket struct */
	/****************************/
	memset(&listener_addr, 0, sizeof(listener_addr));


	/*****************/
	/* Create socket */
	/*****************/
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_udp < 0) {
		perror("Socket UDP setup");
		return -1;
	}


	/***************************/
    /* Set up MME address */
    /***************************/
    listener_addr.sin_addr.s_addr = inet_addr(mme_ip_address); //INADDR_ANY;
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = htons(MME_LISTEN_PORT);
    memset(&(listener_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
    if (bind(sock_udp,(struct sockaddr*)&listener_addr, sizeof(listener_addr)) == -1) {
        perror("Bind MME socket");
        close(sock_udp);
        return -1;
    }

	return sock_udp;
}


void start_listener(char * mme_ip_address)
{
	int sock_udp;
	int flags = 0, n;
	socklen_t from_len;
	struct sockaddr_in client_addr;
	uint8_t buffer[BUFFER_LEN];

	/* Initialise socket structs */
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	from_len = (socklen_t)sizeof(struct sockaddr_in);

	/* Configure the socket */
	sock_udp = configure_udp_socket(mme_ip_address);
	if(sock_udp < 0) {
		printError("Error configuring UDP socket.\n");
		return;
	}
	printOK("UDP socket configured correctly.\n");

	while (1) {

		/* Wait to receive a message */
		n = recvfrom(sock_udp, (char *)buffer, BUFFER_LEN, MSG_WAITALL, ( struct sockaddr *) &client_addr, &from_len); 

		if (n == 0)
			break;
		
		dumpMessage(buffer, n);

		pkbuf_t *responseBuffer;

		S1AP_handle_outcome_t outcome = s1ap_handler_entrypoint(buffer+4, n-4, &responseBuffer);

		if (outcome == NO_RESPONSE)
			continue;

		uint8_t response[responseBuffer->len + 4];
		memcpy(response, buffer, 4);
		memcpy(response+4, responseBuffer->payload, responseBuffer->len);
		
		int ret = sendto(sock_udp, (void *)response, responseBuffer->len + 4,  
        	MSG_CONFIRM, (const struct sockaddr *) &client_addr, 
            from_len); 
    	
		pkbuf_free(responseBuffer);

		if (ret == -1) {
			printError("Failed to send UDP message.\n");
		}
		else {
			printf("Send %d bytes over UDP.\n", ret);
		}
	}

	/* Close the socket when done */
	close(sock_udp);

}


int main(int argc, char const *argv[])
{
	if(argc != 2) {
		printf("RUN: ./listener <MME_IP_ADDRESS>\n");
		return 1;
	}
	core_initialize();

	start_listener((char *)argv[1]);

	return 0;
}
