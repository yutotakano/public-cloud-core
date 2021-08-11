
// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#include "core/ogs-core.h"

#define MAXLINE 1024

int PORT;
  
// Driver code 
int send_message(char *mme_ip, char *payload, int numResponse) {

    int sockfd; 
    uint8_t buffer[MAXLINE]; 
    struct sockaddr_in     servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        ogs_error("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(mme_ip);
      
    uint n, len; 

    uint8_t hexbuf[OGS_MAX_SDU_LEN];
    OGS_HEX(payload, strlen(payload), hexbuf);

    ogs_log_hexdump(OGS_LOG_INFO, hexbuf, strlen(payload) / 2);
      
    sendto(sockfd, (const char *)hexbuf, strlen(payload) / 2, 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    ogs_info("Message sent"); 

    for (int i = 0; i < numResponse; i++) {
        ogs_info("Waiting for response %d of %d.", i, numResponse);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 
        ogs_info("Received message from server"); 
        ogs_log_hexdump(OGS_LOG_INFO, buffer, n);
    }
  
    close(sockfd); 
    return 0; 
}


void runMessage(char * ip_address, int message_number) {
    char *NGSetupRequest =
        "0f0a0c0e"
        "00150041000004001b00090002f83950"
        "00000001005240170a00554552414e53"
        "494d2d676e622d3230382d39332d3100"
        "66000d00000000010002f83900000008"
        "0015400140";
    
    char *RegistrationRequest = 
        "0f0a0c0e"
        "000f4048000005005500020001002600"
        "1a197e004179000d0102f83900000000"
        "00000000102e04f0f0f0f00079001350"
        "02f839000000010002f839000001e470"
        "9c5c005a4001200070400100";

    char *AuthenticationResponse =
        "0f0a0c0e"
        "002e4040000004000a00020001005500"
        "02000100260016157e00572d10d8971a"
        "6e962af4ec600566b2101c3535007940"
        "135002f839000000010002f839000001"
        "e4709c5c";

    switch (message_number) {
        case 1:
            send_message(ip_address, NGSetupRequest, 1);
            break;
        case 2:
            send_message(ip_address, RegistrationRequest, 1);
            break;
        case 3:
            send_message(ip_address, AuthenticationResponse, 1);
            break;
        default:
            ogs_info("Unknown message number %d", message_number);
    }
}

// Driver code
int main(int argc, char const *argv[]) {
    if(argc != 4) {
        printf("RUN: ./udp_client <AMF_IP_ADDRESS> <PORT> [MESSAGE_NUMBER=0]\n");
        return 1;
    }

    char * ip_address = (char *) argv[1];

    PORT = atoi(argv[2]);

    int message_number = atoi(argv[3]);

    if (message_number < 0) {
        for (int i = 1; i <= 9; i++)
            runMessage(ip_address, i);
        return 0;
    }
    runMessage(ip_address, message_number);

    return 0;
}