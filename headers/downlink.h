#ifndef __DOWNLINK_H__
#define __DOWNLINK_H__

typedef struct _downlink_args
{
	int sock_udp;
} downlink_args;

void * downlink_thread(void * args);

# endif