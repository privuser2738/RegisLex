/**
 * @file reminder.c
 * @brief Deadline Reminder System
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t deadline_id;
    regislex_uuid_t user_id;
    regislex_datetime_t remind_at;
    char message[512];
    bool is_sent;
    char notification_type[32];  /* email, sms, push, in_app */
} regislex_reminder_t;

regislex_error_t regislex_reminder_create(const char* deadline_id,
                                           const char* user_id,
                                           int days_before,
                                           regislex_reminder_t** reminder) {
    if (!deadline_id || !user_id || !reminder) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *reminder = (regislex_reminder_t*)platform_calloc(1, sizeof(regislex_reminder_t));
    if (!*reminder) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*reminder)->id);
    regislex_uuid_parse(deadline_id, &(*reminder)->deadline_id);
    regislex_uuid_parse(user_id, &(*reminder)->user_id);
    strcpy((*reminder)->notification_type, "email");

    /* Set remind_at based on days_before */
    regislex_datetime_now(&(*reminder)->remind_at);
    regislex_datetime_add_days(&(*reminder)->remind_at, days_before);

    return REGISLEX_OK;
}

void regislex_reminder_free(regislex_reminder_t* reminder) {
    platform_free(reminder);
}

regislex_error_t regislex_reminder_set_message(regislex_reminder_t* reminder, const char* message) {
    if (!reminder || !message) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(reminder->message, message, sizeof(reminder->message) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_reminder_mark_sent(regislex_reminder_t* reminder) {
    if (!reminder) return REGISLEX_ERROR_INVALID_ARGUMENT;
    reminder->is_sent = true;
    return REGISLEX_OK;
}

bool regislex_reminder_is_due(const regislex_reminder_t* reminder) {
    if (!reminder || reminder->is_sent) return false;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    return regislex_datetime_compare(&now, &reminder->remind_at) >= 0;
}
