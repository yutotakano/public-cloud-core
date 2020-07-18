#ifndef __s1ap__
#define __s1ap__

#include "enb.h"
#include "ue.h"

typedef struct _s1ap_value s1ap_value;
/* ProtocolIE Types */
typedef struct _default_paging_drx default_paging_drx;
typedef struct _supported_tas supported_tas;
typedef struct _id_enb_name id_enb_name;
typedef struct _global_enb_id global_enb_id;
typedef struct _enb_ue_s1ap_id enb_ue_s1ap_id;
typedef struct _non_access_stratum_pdu non_access_stratum_pdu;
typedef struct _non_access_stratum_pdu_auth_resp non_access_stratum_pdu_auth_resp;
typedef struct _non_access_stratum_pdu_sec_comm_complete non_access_stratum_pdu_sec_comm_complete;
typedef struct _tai tai;
typedef struct _eutran_cgi eutran_cgi;
typedef struct _rrc_establishment_cause rrc_establishment_cause;

typedef struct _ProtocolIE ProtocolIE;
typedef struct _S1SetupRequest S1SetupRequest;
typedef struct _s1ap_initiatingMessage s1ap_initiatingMessage;
typedef struct _S1SetupResponse S1SetupResponse;
typedef struct _InitialUEMessage InitialUEMessage;
typedef struct _Authentication_Request Authentication_Request;
typedef struct _Authentication_Response Authentication_Response;
typedef struct _Security_Mode_Command Security_Mode_Command;
typedef struct _Security_Command_Complete Security_Command_Complete;
typedef struct _UE_Capability UE_Capability;
typedef struct _Initial_Context_Setup_Request Initial_Context_Setup_Request;
typedef struct _Initial_Context_Setup_Response Initial_Context_Setup_Response;
typedef struct _E_RAB_Setup_List_C_txt_SU_Res E_RAB_Setup_List_C_txt_SU_Res;
typedef struct _E_RABSetupItemCtxtSURes E_RABSetupItemCtxtSURes;
typedef struct _Uplink_NAS_Transport Uplink_NAS_Transport;
typedef struct _UE_Context_Release_Request UE_Context_Release_Request;
typedef struct _UE_Context_Release_Command UE_Context_Release_Command;
typedef struct _UE_Context_Release_Response UE_Context_Release_Response;
typedef struct _non_access_stratum_pdu_attach_complete non_access_stratum_pdu_attach_complete;
typedef struct _non_access_stratum_pdu_detach non_access_stratum_pdu_detach;
typedef struct _non_access_stratum_pdu_service_request non_access_stratum_pdu_service_request;
typedef struct _UE_Detach_Accept UE_Detach_Accept;
typedef struct _UE_Detach UE_Detach;
typedef struct _InitialUEMessage_ServiceRequest InitialUEMessage_ServiceRequest;

void test(eNB * enb);


int procedure_S1_Setup(eNB * enb);
int procedure_Attach_Default_EPS_Bearer(eNB * enb, UE * ue, uint8_t * ue_ip);
int procedure_UE_Context_Release(eNB *enb, UE * ue);
int procedure_UE_Detach(eNB * enb, UE * ue, uint8_t switch_off);
int procedure_UE_Service_Request(eNB * enb, UE * ue, uint8_t * ue_ip);


#endif