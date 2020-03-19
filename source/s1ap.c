#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <libgc.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/sctp.h>
#include <errno.h>

#include "s1ap.h"
#include "enb.h"
#include "ue.h"
#include "log.h"
#include "crypto.h"

/* eNB Defines */
#define S1_SETUP 	0x0011
#define GLOBAL_ENB_ID 	0x003B
#define ENB_NAME 	0x003C
#define SUPPORTED_TAS 	0x0040
#define DEFAULT_PAGING_DRX 	0x0089
#define PAGING_DRX 	0x40
#define CRITICALITY_REJECT 	0x00
#define CRITICALITY_IGNORE 	0x40
#define BUFFER_LEN 	2048
#define SCTP_S1AP	18
#define S1_SETUP_SUCCESSFUL	0x20
#define S1_SETUP_UNSUCCESSFUL	0x40
#define Cause 0x0002
#define TimeToWait 0x0041
#define MMERelativeCapacity	0x0057
#define ServedGUMMEIs	0x0069


/* UE Defines */
#define INITIAL_UE_MESSAGE	0x000C
#define ID_ENB_UE_S1AP_ID	0x0008
#define ENB_UE_S1AP_ID_LENGTH 4
#define ID_NAS_PDU 0x001A
#define NAS_SECURITY_HEADER_TYPE	0x00
#define NAS_PROTOCOL_DISCRIMINATOR	0x07
#define NAS_EPS_MMM_TYPE	0x41
#define TSC	0x00
#define KEY_SET_IDENTIFIER	0x70
#define SPARE_BIT 0x00
#define EPS_ATTACH_TYPE 0x01
#define EPS_MOBILE_IDENTITY_LENGTH 0x08
#define IMSI_LENGTH 8
#define ESM_LENGTH 0x0020
#define EPS_BEARER_IDENTITY 0x00
#define EPS_SESSION_MANAGEMENT_MESSAGE 0x02 
#define PROCEDURE_TRANSACTION_IDENTITY 0x01
#define PDN_CONNECTIVITY_REQUEST 0xD0
#define PDN_IPV4 0x10
#define INITIAL_REQUEST 0x01
#define ELEMENT_ID 0x27
#define PROTOCOL_CONF_LENGTH 0x1a
#define PROTOCOL_CONF_FLAG 0x80
#define CONTAINER_ID_IPCP 0x8021
#define CONTAINER_ID_IPCP_LENGTH 0x10
#define PPP_IP_CONTROL_PROT_CODE 0x01
#define PPP_IP_CONTROL_PROT_IDENTIFIER 0x00
#define PPP_IP_CONTROL_PROT_LENGTH 0x0010
#define DNS_PRIMARY_TYPE 0x81
#define DNS_LENGTH 0x06
#define DNS_NUM_BYTES 4
#define DNS_SECONDARY_TYPE 0x83
#define CONTAINER_ID_DNS_IP_REQ 0x000d
#define CONTAINER_ID_IP_NAS 0x000a
#define ID_TAI 0x0043
#define TAC 0x0001
#define ID_EUTRAN_CGI 0x0064
#define ID_RRC_ESTABLISHMENT_CAUSE 0x0086
#define MO_SIGNALLING 0x30
#define DOWNLINK_NAS_TRANSPORT 0x0b
#define AUTHENTICATION_REQUEST 0x52
#define SECURITY_MODE_COMMAND 0x5d
#define ID_MME_UE_S1AP_IE_MME 0x0000
#define ID_ENB_UE_S1AP_IE_UE 0x0008
#define RES_LENGTH 8


/* eNB Structures */

struct _s1ap_value
{
	uint8_t len;
	uint8_t * content;
};

struct _default_paging_drx
{
	uint8_t pagingDRX; /* 0x40 */
};

struct _supported_tas
{
	uint8_t preface[4]; /* 00 00 00 40 */
	uint8_t plmn_identity[PLMN_LENGTH];
};

struct _id_enb_name
{
	uint16_t enb_name_id; /* = 0x003C;  60 */
	uint8_t criticality; /* = CRITICALITY_IGNORE;*/
	s1ap_value value; /* enb_name in base64 */
};

struct _global_enb_id
{
	uint8_t plmn_identity[4]; /* PLMN */
	uint32_t enb_id; /* Macro */
};

struct _ProtocolIE
{
	uint16_t id;
	uint8_t criticality; /* = CRITICALITY_REJECT*/
	uint8_t * value; /* global_enb_id */
	uint8_t value_len;
};

struct _S1SetupRequest
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0004 */
	uint8_t * items; /* ProtocolIE(global_enb_id) +  ProtocolIE(enb_name) + ProtocolIE(supported_tas) + ProtocolIE(default_pagging_drx)*/
	uint16_t items_len;
};

struct _s1ap_initiatingMessage
{
	uint16_t procedureCode;
	uint8_t criticality; /* = CRITICALITY_REJECT;*/
	uint8_t * value; /* S1SetupRequest */
	uint16_t value_len;
};

struct _S1SetupResponse
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};





/* UE Structures */

struct _enb_ue_s1ap_id
{
	uint32_t id;
};

struct _non_access_stratum_pdu
{
	/* PROBLEMS */
	/* By default, gcc add a padding in uint16_t variables to make them start in an even offset */
	/* To address it, uint16_t variables have been converted in two uint8_t variables */
	
