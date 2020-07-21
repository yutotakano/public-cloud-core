#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <libgc.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/sctp.h>
#include <errno.h>
#include <unistd.h>

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
#define TimeToWait 0x0041
#define MMERelativeCapacity	0x0057
#define ServedGUMMEIs	0x0069
#define ID_UEAGGREGATE_MAXIMUM_BITRATE 0x0042
#define ID_E_RAB_TO_BE_SETUP_LIST_C_TXT_SU_REQ 0x0018
#define ID_UE_SECURITY_CAPABILITIES 0x006b
#define ID_SECURITY_KEY 0x0049
#define ID_UE_S1AP_IDS 0x0063
#define ID_CAUSE 0x0002
#define ID_TAI 0x0043
#define ID_EUTRAN_CGI 0x0064
#define ID_RRC_ESTABLISHMENT_CAUSE 0x0086
#define ID_UE_RADIO_CAPABILITY 0x004a
#define ID_E_RAB_TO_BE_SETUP_ITEM_C_TXT_SU_REQ 0x0034
#define ID_E_RAB_SETUP_LIST_C_TXT_SU_RES 0x0033
#define ID_E_RAB_SETUP_ITEM_C_TXT_SU_RES 0x0032
#define ID_E_RAB_SETUP_ITEM_C_TXT_SU_RES_LEN 0x0A
#define ID_MME_UE_S1AP_IE 0x0000
#define ID_ENB_UE_S1AP_IE_UE 0x0008
#define ID_ENB_UE_S1AP_ID	0x0008
#define ID_S_TMSI 0x0060


/* UE Defines */
#define INITIAL_UE_MESSAGE	0x000C
#define ENB_UE_S1AP_ID_LENGTH 4
#define ID_NAS_PDU 0x001A
#define NAS_SECURITY_HEADER_TYPE	0x00
#define NAS_SECURITY_HEADER_TYPE_INT_PROTECTED	0x40
#define NAS_SECURITY_HEADER_TYPE_INT_CIPH_PROTECTED	0x20
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
#define TAC 0x0001
#define MO_SIGNALLING 0x30
#define MO_DATA 0x40
#define DOWNLINK_NAS_TRANSPORT 0x0b
#define AUTHENTICATION_REQUEST 0x52
#define SECURITY_MODE_COMMAND 0x5d
#define EMM_INFORMATION 0x61
#define RES_LENGTH 8
#define SECURITY_COMMAND_COMPLETE_ID 0x000d
#define NAS_EPS_MMM_TYPE_AUTHENTICATION_RESPONSE 0x53
#define AUTHENTICATION_RESPONSE_ID 0x000d
#define MESSAGE_AUTHENTICATION_CODE 4
#define NAS_EPS_MMM_TYPE_SECURITY_COMMAND_COMPLETE 0x5e
#define ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST 0xC1
#define INITIAL_CONTEXT_SETUP 0x09
#define UE_CAPABILITY_INFO_INDICATION 0x16
#define SUCCESSFUL_OUTCOME 0x20
#define ESM_CONTAINER_CONTENT_LENGTH 5
#define UPLINK_NAS_TRANSPORT 0x000D
#define NAS_EPS_MMM_TYPE_ATTACH_COMPLETE 0x43
#define UE_CONTEXT_RELEASE_REQUEST 0x12
#define UE_CONTEXT_RELEASE 0x17
#define DETACH_ACCEPT 0x46
#define UE_CONTEXT_RELEASE_RESPONSE 0x17
#define NAS_EPS_MMM_TYPE_DETACH 0x45
#define NAS_SECURITY_HEADER_TYPE_SEVICE_REQUEST 0xC0
#define NAS_KEY_SET_IDENTIFIER(value) (value & 0xE0)
#define SEQUENCE_NUMBER_SHORT(sqn) (sqn & 0x1F)


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

struct _Authentication_Response
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(eNB-UE-S1AP-ID_1) + ProtocolIE(eNB-UE-S1AP-ID_2) +  ProtocolIE(NAS-PDU) + ProtocolIE(EUTRAN-CGI) + ProtocolIE(TAI)*/
	uint16_t items_len;
};

struct _non_access_stratum_pdu_auth_resp
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t nas_eps_mmm_type;
	uint8_t res_len;
	uint8_t res[RES_LENGTH];
};

struct _Security_Mode_Command
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};

struct _Security_Command_Complete
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0003 */
	uint8_t * items; /* ProtocolIE(eNB-UE-S1AP-ID_1) + ProtocolIE(eNB-UE-S1AP-ID_2) +  ProtocolIE(NAS-PDU) + ProtocolIE(EUTRAN-CGI) + ProtocolIE(TAI)*/
	uint16_t items_len;
};

struct _UE_Capability
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0003 */
	uint8_t * items; /* ProtocolIE(id-MME-UE-S1AP-ID) + ProtocolIE(id-eNB-UE-S1AP-ID) +  ProtocolIE(id-UERadioCapability) */
	uint16_t items_len;
};

struct _Initial_Context_Setup_Response
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(id-MME-UE-S1AP-ID) + ProtocolIE(id-eNB-UE-S1AP-ID) +  ProtocolIE(id-E-RABSetupListCtxtSURes) */
	uint16_t items_len;
};

struct _non_access_stratum_pdu_sec_comm_complete
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t message_authentication_code[MESSAGE_AUTHENTICATION_CODE];
	uint8_t sequence_number;
	uint8_t security_header_type_protocol_discriminator_second;
	uint8_t nas_eps_mmm_type;
};

struct _non_access_stratum_pdu_attach_complete
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t message_authentication_code[MESSAGE_AUTHENTICATION_CODE];
	uint8_t sequence_number;
	uint8_t security_header_type_protocol_discriminator_second;
	uint8_t nas_eps_mmm_type;
	uint8_t esm_container[ESM_CONTAINER_CONTENT_LENGTH];
};

struct _non_access_stratum_pdu_detach
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t message_authentication_code[MESSAGE_AUTHENTICATION_CODE];
	uint8_t sequence_number;
	uint8_t security_header_type_protocol_discriminator_second;
	uint8_t nas_eps_mmm_type;
	uint8_t eps_mobile_identity[13]; /* Included Uplink Detach byte */
};

struct _non_access_stratum_pdu_service_request
{
	uint8_t security_header_type_protocol_discriminator;
	uint8_t ksi_sequence_number;
	uint8_t short_mac[2];
};

struct _Initial_Context_Setup_Request
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t unknown;
	uint8_t length;
	uint8_t * value;
};

struct _E_RAB_Setup_List_C_txt_SU_Res
{
	uint16_t id;
	uint8_t criticality;
	uint8_t * value;
	uint8_t value_len;
};

struct _E_RABSetupItemCtxtSURes
{
	uint16_t e_rab_id;
	uint8_t transport_layer_address[IP_LEN];
	uint32_t gtp_teid;
};

struct _Uplink_NAS_Transport
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(eNB-UE-S1AP-ID_1) + ProtocolIE(eNB-UE-S1AP-ID_2) +  ProtocolIE(NAS-PDU) + ProtocolIE(EUTRAN-CGI) + ProtocolIE(TAI)*/
	uint16_t items_len;
};

struct _UE_Context_Release_Request
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0003 */
	uint8_t * items; /* ProtocolIE(id-MME-UE-S1AP-ID) + ProtocolIE(id-eNB-UE-S1AP-ID) +  ProtocolIE(id-Cause) */
	uint16_t items_len;
};

struct _UE_Context_Release_Command
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};

struct _UE_Context_Release_Response
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0003 */
	uint8_t * items; /* ProtocolIE(id-MME-UE-S1AP-ID) + ProtocolIE(id-eNB-UE-S1AP-ID) */
	uint16_t items_len;
};

struct _UE_Detach_Accept
{
	uint8_t status;
	uint8_t procedureCode;
	uint8_t criticality;
	uint8_t length;
	uint8_t * value;
};

struct _UE_Detach
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0005 */
	uint8_t * items; /* ProtocolIE(id-MME-UE-S1AP-ID) + ProtocolIE(id-eNB-UE-S1AP-ID) + ProtocolIE(id-NAS-PDU) + ProtocolIE(id-EUTRAN-CGI) + ProtocolIE(id-TAI) */
	uint16_t items_len;
};

struct _InitialUEMessage_ServiceRequest
{
	uint8_t init; /* = 0x00;*/
	uint16_t numItems; /* 0x0006 */
	uint8_t * items; /* ProtocolIE(id-eNB-UE-S1AP-ID) + ProtocolIE(id-NAS-PDU) + ProtocolIE(id-TAI) + ProtocolIE(id-EUTRAN-CGI) + ProtocolIE(id-RRC-Establishment-Cause) + id-S-TMSI*/
	uint16_t items_len;
};


