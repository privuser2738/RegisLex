/**
 * @file task.c
 * @brief Workflow Task Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef enum {
    TASK_STATUS_PENDING,
    TASK_STATUS_IN_PROGRESS,
    TASK_STATUS_COMPLETED,
    TASK_STATUS_CANCELLED,
    TASK_STATUS_BLOCKED
} task_status_t;

typedef struct regislex_task {
    regislex_uuid_t id;
    regislex_uuid_t workflow_id;
    regislex_uuid_t case_id;
    regislex_uuid_t assigned_to;
    char name[256];
    char description[1024];
    task_status_t status;
    regislex_datetime_t due_date;
    regislex_datetime_t completed_at;
    int priority;
    int sequence;
} regislex_task_t;

regislex_error_t regislex_task_create(const char* name, regislex_task_t** task) {
    if (!name || !task) return REGISLEX_ERROR_INVALID_ARGUMENT;

    *task = (regislex_task_t*)platform_calloc(1, sizeof(regislex_task_t));
    if (!*task) return REGISLEX_ERROR_OUT_OF_MEMORY;

    regislex_uuid_generate(&(*task)->id);
    strncpy((*task)->name, name, sizeof((*task)->name) - 1);
    (*task)->status = TASK_STATUS_PENDING;
    (*task)->priority = 5;

    return REGISLEX_OK;
}

void regislex_task_free(regislex_task_t* task) {
    platform_free(task);
}

regislex_error_t regislex_task_assign(regislex_task_t* task, const char* user_id) {
    if (!task || !user_id) return REGISLEX_ERROR_INVALID_ARGUMENT;
    return regislex_uuid_parse(user_id, &task->assigned_to);
}

regislex_error_t regislex_task_complete(regislex_task_t* task) {
    if (!task) return REGISLEX_ERROR_INVALID_ARGUMENT;
    task->status = TASK_STATUS_COMPLETED;
    regislex_datetime_now(&task->completed_at);
    return REGISLEX_OK;
}

regislex_error_t regislex_task_cancel(regislex_task_t* task) {
    if (!task) return REGISLEX_ERROR_INVALID_ARGUMENT;
    task->status = TASK_STATUS_CANCELLED;
    return REGISLEX_OK;
}

bool regislex_task_is_overdue(const regislex_task_t* task) {
    if (!task || task->status == TASK_STATUS_COMPLETED || task->status == TASK_STATUS_CANCELLED) {
        return false;
    }
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    return regislex_datetime_compare(&now, &task->due_date) > 0;
}
