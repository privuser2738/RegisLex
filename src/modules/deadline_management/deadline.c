/**
 * @file deadline.c
 * @brief Deadline Management Implementation
 *
 * Provides functionality for tracking court dates, statutes of limitations,
 * filing deadlines, reminders, and calendar management.
 */

#include "regislex/regislex.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static regislex_deadline_t* deadline_alloc(void) {
    return (regislex_deadline_t*)platform_calloc(1, sizeof(regislex_deadline_t));
}

static regislex_error_t deadline_from_row(regislex_db_stmt_t* stmt, regislex_deadline_t* dl) {
    if (!stmt || !dl) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int col = 0;

    regislex_db_column_uuid(stmt, col++, &dl->id);
    regislex_db_column_uuid(stmt, col++, &dl->case_id);
    regislex_db_column_uuid(stmt, col++, &dl->matter_id);

    const char* title = regislex_db_column_text(stmt, col++);
    if (title) strncpy(dl->title, title, sizeof(dl->title) - 1);

    const char* description = regislex_db_column_text(stmt, col++);
    if (description) strncpy(dl->description, description, sizeof(dl->description) - 1);

    dl->type = (regislex_deadline_type_t)regislex_db_column_int(stmt, col++);
    dl->status = (regislex_status_t)regislex_db_column_int(stmt, col++);
    dl->priority = (regislex_priority_t)regislex_db_column_int(stmt, col++);

    regislex_db_column_datetime(stmt, col++, &dl->due_date);
    regislex_db_column_datetime(stmt, col++, &dl->start_date);

    dl->is_all_day = regislex_db_column_int(stmt, col++) != 0;
    dl->duration_minutes = (int)regislex_db_column_int(stmt, col++);

    dl->recurrence = (regislex_recurrence_t)regislex_db_column_int(stmt, col++);

    regislex_db_column_uuid(stmt, col++, &dl->assigned_to_id);

    const char* rule_ref = regislex_db_column_text(stmt, col++);
    if (rule_ref) strncpy(dl->rule_reference, rule_ref, sizeof(dl->rule_reference) - 1);

    dl->days_from_trigger = (int)regislex_db_column_int(stmt, col++);
    dl->count_business_days = regislex_db_column_int(stmt, col++) != 0;

    regislex_db_column_datetime(stmt, col++, &dl->completed_at);
    regislex_db_column_uuid(stmt, col++, &dl->completed_by);

    const char* completion_notes = regislex_db_column_text(stmt, col++);
    if (completion_notes) strncpy(dl->completion_notes, completion_notes, sizeof(dl->completion_notes) - 1);

    const char* location = regislex_db_column_text(stmt, col++);
    if (location) strncpy(dl->location, location, sizeof(dl->location) - 1);

    const char* tags = regislex_db_column_text(stmt, col++);
    if (tags) strncpy(dl->tags, tags, sizeof(dl->tags) - 1);

    regislex_db_column_datetime(stmt, col++, &dl->created_at);
    regislex_db_column_datetime(stmt, col++, &dl->updated_at);
    regislex_db_column_uuid(stmt, col++, &dl->created_by);

    return REGISLEX_OK;
}