/* Auxiliary function for debugging purposes */
void dumpMemory(uint8_t * pointer, int len)
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
	uint8_t name[] = {0x07, 0x00, 0x4e, 0x65, 0x72, 0x76, 0x69, 0x6f, 0x6e, 0x5f, 0x65, 0x2d, 0x4e, 0x6f, 0x64, 0x65, 0x42};

	uint8_t * value = generate_s1ap_value((uint8_t *)name, 17);
	protocolIE->value = (uint8_t *) GC_malloc(18);
	memcpy(protocolIE->value, (void *) value, 18);
	protocolIE->value_len = 18;
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
		case ID_MME_UE_S1AP_IE:
			return "id-MME-UE-S1AP-ID";
		case ID_ENB_UE_S1AP_IE_UE:
			return "id-ENB-UE-S1AP-ID";
		case ServedGUMMEIs:
			return "id-ServedGUMMEIs";
		case MMERelativeCapacity:
			return "id-MMERelativeCapacity";
		case TimeToWait:
			return "id-TimeToWait";
		case ID_NAS_PDU:
			return "NAS-PDU";
		case ID_UEAGGREGATE_MAXIMUM_BITRATE:
			return "id-uEaggregateMaximumBitrate";
		case ID_E_RAB_TO_BE_SETUP_LIST_C_TXT_SU_REQ:
			return "id-E-RABToBeSetupListCtxtSUReq";
		case ID_UE_SECURITY_CAPABILITIES:
			return "id-UESecurityCapabilities";
		case ID_SECURITY_KEY:
			return "id-SecurityKey";
		case ID_UE_S1AP_IDS:
			return "id-UE_S1AP_IDs";
		case ID_CAUSE:
			return "id-Cause";
		default:
			return "Unknown";
	}
}

void analyze_S1_Setup_Content(eNB * enb, uint8_t * value, int len)
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
		if(id == ServedGUMMEIs)
		{
			printf("\tPLMN: %.2x%.2x%.2x\n", value[offset+2], value[offset+3], value[offset+4]);
			printf("\tMME-Group-ID: 0x%.2x%.2x\n", value[offset+7], value[offset+8]);
			printf("\tMMEC: 0x%.2x\n", value[offset+10]);
		}
		else
		{
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
		}
		offset += value_len;
	}
}

int analyze_S1_Setup_Response(eNB * enb, uint8_t * buffer)
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
		analyze_S1_Setup_Content(enb, s1setupResp.value, s1setupResp.length);
		ret = 0;
	}
	else if(s1setupResp.status == S1_SETUP_UNSUCCESSFUL)
	{
		/* Unsuccessful response */
		printError("S1 Setup Response Failure\n");
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


int procedure_S1_Setup(eNB * enb)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg;
	uint16_t len = 0;
	uint8_t * initiatingMsg_buffer;
	uint8_t buffer[BUFFER_LEN];
	int flags = 0;
	int recv_len;
	struct sockaddr_in addr;
	int socket = get_mme_socket(enb);
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
	return analyze_S1_Setup_Response(enb, buffer);
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
	cgi.enb_id = htonl(get_enb_id(enb)); /* In memory is represented like 00 0e 00 00 */

	uint8_t * value = generate_s1ap_value((uint8_t *)&cgi, sizeof(eutran_cgi));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(eutran_cgi) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(eutran_cgi) + 1);
	protocolIE->value_len = sizeof(eutran_cgi) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_rrc_establishment_cause(uint8_t cause)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_RRC_ESTABLISHMENT_CAUSE;
	protocolIE->criticality = CRITICALITY_IGNORE;

	rrc_establishment_cause c;
	c.establishment_cause = cause;

	uint8_t * value = generate_s1ap_value((uint8_t *)&c, sizeof(rrc_establishment_cause));
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(rrc_establishment_cause) + 1);
	memcpy(protocolIE->value, (void *) value, sizeof(rrc_establishment_cause) + 1);
	protocolIE->value_len = sizeof(rrc_establishment_cause) + 1;
	GC_free(value);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_cause()
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_CAUSE;
	protocolIE->criticality = CRITICALITY_IGNORE;

	protocolIE->value = (uint8_t *) GC_malloc(3);
	protocolIE->value[0] = 2;
	protocolIE->value[1] = 0;
	protocolIE->value[2] = 0;
	protocolIE->value_len = 3;

	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_s_tmsi(UE * ue)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_S_TMSI;
	protocolIE->criticality = CRITICALITY_REJECT;

	protocolIE->value = (uint8_t *) GC_malloc(7);
	protocolIE->value[0] = 6; /* Length */
	uint16_t mmec = (uint16_t)(get_mme_code(ue) << 6);
	protocolIE->value[1] = (mmec >> 8) & 0xFF;
	protocolIE->value[2] = mmec & 0xFF;
	memcpy(protocolIE->value+3, get_m_tmsi(ue), 4);

	protocolIE->value_len = 7;

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
	rrc_establishment_cause = generate_ProtocolIE_rrc_establishment_cause(MO_SIGNALLING);

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

