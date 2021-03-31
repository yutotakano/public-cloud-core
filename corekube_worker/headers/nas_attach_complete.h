#ifndef __S1AP_HANDLER_NAS_ATTACH_COMPLETE_H__
#define __S1AP_HANDLER_NAS_ATTACH_COMPLETE_H__

#include "nas_message.h"
#include "s1ap/s1ap_message.h"

status_t nas_handle_attach_complete(nas_message_t *nas_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id);

#endif /* __S1AP_HANDLER_NAS_ATTACH_COMPLETE_H__ */