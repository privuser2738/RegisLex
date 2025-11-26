/**
 * @file template.c
 * @brief Workflow Template Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    char name[128];
    char description[512];
    char category[64];
    char definition[8192];  /* JSON workflow definition */
    bool is_system;
    bool is_active;
} regislex_workflow_template_t;

regislex_error_t regislex_wf_template_create(const char* name,
                                              regislex_workflow_template_t** tmpl) {
    if (!name || !tmpl) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *tmpl = (regislex_workflow_template_t*)platform_calloc(1, sizeof(regislex_workflow_template_t));
    if (!*tmpl) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*tmpl)->id);
    strncpy((*tmpl)->name, name, sizeof((*tmpl)->name) - 1);
    (*tmpl)->is_active = true;

    return REGISLEX_OK;
}

void regislex_wf_template_free(regislex_workflow_template_t* tmpl) {
    platform_free(tmpl);
}

regislex_error_t regislex_wf_template_set_definition(regislex_workflow_template_t* tmpl,
                                                      const char* json_def) {
    if (!tmpl || !json_def) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(tmpl->definition, json_def, sizeof(tmpl->definition) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_wf_template_set_category(regislex_workflow_template_t* tmpl,
                                                    const char* category) {
    if (!tmpl || !category) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(tmpl->category, category, sizeof(tmpl->category) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_wf_template_validate(const regislex_workflow_template_t* tmpl) {
    if (!tmpl) return REGISLEX_ERROR_INVALID_ARGUMENT;
    if (tmpl->name[0] == '\0') return REGISLEX_ERROR_VALIDATION;
    if (tmpl->definition[0] == '\0') return REGISLEX_ERROR_VALIDATION;
    /* TODO: Validate JSON structure */
    return REGISLEX_OK;
}

const char* regislex_wf_template_get_name(const regislex_workflow_template_t* tmpl) {
    return tmpl ? tmpl->name : NULL;
}

const char* regislex_wf_template_get_definition(const regislex_workflow_template_t* tmpl) {
    return tmpl ? tmpl->definition : NULL;
}