#define TRANSPORT_LAYER_ADDRESS 0x0F80

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

	printf("OFFSET: %d\n", offset);
	printf("NUM: %d\n", numElems);

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
			else if(value[offset_nas_pdu] == 0x37 || value[offset_nas_pdu] == 0x27)
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
			else if(value[offset_nas_pdu] == EMM_INFORMATION)
			{
				printf("\t\tEMM Information message\n");
				flag = -1;
				ret = 2; /* Special case */
				offset_nas_pdu++;
				/* Detect network name: full name */
				if(value[offset_nas_pdu] == 0x43)
				{
					printf("\t\tNetwork Name - Full name for network\n");
					offset_nas_pdu += 1;
					printf("\t\tLength: %d\n", value[offset_nas_pdu]);
					offset_nas_pdu += value[offset_nas_pdu];
				}
				/* Detect network name: short name */
				if(value[offset_nas_pdu] == 0x45)
				{
					printf("\t\tNetwork Name - Short Name\n");
					offset_nas_pdu += 1;
					printf("\t\tLength: %d\n", value[offset_nas_pdu]);
					offset_nas_pdu += value[offset_nas_pdu];
				}
			}
			else if(value[offset_nas_pdu] == DETACH_ACCEPT)
			{
				printf("\t\tDetach Accept\n");
				offset_nas_pdu++;
				flag = -1;
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
				memcpy(auth_challenge->AUTN, value + offset_nas_pdu + 16 + 1, 16);
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
			if(id == ID_MME_UE_S1AP_IE)
			{
				/* Saving MME S1AP ID */
				set_mme_s1ap_id(ue, value + offset, (uint8_t) value_len);
				ret = 0;
			}
			if(id == ID_UEAGGREGATE_MAXIMUM_BITRATE && value_len == 10)
			{
				if(value[offset] == 0x18)
					printf("\tMaximum Bitrate (Downlink) %dbits/s\n", (value[offset+1] << 24) | (value[offset+2] << 16) | (value[offset+3] << 8) | value[offset+4] );
				if(value[offset+5] == 0x60)
					printf("\tMaximum Bitrate (Uplink) %dbits/s\n", (value[offset+6] << 24) | (value[offset+7] << 16) | (value[offset+8] << 8) | value[offset+9] );
				ret = 0;
			}
			else if(id == ID_E_RAB_TO_BE_SETUP_LIST_C_TXT_SU_REQ)
			{
				int offset_aux = offset;
				if( ((value[offset+1] << 8) | value[offset+2]) !=  ID_E_RAB_TO_BE_SETUP_ITEM_C_TXT_SU_REQ)
				{
					printError("id-E-RABToBeSetupItemCtxtSUReq error");
					ret = 1;
					offset += value_len;
					continue;
				}
				offset_aux += 5;
				printf("\te-RAB-ID: %d\n", value[offset_aux]);
				/* Skip e-RABlevelQoSParameters */
				offset_aux += 6;

				/* Store SPGW IP */
				set_spgw_ip(ue, value + offset_aux);
				printf("\tSPGW IP: %d.%d.%d.%d\n", value[offset_aux], value[offset_aux+1], value[offset_aux+2], value[offset_aux+3]);
				offset_aux += 4;

				/* Store TEID */
				uint32_t teid = (value[offset_aux] << 24) | (value[offset_aux+1] << 16) | (value[offset_aux+2] << 8) | value[offset_aux+3];
				set_gtp_teid(ue,  teid);
				printf("\tGTP TEID: %d\n", teid);
				offset_aux += 4;

				/* Detect if E-RABToBeSetupItemCtxtSUReq contains an EPS-MMM message */
				if( (value[offset_aux+1] & 0x0F) != 0x07)
				{
					printInfo("No NAS message");
					offset += value_len;
					continue;
				}

				offset_aux += 10;
				if( ((value[offset_aux] & 0xE0) >> 5) == 2 )
				{
					/* Decihours */
					uint8_t g3412_value = (value[offset_aux] & 0x1F) * 6;
					printf("\tGPRS Timer: %d min\n", g3412_value);
				}
				offset_aux += 8;
				offset_aux += 4;
				if(value[offset_aux] != ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST)
				{
					printError("No Activate Default EPS Bearer Context Request\n");
					ret = 1;
					offset += value_len;
					continue;
				}
				offset_aux += 2;
				printf("\tEPS Quality of Service (QCI): %d\n", value[offset_aux]);
				offset_aux += 1;

				set_apn_name(ue, value + offset_aux + 1, value[offset_aux]);
				printf("\tAccess Point Name: %s\n", get_apn_name(ue));
				offset_aux += value[offset_aux] + 1;

				/* Store PDN IPv4 */
				if( !(value[offset_aux+1] & 0x01) )
				{
					printError("No PDN IPv4");
					ret = 1;
					offset += value_len;
					continue;
				}
				offset_aux += 2;
				set_pdn_ip(ue, value + offset_aux);
				printf("\tPDN IP: %d.%d.%d.%d\n", value[offset_aux], value[offset_aux+1], value[offset_aux+2], value[offset_aux+3]);
				offset_aux += 4;

				/* Loop to detect GUTI */
				uint8_t guti_f = 1;
				while(guti_f)
				{
					if(value[offset_aux] == 0x27)
					{
						printf("\tProtocol Configuration Options detected\n");
						offset_aux += value[offset_aux+1] + 2;
					}
					else if(value[offset_aux] == 0x5e)
					{
						printf("\tAPN aggregare maximum bit rate detected\n");
						offset_aux += value[offset_aux+1] + 2;
					}
					else if(value[offset_aux] == 0x50)
					{
						printf("\tGUTI(%d): ", value[offset_aux+1]);
						offset_aux += 3;
						guti_f = 0;
					}
				}
				/* Store GUTI */
				set_guti(ue, value + offset_aux);
				for(j = 0; j < GUTI_LEN; j++)
					printf("%.2x", value[offset_aux+j]);
				printf("\n");
				offset_aux += 10;

			}
			else if(id == ID_UE_SECURITY_CAPABILITIES)
			{
				printf("\tEncryption Algorithm: %.4x\n", (value[offset] << 8) | value[offset+1]);
				printf("\tIntegrity Protection Algorithm: %.4x\n", (value[offset+2] << 8) | value[offset+3]);
				ret = 0;
			}
			else if(id == ID_SECURITY_KEY)
			{
				printf("\tSecurity Key: ");
				for(j = 0; j < 32; j++)
				{
					printf("%.2x", value[offset + j]);
				}
				printf("\n");
				ret = 0;
			}
			else if(id == ID_UE_S1AP_IDS)
			{
				printf("\tmME-UE-S1AP-ID: %d\n", (value[offset] << 8) | value[offset+1]);
				printf("\teNB-UE-S1AP-ID: %d\n", (value[offset+2] << 8) | value[offset+3]);
				ret = 0;
			}
			else if(id == ID_CAUSE)
			{
				printf("\tCause (%d) : ", value_len);
				for(j = 0; j < value_len; j++)
				{
					if(j == 16)
					{
						printf("...");
						break;
					}
					printf("%.2x ", value[offset + j]);
				}
				printf("\n");
				ret = 0;
			}
			/* Default case */
			else
			{
				printf("\tValue (%d) : ", value_len);
				for(j = 0; j < value_len; j++)
				{
					if(j == 16)
					{
						printf("...");
						break;
					}
					printf("%.2x ", value[offset + j]);
				}
				printf("\n");
				ret = 0;
			}
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

	if(auth_req.status != 0 || auth_req.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT)
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

	if(smc.status != 0 || smc.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT)
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

int analyze_InitialContextSetupRequest(uint8_t * buffer, UE * ue)
{
	int ret = 1;
	Initial_Context_Setup_Request init_csr;
	uint8_t len;

	memcpy((void *) &init_csr, (void *) buffer, 5);

	/* This condition also allows NAS Messages */
	if(init_csr.status != 0 || (init_csr.procedureCode != (uint8_t)INITIAL_CONTEXT_SETUP && init_csr.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT))
	{
		printError("No Downlink NAS Transport message (Initial Context Setup Request)\n");
		return 1;
	}

	/* Detect unknown byte */
	if(init_csr.unknown == 0x80)
	{
		init_csr.value = (uint8_t *) GC_malloc(init_csr.length);
		memcpy((void *) init_csr.value, (void *) buffer+5, init_csr.length);
		len = init_csr.length;
	}
	else
	{
		init_csr.value = (uint8_t *) GC_malloc(init_csr.unknown);
		memcpy((void *) init_csr.value, (void *) buffer+4, init_csr.unknown);
		len = init_csr.unknown;
	}

	/* Analyze and extract Encryption and Integrity algorithms to derivate NAS session keys */
	printInfo("Analyzing Downlink NAS Transport message (Initial Context Setup Request)\n");
	ret = analyze_downlink_NAS_Transport(init_csr.value, len, NULL, ue);
	GC_free(init_csr.value);
	return ret;
}

int analyze_UEContextReleaseCommand(uint8_t * buffer, UE * ue)
{
	int ret = 1;
	UE_Context_Release_Command ue_crc;

	memcpy((void *) &ue_crc, (void *) buffer, 4);
	ue_crc.value = (uint8_t *) GC_malloc(ue_crc.length);
	memcpy((void *) ue_crc.value, (void *) buffer+4, ue_crc.length);

	/* Special case to detect EMM information messages */
	if(ue_crc.status != 0 || (ue_crc.procedureCode != (uint8_t)UE_CONTEXT_RELEASE && ue_crc.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT))
	{
		printError("Wrong UEContextReleaseCommand message\n");
		return 1;
	}

	/* Analyze and extract Encryption and Integrity algorithms to derivate NAS session keys */
	printInfo("Analyzing UEContextReleaseCommand\n");
	ret = analyze_downlink_NAS_Transport(ue_crc.value, ue_crc.length, NULL, ue);
	GC_free(ue_crc.value);
	return ret;
}

int analyze_UEAccept(uint8_t * buffer, UE * ue)
{
	int ret = 1;
	UE_Detach_Accept ue_da;

	memcpy((void *) &ue_da, (void *) buffer, 4);
	ue_da.value = (uint8_t *) GC_malloc(ue_da.length);
	memcpy((void *) ue_da.value, (void *) buffer+4, ue_da.length);

	if(ue_da.status != 0 || ue_da.procedureCode != (uint8_t)DOWNLINK_NAS_TRANSPORT)
	{
		printInfo("Wrong UEDetachAccept message\n");
		return 1;
	}

	/* Analyze and extract Encryption and Integrity algorithms to derivate NAS session keys */
	printInfo("Analyzing UEDetachAccept\n");
	ret = analyze_downlink_NAS_Transport(ue_da.value, ue_da.length, NULL, ue);
	GC_free(ue_da.value);
	return ret;
}

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
	protocolIE->id = ID_MME_UE_S1AP_IE;
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

void freeSecurityCommandComplete(Security_Command_Complete * scc)
{
	GC_free(scc->items);
	GC_free(scc);
}

void freeUplinkNASTransport(Uplink_NAS_Transport * uplinkNAST)
{
	GC_free(uplinkNAST->items);
	GC_free(uplinkNAST);
}

void freeUECapabilityInfoIndication(UE_Capability * ue_cap)
{
	GC_free(ue_cap->items);
	GC_free(ue_cap);
}

void freeInitialContextSetupResponse(Initial_Context_Setup_Response * ics_resp)
{
	GC_free(ics_resp->items);
	GC_free(ics_resp);
}

void freeUEContextReleaseRequest(UE_Context_Release_Request * ue_crr)
{
	GC_free(ue_crr->items);
	GC_free(ue_crr);
}

void freeUEContextReleaseResponse(UE_Context_Release_Response * ue_crr)
{
	GC_free(ue_crr->items);
	GC_free(ue_crr);
}

void freeUEDetach(UE_Detach * ue_d)
{
	GC_free(ue_d->items);
	GC_free(ue_d);
}

void freeUEMessage_ServiceRequest(InitialUEMessage_ServiceRequest * ue_sr)
{
	GC_free(ue_sr->items);
	GC_free(ue_sr);
}

uint8_t * SecurityCommandComplete_to_buffer(Security_Command_Complete * scc, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + scc->items_len);
	dump[0] = scc->init;
	dump[1] = scc->numItems >> 8;
	dump[2] = (uint8_t)(scc->numItems & 0xFF);
	memcpy(dump+3, scc->items, scc->items_len);
	*len = 3 + scc->items_len;
	return dump;
}

uint8_t * UplinkNASTransport_to_buffer(Uplink_NAS_Transport * uplinkNAST, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + uplinkNAST->items_len);
	dump[0] = uplinkNAST->init;
	dump[1] = uplinkNAST->numItems >> 8;
	dump[2] = (uint8_t)(uplinkNAST->numItems & 0xFF);
	memcpy(dump+3, uplinkNAST->items, uplinkNAST->items_len);
	*len = 3 + uplinkNAST->items_len;
	return dump;
}

uint8_t * UECapabilityInfoIndication_to_buffer(UE_Capability * ue_cap, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ue_cap->items_len);
	dump[0] = ue_cap->init;
	dump[1] = ue_cap->numItems >> 8;
	dump[2] = (uint8_t)(ue_cap->numItems & 0xFF);
	memcpy(dump+3, ue_cap->items, ue_cap->items_len);
	*len = 3 + ue_cap->items_len;
	return dump;
}

