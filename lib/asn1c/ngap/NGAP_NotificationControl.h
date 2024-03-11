/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "NGAP-IEs"
 * 	found in "../support/ngap-r16.4.0/38413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_NGAP_NotificationControl_H_
#define	_NGAP_NotificationControl_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum NGAP_NotificationControl {
	NGAP_NotificationControl_notification_requested	= 0
	/*
	 * Enumeration is extensible
	 */
} e_NGAP_NotificationControl;

/* NGAP_NotificationControl */
typedef long	 NGAP_NotificationControl_t;

/* Implementation */
extern asn_per_constraints_t asn_PER_type_NGAP_NotificationControl_constr_1;
extern asn_TYPE_descriptor_t asn_DEF_NGAP_NotificationControl;
extern const asn_INTEGER_specifics_t asn_SPC_NotificationControl_specs_1;
asn_struct_free_f NotificationControl_free;
asn_struct_print_f NotificationControl_print;
asn_constr_check_f NotificationControl_constraint;
per_type_decoder_f NotificationControl_decode_aper;
per_type_encoder_f NotificationControl_encode_aper;

#ifdef __cplusplus
}
#endif

#endif	/* _NGAP_NotificationControl_H_ */
#include <asn_internal.h>
