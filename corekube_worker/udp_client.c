
// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "s1ap/asn1c/asn_system.h"
#include "core/include/core_lib.h"
#include "core/include/3gpp_types.h"

#define PORT    5566 
#define MAXLINE 1024 

void dumpTheMessage(uint8_t * message, int len)
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
  
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in     servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n, len; 

    char*payload=
        "0f0a0c0e"
        "00110037000004003b00080002f83900"
        "00e000003c40140880654e425f457572"
        "65636f6d5f4c5445426f780040000700"
        "00004002f8390089400140";
    char hexbuf[MAX_SDU_LEN];
    CORE_HEX(payload, strlen(payload), hexbuf);

    dumpTheMessage(hexbuf, strlen(payload) / 2);
      
    sendto(sockfd, (const char *)hexbuf, strlen(payload) / 2, 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Message sent.\n"); 
          
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 
    printf("Received message from server.\n"); 
    dumpTheMessage(buffer, n);
  
    close(sockfd); 
    return 0; 
}
