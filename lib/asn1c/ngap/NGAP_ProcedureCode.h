/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-CommonDataTypes"
 * 	found in "../support/ngap-r16.4.0/38413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_NGAP_ProcedureCode_H_
#define	_NGAP_ProcedureCode_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeInteger.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NGAP_ProcedureCode */
typedef long	 NGAP_ProcedureCode_t;

/* Implementation */
extern asn_per_constraints_t asn_PER_type_NGAP_ProcedureCode_constr_1;
extern asn_TYPE_descriptor_t asn_DEF_NGAP_ProcedureCode;
asn_struct_free_f NGAP_ProcedureCode_free;
asn_struct_print_f NGAP_ProcedureCode_print;
asn_constr_check_f NGAP_ProcedureCode_constraint;
per_type_decoder_f NGAP_ProcedureCode_decode_aper;
per_type_encoder_f NGAP_ProcedureCode_encode_aper;
#define NGAP_ProcedureCode_id_AMFConfigurationUpdate	((NGAP_ProcedureCode_t)0)
#define NGAP_ProcedureCode_id_AMFStatusIndication	((NGAP_ProcedureCode_t)1)
#define NGAP_ProcedureCode_id_CellTrafficTrace	((NGAP_ProcedureCode_t)2)
#define NGAP_ProcedureCode_id_DeactivateTrace	((NGAP_ProcedureCode_t)3)
#define NGAP_ProcedureCode_id_DownlinkNASTransport	((NGAP_ProcedureCode_t)4)
#define NGAP_ProcedureCode_id_DownlinkNonUEAssociatedNRPPaTransport	((NGAP_ProcedureCode_t)5)
#define NGAP_ProcedureCode_id_DownlinkRANConfigurationTransfer	((NGAP_ProcedureCode_t)6)
#define NGAP_ProcedureCode_id_DownlinkRANStatusTransfer	((NGAP_ProcedureCode_t)7)
#define NGAP_ProcedureCode_id_DownlinkUEAssociatedNRPPaTransport	((NGAP_ProcedureCode_t)8)
#define NGAP_ProcedureCode_id_ErrorIndication	((NGAP_ProcedureCode_t)9)
#define NGAP_ProcedureCode_id_HandoverCancel	((NGAP_ProcedureCode_t)10)
#define NGAP_ProcedureCode_id_HandoverNotification	((NGAP_ProcedureCode_t)11)
#define NGAP_ProcedureCode_id_HandoverPreparation	((NGAP_ProcedureCode_t)12)
#define NGAP_ProcedureCode_id_HandoverResourceAllocation	((NGAP_ProcedureCode_t)13)
#define NGAP_ProcedureCode_id_InitialContextSetup	((NGAP_ProcedureCode_t)14)
#define NGAP_ProcedureCode_id_InitialUEMessage	((NGAP_ProcedureCode_t)15)
#define NGAP_ProcedureCode_id_LocationReportingControl	((NGAP_ProcedureCode_t)16)
#define NGAP_ProcedureCode_id_LocationReportingFailureIndication	((NGAP_ProcedureCode_t)17)
#define NGAP_ProcedureCode_id_LocationReport	((NGAP_ProcedureCode_t)18)
#define NGAP_ProcedureCode_id_NASNonDeliveryIndication	((NGAP_ProcedureCode_t)19)
#define NGAP_ProcedureCode_id_NGReset	((NGAP_ProcedureCode_t)20)
#define NGAP_ProcedureCode_id_NGSetup	((NGAP_ProcedureCode_t)21)
#define NGAP_ProcedureCode_id_OverloadStart	((NGAP_ProcedureCode_t)22)
#define NGAP_ProcedureCode_id_OverloadStop	((NGAP_ProcedureCode_t)23)
#define NGAP_ProcedureCode_id_Paging	((NGAP_ProcedureCode_t)24)
#define NGAP_ProcedureCode_id_PathSwitchRequest	((NGAP_ProcedureCode_t)25)
#define NGAP_ProcedureCode_id_PDUSessionResourceModify	((NGAP_ProcedureCode_t)26)
#define NGAP_ProcedureCode_id_PDUSessionResourceModifyIndication	((NGAP_ProcedureCode_t)27)
#define NGAP_ProcedureCode_id_PDUSessionResourceRelease	((NGAP_ProcedureCode_t)28)
#define NGAP_ProcedureCode_id_PDUSessionResourceSetup	((NGAP_ProcedureCode_t)29)
#define NGAP_ProcedureCode_id_PDUSessionResourceNotify	((NGAP_ProcedureCode_t)30)
#define NGAP_ProcedureCode_id_PrivateMessage	((NGAP_ProcedureCode_t)31)
#define NGAP_ProcedureCode_id_PWSCancel	((NGAP_ProcedureCode_t)32)
#define NGAP_ProcedureCode_id_PWSFailureIndication	((NGAP_ProcedureCode_t)33)
#define NGAP_ProcedureCode_id_PWSRestartIndication	((NGAP_ProcedureCode_t)34)
#define NGAP_ProcedureCode_id_RANConfigurationUpdate	((NGAP_ProcedureCode_t)35)
#define NGAP_ProcedureCode_id_RerouteNASRequest	((NGAP_ProcedureCode_t)36)
#define NGAP_ProcedureCode_id_RRCInactiveTransitionReport	((NGAP_ProcedureCode_t)37)
#define NGAP_ProcedureCode_id_TraceFailureIndication	((NGAP_ProcedureCode_t)38)
#define NGAP_ProcedureCode_id_TraceStart	((NGAP_ProcedureCode_t)39)
#define NGAP_ProcedureCode_id_UEContextModification	((NGAP_ProcedureCode_t)40)
#define NGAP_ProcedureCode_id_UEContextRelease	((NGAP_ProcedureCode_t)41)
#define NGAP_ProcedureCode_id_UEContextReleaseRequest	((NGAP_ProcedureCode_t)42)
#define NGAP_ProcedureCode_id_UERadioCapabilityCheck	((NGAP_ProcedureCode_t)43)
#define NGAP_ProcedureCode_id_UERadioCapabilityInfoIndication	((NGAP_ProcedureCode_t)44)
#define NGAP_ProcedureCode_id_UETNLABindingRelease	((NGAP_ProcedureCode_t)45)
#define NGAP_ProcedureCode_id_UplinkNASTransport	((NGAP_ProcedureCode_t)46)
#define NGAP_ProcedureCode_id_UplinkNonUEAssociatedNRPPaTransport	((NGAP_ProcedureCode_t)47)
#define NGAP_ProcedureCode_id_UplinkRANConfigurationTransfer	((NGAP_ProcedureCode_t)48)
#define NGAP_ProcedureCode_id_UplinkRANStatusTransfer	((NGAP_ProcedureCode_t)49)
#define NGAP_ProcedureCode_id_UplinkUEAssociatedNRPPaTransport	((NGAP_ProcedureCode_t)50)
#define NGAP_ProcedureCode_id_WriteReplaceWarning	((NGAP_ProcedureCode_t)51)
#define NGAP_ProcedureCode_id_SecondaryRATDataUsageReport	((NGAP_ProcedureCode_t)52)
#define NGAP_ProcedureCode_id_UplinkRIMInformationTransfer	((NGAP_ProcedureCode_t)53)
#define NGAP_ProcedureCode_id_DownlinkRIMInformationTransfer	((NGAP_ProcedureCode_t)54)
#define NGAP_ProcedureCode_id_RetrieveUEInformation	((NGAP_ProcedureCode_t)55)
#define NGAP_ProcedureCode_id_UEInformationTransfer	((NGAP_ProcedureCode_t)56)
#define NGAP_ProcedureCode_id_RANCPRelocationIndication	((NGAP_ProcedureCode_t)57)
#define NGAP_ProcedureCode_id_UEContextResume	((NGAP_ProcedureCode_t)58)
#define NGAP_ProcedureCode_id_UEContextSuspend	((NGAP_ProcedureCode_t)59)
#define NGAP_ProcedureCode_id_UERadioCapabilityIDMapping	((NGAP_ProcedureCode_t)60)
#define NGAP_ProcedureCode_id_HandoverSuccess	((NGAP_ProcedureCode_t)61)
#define NGAP_ProcedureCode_id_UplinkRANEarlyStatusTransfer	((NGAP_ProcedureCode_t)62)
#define NGAP_ProcedureCode_id_DownlinkRANEarlyStatusTransfer	((NGAP_ProcedureCode_t)63)
#define NGAP_ProcedureCode_id_AMFCPRelocationIndication	((NGAP_ProcedureCode_t)64)
#define NGAP_ProcedureCode_id_ConnectionEstablishmentIndication	((NGAP_ProcedureCode_t)65)

#ifdef __cplusplus
}
#endif

#endif	/* _NGAP_ProcedureCode_H_ */
#include <asn_internal.h>
