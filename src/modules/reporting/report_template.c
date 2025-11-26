/**
 * @file report_template.c
 * @brief Report Template Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    char name[128];
    char description[512];
    char format[32];  /* html, pdf, csv, xlsx */
    char template_content[16384];
    char parameters[2048];  /* JSON parameter definitions */
    bool is_system;
} regislex_report_template_t;

regislex_error_t regislex_report_template_create(const char* name,
                                                  const char* format,
                                                  regislex_report_template_t** tmpl) {
    if (!name || !format || !tmpl) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *tmpl = (regislex_report_template_t*)platform_calloc(1, sizeof(regislex_report_template_t));
    if (!*tmpl) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*tmpl)->id);
    strncpy((*tmpl)->name, name, sizeof((*tmpl)->name) - 1);
    strncpy((*tmpl)->format, format, sizeof((*tmpl)->format) - 1);

    return REGISLEX_OK;
}

void regislex_report_template_free(regislex_report_template_t* tmpl) {
    platform_free(tmpl);
}

regislex_error_t regislex_report_template_set_content(regislex_report_template_t* tmpl,
                                                       const char* content) {
    if (!tmpl || !content) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(tmpl->template_content, content, sizeof(tmpl->template_content) - 1);
    return REGISLEX_OK;
}
