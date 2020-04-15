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
#include "sctp.h"
#include "s1ap.h"
#include "log.h"
#include "data_plane.h"

int main(int argc, char const *argv[])
{
	const char * mme_ip;
    const char * enb_ip;
    int err;
    int type_of_traffic;

    eNB * enb;
    UE * ue1;
    UE * ue2;
    UE * ue3;
    uint8_t key_oai[] = {0x3f, 0x3f, 0x47, 0x3f, 0x2f, 0x3f, 0xd0, 0x94, 0x3f, 0x3f, 0x3f, 0x3f, 0x09, 0x7c, 0x68, 0x62};
    uint8_t op_key_oai[] = {0xdd, 0xaf, 0x9e, 0x16, 0xc4, 0x03, 0x72, 0x8e, 0xd1, 0x52, 0xb6, 0x9d, 0xb0, 0x37, 0x2e, 0xa1};


    /**********************/
    /* GCLib configuration*/
    /**********************/
    /* GC Logs */
    GC_enable_logs();
    /* GC Backtrace */
    //GC_enable_backtrace();


    if(argc != 4)
    {
        printf("USE: ./lte_client_simulator <MME_IP> <ENB_IP> <type_of_traffic>\n");
        exit(1);
    }


    /* Getting parameters */
    mme_ip = argv[1];
    enb_ip = argv[2];
    type_of_traffic = atoi(argv[3]);



    ue1 = init_UE("208", "93", "0000000001", key_oai, op_key_oai);
    ue2 = init_UE("208", "93", "0000000002", key_oai, op_key_oai);
    ue3 = init_UE("208", "93", "0000000003", key_oai, op_key_oai);
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
    err = procedure_Attach_Default_EPS_Bearer(enb, ue2);

    /* Attach UE 3 */
    err = procedure_Attach_Default_EPS_Bearer(enb, ue3);

    /* DATA PLANE */
    open_enb_data_plane_socket(enb);
    setup_ue_data_plane(ue1);

    /* Echo server info */
    uint8_t dest_ip[] = {192, 172, 0, 1};
    int dest_port = 1234;
    sleep(5);
    printf("Starting data plane\n");
    if(type_of_traffic == 0)
    {
        generate_udp_traffic(ue1, enb, dest_ip, dest_port);
    }
    else
    {
        start_uplink_thread(ue1, enb);
    }
    /* UE 1 data plane */
    //start_uplink_thread(ue1, enb);
    //start_uplink_thread(ue2, enb);
    //start_uplink_thread(ue3, enb);
    //generate_udp_traffic(ue1, enb, dest_ip, dest_port);
    //generate_udp_traffic(ue2, enb, dest_ip, dest_port);
    
    while(1);

    free_eNB(enb);
    free_UE(ue1);
    free_UE(ue2);
    free_UE(ue3);
	return 0;
}