uint8_t * InitialContextSetupResponse_to_buffer(Initial_Context_Setup_Response * ics_resp, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ics_resp->items_len);
	dump[0] = ics_resp->init;
	dump[1] = ics_resp->numItems >> 8;
	dump[2] = (uint8_t)(ics_resp->numItems & 0xFF);
	memcpy(dump+3, ics_resp->items, ics_resp->items_len);
	*len = 3 + ics_resp->items_len;
	return dump;
}

uint8_t * UEContextReleaseRequest_to_buffer(UE_Context_Release_Request * ue_crr, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ue_crr->items_len);
	dump[0] = ue_crr->init;
	dump[1] = ue_crr->numItems >> 8;
	dump[2] = (uint8_t)(ue_crr->numItems & 0xFF);
	memcpy(dump+3, ue_crr->items, ue_crr->items_len);
	*len = 3 + ue_crr->items_len;
	return dump;
}

uint8_t * UEContextReleaseResponse_to_buffer(UE_Context_Release_Response * ue_crr, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ue_crr->items_len);
	dump[0] = ue_crr->init;
	dump[1] = ue_crr->numItems >> 8;
	dump[2] = (uint8_t)(ue_crr->numItems & 0xFF);
	memcpy(dump+3, ue_crr->items, ue_crr->items_len);
	*len = 3 + ue_crr->items_len;
	return dump;
}

uint8_t * UEDetach_to_buffer(UE_Detach * ue_d, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ue_d->items_len);
	dump[0] = ue_d->init;
	dump[1] = ue_d->numItems >> 8;
	dump[2] = (uint8_t)(ue_d->numItems & 0xFF);
	memcpy(dump+3, ue_d->items, ue_d->items_len);
	*len = 3 + ue_d->items_len;
	return dump;
}

uint8_t * InitialUEMessage_ServiceRequest_to_buffer(InitialUEMessage_ServiceRequest * ue_sr, uint16_t * len)
{
	uint8_t * dump = (uint8_t *) GC_malloc(3 + ue_sr->items_len);
	dump[0] = ue_sr->init;
	dump[1] = ue_sr->numItems >> 8;
	dump[2] = (uint8_t)(ue_sr->numItems & 0xFF);
	memcpy(dump+3, ue_sr->items, ue_sr->items_len);
	*len = 3 + ue_sr->items_len;
	return dump;
}

ProtocolIE * generate_ProtocolIE_nas_pdu_security_command_complete(UE * ue, Auth_Challenge * auth_challenge)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;

	non_access_stratum_pdu_sec_comm_complete nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu_sec_comm_complete));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE_INT_PROTECTED | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.sequence_number = get_nas_sequence_number(ue);
	nas_pdu.security_header_type_protocol_discriminator_second = NAS_SECURITY_HEADER_TYPE | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.nas_eps_mmm_type = NAS_EPS_MMM_TYPE_SECURITY_COMMAND_COMPLETE;
	/* Generate NAS integrity check*/
	nas_integrity_eia2(auth_challenge->NAS_KEY_INT, &nas_pdu.sequence_number, 3, 0, nas_pdu.message_authentication_code);
	


	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu_sec_comm_complete));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu_sec_comm_complete) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu_sec_comm_complete) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu_sec_comm_complete) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu_sec_comm_complete) + 2;
	GC_free(encapsulated);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_nas_pdu_attach_complete(UE * ue, Auth_Challenge * auth_challenge)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;

	non_access_stratum_pdu_attach_complete nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu_attach_complete));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE_INT_CIPH_PROTECTED | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.sequence_number = get_nas_sequence_number(ue);
	nas_pdu.security_header_type_protocol_discriminator_second = NAS_SECURITY_HEADER_TYPE | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.nas_eps_mmm_type = NAS_EPS_MMM_TYPE_ATTACH_COMPLETE;
	nas_pdu.esm_container[0] = 0x00; /* Length */
	nas_pdu.esm_container[1] = 0x03; /* Length */
	nas_pdu.esm_container[2] = 0x52; /* EPS bearer identity value 5 & EPS session management messages */
	nas_pdu.esm_container[3] = 0x00; /* Procedure transaction identity */
	nas_pdu.esm_container[4] = 0xc2; /* Activate default EPS bearer context accept */

	/* Generate NAS integrity check*/
	nas_integrity_eia2(auth_challenge->NAS_KEY_INT, &nas_pdu.sequence_number, 8, 1, nas_pdu.message_authentication_code);
	


	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu_attach_complete));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu_attach_complete) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu_attach_complete) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu_attach_complete) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu_attach_complete) + 2;
	GC_free(encapsulated);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_nas_pdu_detach(UE * ue, Auth_Challenge * auth_challenge, uint8_t switch_off)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;

	non_access_stratum_pdu_detach nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu_detach));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE_INT_CIPH_PROTECTED | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.sequence_number = get_nas_sequence_number(ue);
	nas_pdu.security_header_type_protocol_discriminator_second = NAS_SECURITY_HEADER_TYPE | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.nas_eps_mmm_type = NAS_EPS_MMM_TYPE_DETACH;
	nas_pdu.eps_mobile_identity[0] = 0x01; /* Uplink byte that indicates detach: EPS Detach */
	if(switch_off != 0)
	{
		nas_pdu.eps_mobile_identity[0] |= 0x08;
		printInfo("Generating Detach Switch Off message...\n");
	}
	else
	{
		printInfo("Generating Detach message...\n");
	}
	nas_pdu.eps_mobile_identity[1] = 0x0B; /* GUTI_CODE + GUTI lenght*/
	nas_pdu.eps_mobile_identity[2] = 0xF6; /* Even number of identity digits + GUTI (6) */
	memcpy(nas_pdu.eps_mobile_identity+3, get_guti(ue), GUTI_LEN); /* Copy GUTI */

	/* Generate NAS integrity check*/
	nas_integrity_eia2(auth_challenge->NAS_KEY_INT, &nas_pdu.sequence_number, 16, 1, nas_pdu.message_authentication_code);
	


	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu_detach));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu_detach) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu_detach) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu_detach) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu_detach) + 2;
	GC_free(encapsulated);
	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_nas_pdu_service_request(UE * ue, Auth_Challenge * auth_challenge)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_NAS_PDU;
	protocolIE->criticality = CRITICALITY_REJECT;
	uint8_t mac[4];

	non_access_stratum_pdu_service_request nas_pdu;
	bzero(&nas_pdu, sizeof(non_access_stratum_pdu_service_request));

	nas_pdu.security_header_type_protocol_discriminator = NAS_SECURITY_HEADER_TYPE_SEVICE_REQUEST | NAS_PROTOCOL_DISCRIMINATOR;
	nas_pdu.ksi_sequence_number = NAS_KEY_SET_IDENTIFIER(0) | SEQUENCE_NUMBER_SHORT(get_nas_sequence_number(ue));

	/* Generate NAS integrity check Short MAC*/
	/* count argument has to be 2 (No idea why) */
	nas_integrity_eia2(auth_challenge->NAS_KEY_INT, (uint8_t *)&nas_pdu, 2, 2, mac);

	/* Short MAC */
	nas_pdu.short_mac[0] = mac[2];
	nas_pdu.short_mac[1] = mac[3];

	/* NAS PDU value is encapsulated in other value */
	uint8_t * value = generate_s1ap_value((uint8_t *)&nas_pdu, sizeof(non_access_stratum_pdu_service_request));
	uint8_t * encapsulated = generate_s1ap_value(value, sizeof(non_access_stratum_pdu_service_request) + 1);
	GC_free(value);
	protocolIE->value = (uint8_t *) GC_malloc(sizeof(non_access_stratum_pdu_service_request) + 2);
	memcpy(protocolIE->value, (void *) encapsulated, sizeof(non_access_stratum_pdu_service_request) + 2);
	protocolIE->value_len = sizeof(non_access_stratum_pdu_service_request) + 2;
	GC_free(encapsulated);

	return protocolIE;
}

ProtocolIE * generate_ProtocolIE_ue_capability_info_indication()
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_UE_RADIO_CAPABILITY;
	protocolIE->criticality = CRITICALITY_REJECT;


	/* NAS PDU value is encapsulated in other value */
	uint8_t value[] = {0x00, 0x94, 0x01, 0x00, 0xf0, 0x18, 0x00, 0x03, 0x08, 0x98, 0x64, 0xa0, 0xc1, 0xb8, 0x3b, 0x07, 0xa0, 0xf8, 0x00, 0x00};
	uint8_t * encapsulated_1 = generate_s1ap_value(value, 20);
	uint8_t * encapsulated_2 = generate_s1ap_value(encapsulated_1, 21);
	GC_free(encapsulated_1);
	protocolIE->value = (uint8_t *) GC_malloc(22);
	memcpy(protocolIE->value, (void *) encapsulated_2, 22);
	protocolIE->value_len = 22;
	GC_free(encapsulated_2);
	return protocolIE;
}

