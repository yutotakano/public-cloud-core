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
#include "enb_emulator.h"
#include "ue_emulator.h"

int main1(int argc, char const *argv[])
{
	const char * mme_ip;
    const char * enb_ip;
    int err;

    eNB * enb;
    UE * ue1;
    UE * ue2;
    uint8_t key_oai[] = {0x3f, 0x3f, 0x47, 0x3f, 0x2f, 0x3f, 0xd0, 0x94, 0x3f, 0x3f, 0x3f, 0x3f, 0x09, 0x7c, 0x68, 0x62};
    uint8_t op_key_oai[] = {0xdd, 0xaf, 0x9e, 0x16, 0xc4, 0x03, 0x72, 0x8e, 0xd1, 0x52, 0xb6, 0x9d, 0xb0, 0x37, 0x2e, 0xa1};


    /**********************/
    /* GCLib configuration*/
    /**********************/
    /* GC Logs */
    GC_enable_logs();
    /* GC Backtrace */
    //GC_enable_backtrace();


    if(argc != 3)
    {
        printf("USE: sudo ./lte_client_simulator <MME_IP> <ENB_IP>\n");
        exit(1);
    }


    /* Getting parameters */
    mme_ip = argv[1];
    enb_ip = argv[2];



    ue1 = init_UE("208", "93", "0000000001", key_oai, op_key_oai);
    //ue2 = init_UE("208", "93", "0000000002", key_oai, op_key_oai);
    //ue3 = init_UE("208", "93", "0000000003", key_oai, op_key_oai);
    enb = init_eNB(0x00e00000, "208", "93", enb_ip);


    /***************/
    /* Connect eNB */
    /***************/
    /* Open SCTP connection */
    //sctp_connect_enb_to_mme(enb, mme_ip);
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
    //err = procedure_Attach_Default_EPS_Bearer(enb, ue2);

    /* Attach UE 3 */
    //err = procedure_Attach_Default_EPS_Bearer(enb, ue3);

    /* DATA PLANE */
    open_enb_data_plane_socket(enb);
    setup_ue_data_plane(ue1);
    //setup_ue_data_plane(ue2);
    //setup_ue_data_plane(ue3);

    /* Echo server info */
    //uint8_t dest_ip[] = {192, 172, 0, 1};
    //int dest_port = 1234;
    sleep(5);
    printf("Starting data plane\n");
    /* UE 1 data plane */
    start_uplink_thread(ue1, enb);
    //start_uplink_thread(ue2, enb);
    //start_uplink_thread(ue3, enb);
    //generate_udp_traffic(ue1, enb, dest_ip, dest_port);
    //generate_udp_traffic(ue2, enb, dest_ip, dest_port);
    
    while(1);

    free_eNB(enb);
    free_UE(ue1);
    free_UE(ue2);
	return 0;
}

#define INIT_MSG 0x01

#define CODE_OK 0x80
#define CODE_UE_BEHAVIOUR 0x02
#define CODE_ENB_BEHAVIOUR 0x01

/*
    Bit 0 (Status)
            0:ERROR
            1:OK
    Bit 1 (Unused)
    Bit 2 (Unused)
    Bit 3 (Unused)
    Bit 4 (Unused)
    Bit 5 (Unused)
    Bit 6 (UE)
    Bit 7 (eNB)
*/

void dMem(uint8_t * pointer, int len)
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

