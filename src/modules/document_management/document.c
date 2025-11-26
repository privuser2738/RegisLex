/**
 * @file document.c
 * @brief Document Entity Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

/* Struct is already defined in include/regislex/modules/document_management/document.h */

regislex_error_t regislex_document_create(regislex_context_t* ctx,
                                           regislex_doc_type_t type,
                                           const char* name,
                                           regislex_document_t** doc) {
    (void)ctx;
    if (!name || !doc) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *doc = (regislex_document_t*)platform_calloc(1, sizeof(regislex_document_t));
    if (!*doc) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*doc)->id);
    strncpy((*doc)->name, name, sizeof((*doc)->name) - 1);
    (*doc)->type = type;
    (*doc)->status = REGISLEX_DOC_STATUS_DRAFT;
    (*doc)->current_version = 1;
    regislex_datetime_now(&(*doc)->created_at);
    memcpy(&(*doc)->updated_at, &(*doc)->created_at, sizeof(regislex_datetime_t));
    return REGISLEX_OK;
}

void regislex_document_free(regislex_document_t* doc) {
    platform_free(doc);
}

const char* regislex_document_get_id(const regislex_document_t* doc) {
    return doc ? doc->id.value : NULL;
}
