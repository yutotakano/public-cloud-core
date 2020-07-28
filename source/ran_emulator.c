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


#define CONTROLLER_PORT 1234


#define INIT_MSG 0xFF
#define CODE_OK 0x80
#define CODE_ENB_BEHAVIOUR 0x01
#define CODE_UE_BEHAVIOUR 0x02
#define CODE_IDLE 0x03
#define CODE_DETACHED 0x04
#define CODE_ATTACHED 0x05
#define CODE_MOVE_TO_CONNECTED 0x06
#define CODE_GET_ENB 0x7
#define CODE_X2_HANDOVER_COMPLETED 0x08
#define CODE_ATTACHED_TO_ENB 0x09


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
            printError("Wrong data format in message");
            /*Generate response buffer*/
            response[0] = (~CODE_OK) | buffer[0];
            *res_len = 1;
            return 1;
        }

        /* Copy Local IP to ue_data structure */
        memcpy(data.local_ip, local_ip, 4);

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
        /* Copy Local IP to enb_data structure */
        memcpy(data.enb_ip, local_ip, 4);

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
        *res_len = 7;
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

void send_code_controller(int sock, struct sockaddr_in * serv_addr, int addr_size, uint8_t code)
{
    uint8_t code_send = code;
    if(sendto(sock, (uint8_t *)&code_send, 1, 0, (const struct sockaddr *) serv_addr, addr_size) == -1)
    {
        perror("send_code_controller");
    }
}

void send_init_controller()
{
    send_code_controller(sockfd_controller, &serv_addr, sizeof(serv_addr), INIT_MSG);
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
        response[0] &= (~CODE_OK);
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

int main(int argc, char const *argv[])
{
    uint32_t ue_address;

    if(argc < 2 || argc > 3)
    {
        printf("USE: ./ran_simulator <RAN_CONTROLLER_IP> <UE_IP (Optional)>\n");
        exit(1);
    }

    if(argc == 3)
    {
        ue_address = inet_addr(argv[2]);
        ue_ip[3] = (ue_address >> 24) & 0xFF;
        ue_ip[2] = (ue_address >> 16) & 0xFF;
        ue_ip[1] = (ue_address >> 8) & 0xFF;
        ue_ip[0] = ue_address & 0xFF;
    }

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
    //memcpy(local_ip, ue_ip, 4);
    bzero(local_ip, 4);

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
