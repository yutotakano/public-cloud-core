#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "sctp.h"
#include "log.h"

#define SCTP_OUT_STREAMS 32
#define SCTP_IN_STREAMS 32
#define SCTP_MAX_ATTEMPTS 2
#define SCTP_TIMEOUT 2
#define SOCKET_READ_TIMEOUT_SEC 5

#define SCTP_4G_CONTROL_PLANE 36412
#define SCTP_5G_CONTROL_PLANE 38412

int sctp_get_sock_info (int sock)
{
    socklen_t sock_len = 0;
    struct sctp_status status = {0};

    if (socket <= 0) {
        return -1;
    }

    memset (&status, 0, sizeof (struct sctp_status));
    sock_len = sizeof (struct sctp_status);

    if (getsockopt (sock, IPPROTO_SCTP, SCTP_STATUS, &status, &sock_len) < 0) {
        perror("Getsockopt SCTP_STATUS failed");
        return -1;
    }

    printf("SCTP Socket Status:\n");
    printf("assoc id .....: %u\n", status.sstat_assoc_id);
    printf("state ........: %d\n", status.sstat_state);
    printf("instrms ......: %u\n", status.sstat_instrms);
    printf("outstrms .....: %u\n", status.sstat_outstrms);
    printf("fragmentation : %u\n", status.sstat_fragmentation_point);
    printf("pending data .: %u\n", status.sstat_penddata);
    printf("unack data ...: %u\n", status.sstat_unackdata);
    printf("rwnd .........: %u\n", status.sstat_rwnd);
    printf("peer info     :\n");
    printf("    state ....: %u\n", status.sstat_primary.spinfo_state);
    printf("    cwnd .....: %u\n", status.sstat_primary.spinfo_cwnd);
    printf("    srtt .....: %u\n", status.sstat_primary.spinfo_srtt);
    printf("    rto ......: %u\n", status.sstat_primary.spinfo_rto);
    printf("    mtu ......: %u\n", status.sstat_primary.spinfo_mtu);

    return 0;
}

int sctp_get_peer_addresses (int sock)
{
    int nb = 0, i = 0;
    struct sockaddr *temp_addr_p = NULL;

    if ((nb = sctp_getpaddrs(sock, -1, &temp_addr_p)) <= 0) {
        perror("Failed to retrieve peer addresses");
        return -1;
    }

    printf("Peer addresses:\n");
    for (i = 0; i < nb; i++) {
    	/* Only IPv4 */
        if (temp_addr_p[i].sa_family == AF_INET) {
            char address[16] = {0};
            struct sockaddr_in *addr = NULL;
            addr = (struct sockaddr_in *)&temp_addr_p[i];
            if (inet_ntop (AF_INET, &addr->sin_addr, address, sizeof (address)) != NULL) {
                printf("    - [%s]\n", address);
            }
        }
    }
	return 0;
}

int sctp_get_local_addresses(int sock)
{
	int nb, i;
	struct sockaddr *temp_addr_p = NULL;

	if((nb = sctp_getladdrs(sock, -1, &temp_addr_p)) <= 0) {
		printf("Failed to retrieve local addresses\n");
		return -1;
	}

	printf("Local addresses:\n");
	for (i = 0; i < nb; i++) {
		if (temp_addr_p[i].sa_family == AF_INET) {
			char address[16];
			struct sockaddr_in *addr;
			memset(address, 0, sizeof(address));
			addr = (struct sockaddr_in*)&temp_addr_p[i];
			if (inet_ntop(AF_INET, &addr->sin_addr, address, sizeof(address)) != NULL) {
				printf("    - [%s]\n", address);
			}
		}
	}
	return 0;
}


int sctp_connect_enb_to_mme(eNB * enb, uint8_t * mme_ip)
{
	int sock_sctp;
	struct sockaddr_in mme_addr;
    struct sockaddr_in enb_addr;
    struct sctp_event_subscribe events;
    struct sctp_initmsg init;
    struct timeval timeout;
    int assoc_id;

    /*******************/
    /* Creating socket */
    /*******************/
    sock_sctp = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sock_sctp < 0)
    {
        perror("sctp socket error");
    	return 1;
    }

    /***********/
    /* Options */
    /***********/
    /* OPT 1 */
    /* Request a number of in/out streams */
    init.sinit_num_ostreams = SCTP_OUT_STREAMS;
    init.sinit_max_instreams = SCTP_IN_STREAMS;
    init.sinit_max_attempts = SCTP_MAX_ATTEMPTS;
    init.sinit_max_init_timeo = SCTP_TIMEOUT;
    if (setsockopt (sock_sctp, IPPROTO_SCTP, SCTP_INITMSG, &init, (socklen_t) sizeof (struct sctp_initmsg)) < 0)
    {
        perror("setsockopt in/out streams");
        close(sock_sctp);
        return 1;
    }

    /* OPT 2 */
    /* Sets the data_io_event to be able to use sendrecv_info */
    /* Subscribe to all events */
    memset(&events, 0, sizeof(events));
    events.sctp_data_io_event = 1;
    events.sctp_shutdown_event = 1;
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) < 0) {
        perror("setsockopt events");
        close(sock_sctp);
        return 1;
    }

    /* OPT 3 */
    /* Set SCTP_NODELAY option to disable Nagle algorithm */
    int nodelay = 1;
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
    {
        perror("setsockopt no_delay");
        close(sock_sctp);
        return 1;
    }

    /* OPT 4 */
    /* Set SO_RCVTIMEO to release recv lock after N seconds */
    timeout.tv_sec = SOCKET_READ_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if(setsockopt(sock_sctp, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt recv_delay");
        close(sock_sctp);
        return 1;
    }


    /***************************/
    /* Set up SCTP eNB address */
    /***************************/
    /* Binding simulator to eNB IP to avoid multi IP issues */
    memcpy((void*) &enb_addr.sin_addr.s_addr, get_enb_ip(enb), sizeof(struct in_addr));
    enb_addr.sin_family = AF_INET;
    enb_addr.sin_port = 0; /* Bind to a random port */
    memset(&(enb_addr.sin_zero), 0, sizeof(struct sockaddr));

    /***********/
    /* Binding */
    /***********/
    if (sctp_bindx(sock_sctp,(struct sockaddr*)&enb_addr, 1, SCTP_BINDX_ADD_ADDR) == -1)
    {
        perror("Bind eNB");
        return 1;
    }


    /***************************/
    /* Set up SCTP MME address */
    /***************************/
    memset(&mme_addr, 0, sizeof(mme_addr));
    //if (inet_pton (AF_INET, mme_ip, &mme_addr.sin_addr.s_addr) != 1) {
    //    perror("IP address to network type error");
    //    return -1;
    //}
    memcpy(&mme_addr.sin_addr.s_addr, mme_ip, 4);
    mme_addr.sin_family = AF_INET;
    #ifdef _5G
    mme_addr.sin_port = htons(SCTP_5G_CONTROL_PLANE);
    #else
    mme_addr.sin_port = htons(SCTP_4G_CONTROL_PLANE);
    #endif

    /******************************/
    /* Connect to the SCTP server */
    /******************************/
    if (sctp_connectx(sock_sctp, (struct sockaddr *) &mme_addr, 1, &assoc_id))
    {
        perror("sctp connect error");
        close(sock_sctp);
        return 1;
    }
    printOK("Connected to the SCTP Server (MME)\n");

    set_mme_socket(enb, sock_sctp);

    return 0;
}

