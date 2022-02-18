#ifndef __CK_API__
#define __CK_API__

#include <stdint.h>

enum ITEM_TYPE
{
	EOM=1, /* 0x01 */
	IMSI, /* 0x02 */
	MSIN, /* 0x03 */
	TMSI, /* 0x04 */
	ENB_UE_S1AP_ID, /* 0x05 */
	UE_TEID, /* 0x06 */
	SPGW_IP, /* 0x07 */
	ENB_IP, /* 0x08 */
	PDN_IP, /* 0x09 */
	UE_NAS_SEQUENCE_NUMBER, /* 0x0A */
	EPC_NAS_SEQUENCE_NUMBER, /* 0x0B */
	UE_NAS_SEQUENCE_NUMBER_NO_INC, /* 0x0C */
	KEY, /* 0x0D */
	OPC, /* 0x0E */
	RAND, /* 0x0F */
	RAND_UPDATE, /* 0x10 */
	MME_UE_S1AP_ID, /* 0x11 */
	EPC_TEID, /* 0x12 */
	AUTH_RES, /* 0x13 */
	ENC_KEY, /* 0x14 */
	INT_KEY, /* 0x15 */
	KASME_1, /* 0x16 */
	KASME_2, /* 0x17 */
	NEW_ENB, /* 0x18 */
	GET_ENB, /* 0x19 */
	KNH_1, /* 0x1A */
	KNH_2, /* 0x1B */
	Target_ENB_UE_S1AP_ID, /* 0x1C */
	NEXT_HOP_CHAINING_COUNT /* 0x1D */
};
typedef uint8_t ITEM_TYPE;

#endif