	uint8_t security_header_type_protocol_discriminator;
	uint8_t nas_eps_mmm_type;
	uint8_t tsc_key_set_identifier_spare_bit_eps_attach_type;
	/* EPS mobile identity */
	uint8_t eps_mobile_identity_length;
	uint8_t imsi[IMSI_LENGTH];
	uint8_t net_capabilities[UE_CAPABILITIES_LENGTH];
	/* ESM Message Container */
	uint8_t esm_msg_container_length[2];
	uint8_t eps_bearer_identity_eps_protocol_discriminator;
	uint8_t procedure_transaction_identity;
	uint8_t nas_eps_session_management_message;
	uint8_t pdn_type_request_type;
	/* Protocol Configuration Options */
	uint8_t element_id;
	uint8_t protocol_conf_length;
	uint8_t protocol_conf_flag;
	/* Container 1 */
	uint8_t container_id_ipcp[2];
	uint8_t container_length;
	/* PPP IP Control Protocol */
	uint8_t ppp_ip_control_prot;
	uint8_t ppp_ip_control_prot_identifier;
	uint8_t ppp_ip_control_prot_length[2];
	/* Primary DNS */
	uint8_t dns_primary_type;
	uint8_t dns_primary_length;
	uint8_t dns_primary[DNS_NUM_BYTES];
	/* Secondary DNS */
	uint8_t dns_secondary_type;
	uint8_t dns_secondary_length;
	uint8_t dns_secondary[DNS_NUM_BYTES];
	/* Container 2 */
	uint8_t container_id_dns_ip_req[2];
	uint8_t container_id_dns_ip_req_len;
	/* Container 3 */
	uint8_t container_id_ip_nas[2];
	uint8_t container_id_ip_nas_len;
};

struct _tai
{
	/* 1 extra byte for padding */
	uint8_t plmn_identity[UE_PLMN_LENGTH+1];
	uint8_t tAC[2];
};

struct _eutran_cgi
{
	/* 1 extra byte for padding */
	uint8_t plmn_identity[UE_PLMN_LENGTH+1];
	uint32_t enb_id;
};

struct _rrc_establishment_cause
{
	uint8_t establishment_cause;
};

struct _Authentication_Request
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};

struct _Auth_Challenge
{
	uint8_t RAND[16];
	uint8_t AUTN[16];
	uint8_t CK[16];
	uint8_t IK[16];
	uint8_t AK[16];
	uint8_t RES[RES_LENGTH];
	uint8_t KASME[32];
	uint8_t NAS_KEY_ENC[16];
	uint8_t NAS_KEY_INT[16];
};

struct _Security_Mode_Command
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};









/* Auxiliary function for debugging purposes */
void dumpMemory(uint8_t * pointer, int len)
{
	int i;
	for(i = 0; i < len; i++)
	{
		if(i % 16 == 0)
			printf("\n");
		printf("%.2x ", pointer[i]);
	}
	printf("\n");
}

void dump_sndrcvinfo(struct sctp_sndrcvinfo * info)
{
    printf("struct sctp_sndrcvinfo {\n");
    printf("\tsinfo_stream = %d\n", info->sinfo_stream);
    printf("\tsinfo_ssn = %d\n", info->sinfo_ssn);
    printf("\tsinfo_flags = %d\n", info->sinfo_flags);
    printf("\tsinfo_ppid = %d\n", ntohl(info->sinfo_ppid));
    printf("\tsinfo_context = %d\n", info->sinfo_context);
    printf("\tsinfo_timetolive = %d\n", info->sinfo_timetolive);
    printf("\tsinfo_tsn = %d\n", ntohl(info->sinfo_tsn));
    printf("\tsinfo_cumtsn = %d\n", info->sinfo_cumtsn);
    printf("\tsinfo_assoc_id = %d\n", info->sinfo_assoc_id);
    printf("}\n");
}

void printChallenge(Auth_Challenge * auth)
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
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->RES[i]);
	printf("\n");
}

void printChallengeDerivatedKeys(Auth_Challenge * auth)
{
	int i;
	printf("KASME: ");
	for(i = 0; i < 32; i++)
		printf("%.2x ", auth->KASME[i]);
	printf("\n");
	printf("NAS ENC: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->NAS_KEY_ENC[i]);
	printf("\n");
	printf("NAS INT: ");
	for(i = 0; i < 16; i++)
		printf("%.2x ", auth->NAS_KEY_INT[i]);
	printf("\n");
}







uint8_t * s1ap_initiatingMessage_to_buffer(s1ap_initiatingMessage * s1ap_initiatingMsg, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + s1ap_initiatingMsg->value_len);
	dump[0] = s1ap_initiatingMsg->procedureCode >> 8;
	dump[1] = s1ap_initiatingMsg->procedureCode & 0xFF;
	dump[2] = s1ap_initiatingMsg->criticality;
	memcpy(dump+3, s1ap_initiatingMsg->value, s1ap_initiatingMsg->value_len);
	*len = 3 + s1ap_initiatingMsg->value_len;
	return dump;
}

uint8_t * S1SetupRequest_to_buffer(S1SetupRequest * s1_setup_req, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + s1_setup_req->items_len);
	dump[0] = s1_setup_req->init;
	dump[1] = s1_setup_req->numItems >> 8;
	dump[2] = (uint8_t)(s1_setup_req->numItems & 0xFF);
	memcpy(dump+3, s1_setup_req->items, s1_setup_req->items_len);
	*len = 3 + s1_setup_req->items_len;
	return dump;
}

uint8_t * ProtocolIE_to_buffer(ProtocolIE * protocolIE, int * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + protocolIE->value_len);
	dump[0] = protocolIE->id >> 8;
	dump[1] = (uint8_t)(protocolIE->id & 0xFF);
	dump[2] = protocolIE->criticality;
	memcpy(dump+3, protocolIE->value, protocolIE->value_len);
	*len = 3 + protocolIE->value_len;
	return dump;

}

void freeS1ap_value(s1ap_value * value)
{
	GC_free(value->content);
	GC_free(value);
	return;
}

void freeS1ap_initiatingMessage(s1ap_initiatingMessage * s1ap_initiatingMsg)
{
	GC_free(s1ap_initiatingMsg->value);
	GC_free(s1ap_initiatingMsg);
	return;
}

void freeS1SetupRequest(S1SetupRequest * s1_setup_req)
{
	GC_free(s1_setup_req->items);
	GC_free(s1_setup_req);
	return;
}

void freeProtocolIE(ProtocolIE * protocolIE)
{
	GC_free(protocolIE->value);
	GC_free(protocolIE);
	return;
}

uint8_t * generate_s1ap_value(uint8_t * content, uint8_t len)
{
	int i;
	uint8_t * aux = (uint8_t *) GC_malloc(len+1);
	aux[0] = len;
	for(i = 0; i < len; i++)
		aux[i+1] = content[i];
	return aux;
}

