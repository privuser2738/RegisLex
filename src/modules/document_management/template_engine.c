/**
 * @file template_engine.c
 * @brief Document Template Engine
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    regislex_uuid_t id;
    char name[128];
    char content[32768];
    char variables[2048];
} doc_template_t;

regislex_error_t regislex_doc_template_create(const char* name, doc_template_t** tmpl) {
    if (!name || !tmpl) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *tmpl = (doc_template_t*)platform_calloc(1, sizeof(doc_template_t));
    if (!*tmpl) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*tmpl)->id);
    strncpy((*tmpl)->name, name, sizeof((*tmpl)->name) - 1);
    return REGISLEX_OK;
}

void regislex_doc_template_free(doc_template_t* tmpl) { platform_free(tmpl); }

regislex_error_t regislex_doc_template_render(const doc_template_t* tmpl, const char* vars_json, char* output, size_t output_size) {
    if (!tmpl || !output) return REGISLEX_ERROR_INVALID_ARGUMENT;
    (void)vars_json;
    strncpy(output, tmpl->content, output_size - 1);
    return REGISLEX_OK;
}
