/**
 * @file legislation.c
 * @brief Legislation Entity
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

/* Struct is already defined in include/regislex/modules/legislative_tracking/legislative.h */

regislex_error_t regislex_legislation_create(regislex_context_t* ctx,
                                              regislex_legislation_type_t type,
                                              const char* bill_number,
                                              const char* title,
                                              regislex_legislation_t** leg) {
    (void)ctx;
    if (!bill_number || !title || !leg) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *leg = (regislex_legislation_t*)platform_calloc(1, sizeof(regislex_legislation_t));
    if (!*leg) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*leg)->id);
    strncpy((*leg)->bill_number, bill_number, sizeof((*leg)->bill_number) - 1);
    strncpy((*leg)->title, title, sizeof((*leg)->title) - 1);
    (*leg)->type = type;
    (*leg)->status = REGISLEX_LEG_STATUS_INTRODUCED;
    regislex_datetime_now(&(*leg)->introduced_date);
    return REGISLEX_OK;
}

void regislex_legislation_free(regislex_legislation_t* leg) {
    platform_free(leg);
}

const char* regislex_legislation_get_id(const regislex_legislation_t* leg) {
    return leg ? leg->id.value : NULL;
}

regislex_leg_status_t regislex_legislation_get_status(const regislex_legislation_t* leg) {
    return leg ? leg->status : REGISLEX_LEG_STATUS_INTRODUCED;
}
