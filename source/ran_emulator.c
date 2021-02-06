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
#include "log.h"
#include "data_plane.h"
#include "enb_emulator.h"
#include "ue_emulator.h"
#include "crypto.h"


#define CONTROLLER_PORT 1234


#define INIT_MSG 0xFF
#define CODE_OK 0x80
#define CODE_ERROR 0x00
#define CODE_ENB_BEHAVIOUR 0x01
#define CODE_UE_BEHAVIOUR 0x02
#define CODE_IDLE 0x03
#define CODE_DETACHED 0x04
#define CODE_ATTACHED 0x05
#define CODE_MOVE_TO_CONNECTED 0x06
#define CODE_GET_ENB 0x7
#define CODE_X2_HANDOVER_COMPLETED 0x08
#define CODE_ATTACHED_TO_ENB 0x09
#define CODE_S1_HANDOVER_COMPLETED 0x0A


uint8_t local_ip[4];
uint8_t ue_ip[4];

int sockfd_controller;
struct sockaddr_in serv_addr;

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
    if( (buffer[0] & CODE_OK) == 0 || buffer[0] == CODE_ERROR)
    {
        printError("Controller error: abort\n");
        return -1;
    }
    /* Message has been verified */

    /* Run as UE */
    if(buffer[0] & CODE_UE_BEHAVIOUR)
    {
        uint16_t command_len = 0;
        uint8_t temp_ip[4];
        ue_data data;
        bzero(&data, sizeof(data));

        uint8_t offset = 0;
        printf("Creating a UE...\n");

        /*
            4: ID
            15: imsi
            16: key
            16: op key
            2: command_length + 1
            X: command + \0
            4: eNB IP
            2: eNB port
        */
        offset += 1;
        /* Getting UE ID */
        for(i = 0; i < 4; i++)
            data.id[i] = buffer[i+offset];
        offset += 4;
        /* Getting MCC */
        memcpy(data.mcc, buffer+offset, 3);
	/* Asumes that MCC is 3 bytes length */
        offset += 3;
        /* Getting MNC */
        memcpy(data.mnc, buffer+offset, 2);
	/* Asumes that MNC is 2 bytes length */
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
        /* Get Multiplexer/EPC IP */
        memcpy(temp_ip, buffer + offset, 4);
        offset += 4;
        /* Get SPGW Port */
        data.spgw_port = buffer[offset] << 8 | buffer[offset+1];
        offset += 2;

        /* No Multiplexer */
        if(data.spgw_port == 2152)
            memcpy(data.ue_ip, ue_ip, 4);
        /* Multiplexer deployed */
        else
            memcpy(data.ue_ip, temp_ip, 4);

        /* Copy control plane actions */
        /* Set len */
        data.control_plane_len = len - offset;
        /* Allocate control plane */
        data.control_plane = calloc(1, data.control_plane_len);
        memcpy(data.control_plane, buffer + offset, data.control_plane_len);
        offset += data.control_plane_len;


        /* Error control */
        if(offset > len)
        {
            /* Wrong data format */
            printError("Wrong data format in message\n");
            /*Generate response buffer*/
            response[0] = CODE_UE_BEHAVIOUR;
            ue_copy_id_to_buffer(response + 1);
            *res_len = 5;
            return 1;
        }

        /* Copy Local IP to ue_data structure */
        memcpy(data.local_ip, local_ip, 4);

        ret = ue_emulator_start(&data);
        if(ret != 0)
            printError("Error starting UE functionality\n");
        /* Generate response buffer */
        response[0] = CODE_UE_BEHAVIOUR;
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
        /* Copy Local IP to enb_data structure */
        memcpy(data.enb_ip, local_ip, 4);

        /* Error control */
        if(offset > len)
        {
            /* Wrong data format */
            printError("Wrong data format in message\n");
            /*Generate response buffer*/
            response[0] = CODE_ENB_BEHAVIOUR;
            enb_copy_id_to_buffer(response + 1);
            *res_len = 5;
            return 1;
        }

        /* Once the data has been structured, the eNB functionality is created */
        ret = enb_emulator_start(&data);
        if(ret == 1)
        {
            printError("Error starting eNB functionality\n");
        }

        /*Generate response buffer*/
        response[0] = CODE_ENB_BEHAVIOUR;
        enb_copy_id_to_buffer(response + 1);
        memcpy(response + 5, ue_ip, 4);
        *res_len = 9;
        return ret;
    }
    return 0;
}

void send_buffer(int sock, struct sockaddr_in * serv_addr, int addr_size, uint8_t * buf, int len)
{
    if(sendto(sock, buf, len, 0, (const struct sockaddr *) serv_addr, addr_size) == -1)
    {
        perror("send_buffer");
    }
}

