#include <libck.h>
#include "db_accesses.h"
#include "corekube_config.h"
#include "nas_security_mode_command.h"
#include "nas_security.h"

#include "nas_authentication_response.h"

int nas_handle_authentication_response(ogs_nas_5gs_authentication_response_t *message, nas_ngap_params_t *params, message_handler_response_t * response) {
    ogs_info("Handling NAS Authentication Response");

    response->num_responses = 1;
    response->responses[0] = ogs_calloc(1, sizeof(ogs_nas_5gs_message_t));

    return nas_build_security_mode_command(response->responses[0]);
}