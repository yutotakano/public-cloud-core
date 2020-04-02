#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <libgc.h>
#include <time.h>
#include <sys/socket.h>

#include "ue.h"
#include "enb.h"
#include "s1ap.h"
#include "log.h"
#include "sctp.h"

#define S1_U_PORT 2152

int open_data_plane_socket(eNB * enb)
{
    int sockfd;
    struct sockaddr_in client_addr;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return 1; 
    }

    /* Binding */
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(S1_U_PORT);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(client_addr.sin_zero),8);
    if (bind(sockfd,(struct sockaddr *)&client_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind");
        return 1;
    }

    set_spgw_socket(enb, sockfd);

    return 0;
}

uint16_t ICMPChecksum(uint16_t *icmph, int len)
{
    
    uint16_t ret = 0;
    uint32_t sum = 0;
    uint16_t odd_byte;
    
    while (len > 1) {
        sum += *icmph++;
        len -= 2;
    }
    
    if (len == 1) {
        *(uint8_t*)(&odd_byte) = * (uint8_t*)icmph;
        sum += odd_byte;
    }
    
    sum =  (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    ret =  ~sum;
    
    return ret; 
}

void send_icmp(UE * ue, eNB * enb, uint8_t * ip_dest)
{
    /********************/
    /* TEST: Data plane */
    /********************/
    struct sockaddr_in serv_addr;
    uint8_t icmp[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x30, 0x1c, 0x17, 0x40, 0x00, 0x40, 0x01, 0x9d, 0x36, 0xc0, 0xac, 0x00, 0x02, 0xc0, 0xac, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t data[] = {0xa3, 0x27, 0x83, 0x5e, 0x00, 0x00, 0x00, 0x00, 0x77, 0x2d, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};

    /* Generate ICMP */
    icmp[28] = 0x08;
    icmp[29] = 0x00;
    icmp[30] = 0x00;
    icmp[31] = 0x00;
    icmp[32] = 0x17;
    icmp[33] = 0xbc;
    icmp[34] = 0x00;
    icmp[35] = 0x01;
    memcpy(icmp + 36, (void *)data, 20);
    *((uint16_t*)(icmp+30)) = ICMPChecksum((uint16_t *) (icmp + 28), 28);


    /* Generate GTP Header */
    icmp[0] = 0x30; /* Flags */
    icmp[1] = 0xFF; /* T-PDU */
    icmp[2] = 0x00; /* Length */
    icmp[3] = 0x30; /* Length */
    /* GTP Teid */
    uint32_t teid = get_gtp_teid(ue);
    icmp[4] = (teid >> 24) & 0xFF;
    icmp[5] = (teid >> 16) & 0xFF;
    icmp[6] = (teid >> 8) & 0xFF;
    icmp[7] = (teid) & 0xFF;

    /* Replace src IP*/
    memcpy(icmp + 20, get_pdn_ip(ue), 4);
    /* Replace dst IP*/
    memcpy(icmp + 24, ip_dest, 4);

    // Filling server information
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &serv_addr.sin_addr.s_addr, get_spgw_ip(ue), sizeof(struct in_addr));
      
    sendto(get_spgw_socket(enb), icmp, 56, 0, (const struct sockaddr *) &serv_addr,  sizeof(serv_addr));
    printf("ICMP sent!\n");
}

int main(int argc, char const *argv[])
{
	const char * mme_ip;
    const char * enb_ip;
    int err;

    eNB * enb;
    UE * ue1;
    UE * ue2;
    uint8_t key_oai[] = {0x3f, 0x3f, 0x47, 0x3f, 0x2f, 0x3f, 0xd0, 0x94, 0x3f, 0x3f, 0x3f, 0x3f, 0x09, 0x7c, 0x68, 0x62};
    uint8_t op_key_oai[] = {0xdd, 0xaf, 0x9e, 0x16, 0xc4, 0x03, 0x72, 0x8e, 0xd1, 0x52, 0xb6, 0x9d, 0xb0, 0x37, 0x2e, 0xa1};


    /**********************/
    /* GCLib configuration*/
    /**********************/
    /* GC Logs */
    GC_enable_logs();
    /* GC Backtrace */
    //GC_enable_backtrace();


    if(argc != 3)
    {
        printf("USE: ./lte_client_simulator <MME_IP> <ENB_IP>\n");
        exit(1);
    }


    /* Getting parameters */
    mme_ip = argv[1];
    enb_ip = argv[2];



    ue1 = init_UE("208", "93", "0000000001", key_oai, op_key_oai);
    ue2 = init_UE("208", "93", "0000000002", key_oai, op_key_oai);
    enb = init_eNB(0x00e00000, "208", "93", enb_ip);


    /***************/
    /* Connect eNB */
    /***************/
    /* Open SCTP connection */
    sctp_connect_enb_to_mme(enb, mme_ip);
    /* Connect eNB to MME */
    err = procedure_S1_Setup(enb);
    if(err == 1)
    {
    	printError("S1 Setup problems\n");
    }
    else
    {
    	printOK("eNB successfully connected\n");
    }

    /* Attach UE 1 */
    err = procedure_Attach_Default_EPS_Bearer(enb, ue1);
    sleep(5);

    /* Attach UE 2 */
    err = procedure_Attach_Default_EPS_Bearer(enb, ue2);

    open_data_plane_socket(enb);
    uint8_t dest_ip[] = {8, 8, 8, 8};
    while(1)
    {
        send_icmp(ue1, enb, dest_ip);
        sleep(2);
    }

    free_eNB(enb);
    free_UE(ue1);
    free_UE(ue2);
	return 0;
}