ProtocolIE * generate_ProtocolIE_default_paging_drx()
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = DEFAULT_PAGING_DRX;
	protocolIE->criticality = CRITICALITY_IGNORE;

	default_paging_drx paging_drx;
	paging_drx.pagingDRX = PAGING_DRX;

	uint8_t * value = generate_s1ap_value((uint8_t *)&paging_drx, sizeof(default_paging_drx));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(default_paging_drx) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(default_paging_drx) + 1);
	protocolIE->value_len = sizeof(default_paging_drx) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_supported_tas(eNB * enb)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = SUPPORTED_TAS;
	protocolIE->criticality = CRITICALITY_REJECT;

	supported_tas sup_tas;
	sup_tas.preface[0] = 0x00;
	sup_tas.preface[1] = 0x00;
	sup_tas.preface[2] = 0x00;
	sup_tas.preface[3] = 0x40;
	memcpy(sup_tas.plmn_identity, get_plmn(enb), PLMN_LENGTH);

	uint8_t * value = generate_s1ap_value((uint8_t *)&sup_tas, sizeof(supported_tas));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(supported_tas) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(supported_tas) + 1);
	protocolIE->value_len = sizeof(supported_tas) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_enb_name(eNB * enb)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ENB_NAME;
	protocolIE->criticality = CRITICALITY_IGNORE;

	/* Hardcoded eNB name until get ASN.1 encoder */
	uint8_t name[] = {0x08, 0x80, 0x65, 0x4e, 0x42, 0x5f, 0x45, 0x75, 0x72, 0x65, 0x63, 0x6f, 0x6d, 0x5f, 0x4c, 0x54, 0x45, 0x42, 0x6f, 0x78};

	uint8_t * value = generate_s1ap_value((uint8_t *)name, 20);
	protocolIE->value = (uint8_t *) GC_malloc(21);
	memcpy(protocolIE->value, (void *) value, 21);
	protocolIE->value_len = 21;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_global_enb_id(eNB * enb)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = GLOBAL_ENB_ID;
	protocolIE->criticality = CRITICALITY_REJECT;

	global_enb_id glob_enb_id;
	glob_enb_id.plmn_identity[0] = 0;
	memcpy(glob_enb_id.plmn_identity + 1, get_plmn(enb), PLMN_LENGTH);
	glob_enb_id.enb_id = htonl(get_enb_id(enb)); /* In memory is represented like 00 00 0e 00 */

	uint8_t * value = generate_s1ap_value((uint8_t *)&glob_enb_id, sizeof(global_enb_id));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(global_enb_id) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(global_enb_id) + 1);
	protocolIE->value_len = sizeof(global_enb_id) + 1;
	GC_free(value);
	return protocolIE;
}

S1SetupRequest * generate_s1_setup_request(eNB * enb)
{
	ProtocolIE * global_enb_id;
	ProtocolIE * enb_name;
	ProtocolIE * supported_tas;
	ProtocolIE * paging_drx;
	S1SetupRequest * s1_setup_req = (S1SetupRequest *) GC_malloc(sizeof(S1SetupRequest));
	s1_setup_req->init = 0x00;
	s1_setup_req->numItems = 0x0004;

	/* 	Generate values */
	global_enb_id = generate_ProtocolIE_global_enb_id(enb);
	enb_name = generate_ProtocolIE_enb_name(enb);
	supported_tas = generate_ProtocolIE_supported_tas(enb);
	paging_drx = generate_ProtocolIE_default_paging_drx();

	int global_enb_id_len;
	uint8_t * global_enb_id_buffer = ProtocolIE_to_buffer(global_enb_id, &global_enb_id_len);
	freeProtocolIE(global_enb_id);

	int enb_name_len;
	uint8_t * enb_name_buffer = ProtocolIE_to_buffer(enb_name, &enb_name_len);
	freeProtocolIE(enb_name);

	int supported_tas_len;
	uint8_t * supported_tas_buffer = ProtocolIE_to_buffer(supported_tas, &supported_tas_len);
	freeProtocolIE(supported_tas);

	int paging_drx_len;
	uint8_t * paging_drx_buffer = ProtocolIE_to_buffer(paging_drx, &paging_drx_len);
	freeProtocolIE(paging_drx);

	int i = 0;
	s1_setup_req->items = (uint8_t *) GC_malloc(global_enb_id_len + enb_name_len + supported_tas_len + paging_drx_len);

	memcpy(s1_setup_req->items + i, global_enb_id_buffer, global_enb_id_len);
	GC_free(global_enb_id_buffer);
	i += global_enb_id_len;

	memcpy(s1_setup_req->items + i, enb_name_buffer, enb_name_len);
	GC_free(enb_name_buffer);
	i += enb_name_len;

	memcpy(s1_setup_req->items + i, supported_tas_buffer, supported_tas_len);
	GC_free(supported_tas_buffer);
	i += supported_tas_len;

	memcpy(s1_setup_req->items + i, paging_drx_buffer, paging_drx_len);
	GC_free(paging_drx_buffer);
	i += paging_drx_len;

	s1_setup_req->items_len = i;

	return s1_setup_req;
}