uint8_t * E_RABSetupItemCtxtSURes_to_buffer(E_RABSetupItemCtxtSURes * e_rabsetup)
{
	/* To avoid 4 bytes alignment, E_RABSetupItemCtxtSURes size is defined manually*/
	uint8_t * dump = GC_malloc(ID_E_RAB_SETUP_ITEM_C_TXT_SU_RES_LEN);
	memcpy(dump, (uint8_t *) &e_rabsetup->e_rab_id, 2);
	memcpy(dump + 2, e_rabsetup->transport_layer_address, IP_LEN);
	memcpy(dump + 6, (uint8_t *) &e_rabsetup->gtp_teid, 4);
	return dump;
}

uint8_t * E_RAB_Setup_List_C_txt_SU_Res_to_buffer(E_RAB_Setup_List_C_txt_SU_Res * e_rabsetup_res, uint16_t * len)
{
	uint8_t * dump = GC_malloc(e_rabsetup_res->value_len + 5);
	dump[0] = 0x00;
	dump[1] = (e_rabsetup_res->id & 0xFF00) >> 8;
	dump[2] = e_rabsetup_res->id & 0xFF;
	dump[3] = e_rabsetup_res->criticality;
	dump[4] = e_rabsetup_res->value_len;
	memcpy(dump + 5, e_rabsetup_res->value, e_rabsetup_res->value_len);
	*len = e_rabsetup_res->value_len + 5;
	return dump;
}

ProtocolIE * generate_ProtocolIE_e_rab_setup_list_c_txt_su_res(eNB * enb, UE * ue, uint8_t * ue_ip)
{
	ProtocolIE * protocolIE = (ProtocolIE *) GC_malloc(sizeof(ProtocolIE));
	protocolIE->id = ID_E_RAB_SETUP_LIST_C_TXT_SU_RES;
	protocolIE->criticality = CRITICALITY_IGNORE;


	E_RABSetupItemCtxtSURes * e_rabsetup = GC_malloc(sizeof(E_RABSetupItemCtxtSURes));
	e_rabsetup->e_rab_id = 0x1f0a; /* e-RAB-ID: 5 */
	memcpy(e_rabsetup->transport_layer_address, ue_ip, IP_LEN);
	e_rabsetup->gtp_teid = htonl(get_gtp_teid(ue));
	uint8_t * e_rabsetup_buf = E_RABSetupItemCtxtSURes_to_buffer(e_rabsetup);
	GC_free(e_rabsetup);

	E_RAB_Setup_List_C_txt_SU_Res * e_rabsetup_res = GC_malloc(sizeof(E_RAB_Setup_List_C_txt_SU_Res));
	e_rabsetup_res->id = ID_E_RAB_SETUP_ITEM_C_TXT_SU_RES;
	e_rabsetup_res->criticality = CRITICALITY_IGNORE;
	e_rabsetup_res->value = e_rabsetup_buf;
	e_rabsetup_res->value_len = ID_E_RAB_SETUP_ITEM_C_TXT_SU_RES_LEN;
	uint16_t len;
	uint8_t * e_rabsetup_res_buf = E_RAB_Setup_List_C_txt_SU_Res_to_buffer(e_rabsetup_res, &len);
	GC_free(e_rabsetup_res);
	GC_free(e_rabsetup_buf);


	uint8_t * value = generate_s1ap_value(e_rabsetup_res_buf, len);
	GC_free(e_rabsetup_res_buf);
	protocolIE->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(protocolIE->value, (void *) value, len + 1);
	protocolIE->value_len = len + 1;
	GC_free(value);
	return protocolIE;
}

