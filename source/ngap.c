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

/* Registration Request */
#define NG_REGISTRATION_REQUEST_LEN 69
#define ID_INITIAL_UE_MESSAGE 0x0F
#define ID_RAN_UE_NGAP_ID 0x55
#define ID_NAS_PDU 0x26
#define EPD_5G_Mobility_Management_Messages 0x7e
#define MESSAGE_TYPE_REGISTRATION_REQUEST 0x41
#define ID_5GS_REGISTRATION_TYPE 0x09
#define NAS_KEY_SET_IDENTIFIER 0x70
#define ID_USER_LOCATION_INFORMATION 0x79
#define ID_NR_CGI 0x40
#define ID_RCC_ESTABLISHMENT_CAUSE 0x5a
#define CAUSE_MT_ACCESS 0x10
#define ID_UE_CONTEXT_REQUEST 0x70

/* Authentication Request */
#define ID_DOWNLINK_NAS_TRANSPORT 0x04
#define ID_AMF_UE_NGAP_ID 0x0a
#define ID_RAN_UE_NGAP_ID 0x55
#define AUTHENTICATION_REQUEST 0x56

/* Authentication Response */
#define ID_UPLINK_NAS_TRANSPORT 0x2e
#define ID_AMF_UE_NGAP_ID 0x0a
#define MESSAGE_TYPE_AUTHENTICATION_RESPONSE 0x57

void memory_dump(uint8_t * mem, int len)
{
	int i;
	for(i = 0; i < len; i++){
		if(i % 16 == 0)
			printf("\n");
		printf("%.2x ", mem[i]);
	}
	printf("\n");
}

uint8_t reverse_byte(uint8_t x)
{
    static const uint8_t table[] = {
        0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
        0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
        0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
        0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
        0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
        0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
        0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
        0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
        0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
        0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
        0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
        0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
        0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
        0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
        0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
        0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
        0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
        0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
        0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
        0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
        0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
        0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
        0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
        0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
        0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
        0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
        0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
        0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
        0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
        0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
        0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
        0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };
    return table[x];
}

