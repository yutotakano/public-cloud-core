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
typedef struct _tai tai;
typedef struct _eutran_cgi eutran_cgi;
typedef struct _rrc_establishment_cause rrc_establishment_cause;

typedef struct _ProtocolIE ProtocolIE;
typedef struct _S1SetupRequest S1SetupRequest;
typedef struct _s1ap_initiatingMessage s1ap_initiatingMessage;
typedef struct _S1SetupResponse S1SetupResponse;
typedef struct _InitialUEMessage InitialUEMessage;
typedef struct _Authentication_Request Authentication_Request;
typedef struct _Auth_Challenge Auth_Challenge;
typedef struct _Authentication_Response Authentication_Response;
typedef struct _Security_Mode_Command Security_Mode_Command;

void test(eNB * enb);


int procedure_S1_Setup(int socket, eNB * enb);
int procedure_Attach_Default_EPS_Bearer(int socket, eNB * enb, UE * ue);
void crypto_test(eNB * enb, UE * ue);
#endif