s1ap_initiatingMessage * generate_s1ap_initiatingMessage(eNB * enb)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = (s1ap_initiatingMessage *) GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = S1_SETUP;
	s1ap_initiatingMsg->criticality = CRITICALITY_REJECT;

	S1SetupRequest * s1_setup_req;
	s1_setup_req = generate_s1_setup_request(enb);

	uint16_t len;
	uint8_t * S1SetupRequest_buf = S1SetupRequest_to_buffer(s1_setup_req, &len);

	freeS1SetupRequest(s1_setup_req);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) S1SetupRequest_buf, len);
	GC_free(S1SetupRequest_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

char * identify_id(uint16_t id)
{
	switch(id)
	{
		case ID_MME_UE_S1AP_IE_MME:
			return "id-MME-UE-S1AP-ID_MME";
		case ID_ENB_UE_S1AP_IE_UE:
			return "id-ENB-UE-S1AP-ID";
		case ServedGUMMEIs:
			return "id-ServedGUMMEIs";
		case MMERelativeCapacity:
			return "id-MMERelativeCapacity";
		case TimeToWait:
			return "id-TimeToWait";
		case Cause:
			return "id-Cause";
		case ID_NAS_PDU:
			return "NAS-PDU";
		default:
			return "Unknown";
	}
}

void analyze_S1_Setup_Content(uint8_t * value, int len)
{
	uint16_t numElems;
	uint16_t i, j;
	int offset = 0;
	/* Skip first byte to get num of protocolIEs */
	numElems = value[1] << 8 | value[2];
	offset += 3;

	for(i = 0; i < numElems; i++)
	{
		/* Security checking */
		if(offset >= len)
			break;
		printf("ProtocolIE %d information:\n", i);
		uint16_t id = value[offset] << 8 | value[offset + 1];
		printf("\tId: %s (0x%.4x)\n", identify_id(id), id);
		offset += 2;
		printf("\tCiritcality: %.2x\n", value[offset]);
		offset++;
		uint8_t value_len = value[offset];
		offset++;
		printf("\tValue (%d) : ", value_len);
		for(j = 0; j < value_len; j++)
		{
			if(value_len == 16)
			{
				printf("...");
				break;
			}
			printf("%.2x ", value[offset + j]);
		}
		printf("\n");
		offset += value_len;
	}
}

int analyze_S1_Setup_Response(uint8_t * buffer)
{
	int ret = 1;
	S1SetupResponse s1setupResp;
	memcpy((void *) &s1setupResp, (void *) buffer, 4);
	s1setupResp.value = (uint8_t *) GC_malloc(s1setupResp.length);
	memcpy((void *) s1setupResp.value, (void *) buffer+4, s1setupResp.length);

	if(s1setupResp.procedureCode != (uint8_t)S1_SETUP || s1setupResp.criticality != CRITICALITY_REJECT)
	{
		printError("No S1 Response message\n");
		ret = 1;
	}
	/* Analyze response type */
	else if(s1setupResp.status == S1_SETUP_SUCCESSFUL)
	{
		/* Successful response */
		printOK("Connected to MME\n");
		printInfo("Analyzing S1 Setup Response Success message\n");
		analyze_S1_Setup_Content(s1setupResp.value, s1setupResp.length);
		ret = 0;
	}
	else if(s1setupResp.status == S1_SETUP_UNSUCCESSFUL)
	{
		/* Unsuccessful response */
		printError("S1 Setup Response Failure\n");
		printInfo("Analyzing S1 Setup Response Failure message\n");
		analyze_S1_Setup_Content(s1setupResp.value, s1setupResp.length);
		ret = 1;
	}
	else
	{
		/* Unknown response */
		printError("Unknown S1 Setup answer from MME\n");
		ret = 1;
	}
	GC_free(s1setupResp.value);
	return ret;
}


int procedure_S1_Setup(int socket, eNB * enb)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg;
	uint16_t len = 0;
	uint8_t * initiatingMsg_buffer;
	uint8_t buffer[BUFFER_LEN];
	int flags = 0;
	int recv_len;
	struct sockaddr_in addr;
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));

	bzero(buffer, BUFFER_LEN);

	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));

    s1ap_initiatingMsg =  generate_s1ap_initiatingMessage(enb);
	initiatingMsg_buffer = s1ap_initiatingMessage_to_buffer(s1ap_initiatingMsg, &len);
	freeS1ap_initiatingMessage(s1ap_initiatingMsg);

	/* Sending request */
	/* MUST BE ON STREAM 0 */
	sctp_sendmsg(socket, (void *) initiatingMsg_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 0, 0, 0);
	GC_free(initiatingMsg_buffer);

	/* Receiving MME answer */
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printError("No data received from MME\n");
		return 1;
	}
	return analyze_S1_Setup_Response(buffer);
}












/*****************/
/*               */
/*  UE MESSAGES  */
/*               */
/*****************/

struct _InitialUEMessage
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(eNB-UE-S1AP-ID) +  ProtocolIE(NAS-PDU) + ProtocolIE(TAI) + ProtocolIE(EUTRAN-CGI) + ProtocolIE(RRC-Establishment-Cause)*/
	uint16_t items_len;
};

uint8_t * InitialUEMessage_to_buffer(InitialUEMessage * initialUEMsg, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + initialUEMsg->items_len);
	dump[0] = initialUEMsg->init;
	dump[1] = initialUEMsg->numItems >> 8;
	dump[2] = (uint8_t)(initialUEMsg->numItems & 0xFF);
	memcpy(dump+3, initialUEMsg->items, initialUEMsg->items_len);
	*len = 3 + initialUEMsg->items_len;
	return dump;
}

