#include "tun.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_tunnel.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/select.h>
#include <unistd.h>

#define MTU_SIZE 1500
#define SUBNET_MASK 0xFFFFFFFF

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            must reserve enough space in *dev.                          *
 **************************************************************************/
int tun_init(char * name, struct ifreq * ifr, int flags)
{
    int fd, err;
    char *clonedev = "/dev/net/tun";

    if( (fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }

    memset(ifr, 0, sizeof(struct ifreq));

    ifr->ifr_flags = flags;

    if (*name)
        strncpy(ifr->ifr_name, name, IFNAMSIZ);

    if( (err = ioctl(fd, TUNSETIFF, (void *)ifr)) < 0 )
    {
        perror("TUNSETIFF");
        close(fd);
        return err;
    }

    strcpy(name, ifr->ifr_name);

    return fd;
}

void tun_close(int tunfd)
{
	if(tunfd != -1)
		close(tunfd);
}

int set_ip(struct ifreq *ifr_tun, int sock, uint8_t * ip)
{
    struct sockaddr_in addr;

    /* set the IP of this end point of tunnel */
    memset( &addr, 0, sizeof(addr) );
    memcpy((void*) &addr.sin_addr.s_addr, ip, sizeof(struct in_addr));
    addr.sin_family = AF_INET;
    memcpy( &ifr_tun->ifr_addr, &addr, sizeof(struct sockaddr) );

    if ( ioctl(sock, SIOCSIFADDR, ifr_tun) < 0) {
    	perror("socket SIOCSIFADDR");
        close(sock);
        return -1;
    }

    return 0; 
}

int open_tun_iface(char * name, uint8_t * ip)
{
	int sock;
	struct ifreq ifr_tun;
    struct sockaddr_in * sa;

	int tunfd = tun_init(name, &ifr_tun, IFF_TUN | IFF_NO_PI);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Open socket");
        tun_close(tunfd);
        return -1;
    }

    /* Set tun IP */
    if (set_ip(&ifr_tun, sock, ip) < 0) {
        tun_close(tunfd);
        return -1;
    }

    sa = (struct sockaddr_in *)&ifr_tun.ifr_netmask;
    memset(sa,0,sizeof(*sa));
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = htonl(SUBNET_MASK);

    /* Set the mask */
    if (ioctl(sock, SIOCSIFNETMASK, &ifr_tun) < 0)
    {
        perror("SIOCSIFNETMASK");
        tun_close(tunfd);
        close(sock);
        return -1;
    }

    /* Read flags */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr_tun) < 0) {
        perror("SIOCGIFFLAGS");
        tun_close(tunfd);
        close(sock);
        return -1;
    }

    /* Set Iface up and flags */
    ifr_tun.ifr_flags |= IFF_UP;
    ifr_tun.ifr_flags |= IFF_RUNNING;

    if (ioctl(sock, SIOCSIFFLAGS, &ifr_tun) < 0)  {
        perror("SIOCGIFFLAGS");
        tun_close(tunfd);
        close(sock);
        return -1;
    }

    /* Set MTU size */
    ifr_tun.ifr_mtu = MTU_SIZE; 
    if (ioctl(sock, SIOCSIFMTU, &ifr_tun) < 0)  {
        perror("SIOCSIFMTU");
        tun_close(tunfd);
        close(sock);
        return -1;
    }

    close(sock);

    return tunfd;
}

int tun_read(int fd, uint8_t * buf, int n)
{
    int nread;
    if((nread=read(fd, buf, n)) < 0){
        perror("Reading tun data");
        return -1;
    }
    return nread;
}

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *         returned.                                                      *
 **************************************************************************/
int tun_write(int fd, uint8_t * buf, int n)
{
    int nwrite;
    if((nwrite=write(fd, buf, n)) < 0)
    {
        perror("Writing data");
        return -1;
    }
    return nwrite;
}

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts them into "buf".     *
 *         (unless EOF, of course)                                        *
 **************************************************************************/
int tun_read_n(int fd, uint8_t * buf, int n)
{
    int nread, left = n;
    while(left > 0)
    {
        if ((nread = tun_read(fd, buf, left)) == 0)
            return 0 ;      
        else
        {
            left -= nread;
            buf += nread;
        }
    }
    return n;  
}