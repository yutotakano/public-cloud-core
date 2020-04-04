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

int tun_descriptors[32];
pthread_mutex_t lock;

int open_enb_data_plane_socket(eNB * enb)
{
    int sockfd;
    struct sockaddr_in enb_addr;

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
        perror("Bind");
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

    return 0;
}

/* Distributor thread */
/* Reads from the eNB socket and analyze the GTP header to redirect the packet to the UE interface */
/*
TODO
while 1:
	readfrom socket
	analyze GTP header
	write packet to UE tun device
*/

int setup_ue_data_plane(UE * ue)
{
	char * tun_name;
	int tunfd, sockfd;
	struct ifreq ifr;

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
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(struct ifreq)) < 0) {
        perror("tun bind");
        close(sockfd);
        return 1;
    }
    set_tun_device(ue, tunfd);
    set_data_plane_socket(ue, sockfd);

    /* Store the tun device in the hash table */
    tun_descriptors[get_ue_id(ue) % MAX_TUN_DESCRIPTORS] = tunfd;

    return 0;
}

void generate_gtp_header(uint8_t * header, UE * ue, int len)
{
	uint16_t real_len = (uint16_t)(len + 8);
	uint32_t teid = get_gtp_teid(ue);
	header[0] = 0x30;
	header[1] = 0xFF;
	header[2] = (real_len & 0xFF00) >> 8;
	header[3] = real_len & 0xFF;
	header[4] = (teid & 0xFF000000) >> 24;
	header[5] = (teid & 0xFF0000) >> 16;
	header[6] = (teid & 0xFF00) >> 8;
	header[7] = teid & 0xFF;
}

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

void generate_udp_traffic(UE * ue, eNB * enb, uint8_t * dest_ip, int port_dest)
{
	uint8_t buffer[BUFFER_DATA_LEN];
	struct sockaddr_in dest_addr, spgw_addr, serv_addr;
	fd_set fds;
    struct timeval timeout;
    int retval;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    int tunfd;
    char msg[] = "Sample text by jon";
    int sockfd;
    int len;
    int sockaddr_len = sizeof(serv_addr);

    /* Get TUN device */
    tunfd = get_tun_device(ue);

    /* Get UE data socket (TUN socket) */
    sockfd = get_data_plane_socket(ue);

    /* Configure destiny address */
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET; 
    dest_addr.sin_port = htons(port_dest);
    memcpy((void*) &dest_addr.sin_addr.s_addr, dest_ip, sizeof(struct in_addr));

    /* Configure SPGW address */
    memset(&spgw_addr, 0, sizeof(spgw_addr));
    spgw_addr.sin_family = AF_INET; 
    spgw_addr.sin_port = htons(S1_U_PORT);
    memcpy((void*) &spgw_addr.sin_addr.s_addr, get_spgw_ip(ue), sizeof(struct in_addr));

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
	    	    printf("Packet: 0x%.2x\n", buffer[8]);
	    	    /* Detect IPv4 packets*/
	    	    if((buffer[8] & 0xF0) == 0x40)
	    	    {
	    	    	generate_gtp_header(buffer, ue, len);
	    	    	break;
	    	    }
	    	}
	    }
	    /* Protect the access to the eNB socket with a lock*/
	    pthread_mutex_lock(&lock);
	    /* Send tho the SPGW */
	    sendto(get_spgw_socket(enb), buffer, len+8, 0, (const struct sockaddr *) &spgw_addr,  sizeof(spgw_addr));
	    /* Unlock the mutex */
	    pthread_mutex_unlock(&lock);


	    /* Receive server answer listening on the TUN device */
	    len = recvfrom(sockfd, buffer, BUFFER_DATA_LEN, 0, ( struct sockaddr *) &serv_addr, (socklen_t *)&sockaddr_len);
	    printf("Answer from SERVER received:");
	    dump_Memory(buffer, len);

	    /* Sleep 5 seconds */
    	sleep(5);
    }
}