ProtocolIE * generate_ProtocolIE_enb_ue_s1ap_id(UE * ue)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_ENB_UE_S1AP_ID;
	protocolIE->criticality = CRITICALITY_REJECT;

	enb_ue_s1ap_id enb_ue_id;
	enb_ue_id.id = htonl(get_ue_s1ap_id(ue));
	uint8_t * value = generate_s1ap_value((uint8_t *)&enb_ue_id, sizeof(enb_ue_s1ap_id));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(enb_ue_s1ap_id) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(enb_ue_s1ap_id) + 1);
	protocolIE->value_len = sizeof(enb_ue_s1ap_id) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_nas_pdu(UE * ue)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;

	non_access_stratum_pdu nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.nas_eps_mmm_type = NAS_EPS_MMM_TYPE;
	nas_pdu.tsc_key_set_identifier_spare_bit_eps_attach_type = TSC | KEY_SET_IDENTIFIER | SPARE_BIT | EPS_ATTACH_TYPE;
	nas_pdu.eps_mobile_identity_length = EPS_MOBILE_IDENTITY_LENGTH;
	memcpy(nas_pdu.imsi, get_ue_plmn(ue), UE_PLMN_LENGTH);
	nas_pdu.imsi[0] |= EPS_MOBILITY_IDENTITY_FLAGS;
	memcpy(nas_pdu.imsi + UE_PLMN_LENGTH, get_ue_msin(ue), UE_MSIN_LENGTH);
	memcpy(nas_pdu.net_capabilities, get_ue_capabilities(ue), UE_CAPABILITIES_LENGTH);


	nas_pdu.esm_msg_container_length[0] = (ESM_LENGTH >> 8) & 0xFF;
	nas_pdu.esm_msg_container_length[1] = (ESM_LENGTH) & 0xFF;
	nas_pdu.eps_bearer_identity_eps_protocol_discriminator = EPS_BEARER_IDENTITY | EPS_SESSION_MANAGEMENT_MESSAGE;
	nas_pdu.procedure_transaction_identity = PROCEDURE_TRANSACTION_IDENTITY;
	nas_pdu.nas_eps_session_management_message = PDN_CONNECTIVITY_REQUEST;
	nas_pdu.pdn_type_request_type = PDN_IPV4 | INITIAL_REQUEST;
	nas_pdu.element_id = ELEMENT_ID;
	nas_pdu.protocol_conf_length = PROTOCOL_CONF_LENGTH;
	nas_pdu.protocol_conf_flag = PROTOCOL_CONF_FLAG;
	nas_pdu.container_id_ipcp[0] = (CONTAINER_ID_IPCP >> 8) & 0xFF;
	nas_pdu.container_id_ipcp[1] = (CONTAINER_ID_IPCP) & 0xFF;
	nas_pdu.container_length = CONTAINER_ID_IPCP_LENGTH;
	nas_pdu.ppp_ip_control_prot = PPP_IP_CONTROL_PROT_CODE;
	nas_pdu.ppp_ip_control_prot_identifier = PPP_IP_CONTROL_PROT_IDENTIFIER;
	nas_pdu.ppp_ip_control_prot_length[0] = (PPP_IP_CONTROL_PROT_LENGTH >> 8) & 0xFF;
	nas_pdu.ppp_ip_control_prot_length[1] = (PPP_IP_CONTROL_PROT_LENGTH) & 0xFF;
	nas_pdu.dns_primary_type = DNS_PRIMARY_TYPE;
	nas_pdu.dns_primary_length = DNS_LENGTH;
	nas_pdu.dns_secondary_type = DNS_SECONDARY_TYPE;
	nas_pdu.dns_secondary_length = DNS_LENGTH;
	nas_pdu.container_id_dns_ip_req[0] = (CONTAINER_ID_DNS_IP_REQ >> 8) & 0xFF;
	nas_pdu.container_id_dns_ip_req[1] = (CONTAINER_ID_DNS_IP_REQ) & 0xFF;
	nas_pdu.container_id_dns_ip_req_len = 0x00;
	nas_pdu.container_id_ip_nas[0] = (CONTAINER_ID_IP_NAS >> 8) & 0xFF;
	nas_pdu.container_id_ip_nas[1] = (CONTAINER_ID_IP_NAS) & 0xFF;
	nas_pdu.container_id_ip_nas_len = 0x00;

	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu) + 2;
	GC_free(encapsulated);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_tai(eNB * enb)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_TAI;
	protocolIE->criticality = CRITICALITY_REJECT;

	tai tai_struct;
	bzero(&tai_struct, sizeof(tai));
	memcpy(tai_struct.plmn_identity+1, get_plmn(enb), PLMN_LENGTH);
	tai_struct.tAC[0] = (TAC >> 8) & 0xFF;
	tai_struct.tAC[1] = (TAC) & 0xFF;

	uint8_t * value = generate_s1ap_value((uint8_t *)&tai_struct, sizeof(tai));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(tai) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(tai) + 1);
	protocolIE->value_len = sizeof(tai) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_eutran_cgi(eNB * enb)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_EUTRAN_CGI;
	protocolIE->criticality = CRITICALITY_IGNORE;

	eutran_cgi cgi;
	bzero(&cgi, sizeof(eutran_cgi));
	memcpy(cgi.plmn_identity+1, get_plmn(enb), PLMN_LENGTH);
	cgi.enb_id = 0x00000e00; /* In memory is represented like 00 0e 00 00 */

	uint8_t * value = generate_s1ap_value((uint8_t *)&cgi, sizeof(eutran_cgi));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(eutran_cgi) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(eutran_cgi) + 1);
	protocolIE->value_len = sizeof(eutran_cgi) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_rrc_establishment_cause()
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_RRC_ESTABLISHMENT_CAUSE;
	protocolIE->criticality = CRITICALITY_IGNORE;

	rrc_establishment_cause cause;
	cause.establishment_cause = MO_SIGNALLING;

	uint8_t * value = generate_s1ap_value((uint8_t *)&cause, sizeof(rrc_establishment_cause));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(rrc_establishment_cause) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(rrc_establishment_cause) + 1);
	protocolIE->value_len = sizeof(rrc_establishment_cause) + 1;
	GC_free(value);
	return protocolIE;
}

