/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-PDU-Contents"
 * 	found in "../support/ngap-r16.4.0/38413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_NGAP_UEContextReleaseCommand_H_
#define	_NGAP_UEContextReleaseCommand_H_


#include <asn_application.h>

/* Including external dependencies */
#include "NGAP_ProtocolIE-Container.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NGAP_UEContextReleaseCommand */
typedef struct NGAP_UEContextReleaseCommand {
	NGAP_ProtocolIE_Container_9520P13_t	 protocolIEs;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} NGAP_UEContextReleaseCommand_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_NGAP_UEContextReleaseCommand;
extern asn_SEQUENCE_specifics_t asn_SPC_NGAP_UEContextReleaseCommand_specs_1;
extern asn_TYPE_member_t asn_MBR_NGAP_UEContextReleaseCommand_1[1];

#ifdef __cplusplus
}
#endif

#endif	/* _NGAP_UEContextReleaseCommand_H_ */
#include <asn_internal.h>
