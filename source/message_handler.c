#include "message_handler.h"

#include "ngap/ogs-ngap.h"

int message_handler_entrypoint(void *incoming, int incoming_len, message_handler_response_t *response) {
	ogs_ngap_message_t msg;

	ogs_pkbuf_t * pkbuf = NULL;
	pkbuf = ogs_pkbuf_alloc(NULL, OGS_MAX_SDU_LEN);
	pkbuf->len = incoming_len;
	memcpy(pkbuf->data, incoming, pkbuf->len);

	int status = ogs_ngap_decode(&msg, pkbuf);
	ogs_pkbuf_free(pkbuf);

	return status;
}