InitialUEMessage * generate_Initial_UE_Message(eNB * enb, UE * ue)
{
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * tai;
	ProtocolIE * eutran_cgi;
	ProtocolIE * rrc_establishment_cause;
	InitialUEMessage * initialUEMsg = GC_malloc(sizeof(InitialUEMessage));
	initialUEMsg->init = 0x00;
	initialUEMsg->numItems = 0x0005;

	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu(ue);
	tai = generate_ProtocolIE_tai(enb);
	eutran_cgi = generate_ProtocolIE_eutran_cgi(enb);
	rrc_establishment_cause = generate_ProtocolIE_rrc_establishment_cause();

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int nas_pdu_len;
	uint8_t * nas_pdu_buffer = ProtocolIE_to_buffer(nas_pdu, &nas_pdu_len);
	freeProtocolIE(nas_pdu);

	int tai_len;
	uint8_t * tai_buffer = ProtocolIE_to_buffer(tai, &tai_len);
	freeProtocolIE(tai);

	int eutran_cgi_len;
	uint8_t * eutran_cgi_buffer = ProtocolIE_to_buffer(eutran_cgi, &eutran_cgi_len);
	freeProtocolIE(eutran_cgi);

	int rrc_establishment_cause_len;
	uint8_t * rrc_establishment_cause_buffer = ProtocolIE_to_buffer(rrc_establishment_cause, &rrc_establishment_cause_len);
	freeProtocolIE(rrc_establishment_cause);

	int i = 0;
	initialUEMsg->items = (uint8_t *) GC_malloc(enb_ue_s1ap_id_len + nas_pdu_len + tai_len + eutran_cgi_len + rrc_establishment_cause_len);

	memcpy(initialUEMsg->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(initialUEMsg->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(initialUEMsg->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	memcpy(initialUEMsg->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(initialUEMsg->items + i, rrc_establishment_cause_buffer, rrc_establishment_cause_len);
	GC_free(rrc_establishment_cause_buffer);
	i += rrc_establishment_cause_len;

	initialUEMsg->items_len = i;

	return initialUEMsg;
}

void freeInitialUEMessage(InitialUEMessage * initialUEMsg)
{
	GC_free(initialUEMsg->items);
	GC_free(initialUEMsg);
	return;
}

s1ap_initiatingMessage * generate_InitiatingMessage(eNB * enb, UE * ue)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = INITIAL_UE_MESSAGE;
	s1ap_initiatingMsg->criticality = CRITICALITY_IGNORE;

	InitialUEMessage * initialUEMsg;
	initialUEMsg = generate_Initial_UE_Message(enb, ue);

	uint16_t len;
	uint8_t * InitialUEMessage_buf = InitialUEMessage_to_buffer(initialUEMsg, &len);

	freeInitialUEMessage(initialUEMsg);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) InitialUEMessage_buf, len);
	GC_free(InitialUEMessage_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

int analyze_downlink_NAS_Transport(uint8_t * value, int len, Auth_Challenge * auth_challenge, UE * ue)
{
	uint16_t numElems;
	uint16_t i, j;
	int offset = 0;
	int offset_nas_pdu = 0;
	int flag;
	int ret = 1;
	/* Skip first byte to get num of protocolIEs */
	numElems = value[1] << 8 | value[2];
	offset += 3;

	for(i = 0; i < numElems; i++)
	{
		/* Security checking */
		if(offset >= len)
			break;
		printf("ProtocolIE %d information:\n", i);
		uint16_t id = value[offset] << 8 | value[offset + 1];
		printf("\tId: %s (0x%.4x)\n", identify_id(id), id);
		offset += 2;
		printf("\tCiritcality: %.2x\n", value[offset]);
		offset++;
		uint8_t value_len = value[offset];
		offset++;
		/* Detect NAS_PDU */
		if(id == ID_NAS_PDU)
		{
			flag = -1;
			printf("\tNAS-PDN: \n");
			offset_nas_pdu = offset;
			/* Skip double NAS encapsulation */
			offset_nas_pdu++;

			/* Check NAS Security Header Type */
			if(value[offset_nas_pdu] == 0x07)
			{
				printf("\t\tPlain NAS Message\n\t\tEPS mobility management message\n");
				offset_nas_pdu++;
			}
			else if(value[offset_nas_pdu] == 0x37)
			{
				printf("\t\tMessage authentication code: 0x%.2x%.2x%.2x%.2x\n", value[offset_nas_pdu+1], value[offset_nas_pdu+2], value[offset_nas_pdu+3], value[offset_nas_pdu+4]);
				printf("\t\tSequence Number: %.2x\n", value[offset_nas_pdu+5]);
				printf("\t\t(Second) Security header type: %.2x\n", value[offset_nas_pdu+6]);
				offset_nas_pdu += 7;
			}
			else
			{
				offset_nas_pdu++;
			}

			/* Detect NAS EPS Mobility Management Message Type */
			if(value[offset_nas_pdu] == AUTHENTICATION_REQUEST)
			{
				printf("\t\tAuthentication request message\n");
				flag = 0;
				offset_nas_pdu++;
			}
			else if(value[offset_nas_pdu] == SECURITY_MODE_COMMAND)
			{
				printf("\t\tSecurity Mode Command\n");
				/* Get Encryption and Integrity algorithms */
				set_nas_session_security_algorithms(ue, value[offset_nas_pdu + 1]);
				flag = 1;
				offset_nas_pdu += 2;
			}
			else
			{
				flag = -1;
				offset_nas_pdu++;
			}

			if(value[offset_nas_pdu] == 0x00)
				printf("\t\tASME (KSIasme)\n");
			else
				flag = -1;
			offset_nas_pdu++;

			if(flag == 0 && auth_challenge != NULL) /* Getting RAND and AUTN */
			{
				printf("\t\tRAND and AUTN extracted\n");
				memcpy(auth_challenge->RAND, value + offset_nas_pdu, 16);
				memcpy(auth_challenge->AUTN, value + offset_nas_pdu + 16, 16);
				ret = 0;
			}
			else if(flag == 1) /* Check UE capabilities */
			{
				printf("\t\tUE security capabilities: [%.2x %.2x %.2x]\n", value[offset_nas_pdu], value[offset_nas_pdu+1], value[offset_nas_pdu+2]);
				ret = 0;
			}
		}
		else
		{
			if(id == ID_MME_UE_S1AP_IE_MME)
			{
				/* Saving MME S1AP ID */
				set_mme_s1ap_id(ue, value + offset, (uint8_t) value_len);
			}
			printf("\tValue (%d) : ", value_len);
			for(j = 0; j < value_len; j++)
			{
				if(value_len == 16)
				{
					printf("...");
					break;
				}
				printf("%.2x ", value[offset + j]);
			}
			printf("\n");
			offset += value_len;
		}
	}
	return ret;
}

int analyze_Authentication_Request(uint8_t * buffer, Auth_Challenge * auth_challenge, UE * ue)
{
	int ret = 1;
	Authentication_Request auth_req;

	memcpy((void *) &auth_req, (void *) buffer, 4);
	auth_req.value = (uint8_t *) GC_malloc(auth_req.length);
	memcpy((void *) auth_req.value, (void *) buffer+4, auth_req.length);

	if(auth_req.status != 0 || auth_req.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT || auth_req.criticality != CRITICALITY_REJECT)
	{
		printError("No Downlink NAS Transport message (Authentication Request)\n");
		return 1;
	}

	/* Analyze and extract RAND and AUTN */
	printInfo("Analyzing Downlink NAS Transport message (Authentication Request)\n");
	ret = analyze_downlink_NAS_Transport(auth_req.value, auth_req.length, auth_challenge, ue);
	GC_free(auth_req.value);
	return ret;
}

int analyze_Security_Mode_Command(uint8_t * buffer, UE * ue)
{
	int ret = 1;
	Security_Mode_Command smc;

	memcpy((void *) &smc, (void *) buffer, 4);
	smc.value = (uint8_t *) GC_malloc(smc.length);
	memcpy((void *) smc.value, (void *) buffer+4, smc.length);

	if(smc.status != 0 || smc.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT || smc.criticality != CRITICALITY_REJECT)
	{
		printError("No Downlink NAS Transport message (Security Mode Command)\n");
		return 1;
	}

	/* Analyze and extract Encryption and Integrity algorithms to derivate NAS session keys */
	printInfo("Analyzing Downlink NAS Transport message (Security Mode Command)\n");
	ret = analyze_downlink_NAS_Transport(smc.value, smc.length, NULL, ue);
	GC_free(smc.value);
	return ret;
}









#define AUTHENTICATION_RESPONSE_ID 0x000d
#define NAS_EPS_MMM_TYPE_AUTHENTICATION_RESPONSE 0x53

struct _Authentication_Response
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(eNB-UE-S1AP-ID_1) + ProtocolIE(eNB-UE-S1AP-ID_2) +  ProtocolIE(NAS-PDU) + ProtocolIE(EUTRAN-CGI) + ProtocolIE(TAI)*/
	uint16_t items_len;
};

void freeAuthenticationResponse(Authentication_Response * auth_res)
{
	GC_free(auth_res->items);
	GC_free(auth_res);
}

uint8_t * AuthenticationResponse_to_buffer(Authentication_Response * auth_res, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + auth_res->items_len);
	dump[0] = auth_res->init;
	dump[1] = auth_res->numItems >> 8;
	dump[2] = (uint8_t)(auth_res->numItems & 0xFF);
	memcpy(dump+3, auth_res->items, auth_res->items_len);
	*len = 3 + auth_res->items_len;
	return dump;
}

ProtocolIE * generate_ProtocolIE_mme_ue_s1ap_id(UE * ue)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_MME_UE_S1AP_IE_MME;
	protocolIE->criticality = CRITICALITY_REJECT;

	/* Intermediate struct is no necessary because id is use directly from the UE struct */
	uint8_t len;
	uint8_t * mme_id = get_mme_s1ap_id(ue, &len);

	uint8_t * value = generate_s1ap_value(mme_id, len);
	protocolIE->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(protocolIE->value, (void *) value, len + 1);
	protocolIE->value_len = len + 1;
	GC_free(value);
	return protocolIE;
}

