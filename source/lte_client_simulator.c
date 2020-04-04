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

#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_tunnel.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#include "ue.h"
#include "enb.h"
#include "s1ap.h"
#include "log.h"
#include "sctp.h"
#include "tun.h"
#include "data_plane.h"

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

void send_movidas(UE * ue, eNB * enb, uint8_t * ip_dest)
{
    /********************/
    /* TEST: Data plane */
    /********************/
    struct sockaddr_in serv_addr, cli_addr;
    char * tun_name;
    uint8_t msg[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t buffer[1024];

    /* Create the TUN interface */
    tun_name = get_tun_name(ue);
    int tunfd = open_tun_iface(tun_name, get_pdn_ip(ue));
    /* Configure TUN file descriptor as non-blocking */
    //int flags = fcntl(tunfd, F_GETFL, 0);
    //if(fcntl(tunfd, F_SETFL, flags | O_NONBLOCK))
    //{
    //    perror("O_NONBLOCK");
    //    return;
    //}

    sleep(10);

    /* Open a socket on the TUN interface */
    int sockfd;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("tun socket creation failed"); 
        return; 
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));
    /* Force to use TUN device */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", tun_name);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(struct ifreq)) < 0) {
        perror("tun bind");
        return;
    }

    // Filling server information
    uint8_t dest_ip[] = {192, 172, 0, 1};
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &serv_addr.sin_addr.s_addr, dest_ip, sizeof(struct in_addr));

    /* Read from the tun interface */
    bzero(buffer, 1024);
    int len = 0;
    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    int p = 0;
    int retval;

    //while(1)
    //{
    //    if(p == 10)
    //    {
    //        sendto(sockfd, msg, 4, 0, (const struct sockaddr *) &serv_addr,  sizeof(serv_addr));
    //        printf("MESSAGE SENT!\n");
    //        p = 0;
    //    }
    //    FD_ZERO(&fds);
    //    FD_SET(tunfd, &fds);
    //    retval = select(tunfd+1, &fds, 0, 0, &timeout);
    //    if (retval == -1){
    //        perror("select()");
    //    }
    //    else if (retval){
    //        len = tun_read(tunfd, buffer, 120);
    //        printf("LA MOVIDA\n");
    //        dump_Memory(buffer, len);
    //    }
    //    else
    //        printf("NADA\n"); 
    //    sleep(1);
    //    p++;
    //}
    int len1 = sizeof(cli_addr);
    uint8_t buf[256];
    uint8_t inter;
    while(1)
    {
        if(p == 3)
        {
            sendto(sockfd, msg, 4, 0, (const struct sockaddr *) &serv_addr,  sizeof(serv_addr));
            printf("MESSAGE SENT!\n");
            p = 0;
        }
        len = tun_read(tunfd, buffer, 120);
        printf("LA MOVIDA\n");
        //dump_Memory(buffer, len);
        if(buffer[0] == 0x45)
        {
            printf("UDP Received\n");
            inter = buffer[15];
            buffer[15] = buffer[19];
            buffer[19] = inter;
            tun_write(tunfd, buffer, len);
            sleep(1);
            printf("AQUI\n");
            len = recvfrom(sockfd, (char *)buf, 256, 0, ( struct sockaddr *) &cli_addr, (socklen_t *)&len1);
            printf("RECV: \n");
            //dump_Memory(buf, len);
            sleep(2);
        }
        p++;
    }
    while(1)
    {
        if(p == 10)
        {
            sendto(sockfd, msg, 4, 0, (const struct sockaddr *) &serv_addr,  sizeof(serv_addr));
            printf("MESSAGE SENT!\n");
            p = 0;
            sleep(1);
        }
        len = tun_read(tunfd, buffer, 120);
        if(len <= 0 && errno == EAGAIN) {
            // If this condition passes, there is no data to be read
            printf("NADA\n");
        }
        else if(len > 0) {
            // Otherwise, you're good to go and buffer should contain "count" bytes.
            printf("LA MOVIDA\n");
            //dump_Memory(buffer, len);
        }
        else {
            // Some other error occurred during read.
            printf("ERROR\n");
        }
        p++;
    }
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

    /* Attach UE 2 */
    //err = procedure_Attach_Default_EPS_Bearer(enb, ue2);

    //open_data_plane_socket(enb);

    uint8_t dest_ip[] = {192, 172, 0, 1};
    int dest_port = 1234;

    /* DATA PLANE */
    open_enb_data_plane_socket(enb);
    setup_ue_data_plane(ue1);
    printf("MOVIDON\n");
    generate_udp_traffic(ue1, enb, dest_ip, dest_port);

    /* TESTING */
    //send_movidas(ue1, enb, dest_ip);

    free_eNB(enb);
    free_UE(ue1);
    free_UE(ue2);
	return 0;
}