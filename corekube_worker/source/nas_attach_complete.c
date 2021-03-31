#include <limits.h> //TODO: this should not be necessary, it's included in core.h
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "core/include/core_lib.h"

#include "nas_attach_complete.h"

status_t nas_handle_attach_complete(nas_message_t *nas_message, S1AP_MME_UE_S1AP_ID_t *mme_ue_id) {
    d_info("Handling NAS Attach Complete");

    // TODO: mark the attach as being complete in the DB

    return CORE_OK;
}