void send_idle_controller(uint32_t ue_id)
{
    uint8_t buffer[5];
    buffer[0] = CODE_IDLE;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 5);
}

void send_ue_detach_controller(uint32_t ue_id)
{
    uint8_t buffer[5];
    buffer[0] = CODE_DETACHED;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 5);
}

void send_ue_attach_controller(uint32_t ue_id)
{
    uint8_t buffer[5];
    buffer[0] = CODE_ATTACHED;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 5);
}

void send_ue_moved_to_connected_controller(uint32_t ue_id)
{
    uint8_t buffer[5];
    buffer[0] = CODE_MOVE_TO_CONNECTED;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 5);
}

uint32_t send_get_enb_ip(uint32_t ue_id, uint32_t enb_id)
{
    uint8_t buffer[9];
    int len, n;
    struct sockaddr_in serv_addr_aux;
    uint32_t enb_ip;
    int sockfd;

    /* Open a new socket for this request */
    if ( (sockfd =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        return -1;
    }

    buffer[0] = CODE_GET_ENB;
    /* Add UE id */
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    /* Add eNB ID */
    buffer[5] = (enb_id >> 24) & 0xFF;
    buffer[6] = (enb_id >> 16) & 0xFF;
    buffer[7] = (enb_id >> 8) & 0xFF;
    buffer[8] = enb_id & 0xFF;
    send_buffer(sockfd, &serv_addr, sizeof(serv_addr), buffer, 9);


    /* Receiving Controller answer */
    bzero(buffer, 9);
    memcpy(&serv_addr_aux, (uint8_t *)&serv_addr, sizeof(serv_addr));

    /* Receive eNB IP */
    n = recvfrom(sockfd, (char *)buffer, 5, 0, (struct sockaddr *) &serv_addr_aux, (uint32_t *)&len);
    if(n <= 1)
    {
        printWarning("Wrong Target-eNB Number: eNB %d does not exists\n", enb_id);
        return -1;
    }
    enb_ip = (buffer[1] << 24) | (buffer[2] << 16) | (buffer[3] << 8) | buffer[4];

    close(sockfd);

    return enb_ip;
}

void send_x2_handover_complete(uint32_t ue_id, uint32_t enb_id)
{
    uint8_t buffer[9];
    buffer[0] = CODE_X2_HANDOVER_COMPLETED;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    buffer[5] = (enb_id >> 24) & 0xFF;
    buffer[6] = (enb_id >> 16) & 0xFF;
    buffer[7] = (enb_id >> 8) & 0xFF;
    buffer[8] = enb_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 9);
}

void send_ue_attach_to_enb_controller(uint32_t ue_id, uint32_t enb_id)
{
    uint8_t buffer[9];
    buffer[0] = CODE_ATTACHED_TO_ENB;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    buffer[5] = (enb_id >> 24) & 0xFF;
    buffer[6] = (enb_id >> 16) & 0xFF;
    buffer[7] = (enb_id >> 8) & 0xFF;
    buffer[8] = enb_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 9);
}

void send_s1_handover_complete(uint32_t ue_id, uint32_t enb_id)
{
    uint8_t buffer[9];
    buffer[0] = CODE_S1_HANDOVER_COMPLETED;
    buffer[1] = (ue_id >> 24) & 0xFF;
    buffer[2] = (ue_id >> 16) & 0xFF;
    buffer[3] = (ue_id >> 8) & 0xFF;
    buffer[4] = ue_id & 0xFF;
    buffer[5] = (enb_id >> 24) & 0xFF;
    buffer[6] = (enb_id >> 16) & 0xFF;
    buffer[7] = (enb_id >> 8) & 0xFF;
    buffer[8] = enb_id & 0xFF;
    send_buffer(sockfd_controller, &serv_addr, sizeof(serv_addr), buffer, 9);
}

void send_init_controller()
{
    uint8_t code_send[5];
    code_send[0] = INIT_MSG;
    memcpy(code_send+1, ue_ip, 4);
    if(sendto(sockfd_controller, code_send, 5, 0, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("send_init_code_controller");
    }
}

void receive_controller()
{
    uint8_t buffer[1024];
    uint8_t response[1024];
    int len, n, res, res_len, sockfd;
    struct sockaddr_in serv_addr_aux;

    sockfd = sockfd_controller;
    memcpy(&serv_addr_aux, (uint8_t *)&serv_addr, sizeof(serv_addr));

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
        send_buffer(sockfd, &serv_addr_aux, sizeof(serv_addr_aux), response, res_len);
        close(sockfd);
        exit(1);
    }
    else
    {
        /* Send OK */
        printOK("Slave running\n");
        response[0] |= CODE_OK;
        send_buffer(sockfd, &serv_addr_aux, sizeof(serv_addr_aux), response, res_len);
    }
}

