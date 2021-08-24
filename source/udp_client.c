
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

    char *SecurityModeComplete =
        "0f0a0c0e"
        "002e4067000004000a00020001005500"
        "0200010026003d3c7e049def51f3007e"
        "005e7700094573806121856151f17100"
        "237e004179000d0102f8390000000000"
        "000000101001002e04f0f0f0f02f0201"
        "01530100007940135002f83900000001"
        "0002f839000001e4709c5c";

    char *InitialContextSetupResponse =
        "0f0a0c0e"
        "200e000f000002000a40020001005540"
        "020001";

    char *RegistrationComplete =
        "0f0a0c0e"
        "002e4035000004000a00020001005500"
        "0200010026000b0a7e0204933826017e"
        "0043007940135002f839000000010002"
        "f839000001e4709c5c";

    char *PDUSessionEstablishmentRequest =
        "0f0a0c0e"
        "002e405e"
        "000004000a0002000100550002000100"
        "260034337e0252ca493e027e00670100"
        "152e0101c1ffff91a12801007b000780"
        "000a00000d0012018122010125090869"
        "6e7465726e6574007940135002f83900"
        "0000010002f839000001e4709c5c";

    char *PDUSessionResourceSetupResponse =
        "0f0a0c0e"
        "201d0024000003000a40020001005540"
        "020001004b40110000010d0003e0c0a8"
        "386f000000010001";

    char *DetachRequest =
        "0f0a0c0e"
        "002e4043000004000a00020001005500"
        "02000100260019187e01f867c361037e"
        "004501000bf202f83902018000000001"
        "007940135002f839000000010002f839"
        "000001e4cb957900";

    char *UEContextReleaseComplete =
        "0f0a0c0e"
        "20290026000003000a40020001005540"
        "020001007940135002f8390000000100"
        "02f839000001e4cb95790000";

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
        case 4:
            send_message(ip_address, SecurityModeComplete, 1);
            break;
        case 5:
            send_message(ip_address, InitialContextSetupResponse, 0);
            break;
        case 6:
            send_message(ip_address, RegistrationComplete, 0);
            break;
        case 7:
            send_message(ip_address, PDUSessionEstablishmentRequest, 1);
            break;
        case 8:
            send_message(ip_address, PDUSessionResourceSetupResponse, 0);
            break;
        case 9:
            send_message(ip_address, DetachRequest, 2);
            break;
        case 10:
            send_message(ip_address, UEContextReleaseComplete, 0);
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