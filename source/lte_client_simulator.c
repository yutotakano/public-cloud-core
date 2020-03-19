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

#include "ue.h"
#include "enb.h"
#include "s1ap.h"
#include "log.h"

#define SCTP_OUT_STREAMS 32
#define SCTP_IN_STREAMS 32
#define SCTP_MAX_ATTEMPTS 5

#define MME_OAI_PORT 36412

int send_S1_setup_request()
{
	return 0;
}

int main(int argc, char const *argv[])
{
	const char * mme_ip;
    int mme_port;
    int sock_sctp;
    struct sctp_event_subscribe events;
    struct sockaddr_in mme_addr;
    struct sctp_initmsg init;
    eNB * enb;
    UE * ue;
    int err;

    /* GC Logs */
    GC_enable_logs();

    /* GC Backtrace */
    //GC_enable_backtrace();

    if(argc != 2)
    {
        printf("USE: ./lte_client_simulator <MME_IP>\n");
        exit(1);
    }
    uint8_t key_oai[] = {0x3f, 0x3f, 0x47, 0x3f, 0x2f, 0x3f, 0xd0, 0x94, 0x3f, 0x3f, 0x3f, 0x3f, 0x09, 0x7c, 0x68, 0x62};
    uint8_t op_key_oai[] = {0xdd, 0xaf, 0x9e, 0x16, 0xc4, 0x03, 0x72, 0x8e, 0xd1, 0x52, 0xb6, 0x9d, 0xb0, 0x37, 0x2e, 0xa1};
    uint8_t key[] = {0x8b, 0xaf, 0x47, 0x3f, 0x2f, 0x8f, 0xd0, 0x94, 0x87, 0xcc, 0xcb, 0xd7, 0x09, 0x7c, 0x68, 0x62};
    uint8_t op_key[] = {0xe7, 0x34, 0xf8, 0x73, 0x40, 0x07, 0xd6, 0xc5, 0xce, 0x7a, 0x05, 0x08, 0x80, 0x9e, 0x7e, 0x9c};
    ue = init_UE("208", "93", "0000000001", UE_DEFAULT_CAPABILITIES, 0x8006692d, key_oai, op_key_oai);
    enb = init_eNB(0x00000e00, "208", "93");

    printUE(ue);

    //crypto_test(enb, ue);

    //return 1;
    /* Getting parameters */
    mme_ip = argv[1];
    mme_port = MME_OAI_PORT;


	sock_sctp = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sock_sctp < 0)
    {
        perror("sctp socket error");
        exit(1);
    }

    /* Sets the data_io_event to be able to use sendrecv_info */
    /* Subscribes to the SCTP_SHUTDOWN event, to handle graceful shutdown */
    bzero(&events, sizeof(events));
    events.sctp_data_io_event          = 1;
    events.sctp_shutdown_event         = 1;
    if (setsockopt(sock_sctp, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events)) != 0) {
        perror("setsockopt events");
        close(sock_sctp);
        exit(1);
    }

    /*
    * Request a number of in/out streams
    */
    init.sinit_num_ostreams = SCTP_OUT_STREAMS;
    init.sinit_max_instreams = SCTP_IN_STREAMS;
    init.sinit_max_attempts = SCTP_MAX_ATTEMPTS;
    if (setsockopt (sock_sctp, IPPROTO_SCTP, SCTP_INITMSG, &init, (socklen_t) sizeof (struct sctp_initmsg)) < 0)
    {
        perror("setsockopt in/out streams");
        close(sock_sctp);
        exit(1);
    }

    /* Set up SCTP Server address */
    memset(&mme_addr, 0, sizeof(mme_addr));
    mme_addr.sin_family = AF_INET;
    mme_addr.sin_addr.s_addr = inet_addr(mme_ip);
    mme_addr.sin_port = htons(mme_port);

    /* Connect to the SCTP server */
    if (connect(sock_sctp, (struct sockaddr*)&mme_addr, sizeof(mme_addr)))
    {
        perror("sctp connect error");
        close(sock_sctp);
        exit(1);
    }
    printOK("Connected to the SCTP Server\n");

    err = procedure_S1_Setup(sock_sctp, enb);

    if(err == 1)
    {
    	printError("S1 Setup problems\n");
    }
    else
    {
    	printOK("eNB successfully connected\n");
    }

    sleep(2);

    err = procedure_Attach_Default_EPS_Bearer(sock_sctp, enb, ue);


    sleep(10);

    free_eNB(enb);
    free_UE(ue);
    close(sock_sctp);
	return 0;
}