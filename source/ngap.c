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

/* Security Mode Command */
#define SECURITY_MODE_COMMAND 0x5D

/* Security Mode Complete */
#define REGISTRATION_REQUEST 0x41
#define SECURITY_MODE_COMPLETE 0x5e

/* Resgistration Accept */
#define ID_INITIAL_CONTEXT_SETUP 0x0E
#define ID_GUAMI 0x1C
#define ID_ALLOWED_NSSAI 0x00
#define ID_UE_SECURITY_CAPABILITIES 0x77
#define ID_SECURITY_KEY 0x5E
#define ID_MOBILE_RESTRICTION_LIST 0x24
#define ID_MASKED_IMEI_SV 0x22
#define REGISTRATION_ACCEPT 0x42

/* PDU Session Establishment Accept */
#define ID_PDU_SESSION_RESOURCE_SETUP 0x1d
#define UL_NAS_TRANSPORT 0x67
#define EPD_5G_Session_Management_Messages 0x2e
#define ID_UE_AGGREGATE_MAXIMUM_BIT_RATE 0x6e
#define ID_PDU_SESSION_RESOURCE_SETUP_LIST_SU_REQ 0x4a

/* Registration Complete */
#define REGISTRATION_COMPLETE 0x43

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

void dumpChallengeDerivatedKeys(Auth_Challenge * auth)
{
	int i;
	printf("NAS ENC: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->NAS_KEY_ENC[i]);
	printf("\n");
	printf("NAS INT: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->NAS_KEY_INT[i]);
	printf("\n");
}

void analyze_message_items(eNB * enb, UE * ue, uint8_t * buffer, int len)
{
	uint16_t num_items;
	int i, j;
	int offset = 3;
	uint8_t aux_offset;

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
			case ID_GUAMI:
				printf("\tGUAMI\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_ALLOWED_NSSAI:
				printf("\tAllowed NSSAI\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_UE_SECURITY_CAPABILITIES:
				printf("\tUE Security Capabilities\n");
				printf("\t\t5G Encryption Algorithms: 0x%.2x\n", buffer[offset + 3] >> 5);
				printf("\t\t5G Integrity Algorithms: 0x%.2x\n", buffer[offset + 5] >> 5);
				printf("\t\t4G Encryption Algorithms: 0x%.2x\n", buffer[offset + 7] >> 5);
				printf("\t\t4G Integrity Algorithms: 0x%.2x\n", buffer[offset + 9] >> 5);
				offset += 3 + buffer[offset + 2];
				break;
			case ID_SECURITY_KEY:
				printf("\tSecurity Key\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_MOBILE_RESTRICTION_LIST:
				printf("\tMobile Restriction List\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_MASKED_IMEI_SV:
				printf("\tMasked IMEI SV\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_UE_AGGREGATE_MAXIMUM_BIT_RATE:
				printf("\tUE Aggregate Maximum Bit Rate\n");
				offset += 3 + buffer[offset + 2];
				break;
			case ID_PDU_SESSION_RESOURCE_SETUP_LIST_SU_REQ:
				printf("\tPDU Session Resource Setup List SU Req\n");
				printf("\tPDU Session ID: %d\n", buffer[offset + 5]);
				aux_offset = buffer[offset + 6] + 5 + 7; /* Header length + NAS PDU length + NSSAI length */
				printf("\tNAS 5GS PDU:\n");
				if(buffer[offset + 7] == 0x7e)
				{
					printf("\t\tExtended protocol discriminator: 5G mobility management messages\n");
					printf("\t\tSecurity header type: 0x%.2x\n", buffer[offset + 8]);
					printf("\t\tMessage Authentication Code: 0x%.2x%.2x%.2x%.2x\n", buffer[offset+9], buffer[offset+10], buffer[offset+11], buffer[offset+12]);
					printf("\t\tSequence Number: %d\n", buffer[offset+13]);
					printf("\t\tDecoded as 5G-EA0 ciphered message!\n");
					printf("\t\tPDU Address: %d.%d.%d.%d\n", buffer[offset + 48], buffer[offset + 49], buffer[offset + 50], buffer[offset + 51]);
					set_pdn_ip(ue, buffer+offset+48);
					/* Transport Layer Address */
					printf("\t\tTransport Layer Address: %d.%d.%d.%d\n", buffer[offset + aux_offset + 10], buffer[offset + aux_offset + 11], buffer[offset + aux_offset + 12], buffer[offset + aux_offset + 13]);
					set_spgw_ip(ue, buffer+offset+aux_offset+10);
					/* Safe TEID */
					uint32_t teid;
					printf("\t\tTEID: 0x%.2x%.2x%.2x%.2x\n", buffer[offset + aux_offset + 14], buffer[offset + aux_offset + 15], buffer[offset + aux_offset + 16], buffer[offset + aux_offset + 17]);
					teid = (buffer[offset + aux_offset + 14] << 24) | buffer[offset + aux_offset + 15] << 16 | buffer[offset + aux_offset + 16] << 8 | buffer[offset + aux_offset + 17];
					set_gtp_teid(ue, teid);
				}

				offset += 3 + buffer[offset + 2];
				break;
			case ID_NAS_PDU:
				if(buffer[offset+5] == 0x02) /* NAS PDU Integrity protected and ciphered */
				{
					printf("\tNAS PDU Integrity protected and ciphered\n");
					if(buffer[offset+13] == REGISTRATION_ACCEPT)
					{
						printf("\tResgistration Accept\n");
						printf("\tResgistration Result: 0x%.2x\n", buffer[offset+15]);
						printf("\t5G-TMSI: 0x%.2x%.2x%.2x%.2x\n", buffer[offset+26], buffer[offset+27], buffer[offset+28], buffer[offset+29]);
						/* Saving 5G-GUTI */
						set_guti(ue, buffer+offset+20);
					}
				}
				if(buffer[offset+5] == 0x03) /*NAS PDU Integrity protected*/
				{
					printf("\tNAS PDU Integrity protected\n");
					printf("\tMessage Authentication Code: 0x%.2x%.2x%.2x%.2x\n", buffer[offset+6], buffer[offset+7], buffer[offset+8], buffer[offset+9]);
					printf("\tSequence Number: %d\n", buffer[offset+10]);
					if(buffer[offset+13] == SECURITY_MODE_COMMAND)
					{
						printf("\tSecurity Mode Command\n");
						printf("\tNAS Security Algorithms:\n");
						printf("\t\tCiphering: %d\n", (buffer[offset+14] & 0xF0) >> 4);
						printf("\t\tIntegrity: %d\n", buffer[offset+14] & 0x0F);
						set_nas_session_security_algorithms(ue, buffer[offset+14]);
					}
				}
				if(buffer[offset+5] == 0) /* NAS PDU Plain message */
				{
					printf("\tNAS PDU Plain message\n");
					printf("\tAuthentication Request\n");
					if(buffer[offset+6] == AUTHENTICATION_REQUEST)
					{
						printf("\tAuthentication Request\n");
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
				}
				offset += 3 + buffer[offset + 2];
				break;
			default:
				printf("\tUnknown protocolIE ID\n");
				offset += 3 + buffer[offset + 2];
				break;
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

int analyze_NG_SecurityModeCommand(UE * ue, eNB * enb, uint8_t * buffer, int len)
{
	printInfo("Analyzing Security Mode Command...\n");
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

int analyze_NG_RegistrationRequest(UE * ue, eNB * enb, uint8_t * buffer, int len)
{
	printInfo("Analyzing Registration Request...\n");
	if(buffer[0] != 0x00 || buffer[1] != ID_INITIAL_CONTEXT_SETUP) {
		printError("Invalid NG Initial Context Setup Request\n");
		return ERROR;
	}
	/* Check criticality */
	if(buffer[2] != CRITICALITY_REJECT) {
		printError("Wrong Criticality\n");
		return ERROR;
	}
	analyze_message_items(enb, ue, buffer+5, buffer[4]);

	return OK;
}

int analyze_NG_PDUSessionEstablishmentAccept(UE * ue, eNB * enb, uint8_t * buffer, int len)
{
	printInfo("Analyzing PDU Session Establishment Accept...\n");
	if(buffer[0] != 0x00 || buffer[1] != ID_PDU_SESSION_RESOURCE_SETUP) {
		printError("Invalid NG PDU Session Establishment Accept\n");
		return ERROR;
	}
	/* Check criticality */
	if(buffer[2] != CRITICALITY_REJECT) {
		printError("Wrong Criticality\n");
		return ERROR;
	}
	analyze_message_items(enb, ue, buffer+5, buffer[4]);

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
	buffer[3] = 0x47; /* Content Length */
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
	buffer[3] = 0x1D; /* Length */
	buffer[4] = 0x1C; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0; /* Security header type */
	buffer[7] = MESSAGE_TYPE_REGISTRATION_REQUEST;
	buffer[8] = ID_5GS_REGISTRATION_TYPE | NAS_KEY_SET_IDENTIFIER;
	/* 5GS mobile identity */
	buffer[9] = 0;
	buffer[10] = 0x0d; /* Length */
	buffer[11] = 0x01; /* SUPI format and Type of Identity */
	memcpy(buffer+12, get_plmn(enb), PLMN_LENGTH);
	buffer[15] = 0x00; /* Routing indicator */
	buffer[16] = 0x00; /* Routing indicator */
	buffer[17] = 0; /* Protection scheme */
	buffer[18] = 0; /* Home network public key identifier */
	memcpy(buffer+19, get_ue_msin(ue), 5); /* Scheme output */
	/* 5GMM Capabilities */
	buffer[24] = 0x10; /* Element ID */
	buffer[25] = 0x01; /* Length */
	buffer[26] = 0x00;
	/* UE Security capabilities */
	buffer[27] = 0x2e; /* Element ID */
	buffer[28] = 0x04; /* Sec. Cap. Length */
	buffer[29] = 0x80; /* 5G Encryption */
	buffer[30] = 0xa0; /* 5G Integrity */
	buffer[31] = 0x80; /* 4G Encryption */
	buffer[32] = 0xa0; /* 4G Integrity */
	return 33;
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

int add_NG_Security_Mode_Complete_Header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_UPLINK_NAS_TRANSPORT;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x66; /* Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 4; /* protocolIEs: 5 items */
	return 7;
}

int add_protocolIE_NAS_PDU_Security_Mode_Complete(UE * ue, eNB * enb, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_NAS_PDU;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x40; /* Length */
	buffer[4] = 0x3F; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0x04; /* Integrity protected and ciphered with new 5GS security context */

	buffer[11] = get_nas_sequence_number(ue);
	/* Plain NAS message */
	buffer[12] = EPD_5G_Mobility_Management_Messages;
	buffer[13] = 0; /* Security Header Type: Plain Message */
	buffer[14] = SECURITY_MODE_COMPLETE;
	/* 5GS Mobile Identity */
	buffer[15] = 0x77; /* Element ID */
	buffer[16] = 0; /* Length */
	buffer[17] = 0x09; /* Length */
	buffer[18] = 0x45; /* Type of identity: IMEISV */
	buffer[19] = 0x73;
	buffer[20] = 0x80; 
	buffer[21] = 0x61;
	buffer[22] = 0x21;
	buffer[23] = 0x85;
	buffer[24] = 0x61;
	buffer[25] = 0x51;
	buffer[26] = 0xf1;
	/* NAS Message container */
	buffer[27] = 0x71; /* Element ID */
	buffer[28] = 0x00; /* Length */
	buffer[29] = 0x26; /* Length */
	/* Plain NAS message */
	buffer[30] = EPD_5G_Mobility_Management_Messages;
	buffer[31] = 0x00; /* Plain message */
	buffer[32] = REGISTRATION_REQUEST;
	buffer[33] = 0x79; /* 5GS registration type and NAS key set identifier */
	/* Mobile Identity */
	buffer[34] = 0x00; /* Length */
	buffer[35] = 0x0D; /* Length */
	buffer[36] = 0x01; /* SUPI format: IMSI */
	memcpy(buffer+37, get_plmn(enb), PLMN_LENGTH); /* PLMN */
	buffer[40] = 0x00; /* Routing Indicator */
	buffer[41] = 0x00; /* Routing Indicator */
	buffer[42] = 0x00; /* Protection scheme ID */
	buffer[43] = 0x00; /* Home network public key identifier */
	memcpy(buffer+44, get_ue_msin(ue), UE_MSIN_LENGTH);
	/* 5GMM capability */
	buffer[49] = 0x10; /* Element ID */
	buffer[50] = 0x01; /* Length */
	buffer[51] = 0x00;
	/* UE security capability */
	buffer[52] = 0x2e; /* Element ID */
	buffer[53] = 0x04; /* Length */
	buffer[54] = 0x80; /* 5G Ciphering */
	buffer[55] = 0xa0; /* 5G Integrity */
	buffer[56] = 0x80; /* 4G Ciphering */
	buffer[57] = 0xa0; /* 4G Integrity */
	/* NSSAI */
	buffer[58] = 0x2f; /* Element ID */
	buffer[59] = 0x05; /* Length */
	buffer[60] = 0x04; /* Length */
	buffer[61] = 0x01; /* S-NSSAI */
	buffer[62] = 0x01; /* S-NSSAI */
	buffer[63] = 0x02; /* S-NSSAI */
	buffer[64] = 0x03; /* S-NSSAI */
	/* 5GS update type */
	buffer[65] = 0x53; /* Element ID */
	buffer[66] = 0x01; /* Length */
	buffer[67] = 0x00;

	/* Calculate Message Authentication Code (bearer 1 and count 0)*/
	nas_integrity_eia2(get_auth_challenge(ue)->NAS_KEY_INT, buffer+11, 57, 0, 1, buffer+7);

	return 68;
}

int generate_NG_Security_Mode_Complete(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_Security_Mode_Complete_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_Security_Mode_Complete(ue, enb, buffer+offset);
	offset += add_protocolIE_User_Location_Information(enb, buffer+offset);

	return offset;
}

int add_NG_Initial_Context_Setup_Response_Header(uint8_t * buffer)
{
	buffer[0] = SUCCESFUL_OUTCOME;
	buffer[1] = ID_INITIAL_CONTEXT_SETUP;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x0F; /* Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 2; /* protocolIEs: 2 items */
	return 7;
}

int generate_NG_InitialContextSetupResponse(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_Initial_Context_Setup_Response_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);

	return offset;
}

int add_NG_Registration_Complete_Header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_UPLINK_NAS_TRANSPORT;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x31; /* Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 4; /* protocolIEs: 4 items */
	return 7;
}

int add_protocolIE_NAS_PDU_Registration_Complete(UE * ue, eNB * enb, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_NAS_PDU;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x0B; /* Length */
	buffer[4] = 0x0A; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0x02; /* Integrity protected and ciphered */
	/* Message authentication code */
	buffer[11] = get_nas_sequence_number(ue);
	/* Plain NAS 5GS Message */
	buffer[12] = EPD_5G_Mobility_Management_Messages;
	buffer[13] = 0x00; /* Plain NAS Message */
	buffer[14] = REGISTRATION_COMPLETE;

	/* Calculate Message Authentication Code (bearer 1 and count 1)*/
	nas_integrity_eia2(get_auth_challenge(ue)->NAS_KEY_INT, buffer+11, 4, 1, 1, buffer+7);
	return 15;
}

int generate_NG_RegistrationComplete(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_Registration_Complete_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_Registration_Complete(ue, enb, buffer+offset);
	offset += add_protocolIE_User_Location_Information(enb, buffer+offset);

	return offset;
}

int add_NG_PDU_Session_Request_Header(uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_UPLINK_NAS_TRANSPORT;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x5a; /* Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 4; /* protocolIEs: 4 items */
	return 7;
}

int add_protocolIE_NAS_PDU_PDU_Session_Request(UE * ue, eNB * enb, uint8_t * buffer)
{
	char dnn[8] = "internet";
	buffer[0] = 0;
	buffer[1] = ID_NAS_PDU;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x34; /* Length */
	buffer[4] = 0x33; /* Length - 1 */
	buffer[5] = EPD_5G_Mobility_Management_Messages;
	buffer[6] = 0x02; /* Integrity protected and ciphered */
	/* Message Authentication Code */
	buffer[11] = get_nas_sequence_number(ue);
	/* Plain NAS 5GS Message */
	buffer[12] = EPD_5G_Mobility_Management_Messages;
	buffer[13] = 0x00; /* Plain NAS Message */
	buffer[14] = UL_NAS_TRANSPORT;
	buffer[15] = 0x01; /* Payload container type: N1 SM information */
	/* Payload container */
	buffer[16] = 0x00; /* Length */
	buffer[17] = 0x12; /* Length */
	buffer[18] = EPD_5G_Session_Management_Messages;
	buffer[19] = 0x01; /* PDU session identity value */
	buffer[20] = 0x01; /* Procedure transaction identity */
	buffer[21] = 0xc1; /* Message type: PDU session establishment request */
	buffer[22] = 0xFF; /* Integrity protection maximum data rate */
	buffer[23] = 0xFF; /* Integrity protection maximum data rate */
	buffer[24] = 0x91; /* PDU session type: IPv4 */
	buffer[25] = 0xa1; /* SSC mode: 1 */
	/* Extended protocol configuration options */
	buffer[26] = 0x7b; /* Element ID */
	buffer[27] = 0x00; /* Length */
	buffer[28] = 0x07; /* Length */
	buffer[29] = 0x80;
	buffer[30] = 0x00; /* Container ID: IP Address allocation via NAS signaling */
	buffer[31] = 0x0a; /* Container ID: IP Address allocation via NAS signaling */
	buffer[32] = 0x00;
	buffer[33] = 0x00; /* Container ID: DNS Server IPv4 Address Request */
	buffer[34] = 0x0d; /* Container ID: DNS Server IPv4 Address Request */
	buffer[35] = 0x00;
	buffer[36] = 0x12; /* Element ID */
	buffer[37] = 0x01; /* PDU session identity value 1 */
	buffer[38] = 0x81; /* Request type: Initial Request */
	buffer[39] = 0x22; /* Element ID */
	buffer[40] = 0x04; /* Length */
	buffer[41] = 0x01; /* SST: 1 */
	buffer[42] = 0x01; /* SD */
	buffer[43] = 0x02; /* SD */
	buffer[44] = 0x03; /* SD */
	buffer[45] = 0x25; /* Element ID */
	buffer[46] = 0x09; /* Length */
	buffer[47] = 0x08; /* DNN Length */
	memcpy(buffer+48, dnn, 8);

	/* Calculate Message Authentication Code (bearer 1 and count 2)*/
	nas_integrity_eia2(get_auth_challenge(ue)->NAS_KEY_INT, buffer+11, 45, 2, 1, buffer+7);

	return 56;
}


int generate_NG_PDUSessionRequest(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_PDU_Session_Request_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_PDU_Session_Request(ue, enb, buffer+offset);
	offset += add_protocolIE_User_Location_Information(enb, buffer+offset);

	return offset;
}



int add_NG_PDU_Session_Response_Header(uint8_t * buffer)
{
	buffer[0] = SUCCESFUL_OUTCOME;
	buffer[1] = ID_PDU_SESSION_RESOURCE_SETUP;
	buffer[2] = CRITICALITY_REJECT;
	buffer[3] = 0x24; /* Length */
	buffer[4] = 0;
	buffer[5] = 0;
	buffer[6] = 3; /* protocolIEs: 3 items */
	return 7;
}

#define ID_PDU_SESSION_RESOURCE_SETUP_LIST_SU_RES 0x4b

int add_protocolIE_NAS_PDU_PDU_Session_Response(UE * ue, uint8_t * buffer)
{
	buffer[0] = 0;
	buffer[1] = ID_PDU_SESSION_RESOURCE_SETUP_LIST_SU_RES;
	buffer[2] = CRITICALITY_IGNORE;
	buffer[3] = 0x11; /* Length */
	buffer[4] = 0x00;
	buffer[5] = 0x00;
	buffer[6] = 0x01; /* PDU Session ID: 1 */
	buffer[7] = 0x0d;
	/* GTP Tunnel */
	buffer[8] = 0x00;
	buffer[9] = 0x03;
	buffer[10] = 0xe0;
	memcpy(buffer+11, get_ue_ip(ue), 4);
	uint32_t teid = get_gtp_teid(ue);
	buffer[15] = (teid >> 24) & 0xFF;
	buffer[16] = (teid >> 16) & 0xFF;
	buffer[17] = (teid >> 8) & 0xFF;
	buffer[18] = teid & 0xFF;
	/* QoS list */
	buffer[19] = 0x00;
	buffer[20] = 0x09;

	return 21;
}

int generate_NG_PDUSessionResponse(eNB * enb, UE * ue, uint8_t * buffer)
{
	int offset = 0;

	/* Clean the buffer */
	bzero(buffer, NG_REGISTRATION_REQUEST_LEN);

	offset = add_NG_PDU_Session_Response_Header(buffer);
	offset += add_protocolIE_AMF_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_RAN_UE_NGAP_ID(ue, buffer+offset);
	offset += add_protocolIE_NAS_PDU_PDU_Session_Response(ue, buffer+offset);

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
	uint8_t kausf[32];
    uint8_t kseaf[32];
    uint8_t kamf[32];
    char supi[15];

	int socket = get_mme_socket(enb);
	from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));

	printInfo("Generating Registration Request...\n");
	/* Registration Request */
	len = generate_NG_Registration_Request(enb, ue, buffer);
	/* Sending request */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("Registration Request sent.\n");

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	flags = 0;
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

	f2345(get_ue_key(ue), get_auth_challenge(ue)->RAND, get_auth_challenge(ue)->RES, get_auth_challenge(ue)->CK, get_auth_challenge(ue)->IK, get_auth_challenge(ue)->AK, get_ue_op_key(ue));
	dumpChallenge(get_auth_challenge(ue));

	printInfo("Generating Authentication Response...\n");
	/* Registration Response */
	len = generate_NG_Authentication_Response(enb, ue, buffer);
	/* Sending request */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("Authentication Request sent\n");

	/* Kausf, Kseaf, Kamf, NAS_ENC and NAS_INT are generated while UE is waiting for Security mode command */
	/* Only integrity is implemented in this emulator to minimize the CPU load */
	generate_kausf(kausf, get_auth_challenge(ue)->CK, get_auth_challenge(ue)->IK, get_auth_challenge(ue)->AUTN, get_ue_mcc(ue), get_ue_mnc(ue));
	generate_kseaf(kseaf, kausf, get_ue_mcc(ue), get_ue_mnc(ue));
	/* Generate SUPI */
	memcpy(supi, get_ue_mcc(ue), 3);
	memcpy(supi+3, get_ue_mnc(ue), 2);
	memcpy(supi+5, get_msin_string(ue), 10);
	generate_kamf(kamf, supi, kseaf);

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	flags = 0;
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from AMF\n");
		return ERROR;
	}
	if(analyze_NG_SecurityModeCommand(ue, enb, buffer, recv_len) == 1)
	{
		printError("Wrong NG Security Mode Command\n");
		return ERROR;
	}

	/* Generate Integrity key and Encryption key */
	generate_5g_nas_enc_key(get_auth_challenge(ue)->NAS_KEY_ENC, kamf, get_nas_session_enc_alg(ue));
    generate_5g_nas_int_key(get_auth_challenge(ue)->NAS_KEY_INT, kamf, get_nas_session_int_alg(ue));
	dumpChallengeDerivatedKeys(get_auth_challenge(ue));

	printInfo("Generating Security Mode Complete...\n");
	/* Security Mode Complete */
	len = generate_NG_Security_Mode_Complete(enb, ue, buffer);
	/* Sending Security Mode Complete */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("Security Mode Complete sent.\n");

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	flags = 0;
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from AMF\n");
		return ERROR;
	}
	if(analyze_NG_RegistrationRequest(ue, enb, buffer, recv_len) == 1)
	{
		printError("Wrong NG Registration Request\n");
		return ERROR;
	}

	printInfo("Generating Initial Context Setup Response...\n");
	/* Initial Context Setup Response */
	len = generate_NG_InitialContextSetupResponse(enb, ue, buffer);
	/* Sending Context Setup Response */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("Initial Context Setup Response sent.\n");


	printInfo("Generating Registration Complete...\n");
	/* Generate Registration Complete */
	len = generate_NG_RegistrationComplete(enb, ue, buffer);
	/* Sending Registration Complete */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("Registration Complete sent.\n");

	printInfo("Generating PDU Session Request...\n");
	/* Generate PDU Session stablishment request */
	len = generate_NG_PDUSessionRequest(enb, ue, buffer);
	/* Sending Registration Complete */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("PDU Session Request sent.\n");

	/* Receiving MME answer */
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from AMF\n");
		return ERROR;
	}
	if(analyze_NG_PDUSessionEstablishmentAccept(ue, enb, buffer, recv_len) == 1)
	{
		printError("Wrong NG PDU Session Establishment Accept\n");
		return ERROR;
	}

	printInfo("Generating PDU Session Setup Response...\n");
	/* Generate PDU Session stablishment request */
	len = generate_NG_PDUSessionResponse(enb, ue, buffer);
	/* Sending Registration Complete */
	/* MUST BE ON STREAM 60 */
	sctp_sendmsg(socket, (void *) buffer, (size_t) len, NULL, 0, htonl(SCTP_NGAP), 0, 0, 0, 0);
	printOK("PDU Session Setup Response sent.\n");

	return OK;
}

