/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "../support/s1ap-r16.4.0/36413-g40.asn"
 * 	`asn1c -pdu=all -fcompound-names -findirect-choice -fno-include-deps -no-gen-BER -no-gen-XER -no-gen-OER -no-gen-UPER`
 */

#ifndef	_S1AP_CSG_Id_H_
#define	_S1AP_CSG_Id_H_


#include <asn_application.h>

/* Including external dependencies */
#include <BIT_STRING.h>

#ifdef __cplusplus
extern "C" {
#endif

/* S1AP_CSG-Id */
typedef BIT_STRING_t	 S1AP_CSG_Id_t;

/* Implementation */
extern asn_per_constraints_t asn_PER_type_S1AP_CSG_Id_constr_1;
extern asn_TYPE_descriptor_t asn_DEF_S1AP_CSG_Id;
asn_struct_free_f S1AP_CSG_Id_free;
asn_struct_print_f S1AP_CSG_Id_print;
asn_constr_check_f S1AP_CSG_Id_constraint;
per_type_decoder_f S1AP_CSG_Id_decode_aper;
per_type_encoder_f S1AP_CSG_Id_encode_aper;

#ifdef __cplusplus
}
#endif

#endif	/* _S1AP_CSG_Id_H_ */
#include <asn_internal.h>
