/**
 * @file data_source.c
 * @brief Report Data Source Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef enum {
    DATA_SOURCE_DATABASE,
    DATA_SOURCE_API,
    DATA_SOURCE_FILE,
    DATA_SOURCE_CALCULATED
} data_source_type_t;

typedef struct {
    regislex_uuid_t id;
    char name[128];
    data_source_type_t type;
    char query[4096];
    char connection_string[512];
    char parameters[1024];
} regislex_data_source_t;

regislex_error_t regislex_data_source_create(const char* name,
                                              data_source_type_t type,
                                              regislex_data_source_t** ds) {
    if (!name || !ds) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *ds = (regislex_data_source_t*)platform_calloc(1, sizeof(regislex_data_source_t));
    if (!*ds) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*ds)->id);
    strncpy((*ds)->name, name, sizeof((*ds)->name) - 1);
    (*ds)->type = type;

    return REGISLEX_OK;
}

void regislex_data_source_free(regislex_data_source_t* ds) {
    platform_free(ds);
}

regislex_error_t regislex_data_source_set_query(regislex_data_source_t* ds, const char* query) {
    if (!ds || !query) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(ds->query, query, sizeof(ds->query) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_data_source_execute(regislex_data_source_t* ds,
                                               regislex_db_context_t* db,
                                               void** result) {
    (void)ds;
    (void)db;
    (void)result;
    return REGISLEX_ERROR_NOT_IMPLEMENTED;
}