/* ============================================================================
 * Deadline Management Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_deadline_create(
    regislex_context_t* ctx,
    const regislex_deadline_t* deadline,
    regislex_deadline_t** out_deadline)
{
    if (!ctx || !deadline || !out_deadline) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_deadline_t* new_dl = deadline_alloc();
    if (!new_dl) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_dl, deadline, sizeof(regislex_deadline_t));

    if (new_dl->id.value[0] == '\0') {
        regislex_uuid_generate(&new_dl->id);
    }

    regislex_datetime_now(&new_dl->created_at);
    memcpy(&new_dl->updated_at, &new_dl->created_at, sizeof(regislex_datetime_t));

    regislex_db_context_t* db = NULL;

    const char* sql =
        "INSERT INTO deadlines ("
        "  id, case_id, matter_id, title, description, type, status, priority,"
        "  due_date, start_date, is_all_day, duration_minutes, recurrence,"
        "  assigned_to_id, rule_reference, days_from_trigger, count_business_days,"
        "  completed_at, completed_by, completion_notes, location, tags,"
        "  created_at, updated_at, created_by"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_deadline_free(new_dl);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_dl->id);
    regislex_db_bind_uuid(stmt, idx++, &new_dl->case_id);
    regislex_db_bind_uuid(stmt, idx++, &new_dl->matter_id);
    regislex_db_bind_text(stmt, idx++, new_dl->title);
    regislex_db_bind_text(stmt, idx++, new_dl->description);
    regislex_db_bind_int(stmt, idx++, new_dl->type);
    regislex_db_bind_int(stmt, idx++, new_dl->status);
    regislex_db_bind_int(stmt, idx++, new_dl->priority);
    regislex_db_bind_datetime(stmt, idx++, &new_dl->due_date);
    regislex_db_bind_datetime(stmt, idx++, &new_dl->start_date);
    regislex_db_bind_int(stmt, idx++, new_dl->is_all_day ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, new_dl->duration_minutes);
    regislex_db_bind_int(stmt, idx++, new_dl->recurrence);
    regislex_db_bind_uuid(stmt, idx++, &new_dl->assigned_to_id);
    regislex_db_bind_text(stmt, idx++, new_dl->rule_reference);
    regislex_db_bind_int(stmt, idx++, new_dl->days_from_trigger);
    regislex_db_bind_int(stmt, idx++, new_dl->count_business_days ? 1 : 0);
    regislex_db_bind_datetime(stmt, idx++, &new_dl->completed_at);
    regislex_db_bind_uuid(stmt, idx++, &new_dl->completed_by);
    regislex_db_bind_text(stmt, idx++, new_dl->completion_notes);
    regislex_db_bind_text(stmt, idx++, new_dl->location);
    regislex_db_bind_text(stmt, idx++, new_dl->tags);
    regislex_db_bind_datetime(stmt, idx++, &new_dl->created_at);
    regislex_db_bind_datetime(stmt, idx++, &new_dl->updated_at);
    regislex_db_bind_uuid(stmt, idx++, &new_dl->created_by);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_deadline_free(new_dl);
        return err;
    }

    *out_deadline = new_dl;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_deadline_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_deadline_t** out_deadline)
{
    if (!ctx || !id || !out_deadline) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "SELECT id, case_id, matter_id, title, description, type, status, priority,"
        "  due_date, start_date, is_all_day, duration_minutes, recurrence,"
        "  assigned_to_id, rule_reference, days_from_trigger, count_business_days,"
        "  completed_at, completed_by, completion_notes, location, tags,"
        "  created_at, updated_at, created_by "
        "FROM deadlines WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_deadline_t* dl = deadline_alloc();
    if (!dl) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    deadline_from_row(stmt, dl);
    regislex_db_finalize(stmt);

    *out_deadline = dl;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_deadline_update(
    regislex_context_t* ctx,
    const regislex_deadline_t* deadline)
{
    if (!ctx || !deadline) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE deadlines SET "
        "  case_id = ?, matter_id = ?, title = ?, description = ?,"
        "  type = ?, status = ?, priority = ?, due_date = ?, start_date = ?,"
        "  is_all_day = ?, duration_minutes = ?, recurrence = ?,"
        "  assigned_to_id = ?, rule_reference = ?, days_from_trigger = ?,"
        "  count_business_days = ?, location = ?, tags = ?, updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &deadline->case_id);
    regislex_db_bind_uuid(stmt, idx++, &deadline->matter_id);
    regislex_db_bind_text(stmt, idx++, deadline->title);
    regislex_db_bind_text(stmt, idx++, deadline->description);
    regislex_db_bind_int(stmt, idx++, deadline->type);
    regislex_db_bind_int(stmt, idx++, deadline->status);
    regislex_db_bind_int(stmt, idx++, deadline->priority);
    regislex_db_bind_datetime(stmt, idx++, &deadline->due_date);
    regislex_db_bind_datetime(stmt, idx++, &deadline->start_date);
    regislex_db_bind_int(stmt, idx++, deadline->is_all_day ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, deadline->duration_minutes);
    regislex_db_bind_int(stmt, idx++, deadline->recurrence);
    regislex_db_bind_uuid(stmt, idx++, &deadline->assigned_to_id);
    regislex_db_bind_text(stmt, idx++, deadline->rule_reference);
    regislex_db_bind_int(stmt, idx++, deadline->days_from_trigger);
    regislex_db_bind_int(stmt, idx++, deadline->count_business_days ? 1 : 0);
    regislex_db_bind_text(stmt, idx++, deadline->location);
    regislex_db_bind_text(stmt, idx++, deadline->tags);
    regislex_db_bind_datetime(stmt, idx++, &now);
    regislex_db_bind_uuid(stmt, idx++, &deadline->id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_deadline_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "DELETE FROM deadlines WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_deadline_complete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* notes)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE deadlines SET "
        "  status = ?, completed_at = ?, completion_notes = ?, updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, REGISLEX_STATUS_COMPLETED);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_text(stmt, 3, notes);
    regislex_db_bind_datetime(stmt, 4, &now);
    regislex_db_bind_uuid(stmt, 5, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_deadline_upcoming(
    regislex_context_t* ctx,
    int days_ahead,
    regislex_deadline_list_t** out_list)
{
    if (!ctx || !out_list || days_ahead < 0) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Calculate future date */
    regislex_datetime_t now;
    regislex_datetime_now(&now);

    /* Simple date addition (not accounting for month boundaries properly) */
    regislex_datetime_t future = now;
    future.day += days_ahead;
    /* Normalize would be needed here for production */

    char now_str[32], future_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));
    regislex_datetime_format(&future, future_str, sizeof(future_str));

    regislex_db_context_t* db = NULL;

    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, case_id, matter_id, title, description, type, status, priority,"
        "  due_date, start_date, is_all_day, duration_minutes, recurrence,"
        "  assigned_to_id, rule_reference, days_from_trigger, count_business_days,"
        "  completed_at, completed_by, completion_notes, location, tags,"
        "  created_at, updated_at, created_by "
        "FROM deadlines "
        "WHERE due_date >= '%s' AND due_date <= '%s' "
        "AND status != %d "
        "ORDER BY due_date ASC",
        now_str, future_str, REGISLEX_STATUS_COMPLETED);

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_deadline_list_t* list = (regislex_deadline_list_t*)platform_calloc(1, sizeof(regislex_deadline_list_t));
    if (!list) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    int capacity = 50;
    list->deadlines = (regislex_deadline_t**)platform_calloc(capacity, sizeof(regislex_deadline_t*));
    if (!list->deadlines) {
        platform_free(list);
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    while ((err = regislex_db_step(stmt)) == REGISLEX_OK) {
        if (list->count >= capacity) {
            capacity *= 2;
            regislex_deadline_t** new_deadlines = (regislex_deadline_t**)platform_realloc(
                list->deadlines, capacity * sizeof(regislex_deadline_t*));
            if (!new_deadlines) {
                regislex_deadline_list_free(list);
                regislex_db_finalize(stmt);
                return REGISLEX_ERROR_OUT_OF_MEMORY;
            }
            list->deadlines = new_deadlines;
        }

        regislex_deadline_t* dl = deadline_alloc();
        if (!dl) {
            regislex_deadline_list_free(list);
            regislex_db_finalize(stmt);
            return REGISLEX_ERROR_OUT_OF_MEMORY;
        }

        deadline_from_row(stmt, dl);
        list->deadlines[list->count++] = dl;
    }

    regislex_db_finalize(stmt);

    *out_list = list;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_deadline_overdue(
    regislex_context_t* ctx,
    regislex_deadline_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    regislex_db_context_t* db = NULL;

    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT id, case_id, matter_id, title, description, type, status, priority,"
        "  due_date, start_date, is_all_day, duration_minutes, recurrence,"
        "  assigned_to_id, rule_reference, days_from_trigger, count_business_days,"
        "  completed_at, completed_by, completion_notes, location, tags,"
        "  created_at, updated_at, created_by "
        "FROM deadlines "
        "WHERE due_date < '%s' AND status != %d "
        "ORDER BY due_date ASC",
        now_str, REGISLEX_STATUS_COMPLETED);

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_deadline_list_t* list = (regislex_deadline_list_t*)platform_calloc(1, sizeof(regislex_deadline_list_t));
    if (!list) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    int capacity = 50;
    list->deadlines = (regislex_deadline_t**)platform_calloc(capacity, sizeof(regislex_deadline_t*));

    while ((err = regislex_db_step(stmt)) == REGISLEX_OK) {
        if (list->count >= capacity) {
            capacity *= 2;
            list->deadlines = (regislex_deadline_t**)platform_realloc(
                list->deadlines, capacity * sizeof(regislex_deadline_t*));
        }

        regislex_deadline_t* dl = deadline_alloc();
        deadline_from_row(stmt, dl);
        list->deadlines[list->count++] = dl;
    }

    regislex_db_finalize(stmt);

    *out_list = list;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_deadline_calculate(
    regislex_context_t* ctx,
    const regislex_datetime_t* trigger_date,
    int days,
    bool count_business_days,
    const char* jurisdiction,
    regislex_datetime_t* out_date)
{
    if (!ctx || !trigger_date || !out_date) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    (void)jurisdiction; /* TODO: Use for holiday lookup */

    /* Copy trigger date as starting point */
    memcpy(out_date, trigger_date, sizeof(regislex_datetime_t));

    if (!count_business_days) {
        /* Simple calendar day addition */
        out_date->day += days;

        /* Normalize (basic - would need full calendar logic in production) */
        while (out_date->day > 31) {
            out_date->day -= 30;
            out_date->month++;
            if (out_date->month > 12) {
                out_date->month = 1;
                out_date->year++;
            }
        }
    } else {
        /* Business day calculation */
        int days_added = 0;
        while (days_added < days) {
            out_date->day++;

            /* Normalize month overflow */
            if (out_date->day > 28) { /* Simplified */
                out_date->day = 1;
                out_date->month++;
                if (out_date->month > 12) {
                    out_date->month = 1;
                    out_date->year++;
                }
            }

            /* Check if weekday (simplified - doesn't handle holidays) */
            /* Would need proper day-of-week calculation */
            bool is_business_day = true; /* Placeholder */

            /* TODO: Check holiday table */
            /* regislex_holiday_check(ctx, out_date, jurisdiction, &is_holiday); */

            if (is_business_day) {
                days_added++;
            }
        }
    }

    return REGISLEX_OK;
}

REGISLEX_API void regislex_deadline_free(regislex_deadline_t* deadline) {
    if (!deadline) return;

    if (deadline->metadata) {
        for (int i = 0; i < deadline->metadata_count; i++) {
            platform_free(deadline->metadata[i].key);
            platform_free(deadline->metadata[i].value);
        }
        platform_free(deadline->metadata);
    }

    platform_free(deadline);
}

REGISLEX_API void regislex_deadline_list_free(regislex_deadline_list_t* list) {
    if (!list) return;

    if (list->deadlines) {
        for (int i = 0; i < list->count; i++) {
            regislex_deadline_free(list->deadlines[i]);
        }
        platform_free(list->deadlines);
    }

    platform_free(list);
}

/* ============================================================================
 * Reminder Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_reminder_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* deadline_id,
    const regislex_reminder_t* reminder,
    regislex_reminder_t** out_reminder)
{
    if (!ctx || !deadline_id || !reminder || !out_reminder) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_reminder_t* new_reminder = (regislex_reminder_t*)platform_calloc(1, sizeof(regislex_reminder_t));
    if (!new_reminder) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_reminder, reminder, sizeof(regislex_reminder_t));
    regislex_uuid_generate(&new_reminder->id);
    memcpy(&new_reminder->deadline_id, deadline_id, sizeof(regislex_uuid_t));

    regislex_datetime_now(&new_reminder->created_at);

    regislex_db_context_t* db = NULL;

    const char* sql =
        "INSERT INTO reminders ("
        "  id, deadline_id, user_id, type, minutes_before, is_sent,"
        "  send_at, sent_at, message, is_active, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_reminder_free(new_reminder);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_reminder->id);
    regislex_db_bind_uuid(stmt, idx++, &new_reminder->deadline_id);
    regislex_db_bind_uuid(stmt, idx++, &new_reminder->user_id);
    regislex_db_bind_int(stmt, idx++, new_reminder->type);
    regislex_db_bind_int(stmt, idx++, new_reminder->minutes_before);
    regislex_db_bind_int(stmt, idx++, new_reminder->is_sent ? 1 : 0);
    regislex_db_bind_datetime(stmt, idx++, &new_reminder->send_at);
    regislex_db_bind_datetime(stmt, idx++, &new_reminder->sent_at);
    regislex_db_bind_text(stmt, idx++, new_reminder->message);
    regislex_db_bind_int(stmt, idx++, new_reminder->is_active ? 1 : 0);
    regislex_db_bind_datetime(stmt, idx++, &new_reminder->created_at);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_reminder_free(new_reminder);
        return err;
    }

    *out_reminder = new_reminder;
    return REGISLEX_OK;
}

REGISLEX_API void regislex_reminder_free(regislex_reminder_t* reminder) {
    if (reminder) {
        platform_free(reminder);
    }
}
