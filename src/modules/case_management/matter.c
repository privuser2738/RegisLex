/**
 * @file matter.c
 * @brief Legal Matter Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    char matter_number[64];
    char title[256];
    char description[1024];
    char matter_type[64];
    char status[32];
    double budget;
    double billed_amount;
} regislex_matter_t;

regislex_error_t regislex_matter_create(regislex_context_t* ctx,
                                         const char* title,
                                         regislex_matter_t** matter) {
    if (!ctx || !title || !matter) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *matter = (regislex_matter_t*)platform_calloc(1, sizeof(regislex_matter_t));
    if (!*matter) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*matter)->id);
    strncpy((*matter)->title, title, sizeof((*matter)->title) - 1);
    strcpy((*matter)->status, "open");

    return REGISLEX_OK;
}

void regislex_matter_free(regislex_matter_t* matter) {
    platform_free(matter);
}

regislex_error_t regislex_matter_set_case(regislex_matter_t* matter, const char* case_id) {
    if (!matter || !case_id) return REGISLEX_ERROR_INVALID_ARGUMENT;
    return regislex_uuid_parse(case_id, &matter->case_id);
}

regislex_error_t regislex_matter_set_budget(regislex_matter_t* matter, double budget) {
    if (!matter || budget < 0) return REGISLEX_ERROR_INVALID_ARGUMENT;
    matter->budget = budget;
    return REGISLEX_OK;
}
