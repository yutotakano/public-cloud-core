#include "data_plane.h"
#include "tun.h"

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

void dump_Memory(uint8_t * pointer, int len)
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

int analyze_gtp_header(uint8_t * buf, int * len)
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

/* Distributor thread */
/* Reads from the eNB socket and analyze the GTP header to redirect the packet to the UE interface */
void * downlink_distributor(void * args)
{
    uint8_t buffer[BUFFER_DATA_LEN];
    struct sockaddr_in spgw_addr;
    int sockfd;
    int sockaddr_len = sizeof(spgw_addr);
    int len, packet_len, teid;
    eNB * enb = (eNB *) args;

    sockfd = get_spgw_socket(enb);

    while(1)
    {
        len = recvfrom(sockfd, buffer, BUFFER_DATA_LEN, 0, ( struct sockaddr *) &spgw_addr, (socklen_t *)&sockaddr_len);
        if(len < 0)
        {
            perror("recv downlink_distributor");
            return NULL;
        }
        /* Detect if buffer contains a GTP packet */
        teid = analyze_gtp_header(buffer, &packet_len);
        if(teid > -1)
        {
            /* Write the packet in the TUN device associated with the packet TEID */
            tun_write(get_tun_device(tun_descriptors[teid % MAX_TUN_DESCRIPTORS]), buffer + 8, packet_len);
        }
    }

    return NULL;
}

int open_enb_data_plane_socket(eNB * enb)
{
    int sockfd;
    struct sockaddr_in enb_addr;
    pthread_t downlink_thread;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return 1; 
    }

    /* Binding */
    enb_addr.sin_family = AF_INET;
    enb_addr.sin_port = htons(S1_U_PORT);
    memcpy((void *)&enb_addr.sin_addr.s_addr, get_enb_ip(enb), sizeof(struct sockaddr));
    bzero(&(enb_addr.sin_zero),8);
    if (bind(sockfd,(struct sockaddr *)&enb_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("Bind eNB");
        return 1;
    }

    set_spgw_socket(enb, sockfd);

    /* Start all threads and mutexes */
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Mutex error");
        return 1;
    }

    /* Create distributor thread */
    if (pthread_create(&downlink_thread, NULL, downlink_distributor, (void *) enb) != 0)
    {
        perror("pthread_create downlink_distributor");
        return 1;
    }

    return 0;
}

int setup_ue_data_plane(UE * ue)
{
    char * tun_name;
    int tunfd, sockfd;
    struct ifreq ifr;
    struct sockaddr_in ue_addr;

    /* Create the TUN interface */
    tun_name = get_tun_name(ue);
    tunfd = open_tun_iface(tun_name, get_pdn_ip(ue));

    /* Creating socket file descriptor  */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    { 
        perror("data plane socket creation failed"); 
        return 1; 
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    /* Force to use UE's TUN device */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", tun_name);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE,tun_name, IF_NAMESIZE) < 0) {
        perror("tun bind");
        close(sockfd);
        return 1;
    }

    /* Binding */
    //ue_addr.sin_family = AF_INET;
    //ue_addr.sin_port = htons(2048 + get_ue_id(ue));
    //memcpy((void *)&ue_addr.sin_addr.s_addr, get_pdn_ip(ue), sizeof(struct sockaddr));
    ////ue_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bzero(&(ue_addr.sin_zero),8);
    //if (bind(sockfd,(struct sockaddr *)&ue_addr, sizeof(struct sockaddr)) == -1)
    //{
    //    perror("Bind UE");
    //    return 1;
    //}


    set_tun_device(ue, tunfd);
    set_data_plane_socket(ue, sockfd);

    /* Store the UE in the hash table to get access to its TUN device in the downlink thread*/
    tun_descriptors[get_random_gtp_teid(ue) % MAX_TUN_DESCRIPTORS] = ue;

    return 0;
}

void generate_gtp_header(uint8_t * header, UE * ue, int len)
{
    gtp_header * h;
    h = (gtp_header *) header;
    h->flags = GTP_FLAGS;
    h->message_type = GTP_MESSAGE_TYPE;
    h->len = htons((uint16_t)(len));
    h->teid = htonl(get_gtp_teid(ue));
}