/*
-1: Controller error
 0: Ok
 1: Slave error 
*/
int analyze_controller_msg(uint8_t * buffer, int len, uint8_t * response, int * res_len)
{
    int i, ret;
    if(len <= 0)
        return 0;
    if( (buffer[0] & CODE_OK) == 0)
    {
        printError("Controller error: abort");
        return -1;
    }
    /* Message has been verified */

    /* Run as UE */
    if(buffer[0] & CODE_UE_BEHAVIOUR)
    {
        uint16_t command_len = 0; 
        ue_data data;
        bzero(&data, sizeof(data));

        uint8_t offset = 0;
        printf("Creating a UE...\n");
        dMem(buffer, len);

        /*
            4: ID
            15: imsi
            16: key
            16: op key
            2: command_length + 1
            X: command + \0
            4: eNB IP
        */
        offset += 1;
        /* Getting UE ID */
        for(i = 0; i < 4; i++)
            data.id[i] = buffer[i+offset];
        offset += 4;
        /* Getting MCC */
        memcpy(data.mcc, buffer+offset, 3);
        offset += 3;
        /* Getting MNC */
        memcpy(data.mnc, buffer+offset, 2);
        offset += 2;
        /* Getting MSIN */
        memcpy(data.msin, buffer+offset, 10);
        offset += 10;
        /* Getting Key */
        memcpy(data.key, buffer+offset, 16);
        offset += 16;
        /* Getting OpKey */
        memcpy(data.op_key, buffer+offset, 16);
        offset += 16;
        /* Getting command lenght */
        command_len = (buffer[offset] << 8) | buffer[offset+1];
        offset += 2;
        /* Getting command */
        data.command = calloc(1, command_len+1);
        memcpy(data.command, buffer + offset, command_len);
        offset += command_len;
        /* Getting eNB IP */
        memcpy(data.enb_ip, buffer + offset, 4);
        offset += 4;

        /* Error control */
        if(offset > len)
        {
            /* Wrong data format */
            printError("Wrong data format in message");
            /*Generate response buffer*/
            response[0] = (~CODE_OK) | buffer[0];
            *res_len = 1;
            return 1;
        }

        ret = ue_emulator_start(&data);
        /* Generate response buffer */
        /*Generate response buffer*/
        response[0] = buffer[0];
        ue_copy_id_to_buffer(response + 1);
        *res_len = 5;

        return ret;
    }
    /* Run as eNB */
    else if(buffer[0] & CODE_ENB_BEHAVIOUR)
    {
        enb_data data;
        bzero(&data, sizeof(data));

        uint8_t offset = 0;
        printf("Creating a eNB...\n");

        /* Getting eNB from buffer */
        offset += 1;
        /* Getting eNB ID */
        for(i = 0; i < 4; i++)
            data.id[i] = buffer[i+offset];
        offset += 4;
        /* Getting eNB MCC as Strings (+ 0x30) */
        for(i = 0; i < 3; i++)
            data.mcc[i] = buffer[offset+i] + 0x30;
        offset += 3;
        /* Getting eNB MNC as Strings (+ 0x30)*/
        for(i = 0; i < 2; i++)
            data.mnc[i] = buffer[offset+i] + 0x30;
        offset += 2;
        /* Getting EPC IP */
        for(i = 0; i < 4; i++)
        {
            data.epc_ip[i] = buffer[offset+i];
        }
        offset += 4;

        /* Error control */
        if(offset > len)
        {
            /* Wrong data format */
            printError("Wrong data format in message");
            /*Generate response buffer*/
            response[0] = (~CODE_OK) | buffer[0];
            *res_len = 1;
            return 1;
        }

        /* Once the data has been structured, the eNB functionality is created */
        ret = enb_emulator_start(&data);

        /*Generate response buffer*/
        response[0] = buffer[0];
        enb_copy_id_to_buffer(response + 1);
        *res_len = 5;
        return ret;
    }
    return 0;
}

void send_init_controller(int sock, struct sockaddr_in * serv_addr)
{
    uint8_t init = INIT_MSG;
    sendto(sock, (uint8_t *)&init, 1, 0, (const struct sockaddr *) serv_addr, sizeof(*serv_addr));
}

void send_code_controller(int sock, struct sockaddr_in * serv_addr, uint8_t code)
{
    uint8_t code_send = code;
    if(sendto(sock, (uint8_t *)&code_send, 1, 0, (const struct sockaddr *) serv_addr, sizeof(*serv_addr)) == -1)
    {
        perror("send_code_controller");
    }
}

void send_buffer(int sock, struct sockaddr_in * serv_addr, uint8_t * buf, int len)
{
    if(sendto(sock, buf, len, 0, (const struct sockaddr *) serv_addr, sizeof(*serv_addr)) == -1)
    {
        perror("send_buffer");
    }
}

void receive_controller(int sock, struct sockaddr_in * serv_addr)
{
    uint8_t buffer[1024];
    uint8_t response[1024];
    int len, n, res, res_len, sockfd;
    struct sockaddr_in serv_addr_aux;

    sockfd = sock;
    memcpy(&serv_addr_aux, serv_addr, sizeof(*serv_addr));

    n = recvfrom(sockfd, (char *)buffer, 1024, 0, (struct sockaddr *) &serv_addr_aux, (uint32_t *)&len);
    res = analyze_controller_msg(buffer, n, response, &res_len);
    if(res < 0)
    {
        close(sockfd);
        exit(1);
    }
    else if(res == 1)
    {
        printError("Slave error: Sending error to controller\n");
        /* Send slave error code to controller */
        response[0] &= (~CODE_OK);
        send_buffer(sockfd, &serv_addr_aux, response, res_len);
    }
    else
    {
        /* Send OK */
        printOK("Slave running\n");
        response[0] |= CODE_OK;
        send_buffer(sockfd, &serv_addr_aux, response, res_len);
    }
}

int main(int argc, char const *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if(argc != 2)
    {
        printf("USE: ./ran_simulator <RAN_CONTROLLER_IP>\n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(&(serv_addr.sin_zero), '\0', 8);

    if(serv_addr.sin_addr.s_addr == (in_addr_t)(-1))
    {
        printf("Invalid IP address\n");
        exit(1);
    }

    if ( (sockfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(1);
    }

    /* Send INIT message */
    send_init_controller(sockfd, &serv_addr);

    while(1)
    {
        /* Receive Controller instructions */
        receive_controller(sockfd, &serv_addr);
    }

    return 0;
}