struct _non_access_stratum_pdu_auth_resp
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t nas_eps_mmm_type;
	uint8_t res_len;
	uint8_t res[RES_LENGTH];
};

ProtocolIE * generate_ProtocolIE_nas_pdu_auth_response(Auth_Challenge * auth_challenge)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;

	non_access_stratum_pdu_auth_resp nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu_auth_resp));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.nas_eps_mmm_type = NAS_EPS_MMM_TYPE_AUTHENTICATION_RESPONSE;
	nas_pdu.res_len = RES_LENGTH;
	memcpy(nas_pdu.res, auth_challenge->RES, RES_LENGTH);

	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu_auth_resp));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu_auth_resp) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu_auth_resp) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu_auth_resp) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu_auth_resp) + 2;
	GC_free(encapsulated);
	return protocolIE;
}

Authentication_Response * generate_Authentication_Response(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	/* TODO */
	/* Replicate generate_Initial_UE_Message function  */

	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * eutran_cgi;
	ProtocolIE * tai;


	Authentication_Response * auth_resp = GC_malloc(sizeof(InitialUEMessage));
	auth_resp->init = 0x00;
	auth_resp->numItems = 0x0005;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu_auth_response(auth_challenge);
	eutran_cgi = generate_ProtocolIE_eutran_cgi(enb);
	tai = generate_ProtocolIE_tai(enb);
	/* Special case: criticality ignore*/
	tai->criticality = CRITICALITY_IGNORE;

	int mme_ue_s1ap_id_len;
	uint8_t * mme_ue_s1ap_id_buffer = ProtocolIE_to_buffer(mme_ue_s1ap_id, &mme_ue_s1ap_id_len);
	freeProtocolIE(mme_ue_s1ap_id);

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int nas_pdu_len;
	uint8_t * nas_pdu_buffer = ProtocolIE_to_buffer(nas_pdu, &nas_pdu_len);
	freeProtocolIE(nas_pdu);

	int eutran_cgi_len;
	uint8_t * eutran_cgi_buffer = ProtocolIE_to_buffer(eutran_cgi, &eutran_cgi_len);
	freeProtocolIE(eutran_cgi);

	int tai_len;
	uint8_t * tai_buffer = ProtocolIE_to_buffer(tai, &tai_len);
	freeProtocolIE(tai);

	int i = 0;
	auth_resp->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + nas_pdu_len + eutran_cgi_len + tai_len);

	memcpy(auth_resp->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(auth_resp->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(auth_resp->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(auth_resp->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(auth_resp->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	auth_resp->items_len = i;

	return auth_resp;
}



s1ap_initiatingMessage * generate_S1AP_Authentication_Response(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = AUTHENTICATION_RESPONSE_ID;
	s1ap_initiatingMsg->criticality = CRITICALITY_IGNORE;

	Authentication_Response * auth_resp;
	auth_resp = generate_Authentication_Response(enb, ue, auth_challenge);

	uint16_t len;
	uint8_t * AuthenticationResponse_buf = AuthenticationResponse_to_buffer(auth_resp, &len);

	freeAuthenticationResponse(auth_resp);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) AuthenticationResponse_buf, len);
	GC_free(AuthenticationResponse_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}












int procedure_Attach_Default_EPS_Bearer(int socket, eNB * enb, UE * ue)
{
	s1ap_initiatingMessage * s1ap_attach_request;
	s1ap_initiatingMessage * s1ap_auth_response;
	Auth_Challenge auth_challenge;
	int err;
	uint16_t len = 0;
	uint8_t * initiatingMsg_buffer;
	uint8_t * authentication_res_buffer;
	uint8_t buffer[BUFFER_LEN];
	int flags = 0;
	int recv_len;
	struct sockaddr_in addr;
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));

	bzero(buffer, BUFFER_LEN);
	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));



	/*********************/
	/* 	 Attach Request  */
	/*********************/
	s1ap_attach_request = generate_InitiatingMessage(enb, ue);
	initiatingMsg_buffer = s1ap_initiatingMessage_to_buffer(s1ap_attach_request, &len);
	freeS1ap_initiatingMessage(s1ap_attach_request);
	/* Sending request */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) initiatingMsg_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(initiatingMsg_buffer);

	/* Receiving MME answer */
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printf("%s\n", strerror(errno));
		printError("SCTP (%s)\n", strerror(errno));
		return 1;
	}

	/* Analyze buffer and get AUTHENTICATION REQUEST*/
	err = analyze_Authentication_Request(buffer, &auth_challenge, ue);
	if(err == 1)
    {
    	printError("Authentication Request error\n");
    }
    else
    {
    	printOK("Authentication Request\n");
    }

    /* TESTING */

    uint8_t rand[16] = {0x5f, 0x9b, 0x8e, 0x54, 0x29, 0x48, 0x8a, 0x36, 0x03, 0x47, 0x4f, 0x56, 0xd1, 0x45, 0x3d, 0x0c};
    uint8_t autn[16] = {0x4e, 0x87, 0xae, 0xc2, 0x72, 0x51, 0x80, 0x0, 0x4f, 0x74, 0x26, 0xa3, 0x23, 0xf0, 0x9f, 0x19};

    memcpy(auth_challenge.RAND, rand, 16);
    memcpy(auth_challenge.AUTN, autn, 16);

    //f1(get_ue_key(ue), rand, autn, autn + 6, mac_a, get_ue_op_key(ue));

	/* Calculate Challenge */
	f2345(get_ue_key(ue), auth_challenge.RAND, auth_challenge.RES, auth_challenge.CK, auth_challenge.IK, auth_challenge.AK, get_ue_op_key(ue));
	printChallenge(&auth_challenge);
	printOK("RES successfully calculated: ");
	dumpMemory(auth_challenge.RES, 8);

	/***************************/
	/* Authentication Response */
	/***************************/
	s1ap_auth_response = generate_S1AP_Authentication_Response(enb, ue, &auth_challenge);
	authentication_res_buffer = s1ap_initiatingMessage_to_buffer(s1ap_auth_response, &len);
	freeS1ap_initiatingMessage(s1ap_auth_response);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) authentication_res_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(authentication_res_buffer);

	/* KASME, NAS_ENC and NAS_INT are generated while UE is waiting for Security mode command */
	/* Only integrity is implemented in this simulator to minimize the CPU load */
	generate_kasme(auth_challenge.CK, auth_challenge.IK, auth_challenge.AK, auth_challenge.AUTN, get_plmn(enb), PLMN_LENGTH, auth_challenge.KASME);
	printChallengeDerivatedKeys(&auth_challenge);



	/* Receiving MME answer */
	flags = 0;
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		printf("%s\n", strerror(errno));
		printError("SCTP (%s)\n", strerror(errno));
		return 1;
	}

	/* Analyze buffer and get AUTHENTICATION REQUEST*/
	err = analyze_Security_Mode_Command(buffer, ue);
	if(err == 1)
    {
    	printError("Security Mode Command\n");
    }
    else
    {
    	printOK("Security Mode Command\n");
    }

    /* Temporary encryption and integrity keys generation */
	generate_nas_enc_keys(auth_challenge.KASME, auth_challenge.NAS_KEY_ENC, get_nas_session_enc_alg(ue));
	generate_nas_int_keys(auth_challenge.KASME, auth_challenge.NAS_KEY_INT, get_nas_session_int_alg(ue));
	printChallengeDerivatedKeys(&auth_challenge);

	return 0;
}


