#ifndef __S1AP_HANDLER_DOWNLINKNASTRANSPORT_H__
#define __S1AP_HANDLER_DOWNLINKNASTRANSPORT_H__

#include "s1ap/asn1c/asn_system.h"
#include "s1ap/s1ap_message.h"
#include "s1ap_handler.h"

status_t generate_downlinknastransport(pkbuf_t *nas_pdu, S1AP_MME_UE_S1AP_ID_t mme_ue_id, S1AP_ENB_UE_S1AP_ID_t enb_ue_id, s1ap_message_t *downlink_nas_transport);

#endif /* __S1AP_HANDLER_DOWNLINKNASTRANSPORT_H__ */