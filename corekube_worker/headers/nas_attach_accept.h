#ifndef __S1AP_HANDLER_NAS_ATTACH_ACCEPT_H__
#define __S1AP_HANDLER_NAS_ATTACH_ACCEPT_H__

#include "nas/nas_message.h"
#include "s1ap/s1ap_message.h"
#include <libck.h>

status_t emm_build_attach_accept(pkbuf_t **emmbuf, pkbuf_t *esmbuf, corekube_db_pulls_t *db_pulls);

#endif /* __S1AP_HANDLER_NAS_ATTACH_ACCEPT_H__ */