#include "db_accesses.h"

#include <libck.h>
#include <stdarg.h>
#include <pthread.h>
extern int db_sock;
extern pthread_mutex_t db_sock_mutex;

int db_access(corekube_db_pulls_t * dbPulls, ITEM_TYPE dbKey, uint8_t * dbKeyValue, int numPush, int numPull, ...) {
    ogs_info("DB access");

    int bufferInputSize = db_buffer_input_size(numPush, numPull);
    uint8_t * inputBuffer = ogs_calloc(bufferInputSize, sizeof(uint8_t));

	va_list ap;
	va_start(ap, numPull);
    int dbReq = form_db_request(inputBuffer, ap, dbKey, dbKeyValue, numPush, numPull);
    ogs_assert(dbReq == OGS_OK);
	va_end(ap);

    pthread_mutex_lock(&db_sock_mutex);
    send_request(db_sock, inputBuffer, bufferInputSize);
    ogs_free(inputBuffer);

    if (numPull > 0) {
        int bufferOutputSize = db_buffer_output_size(numPull);
        uint8_t * outputBuffer = ogs_calloc(bufferOutputSize, sizeof(uint8_t));

        dbPulls->head = outputBuffer;

        int n = recv_response(db_sock, outputBuffer, bufferOutputSize);
        extract_db_values(outputBuffer, n, dbPulls);
    }
    pthread_mutex_unlock(&db_sock_mutex);

    return OGS_OK;
}

int db_buffer_input_size(int numPush, int numPull) {
    // libck input format is as follows:
    //   17 bytes for key and key value
    //   17 bytes * number of push values
    //   1 byte to signal end of push values
    //   1 byte * number of pull values

    return 18 + (17 * numPush) + numPull;
}

int form_db_request(uint8_t *buffer, va_list ap, ITEM_TYPE dbKey, uint8_t * dbKeyValue, int numPush, int numPull) {
    ogs_info("Forming DB request");

    uint8_t * ptr = add_identifier(buffer, dbKey, dbKeyValue);

	for(int i = 0; i < numPush; i++) {
		ITEM_TYPE type = (ITEM_TYPE) va_arg(ap, int);
		uint8_t * item = va_arg(ap, uint8_t*);
		ptr = push_item(ptr, type, item);
	}

    *ptr = 0;
    ptr++;
    for(int i = 0; i < numPull; i++) {
		ITEM_TYPE item = (ITEM_TYPE) va_arg(ap, int);
		ptr = pull_item(ptr, item);
	}

    *ptr = EOM;
    ptr++;

    return OGS_OK;
}

int db_buffer_output_size(int numPull) {
    // libck output format is as follows:
    //   17 bytes * number of pull values

    return 17 * numPull;
}
