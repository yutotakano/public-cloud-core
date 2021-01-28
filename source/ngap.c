#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <libgc.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#include "ngap.h"
#include "enb.h"
#include "ue.h"
#include "log.h"
#include "crypto.h"

#define SCTP_NGAP	60
#define BUFFER_LEN 1024
#define OK 0
#define ERROR 1

/* Globals */
#define CRITICALITY_REJECT 	0x00
#define CRITICALITY_IGNORE 	0x40
#define SUCCESFUL_OUTCOME 0x20

/* NG Setup */
#define NG_SETUP_REQUEST_LEN 53
#define ID_NG_SETUP 0x15
#define ID_GLOBAL_RAN_NODE_ID 0x1b
#define GLOBAL_GNB_ID 0
#define ID_RAN_NODE_NAME 0x52
#define RAN_NODE_NAME "Nervion"
#define ID_SUPPORTED_TA_LIST 0x66
#define ID_DEFAULT_PAGING_DRX 0x15
#define PAGING_DRX_128 0x40

/* NG Setup Response */
#define ID_AMF_NAME 0x01
#define ID_SERVER_GUAMI_LIST 0x60
#define ID_RELATIVE_AMF_CAPACITY 0x56
#define ID_PLMN_SUPPORT_LIST 0x50

void memory_dump(uint8_t * mem, int len)
{
	int i;
	printf("(%d)", len);
	for(i = 0; i < len; i++){
		if(i % 16 == 0)
			printf("\n");
		printf("%.2x ", mem[i]);
	}
	printf("\n");
}

void analyze_message_items(eNB * enb, UE * ue, uint8_t * buffer, int len)
{
	uint16_t num_items;
	int i;
	int offset = 3;

	num_items = (buffer[1] << 8) | buffer[2];

	for(i = 0; i < num_items; i++) {
		/* Skip the first byte */
		offset++;
		switch(buffer[offset]) {
			case ID_AMF_NAME:
				printf("\tAMFName\n");
				/* Unnecesary data */
				offset += 3 + buffer[offset + 2];
				break;
			case ID_SERVER_GUAMI_LIST:
				printf("\tServedGUAMIList\n");
				/* Unnecesary data */
				offset += 3 + buffer[offset + 2];
				break;
			case ID_RELATIVE_AMF_CAPACITY:
				printf("\tRelativeAMFCapacity\n");
				/* Unnecesary data */
				offset += 3 + buffer[offset + 2];
				break;
			case ID_PLMN_SUPPORT_LIST:
				printf("\tPLMNSupportList\n");
				/* Unnecesary data */
				offset += 3 + buffer[offset + 2];
				break;
			default:
				printf("Unknown protocolIE ID\n");
		}
	}
}

int analyze_NG_SetupResponse(eNB * enb, uint8_t * buffer, int len)
{
	if(buffer[0] != SUCCESFUL_OUTCOME || buffer[1] != ID_NG_SETUP) {
		printError("Invalid NG Setup Response\n");
		return ERROR;
	}
	/* Check criticality */
	if(buffer[2] != CRITICALITY_REJECT) {
		printError("Wrong Criticality\n");
		return ERROR;
	}
	analyze_message_items(enb, NULL, buffer+4, buffer[3]);

	return ERROR;
}

int add_NG_Setup_header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_NG_SETUP;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x35; /* Content length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 4; /* protocolIEs: 4 items */
	return 7;
}

int add_protocolIE_Global_RAN_Node_ID(eNB * enb, uint8_t * buffer)
{
	uint32_t enb_id = get_enb_id(enb);
	buffer[0] = 0;
	buffer[1] = ID_GLOBAL_RAN_NODE_ID;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 8; /* Content length */
	buffer[4] = GLOBAL_GNB_ID;
	memcpy(buffer+5, get_plmn(enb), PLMN_LENGTH); /* PLMN */
	/* gNB ID */
	buffer[8] = (enb_id >> 24) & 0xFF;
	buffer[9] = (enb_id >> 16) & 0xFF;
	buffer[10] = (enb_id >> 8) & 0xFF;
	buffer[11] = enb_id & 0xFF;
	return 12;
}

int add_protocolIE_RAN_Node_Name(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_RAN_NODE_NAME;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 9; /* gNB length */
	buffer[4] = 0x03; /* Name CRC */
	buffer[5] = 0; /* Name CRC */
	memcpy(buffer+6, RAN_NODE_NAME, 7);
	return 13;
}

int add_protocolIE_TA_List(eNB * enb, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_SUPPORTED_TA_LIST;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x10; /* Length */
	buffer[4] = 0;
	buffer[5] = 0; /* Supported TA Item */
	buffer[6] = 0; /* tAC */
	buffer[7] = 0; /* tAC */
	buffer[8] = 1; /* tAC */
	buffer[9] = 0; /* Broadcast PLMN Item */
	memcpy(buffer+10, get_plmn(enb), PLMN_LENGTH); /* pLMN Identity */
	buffer[13] = 0; /* tAI Slice Support List */
	buffer[14] = 0; /* tAI Slice Support List */
	buffer[15] = 0x10; /* s-NSSAI */
	buffer[16] = 0x08; /* s-NSSAI */
	buffer[17] = 0x01; /* s-NSSAI */
	buffer[18] = 0x02; /* s-NSSAI */
	buffer[19] = 0x03; /* s-NSSAI */
	return 20;
}

int add_protocolIE_DefaultPaging_DRX(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_DEFAULT_PAGING_DRX;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 1; /* Length */
	buffer[4] = PAGING_DRX_128;
	return 5;
}

int generate_NG_SetupRequest(uint8_t * buffer, eNB * enb)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_SETUP_REQUEST_LEN);

	/* NGSetup header */
	offset = add_NG_Setup_header(buffer);
	offset += add_protocolIE_Global_RAN_Node_ID(enb, buffer+offset);
	offset += add_protocolIE_RAN_Node_Name(buffer+offset);
	offset += add_protocolIE_TA_List(enb, buffer+offset);
	offset += add_protocolIE_DefaultPaging_DRX(buffer+offset);

	return offset;
}

void test_procedure_NG_Setup(eNB * enb)
{
	uint8_t buffer[BUFFER_LEN];
	int len;
	len = generate_NG_SetupRequest(buffer, enb);
	memory_dump(buffer, len);
}


int procedure_NG_Setup(eNB * enb)
{
	uint8_t buffer[BUFFER_LEN];
	uint16_t len = 0;
	int flags = 0;
	int recv_len;
	struct sockaddr_in addr;
	struct sctp_sndrcvinfo sndrcvinfo;
	socklen_t from_len;

	int socket = get_mme_socket(enb);
	from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));

	len = generate_NG_SetupRequest(buffer, enb);

	/* Sending request */
	/* MUST BE ON STREAM 0 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from MME\n");
		return 1;
	}
	return analyze_NG_SetupResponse(enb, buffer, recv_len);
}