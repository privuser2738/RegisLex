/**
 * @file tracker.c
 * @brief Legislative Tracking Service
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t legislation_id;
    char event_type[64];
    char description[1024];
    regislex_datetime_t event_date;
} legislative_event_t;

regislex_error_t regislex_leg_tracker_add_event(const char* leg_id, const char* event_type, const char* description) {
    (void)leg_id; (void)event_type; (void)description;
    return REGISLEX_OK;
}

regislex_error_t regislex_leg_tracker_subscribe(const char* leg_id, const char* user_id) {
    (void)leg_id; (void)user_id;
    return REGISLEX_OK;
}

regislex_error_t regislex_leg_tracker_unsubscribe(const char* leg_id, const char* user_id) {
    (void)leg_id; (void)user_id;
    return REGISLEX_OK;
}

regislex_error_t regislex_leg_tracker_get_updates(const char* since, legislative_event_t** events, int* count) {
    (void)since;
    if (events) *events = NULL;
    if (count) *count = 0;
    return REGISLEX_OK;
}
