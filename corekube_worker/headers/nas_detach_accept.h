#ifndef __S1AP_HANDLER_NAS_DETACH_ACCEPT_H__
#define __S1AP_HANDLER_NAS_DETACH_ACCEPT_H__

#include "nas_message.h"
#include "s1ap/s1ap_message.h"

#include <libck.h>

status_t nas_build_detach_accept(nas_eps_mobile_identity_t *mobile_identity, corekube_db_pulls_t *db_pulls, pkbuf_t **emmbuf);

#endif /* __S1AP_HANDLER_NAS_DETACH_ACCEPT_H__ */