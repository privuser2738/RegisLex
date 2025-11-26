/**
 * @file version_control.c
 * @brief Document Version Control
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t document_id;
    int version_number;
    char file_path[512];
    char checksum[65];
    char comment[512];
    regislex_uuid_t created_by;
    regislex_datetime_t created_at;
} document_version_t;

regislex_error_t regislex_doc_version_create(const char* doc_id, int version, document_version_t** ver) {
    if (!doc_id || !ver) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *ver = (document_version_t*)platform_calloc(1, sizeof(document_version_t));
    if (!*ver) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*ver)->id);
    regislex_uuid_parse(doc_id, &(*ver)->document_id);
    (*ver)->version_number = version;
    regislex_datetime_now(&(*ver)->created_at);
    return REGISLEX_OK;
}

void regislex_doc_version_free(document_version_t* ver) { platform_free(ver); }

regislex_error_t regislex_doc_checkout(const char* doc_id, const char* user_id) {
    (void)doc_id; (void)user_id;
    return REGISLEX_OK;
}

regislex_error_t regislex_doc_checkin(const char* doc_id, const char* user_id, const char* comment) {
    (void)doc_id; (void)user_id; (void)comment;
    return REGISLEX_OK;
}
