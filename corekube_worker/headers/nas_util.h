#ifndef __S1AP_HANDLER_NAS_UTIL_H__
#define __S1AP_HANDLER_NAS_UTIL_H__

#include "nas_message.h"
#include "s1ap/s1ap_message.h"
#include <libck.h>

status_t decode_nas_emm(S1AP_NAS_PDU_t *NAS_PDU, S1AP_MME_UE_S1AP_ID_t *mme_ue_id, nas_message_t *nas_message);

#endif /* __S1AP_HANDLER_NAS_UTIL_H__ */