void dumpChallenge(Auth_Challenge * auth)
{
	int i;
	printf("RAND: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->RAND[i]);
	printf("\n");
	printf("AUTN: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->AUTN[i]);
	printf("\n");
	printf("CK: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->CK[i]);
	printf("\n");
	printf("IK: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->IK[i]);
	printf("\n");
	printf("AK: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->AK[i]);
	printf("\n");
	printf("RES: ");
	for(i = 0; i < 8; i++)
		printf("%.2x ", auth->RES[i]);
	printf("\n");
}

void analyze_message_items(eNB * enb, UE * ue, uint8_t * buffer, int len)
{
	uint16_t num_items;
	int i, j;
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
			case ID_AMF_UE_NGAP_ID:
				printf("\tAMF-UE-NGAP-ID\n");
				set_mme_s1ap_id(ue, buffer + offset + 3, (uint8_t) buffer[offset+2]);
				offset += 3 + buffer[offset + 2];
				break;
			case ID_RAN_UE_NGAP_ID:
				printf("\tRAN-UE-NGAP-ID\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_NAS_PDU:
				printf("\tAuthentication Request\n");
				if(buffer[offset+6] == AUTHENTICATION_REQUEST)
				{
					printf("\tAuthentication Parameter RAND\n");
					printf("\t\t");
					for(j = 0; j < 16; j++)
						printf("%.2x ", buffer[offset+12+j]);
					printf("\n");
					memcpy(get_auth_challenge(ue)->RAND, buffer+offset+12, 16);
					printf("\tAuthentication Parameter AUTN\n");
					printf("\t\t");
					for(j = 0; j < 16; j++)
						printf("%.2x ", buffer[offset+30+j]);
					printf("\n");
					memcpy(get_auth_challenge(ue)->AUTN, buffer+offset+30, 16);
				}
				offset += 3 + buffer[offset + 2];
				break;
			default:
				printf("Unknown protocolIE ID\n");
		}
	}
}

int analyze_NG_SetupResponse(eNB * enb, uint8_t * buffer, int len)
{
	printInfo("Analyzing NG Setup Response...\n");
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

	return OK;
}

int analyze_NG_AuthenticationRequest(UE * ue, eNB * enb, uint8_t * buffer, int len)
{
	printInfo("Analyzing Authentication Request...\n");
	if(buffer[0] != 0x00 || buffer[1] != ID_DOWNLINK_NAS_TRANSPORT) {
		printError("Invalid NG Downlink Nas Transport\n");
		return ERROR;
	}
	/* Check criticality */
	if(buffer[2] != CRITICALITY_IGNORE) {
		printError("Wrong Criticality\n");
		return ERROR;
	}
	analyze_message_items(enb, ue, buffer+4, buffer[3]);

	return OK;
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
		printError("No data received from AMF\n");
		return ERROR;
	}
	return analyze_NG_SetupResponse(enb, buffer, recv_len);
}

int add_NG_Registration_Request_Header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_INITIAL_UE_MESSAGE;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x41; /* Content Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 5; /* protocolIEs: 5 items */
	return 7;
}

int add_protocolIE_RAN_UE_NGAP_ID(UE * ue, uint8_t * buffer)
{
	uint32_t ngap_id = get_ue_s1ap_id(ue);
	buffer[0] = 0;
	buffer[1] = ID_RAN_UE_NGAP_ID;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 2; /* ID Length */
	buffer[4] = reverse_byte((ngap_id >> 8) & 0xFF);
	buffer[5] = reverse_byte(ngap_id & 0xFF);
	return 6;
}

int add_protocolIE_NAS_PDU_Registration_Request(UE * ue, eNB * enb, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_NAS_PDU;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x17; /* Length */
	buffer[4] = 0x16; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0; /* Security header type */
	buffer[7] = MESSAGE_TYPE_REGISTRATION_REQUEST;
	buffer[8] = ID_5GS_REGISTRATION_TYPE | NAS_KEY_SET_IDENTIFIER;
	/* 5GS mobile identity */
	buffer[9] = 0;
	buffer[10] = 0x0C; /* Length */
	buffer[11] = 0x01; /* SUPI format and Type of Identity */
	memcpy(buffer+12, get_plmn(enb), PLMN_LENGTH);
	buffer[15] = 0xF0; /* Routing indicator */
	buffer[16] = 0xFF; /* Routing indicator */
	buffer[17] = 0; /* Protection scheme */
	buffer[18] = 0; /* Home network public key identifier */
	memcpy(buffer+19, get_ue_msin(ue)+1, 4); /* Scheme output */
	/* UE Security capabilities */
	buffer[23] = 0x2e; /* Element ID */
	buffer[24] = 0x02; /* Sec. Cap. Length */
	buffer[25] = 0x80; /* Encryption */
	buffer[26] = 0x20; /* Integrity */
	return 27;
}

int add_protocolIE_User_Location_Information(eNB * enb, uint8_t * buffer)
{
	uint32_t enb_id = get_enb_id(enb);
	buffer[0] = 0;
	buffer[1] = ID_USER_LOCATION_INFORMATION;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x0F;
	/* nR-CGI */
	buffer[4] = ID_NR_CGI;
	memcpy(buffer+5, get_plmn(enb), PLMN_LENGTH); /* PLMN */
	buffer[8] = 0;
	buffer[9] = (enb_id >> 24) & 0xFF;
	buffer[10] = (enb_id >> 16) & 0xFF;
	buffer[11] = (enb_id >> 8) & 0xFF;
	buffer[12] = enb_id & 0xFF;
	memcpy(buffer+13, get_plmn(enb), PLMN_LENGTH); /* PLMN */
	buffer[16] = 0x00; /* tAI */
	buffer[17] = 0x00; /* tAI */
	buffer[18] = 0x01; /* tAI */
	return 19;
}

int add_protocolIE_RRC_Establishment_Cause(uint8_t cause, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_RCC_ESTABLISHMENT_CAUSE;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x01; /* Cause Length */
	buffer[4] = cause;
	return 5;
}

int add_protocolIE_UE_Context_Request(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_UE_CONTEXT_REQUEST;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x01; /* UEContextRequest Length */
	buffer[4] = 0;
	return 5;
}

int generate_NG_Registration_Request(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_Registration_Request_Header(buffer);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_Registration_Request(ue, enb, buffer+offset);
	offset += add_protocolIE_User_Location_Information(enb, buffer+offset);
	offset += add_protocolIE_RRC_Establishment_Cause(CAUSE_MT_ACCESS, buffer+offset);
	offset += add_protocolIE_UE_Context_Request(buffer+offset);

	return offset;
}

int add_NG_Registration_Response_Header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_UPLINK_NAS_TRANSPORT;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x3C; /* Content Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 4; /* protocolIEs: 4 items */
	return 7;
}

int add_protocolIE_AMF_UE_NGAP_ID(UE * ue, uint8_t * buffer)
{
	uint8_t len;
	uint8_t * amf_id;
	amf_id =  get_mme_s1ap_id(ue, &len);
	buffer[0] = 0;
	buffer[1] = ID_AMF_UE_NGAP_ID;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = len; /* ID Length */
	memcpy(buffer+4, amf_id, len);
	return 4+len;
}

int add_protocolIE_NAS_PDU_Registration_Response(UE * ue, eNB * enb, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_NAS_PDU;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x16; /* Length */
	buffer[4] = 0x15; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0; /* Plain message */
	buffer[7] = MESSAGE_TYPE_AUTHENTICATION_RESPONSE;
	buffer[8] = 0x2d; /* Element ID */
	buffer[9] = 16; /* RES Length */
	generate_res_5g(buffer+10, get_auth_challenge(ue)->CK, get_auth_challenge(ue)->IK, get_auth_challenge(ue)->RAND, get_auth_challenge(ue)->RES, get_ue_mcc(ue), get_ue_mnc(ue)); /* RES* */
	return 26;
}

int generate_NG_Authentication_Response(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_Registration_Response_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_Registration_Response(ue, enb, buffer+offset);
	offset += add_protocolIE_User_Location_Information(enb, buffer+offset);

	return offset;
}

int procedure_Registration_Request(eNB * enb, UE * ue)
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

	/* Registration Request */
	len = generate_NG_Registration_Request(enb, ue, buffer);

	/* Sending request */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from AMF\n");
		return ERROR;
	}
	if(analyze_NG_AuthenticationRequest(ue, enb, buffer, recv_len) == 1)
	{
		printError("Wrong NG Authentication Request\n");
		return ERROR;
	}

	//uint8_t key[16] = {0x51, 0x22, 0x25, 0x02, 0x14, 0xc3, 0x3e, 0x72, 0x3a, 0x5d, 0xd5, 0x23, 0xfc, 0x14, 0x5f, 0xc0};
	//uint8_t opc[16] = {0x98, 0x1d, 0x46, 0x4c, 0x7c, 0x52, 0xeb, 0x6e, 0x50, 0x36, 0x23, 0x49, 0x84, 0xad, 0x0b, 0xcf};
	//uint8_t rand[16] = {0x37, 0x98, 0x50, 0x22, 0x34, 0x07, 0x4f, 0xe7, 0x0a, 0xaa, 0x5a, 0xf5, 0xca, 0x78, 0x59, 0xa3};
	//uint8_t ck[16];
	//uint8_t ik[16];
	//uint8_t ak[6];
	//uint8_t res[8];
	//uint8_t out[16];
	//uint8_t autn[16] = {0x98, 0x77, 0x44, 0x1d, 0x6a, 0x3b, 0x80, 0x00, 0x39, 0xa3, 0x14, 0x56, 0x7e, 0x90, 0x7e, 0xf7};
	//key: 5122250214c33e723a5dd523fc145fc0
	//opc: 981d464c7c52eb6e5036234984ad0bcf
	//rand: 3798502234074fe70aaa5af5ca7859a3
	//autn: 9877441d6a3b800039a314567e907ef7
	//res: da3de98d4e7c010d863cb52bf1f9fc33

	f2345(get_ue_key(ue), get_auth_challenge(ue)->RAND, get_auth_challenge(ue)->RES, get_auth_challenge(ue)->CK, get_auth_challenge(ue)->IK, get_auth_challenge(ue)->AK, get_ue_op_key(ue));
	dumpChallenge(get_auth_challenge(ue));

	//f2345(key, rand, res, ck, ik, ak, opc);
	//generate_res_5g(out, ck, ik, rand, res);
	//int i;
	//printf("RES 5G: ");
	//for(i = 0; i < 16; i++)
	//	printf("%.2x ", out[i]);
	//printf("\n");

	/* Registration Response */
	len = generate_NG_Authentication_Response(enb, ue, buffer);

	/* Sending request */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from AMF\n");
		return ERROR;
	}
	//if(analyze_NG_AuthenticationRequest(ue, enb, buffer, recv_len) == 1)
	//{
	//	printError("Wrong NG Authentication Request\n");
	//	return ERROR;
	//}


	return ERROR;
}

