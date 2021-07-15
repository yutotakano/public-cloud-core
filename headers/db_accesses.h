#ifndef __COREKUBE_DB_ACCESSES_H__
#define __COREKUBE_DB_ACCESSES_H__

#include "core/ogs-core.h"
#include <libck.h>

extern int __corekube_log_domain;

#undef OGS_LOG_DOMAIN
#define OGS_LOG_DOMAIN __corekube_log_domain

#define DB_ACCESS_REGISTRATION_REQUEST \
    1, 1, \
    IMSI, \
    IMSI

int db_access(corekube_db_pulls_t ** dbPulls, ITEM_TYPE dbKey, uint8_t * dbKeyValue, int numPush, int numPull, ...);

int db_buffer_input_size(int numPush, int numPull);

int form_db_request(uint8_t *buffer, va_list ap, ITEM_TYPE dbKey, uint8_t * dbKeyValue, int numPush, int numPull);

int db_buffer_output_size(int numPull);

#endif /* __COREKUBE_DB_ACCESSES_H__ */