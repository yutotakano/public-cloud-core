#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "log.h"
#include "uplink.h"
#include "downlink.h"


#define MME_LISTEN_PORT 36412
#define SCTP_OUT_STREAMS 32
#define SCTP_IN_STREAMS 32
#define SCTP_MAX_ATTEMPTS 2
#define SCTP_TIMEOUT 1
#define SOCKET_READ_TIMEOUT_SEC 1
#define SOCKET_LISTEN_QUEUE 5 /* Extract from openair-cn (openair-cn/SCTP/sctp_primitives_server.c) */
#define UDP_PORT 32566

int configure_sctp_socket(char * mme_ip_address)
{
	int sock_sctp;
	struct sockaddr_in listener_addr;
	struct sctp_event_subscribe events;
    struct sctp_initmsg init;


	/*****************/
	/* Create socket */
	/*****************/
	sock_sctp = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if(sock_sctp < 0) {
		perror("Socket SCTP setup");
		return -1;
	}


	/****************************/
	/* Configure socket options */
	/****************************/
	/* OPT 1 */
    /* Request a number of in/out streams */
    init.sinit_num_ostreams = SCTP_OUT_STREAMS;
    init.sinit_max_instreams = SCTP_IN_STREAMS;
    init.sinit_max_attempts = SCTP_MAX_ATTEMPTS;
    init.sinit_max_init_timeo = SCTP_TIMEOUT;
    if (setsockopt (sock_sctp, IPPROTO_SCTP, SCTP_INITMSG, &init, (socklen_t) sizeof (struct sctp_initmsg)) < 0) {
        perror("setsockopt in/out streams");
        close(sock_sctp);
        return -1;
    }

    /* OPT 2 */
    /* Subscribe to all events */
    memset(&events, 0, sizeof(events));
    events.sctp_data_io_event = 1;
    events.sctp_shutdown_event = 1;
    //memset((void *)&events, 1, sizeof(struct sctp_event_subscribe));
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(struct sctp_event_subscribe)) < 0) {
        perror("setsockopt events");
        close(sock_sctp);
        return -1;
    }

    /* OPT 3 */
    /* Set SCTP_NODELAY option to disable Nagle algorithm */
    int nodelay = 1;
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay)) < 0) {
        perror("setsockopt no_delay");
        close(sock_sctp);
        return -1;
    }


	/***************************/
    /* Set up SCTP MME address */
    /***************************/
    listener_addr.sin_addr.s_addr = inet_addr(mme_ip_address);
    listener_addr.sin_family = AF_INET;
    listener_addr.sin_port = htons(MME_LISTEN_PORT);
    memset(&(listener_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
    if (sctp_bindx(sock_sctp,(struct sockaddr*)&listener_addr, 1, SCTP_BINDX_ADD_ADDR) == -1) {
        perror("Bind MME socket");
        close(sock_sctp);
        return -1;
    }
	
	/**********/
	/* Listen */
	/**********/
	if (listen(sock_sctp, SOCKET_LISTEN_QUEUE) < 0) {
    	perror("MME Socket listen");
    	close(sock_sctp);
		return -1;
	}

	return sock_sctp;
}

int configure_udp_socket(char * mme_ip_address)
{
	int sock_udp;
	struct sockaddr_in udp_addr;

	/*****************/
	/* Create socket */
	/*****************/
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_udp < 0) { 
        perror("socket creation failed"); 
        return -1;
    }

    udp_addr.sin_addr.s_addr = inet_addr(mme_ip_address);
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(UDP_PORT);
    memset(&(udp_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
    if(bind(sock_udp, (const struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) 
    { 
        perror("Bind MME UDP socket"); 
    	close(sock_udp);
    	return -1;
    }

    return sock_udp;
}


void frontend(char * mme_ip_address, char * k8s_lb_ip_address)
{
	int sock_sctp, sock_udp, sock_enb, enb_count = 0;
	uplink_args * uargs;
	downlink_args * dargs;

	pthread_t downlink_t, uplink_t;


	sock_sctp = configure_sctp_socket(mme_ip_address);
	if(sock_sctp < 0) {
		printError("Error configuring SCTP socket.\n");
		return;
	}
	printOK("SCTP socket configured correctly.\n");

	sock_udp = configure_udp_socket(mme_ip_address);
	if(sock_udp < 0) {
		printError("Error configuring UDP socket.\n");
		close(sock_sctp);
		return;
	}
	printOK("UDP socket configured correctly.\n");

	/* Each eNB has one dedicated socket and one thread in the uplink direction */
	while(1) {
		if ((sock_enb = accept(sock_sctp, NULL, NULL)) < 0) {
			perror("MME Socket accept");
			continue;
		}
		printOK("New eNB (%d) connected.\n", enb_count);

		/* Assemble downlink thread arguments structure */
		dargs = (downlink_args *) malloc(sizeof(downlink_args));
		if(dargs == NULL) {
			printError("Error allocating downlink args structure.");
			close(sock_enb);
			continue;
		}
		dargs->sock_udp = sock_udp;

		/* Create downlink thread */
		if(pthread_create(&downlink_t, NULL, downlink_thread, (void *)dargs) != 0) {
			perror("downlink thread");
			printError("Closing open sockets.\n");
			close(sock_sctp);
			close(sock_udp);
			return;
		}
		printOK("Downlink thread initialized correctly for eNB %d.\n", enb_count);

		/* Assemble uplink thread arguments structure */
		uargs = (uplink_args *) malloc(sizeof(uplink_args));
		if(uargs == NULL) {
			printError("Error allocating uplink args structure.\n");
			close(sock_enb);
			/* Kill downlink thread */
			pthread_cancel(downlink_t);
			continue;
		}
		/* Set eNB socket */
		uargs->sock_enb = sock_enb;
		uargs->epc_sock = sock_udp;
		uargs->epc_addr = inet_addr(k8s_lb_ip_address);

		/* Create uplink thread */
		if(pthread_create(&uplink_t, NULL, uplink_thread, (void *)uargs) != 0) {
			perror("Error creating uplink thread");
			close(sock_enb);
			free(uargs);
			continue;
		}
		printOK("Uplink thread initialized correctly for eNB %d.\n", enb_count);

		enb_count++;
	}

	printInfo("Closing SCTP socket...\n");
	close(sock_enb);
	printInfo("Closing UDP socket...\n");
	close(sock_udp);
}


int main(int argc, char const *argv[])
{
	if(argc != 3) {
		printf("RUN: ./corekube_frontend <FRONTEND_IP_ADDRESS> <K8S_LOADBALANCER_IP_ADDRESS>\n");
		return 1;
	}

	frontend((char *)argv[1], (char *)argv[2]);

	return 0;
}