Security_Command_Complete * generate_Security_Command_Complete(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	/* Replicate generate_Initial_UE_Message function  */

	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * eutran_cgi;
	ProtocolIE * tai;


	Security_Command_Complete * scc = GC_malloc(sizeof(InitialUEMessage));
	scc->init = 0x00;
	scc->numItems = 0x0005;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu_security_command_complete(ue, auth_challenge);
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
	scc->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + nas_pdu_len + eutran_cgi_len + tai_len);

	memcpy(scc->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(scc->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(scc->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(scc->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(scc->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	scc->items_len = i;

	return scc;
}

s1ap_initiatingMessage * generate_S1AP_Security_Command_Complete(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = SECURITY_COMMAND_COMPLETE_ID;
	s1ap_initiatingMsg->criticality = CRITICALITY_IGNORE;

	Security_Command_Complete * scc;
	scc = generate_Security_Command_Complete(enb, ue, auth_challenge);

	uint16_t len;
	uint8_t * SecurityCommandComplete_buf = SecurityCommandComplete_to_buffer(scc, &len);

	freeSecurityCommandComplete(scc);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) SecurityCommandComplete_buf, len);
	GC_free(SecurityCommandComplete_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

Uplink_NAS_Transport * generate_Uplink_NAS_Transport(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	/* Replicate generate_Initial_UE_Message function  */

	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * eutran_cgi;
	ProtocolIE * tai;


	Uplink_NAS_Transport * uplinkNAST = GC_malloc(sizeof(InitialUEMessage));
	uplinkNAST->init = 0x00;
	uplinkNAST->numItems = 0x0005;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu_attach_complete(ue, auth_challenge);
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
	uplinkNAST->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + nas_pdu_len + eutran_cgi_len + tai_len);

	memcpy(uplinkNAST->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(uplinkNAST->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(uplinkNAST->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(uplinkNAST->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(uplinkNAST->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	uplinkNAST->items_len = i;

	return uplinkNAST;
}

s1ap_initiatingMessage * generate_S1AP_Attach_Complete(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = UPLINK_NAS_TRANSPORT;
	s1ap_initiatingMsg->criticality = CRITICALITY_IGNORE;

	Uplink_NAS_Transport * uplinkNAST;
	uplinkNAST = generate_Uplink_NAS_Transport(enb, ue, auth_challenge);

	uint16_t len;
	uint8_t * UplinkNASTransport_buf = UplinkNASTransport_to_buffer(uplinkNAST, &len);

	freeUplinkNASTransport(uplinkNAST);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) UplinkNASTransport_buf, len);
	GC_free(UplinkNASTransport_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

UE_Capability * generate_ue_capability_info_indication(eNB * enb, UE * ue)
{
	/* Replicate generate_Initial_UE_Message function  */

	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * ue_radio_capability;


	UE_Capability * ue_cap = GC_malloc(sizeof(InitialUEMessage));
	ue_cap->init = 0x00;
	ue_cap->numItems = 0x0003;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	ue_radio_capability = generate_ProtocolIE_ue_capability_info_indication();

	int mme_ue_s1ap_id_len;
	uint8_t * mme_ue_s1ap_id_buffer = ProtocolIE_to_buffer(mme_ue_s1ap_id, &mme_ue_s1ap_id_len);
	freeProtocolIE(mme_ue_s1ap_id);

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int ue_radio_capability_len;
	uint8_t * ue_radio_capability_buffer = ProtocolIE_to_buffer(ue_radio_capability, &ue_radio_capability_len);
	freeProtocolIE(ue_radio_capability);

	int i = 0;
	ue_cap->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + ue_radio_capability_len);

	memcpy(ue_cap->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(ue_cap->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(ue_cap->items + i, ue_radio_capability_buffer, ue_radio_capability_len);
	GC_free(ue_radio_capability_buffer);
	i += ue_radio_capability_len;

	ue_cap->items_len = i;

	return ue_cap;
}

s1ap_initiatingMessage * generate_S1AP_ue_capability_info_indication(eNB * enb, UE * ue)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = UE_CAPABILITY_INFO_INDICATION;
	s1ap_initiatingMsg->criticality = CRITICALITY_IGNORE;

	UE_Capability * ue_cap;
	ue_cap = generate_ue_capability_info_indication(enb, ue);

	uint16_t len;
	uint8_t * ue_capability_info_complete_buf = UECapabilityInfoIndication_to_buffer(ue_cap, &len);

	freeUECapabilityInfoIndication(ue_cap);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) ue_capability_info_complete_buf, len);
	GC_free(ue_capability_info_complete_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

Initial_Context_Setup_Response * generate_initial_context_setup_response(eNB * enb, UE * ue, uint8_t * ue_ip)
{
	/* Replicate generate_Initial_UE_Message function  */

	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * initial_context_setup_response;


	Initial_Context_Setup_Response * ics_resp = GC_malloc(sizeof(InitialUEMessage));
	ics_resp->init = 0x00;
	ics_resp->numItems = 0x0003;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	/* Special case with criticality: ignore */
	mme_ue_s1ap_id->criticality = CRITICALITY_IGNORE;
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	/* Special case with criticality: ignore */
	enb_ue_s1ap_id->criticality = CRITICALITY_IGNORE;
	initial_context_setup_response = generate_ProtocolIE_e_rab_setup_list_c_txt_su_res(enb, ue, ue_ip);

	int mme_ue_s1ap_id_len;
	uint8_t * mme_ue_s1ap_id_buffer = ProtocolIE_to_buffer(mme_ue_s1ap_id, &mme_ue_s1ap_id_len);
	freeProtocolIE(mme_ue_s1ap_id);

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int initial_context_setup_response_len;
	uint8_t * initial_context_setup_response_len_buffer = ProtocolIE_to_buffer(initial_context_setup_response, &initial_context_setup_response_len);
	freeProtocolIE(initial_context_setup_response);

	int i = 0;
	ics_resp->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + initial_context_setup_response_len);

	memcpy(ics_resp->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(ics_resp->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(ics_resp->items + i, initial_context_setup_response_len_buffer, initial_context_setup_response_len);
	GC_free(initial_context_setup_response_len_buffer);
	i += initial_context_setup_response_len;

	ics_resp->items_len = i;

	return ics_resp;
}

s1ap_initiatingMessage * generate_S1AP_initial_context_setup_response(eNB * enb, UE * ue, uint8_t * ue_ip)
{
	s1ap_initiatingMessage * s1ap_initiatingMsg = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_initiatingMsg->procedureCode = INITIAL_CONTEXT_SETUP;
	s1ap_initiatingMsg->criticality = CRITICALITY_REJECT;

	Initial_Context_Setup_Response * ics_resp;
	ics_resp = generate_initial_context_setup_response(enb, ue, ue_ip);

	uint16_t len;
	uint8_t * initial_context_setup_response_buf = InitialContextSetupResponse_to_buffer(ics_resp, &len);

	freeInitialContextSetupResponse(ics_resp);

	s1ap_initiatingMsg->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_initiatingMsg->value + 1, (void *) initial_context_setup_response_buf, len);
	GC_free(initial_context_setup_response_buf);
	s1ap_initiatingMsg->value[0] = (uint8_t) len;
	s1ap_initiatingMsg->value_len = len + 1;
	return s1ap_initiatingMsg;
}

UE_Context_Release_Request * generate_ue_context_release_request(UE * ue)
{
	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * cause;


	UE_Context_Release_Request * ue_crr = GC_malloc(sizeof(InitialUEMessage));
	ue_crr->init = 0x00;
	ue_crr->numItems = 0x0003;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	mme_ue_s1ap_id->criticality = CRITICALITY_REJECT;

	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	enb_ue_s1ap_id->criticality = CRITICALITY_REJECT;

	cause = generate_ProtocolIE_cause();
	cause->criticality = CRITICALITY_IGNORE;

	int mme_ue_s1ap_id_len;
	uint8_t * mme_ue_s1ap_id_buffer = ProtocolIE_to_buffer(mme_ue_s1ap_id, &mme_ue_s1ap_id_len);
	freeProtocolIE(mme_ue_s1ap_id);

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int cause_len;
	uint8_t * cause_buffer = ProtocolIE_to_buffer(cause, &cause_len);
	freeProtocolIE(cause);

	int i = 0;
	ue_crr->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + cause_len);

	memcpy(ue_crr->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(ue_crr->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(ue_crr->items + i, cause_buffer, cause_len);
	GC_free(cause_buffer);
	i += cause_len;

	ue_crr->items_len = i;

	return ue_crr;
}

UE_Detach * generate_ue_detach(eNB * enb, UE * ue, Auth_Challenge * auth_challenge, uint8_t switch_off)
{
	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * eutran_cgi;
	ProtocolIE * tai;


	UE_Detach * ue_d = GC_malloc(sizeof(InitialUEMessage));
	ue_d->init = 0x00;
	ue_d->numItems = 0x0005;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu_detach(ue, auth_challenge, switch_off);
	eutran_cgi = generate_ProtocolIE_eutran_cgi(enb);
	tai = generate_ProtocolIE_tai(enb);

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
	ue_d->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len + nas_pdu_len + eutran_cgi_len + tai_len);

	memcpy(ue_d->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(ue_d->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(ue_d->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(ue_d->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(ue_d->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	ue_d->items_len = i;

	return ue_d;
}

/* ProtocolIE(id-eNB-UE-S1AP-ID) + ProtocolIE(id-NAS-PDU) + ProtocolIE(id-TAI) + ProtocolIE(id-EUTRAN-CGI) + ProtocolIE(id-RRC-Establishment-Cause) + id-S-TMSI*/
InitialUEMessage_ServiceRequest * generate_InitialUEMessage_ServiceRequest(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	ProtocolIE * enb_ue_s1ap_id;
	ProtocolIE * nas_pdu;
	ProtocolIE * tai;
	ProtocolIE * eutran_cgi;
	ProtocolIE * rrc_establishment_cause;
	ProtocolIE * s_tmsi;


	InitialUEMessage_ServiceRequest * ue_sr = GC_malloc(sizeof(InitialUEMessage));
	ue_sr->init = 0x00;
	ue_sr->numItems = 0x0006;
	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	nas_pdu = generate_ProtocolIE_nas_pdu_service_request(ue, auth_challenge);
	eutran_cgi = generate_ProtocolIE_eutran_cgi(enb);
	tai = generate_ProtocolIE_tai(enb);
	rrc_establishment_cause = generate_ProtocolIE_rrc_establishment_cause(MO_DATA);
	s_tmsi = generate_ProtocolIE_s_tmsi(ue);


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

	int s_tmsi_len;
	uint8_t * s_tmsi_buffer = ProtocolIE_to_buffer(s_tmsi, &s_tmsi_len);
	freeProtocolIE(s_tmsi);

	int i = 0;
	ue_sr->items = (uint8_t *) GC_malloc(enb_ue_s1ap_id_len + nas_pdu_len + tai_len + eutran_cgi_len + rrc_establishment_cause_len + s_tmsi_len);

	memcpy(ue_sr->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	memcpy(ue_sr->items + i, nas_pdu_buffer, nas_pdu_len);
	GC_free(nas_pdu_buffer);
	i += nas_pdu_len;

	memcpy(ue_sr->items + i, tai_buffer, tai_len);
	GC_free(tai_buffer);
	i += tai_len;

	memcpy(ue_sr->items + i, eutran_cgi_buffer, eutran_cgi_len);
	GC_free(eutran_cgi_buffer);
	i += eutran_cgi_len;

	memcpy(ue_sr->items + i, rrc_establishment_cause_buffer, rrc_establishment_cause_len);
	GC_free(rrc_establishment_cause_buffer);
	i += rrc_establishment_cause_len;

	memcpy(ue_sr->items + i, s_tmsi_buffer, s_tmsi_len);
	GC_free(s_tmsi_buffer);
	i += s_tmsi_len;

	ue_sr->items_len = i;

	return ue_sr;
}

UE_Context_Release_Response * generate_ue_context_release_response(UE * ue)
{
	ProtocolIE * mme_ue_s1ap_id;
	ProtocolIE * enb_ue_s1ap_id;


	UE_Context_Release_Response * ue_crr = GC_malloc(sizeof(InitialUEMessage));
	ue_crr->init = 0x20;
	ue_crr->numItems = 0x0002;
	mme_ue_s1ap_id = generate_ProtocolIE_mme_ue_s1ap_id(ue);
	mme_ue_s1ap_id->criticality = CRITICALITY_REJECT;

	enb_ue_s1ap_id = generate_ProtocolIE_enb_ue_s1ap_id(ue);
	enb_ue_s1ap_id->criticality = CRITICALITY_REJECT;

	int mme_ue_s1ap_id_len;
	uint8_t * mme_ue_s1ap_id_buffer = ProtocolIE_to_buffer(mme_ue_s1ap_id, &mme_ue_s1ap_id_len);
	freeProtocolIE(mme_ue_s1ap_id);

	int enb_ue_s1ap_id_len;
	uint8_t * enb_ue_s1ap_id_buffer = ProtocolIE_to_buffer(enb_ue_s1ap_id, &enb_ue_s1ap_id_len);
	freeProtocolIE(enb_ue_s1ap_id);

	int i = 0;
	ue_crr->items = (uint8_t *) GC_malloc(mme_ue_s1ap_id_len + enb_ue_s1ap_id_len);

	memcpy(ue_crr->items + i, mme_ue_s1ap_id_buffer, mme_ue_s1ap_id_len);
	GC_free(mme_ue_s1ap_id_buffer);
	i += mme_ue_s1ap_id_len;

	memcpy(ue_crr->items + i, enb_ue_s1ap_id_buffer, enb_ue_s1ap_id_len);
	GC_free(enb_ue_s1ap_id_buffer);
	i += enb_ue_s1ap_id_len;

	ue_crr->items_len = i;

	return ue_crr;
}

s1ap_initiatingMessage * generate_UE_Context_Release_Request(UE * ue)
{
	s1ap_initiatingMessage * s1ap_ue_context_release_req = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_ue_context_release_req->procedureCode = UE_CONTEXT_RELEASE_REQUEST;
	s1ap_ue_context_release_req->criticality = CRITICALITY_IGNORE;

	UE_Context_Release_Request * ue_crr;
	ue_crr = generate_ue_context_release_request(ue);

	uint16_t len;
	uint8_t * ue_crr_buf = UEContextReleaseRequest_to_buffer(ue_crr, &len);

	freeUEContextReleaseRequest(ue_crr);

	s1ap_ue_context_release_req->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_ue_context_release_req->value + 1, (void *) ue_crr_buf, len);
	GC_free(ue_crr_buf);
	s1ap_ue_context_release_req->value[0] = (uint8_t) len;
	s1ap_ue_context_release_req->value_len = len + 1;
	return s1ap_ue_context_release_req;
}

s1ap_initiatingMessage * generate_UE_Context_Release_Response(UE * ue)
{
	s1ap_initiatingMessage * s1ap_ue_context_release_res = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_ue_context_release_res->procedureCode = (SUCCESSFUL_OUTCOME << 8) | UE_CONTEXT_RELEASE_RESPONSE;
	s1ap_ue_context_release_res->criticality = CRITICALITY_REJECT;

	UE_Context_Release_Response * ue_crr;
	ue_crr = generate_ue_context_release_response(ue);

	uint16_t len;
	uint8_t * ue_crr_buf = UEContextReleaseResponse_to_buffer(ue_crr, &len);

	freeUEContextReleaseResponse(ue_crr);

	s1ap_ue_context_release_res->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_ue_context_release_res->value + 1, (void *) ue_crr_buf, len);
	GC_free(ue_crr_buf);
	s1ap_ue_context_release_res->value[0] = (uint8_t) len;
	s1ap_ue_context_release_res->value_len = len + 1;
	return s1ap_ue_context_release_res;
}

s1ap_initiatingMessage * generate_s1ap_UE_Detach(eNB * enb, UE * ue, Auth_Challenge * auth_challenge, uint8_t switch_off)
{
	s1ap_initiatingMessage * s1ap_ue_detach = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_ue_detach->procedureCode = UPLINK_NAS_TRANSPORT;
	s1ap_ue_detach->criticality = CRITICALITY_IGNORE;

	UE_Detach * ue_d;
	ue_d = generate_ue_detach(enb, ue, auth_challenge, switch_off);

	uint16_t len;
	uint8_t * ue_d_buf = UEDetach_to_buffer(ue_d, &len);

	freeUEDetach(ue_d);

	s1ap_ue_detach->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_ue_detach->value + 1, (void *) ue_d_buf, len);
	GC_free(ue_d_buf);
	s1ap_ue_detach->value[0] = (uint8_t) len;
	s1ap_ue_detach->value_len = len + 1;
	return s1ap_ue_detach;
}

s1ap_initiatingMessage * generate_Initial_UE_Message_Service_Request(eNB * enb, UE * ue, Auth_Challenge * auth_challenge)
{
	s1ap_initiatingMessage * s1ap_ue_serv_req = GC_malloc(sizeof(s1ap_initiatingMessage));
	s1ap_ue_serv_req->procedureCode = INITIAL_UE_MESSAGE;
	s1ap_ue_serv_req->criticality = CRITICALITY_IGNORE;

	InitialUEMessage_ServiceRequest * ue_sr;
	ue_sr = generate_InitialUEMessage_ServiceRequest(enb, ue, auth_challenge); /* todo */

	uint16_t len;
	uint8_t * ue_sr_buf = InitialUEMessage_ServiceRequest_to_buffer(ue_sr, &len);

	freeUEMessage_ServiceRequest(ue_sr);

	s1ap_ue_serv_req->value = (uint8_t *) GC_malloc(len + 1);
	memcpy(s1ap_ue_serv_req->value + 1, (void *) ue_sr_buf, len);
	GC_free(ue_sr_buf);
	s1ap_ue_serv_req->value[0] = (uint8_t) len;
	s1ap_ue_serv_req->value_len = len + 1;
	return s1ap_ue_serv_req;
}







/*******************/
/* Main Procedures */
/*******************/

int procedure_Attach_Default_EPS_Bearer(eNB * enb, UE * ue, uint8_t * ue_ip)
{
	s1ap_initiatingMessage * s1ap_attach_request;
	s1ap_initiatingMessage * s1ap_auth_response;
	s1ap_initiatingMessage * s1ap_sec_comm_complete;
	s1ap_initiatingMessage * ue_capability;
	s1ap_initiatingMessage * init_context_setup_response;
	s1ap_initiatingMessage * attach_complete;
	int err;
	uint16_t len = 0;
	uint8_t * initiatingMsg_buffer;
	uint8_t * authentication_res_buffer;
	uint8_t * security_command_complete_buffer;
	uint8_t * ue_capability_buffer;
	uint8_t * init_context_setup_response_buffer;
	uint8_t * attach_complete_buffer;
	uint8_t buffer[BUFFER_LEN];
	Auth_Challenge * auth_challenge = get_auth_challenge(ue);
	int flags = 0;
	int recv_len;
	struct sockaddr_in addr;
	int socket = get_mme_socket(enb);
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));

	bzero(buffer, BUFFER_LEN);
	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));

	/* Reset UE NAS sequence number */
	reset_nas_sequence_number(ue);

	printInfo("Starting Attach and Default EPS Bearer procedure...\n");

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

	while(1)
	{
		/* Receiving MME answer */
		recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
		if(recv_len < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			{
				printError("SCTP Timeout (%s)\n", strerror(errno));
			}
			else
			{
				printError("SCTP (%s)\n", strerror(errno));
			}
			return 1;
		}

		/* Analyze buffer and get AUTHENTICATION REQUEST*/
		err = analyze_Authentication_Request(buffer, auth_challenge, ue);
		if(err == 1)
		{
			printError("Authentication Request error\n");
			return 1;
		}
		else if(err == 2)
		{
			/* Special case srsEPC: EPC sends EMM information message after Attach complete and its detected in the authentication of the following UE */
			printInfo("Special Case: EMM information message received\n");
		}
		else
		{
			printOK("Authentication Request\n");
			break;
		}
	}

	/* Calculate Challenge */
	f2345(get_ue_key(ue), auth_challenge->RAND, auth_challenge->RES, auth_challenge->CK, auth_challenge->IK, auth_challenge->AK, get_ue_op_key(ue));
	printChallenge(auth_challenge);
	printOK("RES successfully calculated: ");
	dumpMemory(auth_challenge->RES, 8);

	/***************************/
	/* Authentication Response */
	/***************************/
	s1ap_auth_response = generate_S1AP_Authentication_Response(enb, ue, auth_challenge);
	authentication_res_buffer = s1ap_initiatingMessage_to_buffer(s1ap_auth_response, &len);
	freeS1ap_initiatingMessage(s1ap_auth_response);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) authentication_res_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(authentication_res_buffer);

	/* KASME, NAS_ENC and NAS_INT are generated while UE is waiting for Security mode command */
	/* Only integrity is implemented in this simulator to minimize the CPU load */
	generate_kasme(auth_challenge->CK, auth_challenge->IK, auth_challenge->AK, auth_challenge->AUTN, get_plmn(enb), PLMN_LENGTH, auth_challenge->KASME);


	/*************************/
	/* Security Mode Command */
	/*************************/
	/* Receiving MME answer */
	flags = 0;
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
		{
			printError("SCTP Timeout (%s)\n", strerror(errno));
		}
		else
		{
			printError("SCTP (%s)\n", strerror(errno));
		}
		return 1;
	}

	/* Analyze buffer and get Security Mode Command*/
	err = analyze_Security_Mode_Command(buffer, ue);
	if(err == 1)
    {
    	printError("Security Mode Command\n");
    	return 1;
    }
    else
    {
    	printOK("Security Mode Command\n");
    }

    /* Temporary encryption and integrity keys generation */
	generate_nas_enc_keys(auth_challenge->KASME, auth_challenge->NAS_KEY_ENC, get_nas_session_enc_alg(ue));
	generate_nas_int_keys(auth_challenge->KASME, auth_challenge->NAS_KEY_INT, get_nas_session_int_alg(ue));
	printChallengeDerivatedKeys(auth_challenge);

	/**************************/
	/* Security Mode Complete */
	/**************************/
	s1ap_sec_comm_complete = generate_S1AP_Security_Command_Complete(enb, ue, auth_challenge);
	security_command_complete_buffer = s1ap_initiatingMessage_to_buffer(s1ap_sec_comm_complete, &len);
	freeS1ap_initiatingMessage(s1ap_sec_comm_complete);

	/* Sending Security Mode Complete */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) security_command_complete_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(security_command_complete_buffer);

	/* Receiving MME answer */
	flags = 0;
	recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
	if(recv_len < 0)
	{
		if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
		{
			printError("SCTP Timeout (%s)\n", strerror(errno));
		}
		else
		{
			printError("SCTP (%s)\n", strerror(errno));
		}
		return 1;
	}

	/* Analyze buffer and get Initial Context Setup Request*/
	err = analyze_InitialContextSetupRequest(buffer, ue);
	if(err == 1)
    {
    	printError("Initial Context Setup Request\n");
    	return 1;
    }
    else
    {
    	printOK("Initial Context Setup Request\n");
    }


	/*********************************/
	/* UE Capability Info Indication */
	/*********************************/
	ue_capability = generate_S1AP_ue_capability_info_indication(enb, ue);
	ue_capability_buffer = s1ap_initiatingMessage_to_buffer(ue_capability, &len);
	freeS1ap_initiatingMessage(ue_capability);

	/* Sending UE Capability Info Indication */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_capability_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_capability_buffer);


	/**********************************/
	/* Initial Context Setup Response */
	/**********************************/
	init_context_setup_response = generate_S1AP_initial_context_setup_response(enb, ue, ue_ip);
	init_context_setup_response_buffer = s1ap_initiatingMessage_to_buffer(init_context_setup_response, &len);
	/* Special case with successful outcome flag */
	init_context_setup_response_buffer[0] = SUCCESSFUL_OUTCOME;
	freeS1ap_initiatingMessage(init_context_setup_response);

	/* Sending Initial Context Setup Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) init_context_setup_response_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(init_context_setup_response_buffer);

	/****************************************************************/
	/* Attach Complete & Activate default EPS bearer context accept */
	/****************************************************************/
	attach_complete = generate_S1AP_Attach_Complete(enb, ue, auth_challenge);
	attach_complete_buffer = s1ap_initiatingMessage_to_buffer(attach_complete, &len);
	freeS1ap_initiatingMessage(attach_complete);

	/* Sending Attach Complete & Activate default EPS bearer context accept */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) attach_complete_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(attach_complete_buffer);

	printOK("Attach Complete & Activate default EPS bearer context accept\n");

	return 0;
}

int procedure_UE_Context_Release(eNB * enb, UE * ue)
{
	s1ap_initiatingMessage * ue_context_release_request;
	s1ap_initiatingMessage * ue_context_release_response;
	uint8_t * ue_context_release_request_buffer;
	uint8_t * ue_context_release_response_buffer;
	uint16_t len = 0;
	int err;
	int socket = get_mme_socket(enb);
	uint8_t buffer[BUFFER_LEN];
	int flags;
	int recv_len;
	struct sockaddr_in addr;
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));
	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));

	if(ue == NULL)
	{
		printError("UE NULL reference\n");
		return 1;
	}

	printInfo("Starting UE Context Release procedure...\n");


	/*****************************/
	/* UE Context Release Request*/
	/*****************************/
	ue_context_release_request = generate_UE_Context_Release_Request(ue);
	ue_context_release_request_buffer = s1ap_initiatingMessage_to_buffer(ue_context_release_request, &len);
	freeS1ap_initiatingMessage(ue_context_release_request);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_context_release_request_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_context_release_request_buffer);

	while(1)
	{
		/* Receiving MME answer */
		flags = 0;
		recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
		if(recv_len < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			{
				printError("SCTP Timeout (%s)\n", strerror(errno));
			}
			else
			{
				printError("SCTP (%s)\n", strerror(errno));
			}
			return 1;
		}

		/* Analyze buffer and get Initial Context Setup Request*/
		err = analyze_UEContextReleaseCommand(buffer, ue);
		if(err == 1)
	    {
	    	printError("UE Context Release Command\n");
	    	return 1;
	    }
	    else if(err == 2)
		{
			/* Special case srsEPC: EPC sends EMM information message after Attach complete and its detected in the authentication of the following UE */
			printInfo("Special Case: EMM information message received\n");
		}
	    else
	    {
	    	printOK("UE Context Release Command\n");
	    	break;
	    }
	}


    /*****************************/
	/* UE Context Release Response*/
	/*****************************/
	ue_context_release_response = generate_UE_Context_Release_Response(ue);
	ue_context_release_response_buffer = s1ap_initiatingMessage_to_buffer(ue_context_release_response, &len);
	freeS1ap_initiatingMessage(ue_context_release_response);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_context_release_response_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_context_release_response_buffer);

	return 0;
}