void * _generate_udp_traffic(void * args)
{
    uint8_t buffer[BUFFER_DATA_LEN];
    struct sockaddr_in dest_addr, spgw_addr;
    fd_set fds;
    struct timeval timeout;
    int retval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    int tunfd;
    char msg[] = "Uplink traffic";
    int sockfd;
    int len;
    int sockaddr_len = sizeof(dest_addr);
    udp_generator gen;

    /* Copy args */
    memcpy(&gen, args, sizeof(udp_generator));
    free(args);

    /* Get TUN device */
    tunfd = get_tun_device(gen.ue);

    /* Get UE data socket (TUN socket) */
    sockfd = get_data_plane_socket(gen.ue);

    /* Configure destiny address */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET; 
    dest_addr.sin_port = htons(gen.port_dest);
    memcpy((void*) &dest_addr.sin_addr.s_addr, gen.dest_ip, sizeof(struct in_addr));

    /* Configure SPGW address */
    memset(&spgw_addr, 0, sizeof(spgw_addr));
    spgw_addr.sin_family = AF_INET; 
    spgw_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &spgw_addr.sin_addr.s_addr, get_spgw_ip(gen.ue), sizeof(struct in_addr));

    while(1)
    {
        sendto(sockfd, msg, strlen(msg), 0, (const struct sockaddr *) &dest_addr,  sizeof(dest_addr));
        while(1)
        {
            FD_ZERO(&fds);
            FD_SET(tunfd, &fds);
            retval = select(tunfd+1, &fds, 0, 0, &timeout);
            if (retval == -1){
                perror("select()");
            }
            else if (retval){
                /* Reuse of the buffer with enough space for the GTP header */
                len = tun_read(tunfd, buffer+8, BUFFER_DATA_LEN); /* We call read to clean the buffer from external traffic */
                /* Detect IPv4 packets*/
                if((buffer[8] & 0xF0) == 0x40)
                {
                    generate_gtp_header(buffer, gen.ue, len);
                    break;
                }
            }
        }
        /* Protect the access to the eNB socket with a lock*/
        pthread_mutex_lock(&lock);
        /* Send tho the SPGW */
        sendto(get_spgw_socket(gen.enb), buffer, len+8, 0, (const struct sockaddr *) &spgw_addr,  sizeof(spgw_addr));
        /* Unlock the mutex */
        pthread_mutex_unlock(&lock);


        /* Receive server answer listening on the TUN device */
        //len = read(sockfd, buffer, BUFFER_DATA_LEN);
        len = recvfrom(get_data_plane_socket(gen.ue), buffer, BUFFER_DATA_LEN, 0, ( struct sockaddr *) &dest_addr, (socklen_t *)&sockaddr_len);
        printf("UE %d has received: %s\n", get_ue_id(gen.ue), buffer);

        /* Sleep 5 seconds */
        sleep(5);
    }

    return NULL;
}

void * _start_uplink_thread(void * args)
{
    uint8_t buffer[BUFFER_DATA_LEN];
    struct sockaddr_in spgw_addr;
    fd_set fds;
    struct timeval timeout;
    int retval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    int tunfd;
    int len;
    udp_generator gen;

    /* Copy args */
    memcpy(&gen, args, sizeof(udp_generator));
    free(args);

    /* Get TUN device */
    tunfd = get_tun_device(gen.ue);


    /* Configure SPGW address */
    memset(&spgw_addr, 0, sizeof(spgw_addr));
    spgw_addr.sin_family = AF_INET; 
    spgw_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &spgw_addr.sin_addr.s_addr, get_spgw_ip(gen.ue), sizeof(struct in_addr));

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
            else if (retval){
                /* Reuse of the buffer with enough space for the GTP header */
                len = tun_read(tunfd, buffer+8, BUFFER_DATA_LEN); /* We call read to clean the buffer from external traffic */
                /* Detect IPv4 packets*/
                if((buffer[8] & 0xF0) == 0x40)
                {
                    generate_gtp_header(buffer, gen.ue, len);
                    break;
                }
            }
        }
        /* Send tho the SPGW */
        sendto(get_spgw_socket(gen.enb), buffer, len+8, 0, (const struct sockaddr *) &spgw_addr,  sizeof(spgw_addr));
    }

    return NULL;
}

void start_uplink_thread(UE * ue, eNB * enb)
{
    pthread_t data_plane_thread;
    udp_generator * gen = calloc(1, sizeof(udp_generator));
    gen->ue = ue;
    gen->enb = enb;
    printf("Creating data plane thread for UE %d\n", get_ue_id(ue));
    /* Create distributor thread */
    if (pthread_create(&data_plane_thread, NULL, _start_uplink_thread, (void *) gen) != 0)
    {
        perror("pthread_create downlink_distributor");
    }
}

void generate_udp_traffic(UE * ue, eNB * enb, uint8_t * dest_ip, int port_dest)
{
    pthread_t data_plane_thread;
    udp_generator * gen = calloc(1, sizeof(udp_generator));
    gen->ue = ue;
    gen->enb = enb;
    gen->dest_ip = dest_ip;
    gen->port_dest = port_dest;
    printf("Creating data plane thread for UE %d\n", get_ue_id(ue));
    /* Create distributor thread */
    if (pthread_create(&data_plane_thread, NULL, _generate_udp_traffic, (void *) gen) != 0)
    {
        perror("pthread_create downlink_distributor");
    }
}
