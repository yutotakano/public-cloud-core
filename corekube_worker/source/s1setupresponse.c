/*************************************************************************** 

    Copyright (C) 2019 NextEPC Inc. All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***************************************************************************/

#include "s1setupresponse.h"
#include "s1ap_conv.h"


void s1ap_build_setup_resp(s1ap_message_t *response, S1AP_PLMNidentity_t *derivedPLMNidentity)
{
    d_info("Building S1AP S1SetupResponse message");

    S1AP_SuccessfulOutcome_t *successfulOutcome = NULL;
    S1AP_S1SetupResponse_t *S1SetupResponse = NULL;

    S1AP_S1SetupResponseIEs_t *ie = NULL;
    S1AP_ServedGUMMEIs_t *ServedGUMMEIs = NULL;
    S1AP_RelativeMMECapacity_t *RelativeMMECapacity = NULL;

    response->present = S1AP_S1AP_PDU_PR_successfulOutcome;
    response->choice.successfulOutcome = 
        core_calloc(1, sizeof(S1AP_SuccessfulOutcome_t));

    successfulOutcome = response->choice.successfulOutcome;
    successfulOutcome->procedureCode = S1AP_ProcedureCode_id_S1Setup;
    successfulOutcome->criticality = S1AP_Criticality_reject;
    successfulOutcome->value.present =
        S1AP_SuccessfulOutcome__value_PR_S1SetupResponse;

    S1SetupResponse = &successfulOutcome->value.choice.S1SetupResponse;

    ie = core_calloc(1, sizeof(S1AP_S1SetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&S1SetupResponse->protocolIEs, ie);

    ie->id = S1AP_ProtocolIE_ID_id_ServedGUMMEIs;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_S1SetupResponseIEs__value_PR_ServedGUMMEIs;

    ServedGUMMEIs = &ie->value.choice.ServedGUMMEIs;

    ie = core_calloc(1, sizeof(S1AP_S1SetupResponseIEs_t));
    ASN_SEQUENCE_ADD(&S1SetupResponse->protocolIEs, ie);

    ie->id = S1AP_ProtocolIE_ID_id_RelativeMMECapacity;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupResponseIEs__value_PR_RelativeMMECapacity;

    RelativeMMECapacity = &ie->value.choice.RelativeMMECapacity;

    S1AP_ServedGUMMEIsItem_t *ServedGUMMEIsItem = NULL;
    ServedGUMMEIsItem = (S1AP_ServedGUMMEIsItem_t *)
        core_calloc(1, sizeof(S1AP_ServedGUMMEIsItem_t));

    S1AP_PLMNidentity_t *PLMNidentity = NULL;
    PLMNidentity = (S1AP_PLMNidentity_t *)
        core_calloc(1, sizeof(S1AP_PLMNidentity_t));
    s1ap_buffer_to_OCTET_STRING(
                derivedPLMNidentity->buf, derivedPLMNidentity->size, PLMNidentity);
    ASN_SEQUENCE_ADD(
            &ServedGUMMEIsItem->servedPLMNs.list, PLMNidentity);

    S1AP_MME_Group_ID_t *MME_Group_ID = NULL;
    MME_Group_ID = (S1AP_MME_Group_ID_t *)
        core_calloc(1, sizeof(S1AP_MME_Group_ID_t));
    s1ap_uint16_to_OCTET_STRING(0x0004, MME_Group_ID);
    ASN_SEQUENCE_ADD(
            &ServedGUMMEIsItem->servedGroupIDs.list, MME_Group_ID);

    S1AP_MME_Code_t *MME_Code = NULL ;
    MME_Code = (S1AP_MME_Code_t *)
        core_calloc(1, sizeof(S1AP_MME_Code_t));
    s1ap_uint8_to_OCTET_STRING(0x01, MME_Code);
    ASN_SEQUENCE_ADD(&ServedGUMMEIsItem->servedMMECs.list, MME_Code);

    ASN_SEQUENCE_ADD(&ServedGUMMEIs->list, ServedGUMMEIsItem);

    *RelativeMMECapacity = 0xff; // TAKEN FROM src/mme/mme_context.c#L175
}