int procedure_UE_Detach(eNB * enb, UE * ue, uint8_t switch_off)
{
	s1ap_initiatingMessage * ue_detach;
	s1ap_initiatingMessage * ue_context_release_response;
	uint8_t * ue_detach_buffer;
	uint8_t * ue_context_release_response_buffer;
	Auth_Challenge * auth_challenge = get_auth_challenge(ue);
	uint16_t len = 0;
	int err;
	int socket = get_mme_socket(enb);
	uint8_t buffer[BUFFER_LEN];
	int flags;
	int recv_len;
	struct sockaddr_in addr;
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	bzero((void *)&addr, sizeof(struct sockaddr_in));
	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));

	if(ue == NULL)
	{
		printError("UE NULL reference\n");
		return 1;
	}

	printInfo("Starting UE Detach procedure...\n");


	/*************/
	/* UE Detach */
	/*************/
	ue_detach = generate_s1ap_UE_Detach(enb, ue, auth_challenge, switch_off);
	ue_detach_buffer = s1ap_initiatingMessage_to_buffer(ue_detach, &len);
	freeS1ap_initiatingMessage(ue_detach);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_detach_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_detach_buffer);


	/***********************************************/
	/* UE Detach Accept and UEContextReleaseCommand*/
	/***********************************************/

	while(1)
	{
		/* Receiving MME answer */
		flags = 0;
		recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
		if(recv_len < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			{
				printError("SCTP Timeout (%s)\n", strerror(errno));
			}
			else
			{
				printError("SCTP (%s)\n", strerror(errno));
			}
			return 1;
		}

		/* Analyze buffer */
		err = analyze_UEAccept(buffer, ue);
		if(err == 1)
		{
			/* Analyze buffer and get Initial Context Setup Request*/
			err = analyze_UEContextReleaseCommand(buffer, ue);
			if(err == 1)
			{
				printError("Detach procedure error\n");
				return 1;
			}
			else
			{
				printOK("UE Context Release Command\n");
				break;
			}
		}
		else if(err == 2)
		{
			/* Special case srsEPC: EPC sends EMM information message after Attach complete and its detected in the authentication of the following UE */
			printInfo("Special Case: EMM information message received\n");
		}
		else
		{
			printOK("UE Detach Accept\n");
		}
	}




    /*****************************/
	/* UE Context Release Response*/
	/*****************************/
	ue_context_release_response = generate_UE_Context_Release_Response(ue);
	ue_context_release_response_buffer = s1ap_initiatingMessage_to_buffer(ue_context_release_response, &len);
	freeS1ap_initiatingMessage(ue_context_release_response);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_context_release_response_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_context_release_response_buffer);

	return 0;
}

