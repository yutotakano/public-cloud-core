#ifndef __S1AP_HANDLER_NAS_AUTHENTICATION_REQUEST_H__
#define __S1AP_HANDLER_NAS_AUTHENTICATION_REQUEST_H__

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"

status_t generate_nas_authentication_request(c_uint8_t *rand, c_uint8_t *autn, pkbuf_t **pkbuf);

#endif /* __S1AP_HANDLER_NAS_AUTHENTICATION_REQUEST_H__ */