void dumpM(char * text, uint8_t * mem, uint8_t len)
{
    int i;
    printf("%s: ", text);
    for(i = 0; i < len; i++)
        printf("%.2x", mem[i]);
    printf("\n");
}

int main(int argc, char const *argv[])
{
    uint32_t ue_address;

    setvbuf(stdout, NULL, _IONBF, 0); 

    /* TESTING */
    /*
    uint8_t  key[32] = {0x8b, 0xaf, 0x47, 0x3f, 0x2f, 0x8f, 0xd0, 0x94, 0x87, 0xcc, 0xcb, 0xd7, 0x09, 0x7c, 0x68, 0x62};
    uint8_t  opc[32] = {0x8e, 0x27, 0xb6, 0xaf, 0x0e, 0x69, 0x2e, 0x75, 0x0f, 0x32, 0x66, 0x7a, 0x3b, 0x14, 0x60, 0x5d};
    uint8_t rand[32] = {0x98, 0xf8, 0x81, 0xd5, 0x47, 0x4e, 0xc8, 0xa6, 0x40, 0x60, 0x6e, 0x0f, 0x50, 0x51, 0xfa, 0x86};
    uint8_t autn[32] = {0xea, 0x2d, 0xe7, 0x5e, 0xd2, 0x01, 0x80, 0x00, 0x4f, 0x82, 0x6a, 0x18, 0xc2, 0xc1, 0x78, 0xe5};
    uint8_t res[8];
    uint8_t ck[16];
    uint8_t ik[16];
    uint8_t ak[6];
    uint8_t res_asterisk[16];
    char supi[15] = "208930000000003";
    uint8_t kausf[32];
    uint8_t kseaf[32];
    uint8_t kamf[32];
    uint8_t enc_key[16];
    uint8_t int_key[16];

    f2345(key, rand, res, ck, ik, ak, opc);
    dumpM("Key", key, 32);
    dumpM("OPC", opc, 32);
    dumpM("Rand", rand, 32);
    dumpM("Autn", autn, 32);
    dumpM("RES", res, 8);
    dumpM("CK", ck, 16);
    dumpM("IK", ik, 16);
    dumpM("ak", ak, 6);
    generate_res_5g(res_asterisk, ck, ik, rand, res, (uint8_t *)supi, (uint8_t *)supi+3);
    dumpM("RES*", res_asterisk, 16);
    generate_kausf(kausf, ck, ik, autn, (uint8_t *)supi, (uint8_t *)supi+3);
    dumpM("Kausf", kausf, 32);
    generate_kseaf(kseaf, kausf, (uint8_t *)supi, (uint8_t *)supi+3);
    dumpM("Kseaf", kseaf, 32);
    generate_kamf(kamf, supi, kseaf);
    dumpM("Kamf", kamf, 32);
    generate_5g_nas_enc_key(enc_key, kamf, 0x00);
    generate_5g_nas_int_key(int_key, kamf, 0x02);
    dumpM("EncKey", enc_key, 16);
    dumpM("IntKey", int_key, 16);


    return 0;
    */

    if(argc != 3)
    {
        printf("USE: ./ran_simulator <RAN_CONTROLLER_IP> <UE_IP (Optional)>\n");
        exit(1);
    }

    printOK("Starting Nervion emulator...\n");

    #ifdef _5G
    printOK("Nervion 5G\n");
    #else
    printOK("Nervion 4G\n");
    #endif


    printf("Nervion parameters: \n\tRAN Controller IP: %s\n\tNODE IP: %s\n", argv[1], argv[2]);


    ue_address = inet_addr(argv[2]);
    ue_ip[3] = (ue_address >> 24) & 0xFF;
    ue_ip[2] = (ue_address >> 16) & 0xFF;
    ue_ip[1] = (ue_address >> 8) & 0xFF;
    ue_ip[0] = ue_address & 0xFF;


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(CONTROLLER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(&(serv_addr.sin_zero), '\0', 8);

    if(serv_addr.sin_addr.s_addr == (in_addr_t)(-1))
    {
        printf("Invalid IP address\n");
        exit(1);
    }

    /* Store Local IP */
    /* Local IP is set to 0.0.0.0 */
    memcpy(local_ip, ue_ip, 4);
    //bzero(local_ip, 4);

    if ( (sockfd_controller =  socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(1);
    }

    /* Send INIT message */
    send_init_controller();

    while(1)
    {
        /* Receive Controller instructions */
        receive_controller();
    }

    return 0;
}