int procedure_UE_Service_Request(eNB * enb, UE * ue, uint8_t * ue_ip)
{
	s1ap_initiatingMessage * ue_service_request;
	s1ap_initiatingMessage * init_context_setup_response;
	uint8_t * ue_service_request_buffer;
	uint8_t buffer[BUFFER_LEN];
	uint8_t * init_context_setup_response_buffer;
	Auth_Challenge * auth_challenge = get_auth_challenge(ue);
	int socket = get_mme_socket(enb);
	uint16_t len = 0;
	int err;
	socklen_t from_len = (socklen_t)sizeof(struct sockaddr_in);
	int flags;
	int recv_len;
	struct sockaddr_in addr;

	/* SCTP parameters */
	struct sctp_sndrcvinfo sndrcvinfo;
	bzero(&sndrcvinfo, sizeof(struct sctp_sndrcvinfo));


	/**********************/
	/* UE Service Request */
	/**********************/
	ue_service_request = generate_Initial_UE_Message_Service_Request(enb, ue, auth_challenge);
	ue_service_request_buffer = s1ap_initiatingMessage_to_buffer(ue_service_request, &len);
	freeS1ap_initiatingMessage(ue_service_request);

	/* Sending Authentication Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) ue_service_request_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(ue_service_request_buffer);



	/******************************/
	/* InitialContextSetupRequest */
	/******************************/

	while(1)
	{
		/* Receiving MME answer */
		flags = 0;
		recv_len = sctp_recvmsg(socket, (void *)buffer, BUFFER_LEN, (struct sockaddr *)&addr, &from_len, &sndrcvinfo, &flags);
		if(recv_len < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			{
				printError("SCTP Timeout (%s)\n", strerror(errno));
			}
			else
			{
				printError("SCTP (%s)\n", strerror(errno));
			}
			return 1;
		}

		/* Analyze buffer and get Initial Context Setup Request*/
		err = analyze_InitialContextSetupRequest(buffer, ue);
		if(err == 1)
		{
			/* Analyze buffer and get Initial Context Setup Request*/
			printError("Initial Context Setup Request\n");
	    	return 1;
		}
		else if(err == 2)
		{
			/* EPC sends EMM information message after Attach complete and its detected in the authentication of the following UE */
			printInfo("Special Case: EMM information message received\n");
		}
		else
		{
			printOK("Initial Context Setup Request\n");
			break;
		}
	}


	/**********************************/
	/* Initial Context Setup Response */
	/**********************************/
	init_context_setup_response = generate_S1AP_initial_context_setup_response(enb, ue, ue_ip);
	init_context_setup_response_buffer = s1ap_initiatingMessage_to_buffer(init_context_setup_response, &len);
	/* Special case with successful outcome flag */
	init_context_setup_response_buffer[0] = SUCCESSFUL_OUTCOME;
	freeS1ap_initiatingMessage(init_context_setup_response);

	/* Sending Initial Context Setup Response */
	/* MUST BE ON STREAM 1 */
	sctp_sendmsg(socket, (void *) init_context_setup_response_buffer, (size_t) len, NULL, 0, htonl(SCTP_S1AP), 0, 1, 0, 0);
	GC_free(init_context_setup_response_buffer);


	return 0;
}



/* TODO: Implement an authomatic way to detect EMM information messages without a loop */

/* TODO: Modify analyze_NAS function to extract data within a loop */

/* TODO: Create a geneneric way to receive messages */
