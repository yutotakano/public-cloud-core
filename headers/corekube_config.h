/**
 * Configuration Information for CoreKube
 */


/**
 * Common Values
 */


/**
 * Values used to build the NG Setup Response
 */

#define CoreKube_AMFName "CoreKube5G_Worker"

#define CoreKube_MCC 208
#define CoreKube_MNC 93
#define CoreKube_MNClen 2

#define CoreKube_AMFRegionID 2
#define CoreKube_AMFSetID 1
#define CoreKube_AMFPointer 0

#define CoreKube_RelativeCapacity 255

#define CoreKube_NSSAI_sST 1


/**
 * Values used to build the Authentication Request
 */

#define CoreKube_AMF_Field {0x80, 0x00}

#define CoreKube_SQN_Value {0x00, 0x00, 0x00, 0x00, 0x00, 0x40};

#define CoreKube_NGKSI_TSC 0
#define CoreKube_NGKSI_Value 0

#define CoreKube_ABBA_Length 2
#define CoreKube_ABBA_Value "0000"

/**
 * Values used for the NAS message security
 */

#define COREKUBE_NAS_SECURITY_DOWNLINK_DIRECTION 1
#define COREKUBE_NAS_SECURITY_UPLINK_DIRECTION 0

#define COREKUBE_NAS_SECURITY_MAC_SIZE 4

#define COREKUBE_NAS_SECURITY_ACCESS_TYPE_3GPP_ACCESS 1

#define COREKUBE_NAS_SECURITY_ENC_ALGORITHM OGS_NAS_SECURITY_ALGORITHMS_EEA0
#define COREKUBE_NAS_SECURITY_INT_ALGORITHM OGS_NAS_SECURITY_ALGORITHMS_128_EIA2
#define COREKUBE_NAS_SECURITY_5GEA_SUPPORT 0
#define COREKUBE_NAS_SECURITY_5GIA_SUPPORT 0
#define COREKUBE_NAS_SECURITY_EEA_SUPPORT 0b10000000 // only EEA0
#define COREKUBE_NAS_SECURITY_EIA_SUPPORT 0b00100000 // only 128_EIA2

/**
 * Values used for the NAS Security Mode Command
 */

#define COREKUBE_UE_SECURITY_CAPABILITY_NR_EA 0xF0
#define COREKUBE_UE_SECURITY_CAPABILITY_NR_IA 0xF0
#define COREKUBE_UE_SECURITY_CAPABILITY_EUTRA_EA 0xF0
#define COREKUBE_UE_SECURITY_CAPABILITY_EUTRA_IA 0xF0

/**
 * Values used for the NAS Registration Accept
 */

#define COREKUBE_DEFAULT_TAC 1

#define COREKUBE_DEFAULT_sNSSAI_VALUE "0000"