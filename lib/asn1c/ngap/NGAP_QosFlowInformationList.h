/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-IEs"
 * 	found in "../support/ngap-r16.4.0/38413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_NGAP_QosFlowInformationList_H_
#define	_NGAP_QosFlowInformationList_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include <constr_SEQUENCE_OF.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct NGAP_QosFlowInformationItem;

/* NGAP_QosFlowInformationList */
typedef struct NGAP_QosFlowInformationList {
	A_SEQUENCE_OF(struct NGAP_QosFlowInformationItem) list;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} NGAP_QosFlowInformationList_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_NGAP_QosFlowInformationList;
extern asn_SET_OF_specifics_t asn_SPC_NGAP_QosFlowInformationList_specs_1;
extern asn_TYPE_member_t asn_MBR_NGAP_QosFlowInformationList_1[1];
extern asn_per_constraints_t asn_PER_type_NGAP_QosFlowInformationList_constr_1;

#ifdef __cplusplus
}
#endif

#endif	/* _NGAP_QosFlowInformationList_H_ */
#include <asn_internal.h>