void crypto_test(eNB * enb, UE * ue)
{
	Auth_Challenge auth_challenge;
	uint8_t rand[16] = {0x03, 0x62, 0x58, 0x1c, 0x59, 0x21, 0xe6, 0x2a, 0xc4, 0x49, 0xd6, 0x12, 0xda, 0x06, 0xd8, 0x79};
    uint8_t autn[16] = {0x5, 0x9, 0x2f, 0x46, 0x5d, 0xf7, 0x80, 0x0, 0xf9, 0xe9, 0x41, 0x1e, 0x8f, 0xea, 0x42, 0x2};

    memcpy(auth_challenge.RAND, rand, 16);
    memcpy(auth_challenge.AUTN, autn, 16);

	/* Calculate Challenge */
	f2345(get_ue_key(ue), auth_challenge.RAND, auth_challenge.RES, auth_challenge.CK, auth_challenge.IK, auth_challenge.AK, get_ue_op_key(ue));
	printChallenge(&auth_challenge);
	printOK("RES successfully calculated: ");
	dumpMemory(auth_challenge.RES, 8);

	/* KASME, NAS_ENC and NAS_INT are generated while UE is waiting for Security mode command */
	/* Only integrity is implemented in this simulator to minimize the CPU load */
	generate_kasme(auth_challenge.CK, auth_challenge.IK, auth_challenge.AK, auth_challenge.AUTN, get_plmn(enb), PLMN_LENGTH, auth_challenge.KASME);
	
	set_nas_session_security_algorithms(ue, 0x02);

	generate_nas_enc_keys(auth_challenge.KASME, auth_challenge.NAS_KEY_ENC, get_nas_session_enc_alg(ue));
	generate_nas_int_keys(auth_challenge.KASME, auth_challenge.NAS_KEY_INT, get_nas_session_int_alg(ue));

	printChallengeDerivatedKeys(&auth_challenge);
}





