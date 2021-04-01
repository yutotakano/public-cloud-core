
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
#include "core/include/core_debug.h"
#include "core/include/3gpp_types.h"

#define PORT    5566 
#define MAXLINE 1024 
  
// Driver code 
int send_message(char *mme_ip, char *payload) {

    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in     servaddr; 
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        d_error("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr(mme_ip);
      
    uint n, len; 

    char hexbuf[MAX_SDU_LEN];
    CORE_HEX(payload, strlen(payload), hexbuf);

    d_print_hex(hexbuf, strlen(payload) / 2);
      
    sendto(sockfd, (const char *)hexbuf, strlen(payload) / 2, 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    d_info("Message sent"); 
          
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 
    d_info("Received message from server"); 
    d_print_hex(buffer, n);
  
    close(sockfd); 
    return 0; 
}

// Driver code
int main(int argc, char const *argv[]) {
    if(argc != 3) {
        printf("RUN: ./udp_client <MME_IP_ADDRESS> [MESSAGE_NUMBER=0]\n");
        return 1;
    }

    int message_number = atoi(argv[2]);

    char *S1SetupRequest =
        "0f0a0c0e"
        "00110037000004003b00080002f83900"
        "00e000003c40140880654e425f457572"
        "65636f6d5f4c5445426f780040000700"
        "00004002f8390089400140";

    char *InitualUEMessage =
        "0f0a0c0e"
        "000c405c000005000800048006692d00"
        "1a003231074171082980390000000010"
        "02802000200201d011271a8080211001"
        "00001081060000000083060000000000"
        "0d00000a00004300060002f839000100"
        "6440080002f83900e000000086400130";

    char *UplinkNASTransport =
        "0f0a0c0e"
        "000d403a00000500000005c0deadbeee"
        "000800048006692d001a000c0b075308"
        "5206154f10536d29006440080002f839"
        "00e00000004340060002f8390001";
    
    char *SecurityModeComplete =
        "0f0a0c0e"
        "000d4034000005000000020001000800"
        "048006692d001a000908475ab132f300"
        "075e006440080002f83900e000000043"
        "40060002f8390001";

    char *InitialContextSetupResponse = 
        "0f0a0c0e"
        "20090024000003000040020001000840"
        "048006692d0033400f000032400a0a1f"
        "c0a83866ca6fe0dd";
    
    char *AttachComplete = 
        "0f0a0c0e"
        "000d4039000005000000020001000800"
        "048006692d001a000e0d276a60cd5501"
        "074300035200c2006440080002f83900"
        "e00000004340060002f8390001";

    char *DetachRequest =
        // the follow detach request is encapsulated
        // within a UplinkNASTransport message, and is
        // taken from oai.pcap (Jon's RAN emulator)
        "0f0a0c0e"
        "000d"
        "40410000050000000200010008000480"
        "000010001a00161527bfb377f7020745"
        "010bf602f839000401"
        "00000001" // this is the TMSI
        "006440"
        "080002f83900e00000004300060002f8"
        "390001";
        // an (unsupported) alternative detach request,
        // encapsulated within an InitialUEMessage
        // (which does not contain the MME_UE_ID, so
        // struggles with accessing the DB to deprotect
        // the NAS-PDU)
        /*"000c"
        "403e000005000800020004001a001615"
        "1705a3e7ac0a0745010bf600f16004d0"
        "47"
        "00000001" // this is the TMSI
        "004300060000f160000a00"
        "6440080000f160070801500086400130"
        "0000";*/


    switch (message_number) {
        case 1:
            send_message((char *)argv[1], S1SetupRequest);
            break;
        case 2:
            send_message((char *)argv[1], InitualUEMessage);
            break;
        case 3:
            send_message((char *)argv[1], UplinkNASTransport);
            break;
        case 4:
            send_message((char *)argv[1], SecurityModeComplete);
            break;
        case 5:
            send_message((char *)argv[1], InitialContextSetupResponse);
            break;
        case 6:
            send_message((char *)argv[1], AttachComplete);
            break;
        case 7:
            send_message((char *)argv[1], DetachRequest);
            break;
        default:
            send_message((char *)argv[1], S1SetupRequest);
            send_message((char *)argv[1], InitualUEMessage);
            send_message((char *)argv[1], UplinkNASTransport);

    }

    return 0;
}
