/**
 * @file alert.c
 * @brief Legislative Alert System
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t user_id;
    char keywords[512];
    char jurisdictions[256];
    char alert_type[32];
    bool is_active;
} legislative_alert_t;

regislex_error_t regislex_leg_alert_create(const char* user_id, legislative_alert_t** alert) {
    if (!user_id || !alert) return REGISLEX_ERROR_INVALID_ARGUMENT;
    *alert = (legislative_alert_t*)platform_calloc(1, sizeof(legislative_alert_t));
    if (!*alert) return REGISLEX_ERROR_OUT_OF_MEMORY;
    regislex_uuid_generate(&(*alert)->id);
    regislex_uuid_parse(user_id, &(*alert)->user_id);
    (*alert)->is_active = true;
    return REGISLEX_OK;
}

void regislex_leg_alert_free(legislative_alert_t* alert) { platform_free(alert); }

regislex_error_t regislex_leg_alert_set_keywords(legislative_alert_t* alert, const char* keywords) {
    if (!alert || !keywords) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(alert->keywords, keywords, sizeof(alert->keywords) - 1);
    return REGISLEX_OK;
}
