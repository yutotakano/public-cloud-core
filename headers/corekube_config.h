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

#define CoreKube_MCC 209
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