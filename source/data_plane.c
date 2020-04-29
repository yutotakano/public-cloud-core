#include "data_plane.h"
#include "tun.h"
#include "log.h"

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
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define S1_U_PORT 2152
#define MAX_TUN_DESCRIPTORS 32
#define BUFFER_DATA_LEN 2048
#define GTP_FLAGS 0x30
#define GTP_MESSAGE_TYPE 0xFF

UE * tun_descriptors[MAX_TUN_DESCRIPTORS];
pthread_mutex_t lock;

typedef struct _gtp_header
{
    uint8_t flags;
    uint8_t message_type;
    uint16_t len;
    uint32_t teid;
} gtp_header;

typedef struct _udp_generator
{
    eNB * enb;
    UE * ue;
    uint8_t * dest_ip;
    int port_dest;
} udp_generator;

/* Global variables */
int tunfd;
char tun_name[32];
uint8_t ue_ip[4];
struct sockaddr_in spgw_addr;
uint32_t gtp_teid;
int sockfd;
pthread_t uplink_t;
pthread_t downlink_t;


uint32_t analyze_gtp_header(uint8_t * buf, int * len)
{
    gtp_header * h;
    h = (gtp_header *) buf;
    if((h->flags & 0xF0) == GTP_FLAGS && h->message_type == GTP_MESSAGE_TYPE)
    {
        *len = ntohs(h->len);
        return h->teid;
    }
    return -1;
}

void generate_gtp_header(uint8_t * header, uint32_t teid, int len)
{
    gtp_header * h;
    h = (gtp_header *) header;
    h->flags = GTP_FLAGS;
    h->message_type = GTP_MESSAGE_TYPE;
    h->len = htons((uint16_t)(len));
    h->teid = htonl(teid);
}


void * uplink_thread(void * args)
{
    uint8_t buffer[BUFFER_DATA_LEN];
    int len;

    /* Async read */
    fd_set fds;
    struct timeval timeout;
    int retval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    printOK("Uplink thread created\n");

    while(1)
    {
        while(1)
        {
            FD_ZERO(&fds);
            FD_SET(tunfd, &fds);
            retval = select(tunfd+1, &fds, 0, 0, &timeout);
            if (retval == -1){
                perror("select()");
            }
            else if (retval)
            {
                /* Reuse of the buffer with enough space for the GTP header */
                len = tun_read(tunfd, buffer+8, BUFFER_DATA_LEN); /* We call read to clean the buffer from external traffic */
                generate_gtp_header(buffer, gtp_teid, len);
                break;
            }
        }
        /* Send tho the SPGW */
        sendto(sockfd, buffer, len+8, 0, (const struct sockaddr *) &spgw_addr,  sizeof(spgw_addr));
    }
}

void * downlink_thread(void * args)
{
    uint8_t buffer[BUFFER_DATA_LEN];
    int sockaddr_len = sizeof(spgw_addr);
    int packet_len;

    /* Async read */
    fd_set fds;
    struct timeval timeout;
    int retval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    printOK("Downlink thread created\n");

    while(1)
    {
        while(1)
        {
            FD_ZERO(&fds);
            FD_SET(sockfd, &fds);
            retval = select(sockfd+1, &fds, 0, 0, &timeout);
            if (retval == -1){
                perror("select error");
            }
            else if(retval)
            {
                recvfrom(sockfd, buffer, BUFFER_DATA_LEN, 0, ( struct sockaddr *) &spgw_addr, (socklen_t *)&sockaddr_len);
                break;
            }
        }
        /* Detect if buffer contains a GTP packet */
        if(gtp_teid != analyze_gtp_header(buffer, &packet_len))
        {
            /* Write the packet in the TUN device associated with the packet TEID */
            tun_write(tunfd, buffer + 8, packet_len);
        }
    }
    return NULL;
}

int start_data_plane(uint8_t * local_ip, char * msin, uint8_t * _ue_ip, uint8_t * _spgw_ip, uint32_t _gtp_teid)
{
    struct sockaddr_in ue_addr;


    /* Generate TUN name */
    sprintf(tun_name, "tun%d", atoi(msin));
    /* Store parameters */
    memcpy(ue_ip, _ue_ip, 4);
    gtp_teid = _gtp_teid;

    /* Configure SPGW address */
    memset(&spgw_addr, 0, sizeof(spgw_addr));
    spgw_addr.sin_family = AF_INET; 
    spgw_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &spgw_addr.sin_addr.s_addr, _spgw_ip, sizeof(struct in_addr));

    /* Create TUN device */
    tunfd = open_tun_iface(tun_name, ue_ip);
    if(tunfd == -1)
    {
        printError("Create TUN device error\n");
        return 1;
    }

    /* Creating socket file descriptor  */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    { 
        perror("data plane socket creation failed");
        tun_close(tunfd);
        return 1; 
    }

    /* Binding */
    ue_addr.sin_family = AF_INET;
    ue_addr.sin_port = htons(S1_U_PORT);
    memcpy((void *)&ue_addr.sin_addr.s_addr, local_ip, sizeof(struct sockaddr));
    bzero(&(ue_addr.sin_zero),8);
    if (bind(sockfd,(struct sockaddr *)&ue_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind eNB");
        return 1;
    }

    /* Create uplink thread */
    if (pthread_create(&uplink_t, NULL, uplink_thread, 0) != 0)
    {
        perror("pthread_create uplink_thread");
        close(sockfd);
        tun_close(tunfd);
        return 1;
    }
    /* Create downlink thread */
    if (pthread_create(&downlink_t, NULL, downlink_thread, 0) != 0)
    {
        perror("pthread_create uplink_thread");
        if(pthread_cancel(uplink_t) != 0)
            perror("pthread_cancel error");
        close(sockfd);
        tun_close(tunfd);
        return 1;
    }

    /* Return */
    return 0;
}
