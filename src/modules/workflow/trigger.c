/**
 * @file trigger.c
 * @brief Workflow Trigger System
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef enum {
    TRIGGER_TYPE_MANUAL,
    TRIGGER_TYPE_SCHEDULE,
    TRIGGER_TYPE_EVENT,
    TRIGGER_TYPE_CONDITION
} trigger_type_t;

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t workflow_id;
    trigger_type_t type;
    char event_name[128];
    char schedule[64];  /* cron expression */
    char condition[512];
    bool is_active;
} regislex_trigger_t;

regislex_error_t regislex_trigger_create(trigger_type_t type, regislex_trigger_t** trigger) {
    if (!trigger) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *trigger = (regislex_trigger_t*)platform_calloc(1, sizeof(regislex_trigger_t));
    if (!*trigger) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*trigger)->id);
    (*trigger)->type = type;
    (*trigger)->is_active = true;

    return REGISLEX_OK;
}

void regislex_trigger_free(regislex_trigger_t* trigger) {
    platform_free(trigger);
}

regislex_error_t regislex_trigger_set_event(regislex_trigger_t* trigger, const char* event) {
    if (!trigger || !event) return REGISLEX_ERROR_INVALID_ARGUMENT;
    trigger->type = TRIGGER_TYPE_EVENT;
    strncpy(trigger->event_name, event, sizeof(trigger->event_name) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_trigger_set_schedule(regislex_trigger_t* trigger, const char* cron) {
    if (!trigger || !cron) return REGISLEX_ERROR_INVALID_ARGUMENT;
    trigger->type = TRIGGER_TYPE_SCHEDULE;
    strncpy(trigger->schedule, cron, sizeof(trigger->schedule) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_trigger_set_condition(regislex_trigger_t* trigger, const char* condition) {
    if (!trigger || !condition) return REGISLEX_ERROR_INVALID_ARGUMENT;
    trigger->type = TRIGGER_TYPE_CONDITION;
    strncpy(trigger->condition, condition, sizeof(trigger->condition) - 1);
    return REGISLEX_OK;
}

bool regislex_trigger_evaluate(const regislex_trigger_t* trigger, const void* context) {
    if (!trigger || !trigger->is_active) return false;
    (void)context;

    switch (trigger->type) {
        case TRIGGER_TYPE_MANUAL:
            return true;
        case TRIGGER_TYPE_SCHEDULE:
            /* TODO: Evaluate cron expression */
            return false;
        case TRIGGER_TYPE_EVENT:
            /* TODO: Check if event matches */
            return false;
        case TRIGGER_TYPE_CONDITION:
            /* TODO: Evaluate condition expression */
            return false;
    }
    return false;
}
