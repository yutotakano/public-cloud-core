#ifndef __tun__
#define __tun__

#include <stdint.h>

int open_tun_iface(char * name, uint8_t * ip);
int update_ip(uint8_t * new_ip);
void tun_close(int tunfd);

int tun_write(int fd, uint8_t * buf, int n);
int tun_read(int fd, uint8_t * buf, int n);
int tun_read_n(int fd, uint8_t * buf, int n);

#endif