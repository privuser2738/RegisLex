/**
 * @file workflow_engine.c
 * @brief Workflow Automation Engine Implementation
 *
 * Provides functionality for automating repetitive tasks,
 * document generation, intake processes, and status updates.
 */

#include "regislex/regislex.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

/* Workflow execution context */
typedef struct {
    regislex_context_t* ctx;
    regislex_workflow_t* workflow;
    regislex_workflow_run_t* run;
    char* trigger_data;
    int current_action_index;
    bool cancelled;
} workflow_exec_ctx_t;

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static regislex_workflow_t* workflow_alloc(void) {
    return (regislex_workflow_t*)platform_calloc(1, sizeof(regislex_workflow_t));
}

static regislex_task_t* task_alloc(void) {
    return (regislex_task_t*)platform_calloc(1, sizeof(regislex_task_t));
}

static regislex_workflow_run_t* workflow_run_alloc(void) {
    return (regislex_workflow_run_t*)platform_calloc(1, sizeof(regislex_workflow_run_t));
}

static regislex_error_t workflow_from_row(regislex_db_stmt_t* stmt, regislex_workflow_t* wf) {
    if (!stmt || !wf) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int col = 0;

    regislex_db_column_uuid(stmt, col++, &wf->id);

    const char* name = regislex_db_column_text(stmt, col++);
    if (name) strncpy(wf->name, name, sizeof(wf->name) - 1);

    const char* desc = regislex_db_column_text(stmt, col++);
    if (desc) strncpy(wf->description, desc, sizeof(wf->description) - 1);

    const char* category = regislex_db_column_text(stmt, col++);
    if (category) strncpy(wf->category, category, sizeof(wf->category) - 1);

    wf->status = (regislex_workflow_status_t)regislex_db_column_int(stmt, col++);
    wf->version = (int)regislex_db_column_int(stmt, col++);
    wf->run_once = regislex_db_column_int(stmt, col++) != 0;
    wf->allow_parallel = regislex_db_column_int(stmt, col++) != 0;
    wf->timeout_minutes = (int)regislex_db_column_int(stmt, col++);

    regislex_db_column_datetime(stmt, col++, &wf->created_at);
    regislex_db_column_datetime(stmt, col++, &wf->updated_at);
    regislex_db_column_uuid(stmt, col++, &wf->created_by);

    return REGISLEX_OK;
}

static regislex_error_t task_from_row(regislex_db_stmt_t* stmt, regislex_task_t* task) {
    if (!stmt || !task) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int col = 0;

    regislex_db_column_uuid(stmt, col++, &task->id);
    regislex_db_column_uuid(stmt, col++, &task->case_id);
    regislex_db_column_uuid(stmt, col++, &task->matter_id);
    regislex_db_column_uuid(stmt, col++, &task->workflow_run_id);
    regislex_db_column_uuid(stmt, col++, &task->parent_task_id);

    const char* title = regislex_db_column_text(stmt, col++);
    if (title) strncpy(task->title, title, sizeof(task->title) - 1);

    const char* desc = regislex_db_column_text(stmt, col++);
    if (desc) strncpy(task->description, desc, sizeof(task->description) - 1);

    task->status = (regislex_task_status_t)regislex_db_column_int(stmt, col++);
    task->priority = (regislex_priority_t)regislex_db_column_int(stmt, col++);

    regislex_db_column_uuid(stmt, col++, &task->assigned_to_id);
    regislex_db_column_uuid(stmt, col++, &task->assigned_by);

    regislex_db_column_datetime(stmt, col++, &task->due_date);

    task->estimated_minutes = (int)regislex_db_column_int(stmt, col++);
    task->actual_minutes = (int)regislex_db_column_int(stmt, col++);
    task->percent_complete = (int)regislex_db_column_int(stmt, col++);

    const char* notes = regislex_db_column_text(stmt, col++);
    if (notes) strncpy(task->completion_notes, notes, sizeof(task->completion_notes) - 1);

    task->requires_approval = regislex_db_column_int(stmt, col++) != 0;
    regislex_db_column_uuid(stmt, col++, &task->approver_id);
    regislex_db_column_datetime(stmt, col++, &task->approved_at);

    regislex_db_column_datetime(stmt, col++, &task->started_at);
    regislex_db_column_datetime(stmt, col++, &task->completed_at);
    regislex_db_column_datetime(stmt, col++, &task->created_at);
    regislex_db_column_datetime(stmt, col++, &task->updated_at);
    regislex_db_column_uuid(stmt, col++, &task->created_by);

    return REGISLEX_OK;
}

/* Execute a single workflow action */
static regislex_error_t execute_action(
    workflow_exec_ctx_t* exec_ctx,
    regislex_action_t* action)
{
    if (!exec_ctx || !action) return REGISLEX_ERROR_INVALID_ARGUMENT;

    regislex_error_t err = REGISLEX_OK;

    /* Log action start */
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry),
             "Executing action: %s (type: %d)\n", action->name, action->type);

    /* Append to execution log */
    if (exec_ctx->run) {
        strncat(exec_ctx->run->execution_log, log_entry,
                sizeof(exec_ctx->run->execution_log) - strlen(exec_ctx->run->execution_log) - 1);
    }

    switch (action->type) {
        case REGISLEX_ACTION_SEND_EMAIL:
            /* TODO: Implement email sending */
            /* Would integrate with SMTP or email service API */
            break;

        case REGISLEX_ACTION_SEND_SMS:
            /* TODO: Implement SMS sending */
            /* Would integrate with SMS gateway API */
            break;

        case REGISLEX_ACTION_CREATE_TASK: {
            /* Create a task from action parameters */
            regislex_task_t task = {0};

            /* Parse parameters to populate task */
            for (int i = 0; i < action->param_count; i++) {
                if (strcmp(action->params[i].type, "title") == 0) {
                    strncpy(task.title, action->params[i].value, sizeof(task.title) - 1);
                } else if (strcmp(action->params[i].type, "description") == 0) {
                    strncpy(task.description, action->params[i].value, sizeof(task.description) - 1);
                } else if (strcmp(action->params[i].type, "priority") == 0) {
                    task.priority = (regislex_priority_t)atoi(action->params[i].value);
                }
            }

            task.status = REGISLEX_TASK_PENDING;
            if (exec_ctx->run) {
                memcpy(&task.workflow_run_id, &exec_ctx->run->id, sizeof(regislex_uuid_t));
            }

            regislex_task_t* new_task = NULL;
            err = regislex_task_create(exec_ctx->ctx, &task, &new_task);
            if (new_task) regislex_task_free(new_task);
            break;
        }

        case REGISLEX_ACTION_CREATE_DEADLINE: {
            /* Create a deadline from action parameters */
            regislex_deadline_t dl = {0};

            for (int i = 0; i < action->param_count; i++) {
                if (strcmp(action->params[i].type, "title") == 0) {
                    strncpy(dl.title, action->params[i].value, sizeof(dl.title) - 1);
                } else if (strcmp(action->params[i].type, "days_from_now") == 0) {
                    int days = atoi(action->params[i].value);
                    regislex_datetime_now(&dl.due_date);
                    dl.due_date.day += days;
                }
            }

            dl.status = REGISLEX_STATUS_PENDING;

            regislex_deadline_t* new_dl = NULL;
            err = regislex_deadline_create(exec_ctx->ctx, &dl, &new_dl);
            if (new_dl) regislex_deadline_free(new_dl);
            break;
        }

        case REGISLEX_ACTION_UPDATE_STATUS:
            /* Update case/matter status */
            /* TODO: Implement status update */
            break;

        case REGISLEX_ACTION_ASSIGN_USER:
            /* Assign case/task to user */
            /* TODO: Implement user assignment */
            break;

        case REGISLEX_ACTION_ADD_NOTE:
            /* Add note to case/matter */
            /* TODO: Implement note addition */
            break;

        case REGISLEX_ACTION_WEBHOOK: {
            /* Call external webhook */
            /* TODO: Implement HTTP POST to webhook URL */
            break;
        }

        case REGISLEX_ACTION_DELAY:
            /* Wait for specified time */
            if (action->delay_minutes > 0) {
                platform_sleep_ms(action->delay_minutes * 60 * 1000);
            }
            break;

        case REGISLEX_ACTION_CONDITION:
            /* Evaluate condition and branch */
            /* TODO: Implement condition evaluation */
            break;

        case REGISLEX_ACTION_APPROVAL:
            /* Wait for approval */
            /* TODO: Implement approval workflow */
            break;

        case REGISLEX_ACTION_NOTIFY:
            /* Send in-app notification */
            /* TODO: Implement notification system */
            break;

        case REGISLEX_ACTION_GENERATE_REPORT:
            /* Generate a report */
            /* TODO: Integrate with reporting module */
            break;

        case REGISLEX_ACTION_CREATE_DOCUMENT:
            /* Generate document from template */
            /* TODO: Integrate with document management module */
            break;

        case REGISLEX_ACTION_CUSTOM_SCRIPT:
            /* Execute custom script/plugin */
            /* TODO: Implement plugin system */
            break;

        default:
            err = REGISLEX_ERROR_NOT_SUPPORTED;
            break;
    }

    return err;
}

/* ============================================================================
 * Workflow Management Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_workflow_create(
    regislex_context_t* ctx,
    const regislex_workflow_t* workflow,
    regislex_workflow_t** out_workflow)
{
    if (!ctx || !workflow || !out_workflow) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_workflow_t* new_wf = workflow_alloc();
    if (!new_wf) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_wf, workflow, sizeof(regislex_workflow_t));

    if (new_wf->id.value[0] == '\0') {
        regislex_uuid_generate(&new_wf->id);
    }

    new_wf->version = 1;
    new_wf->status = REGISLEX_WORKFLOW_DRAFT;

    regislex_datetime_now(&new_wf->created_at);
    memcpy(&new_wf->updated_at, &new_wf->created_at, sizeof(regislex_datetime_t));

    regislex_db_context_t* db = NULL;

    const char* sql =
        "INSERT INTO workflows ("
        "  id, name, description, category, status, version,"
        "  run_once, allow_parallel, timeout_minutes,"
        "  created_at, updated_at, created_by"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_workflow_free(new_wf);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_wf->id);
    regislex_db_bind_text(stmt, idx++, new_wf->name);
    regislex_db_bind_text(stmt, idx++, new_wf->description);
    regislex_db_bind_text(stmt, idx++, new_wf->category);
    regislex_db_bind_int(stmt, idx++, new_wf->status);
    regislex_db_bind_int(stmt, idx++, new_wf->version);
    regislex_db_bind_int(stmt, idx++, new_wf->run_once ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, new_wf->allow_parallel ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, new_wf->timeout_minutes);
    regislex_db_bind_datetime(stmt, idx++, &new_wf->created_at);
    regislex_db_bind_datetime(stmt, idx++, &new_wf->updated_at);
    regislex_db_bind_uuid(stmt, idx++, &new_wf->created_by);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_workflow_free(new_wf);
        return err;
    }

    *out_workflow = new_wf;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_workflow_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_workflow_t** out_workflow)
{
    if (!ctx || !id || !out_workflow) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "SELECT id, name, description, category, status, version,"
        "  run_once, allow_parallel, timeout_minutes,"
        "  created_at, updated_at, created_by "
        "FROM workflows WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_workflow_t* wf = workflow_alloc();
    if (!wf) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    workflow_from_row(stmt, wf);
    regislex_db_finalize(stmt);

    /* TODO: Load triggers and actions */

    *out_workflow = wf;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_workflow_update(
    regislex_context_t* ctx,
    const regislex_workflow_t* workflow)
{
    if (!ctx || !workflow) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE workflows SET "
        "  name = ?, description = ?, category = ?, status = ?,"
        "  run_once = ?, allow_parallel = ?, timeout_minutes = ?,"
        "  updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    int idx = 1;
    regislex_db_bind_text(stmt, idx++, workflow->name);
    regislex_db_bind_text(stmt, idx++, workflow->description);
    regislex_db_bind_text(stmt, idx++, workflow->category);
    regislex_db_bind_int(stmt, idx++, workflow->status);
    regislex_db_bind_int(stmt, idx++, workflow->run_once ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, workflow->allow_parallel ? 1 : 0);
    regislex_db_bind_int(stmt, idx++, workflow->timeout_minutes);
    regislex_db_bind_datetime(stmt, idx++, &now);
    regislex_db_bind_uuid(stmt, idx++, &workflow->id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_workflow_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    /* Delete associated triggers and actions first (cascade) */
    /* The database schema should handle this with ON DELETE CASCADE */

    const char* sql = "DELETE FROM workflows WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_workflow_list(
    regislex_context_t* ctx,
    const char* category,
    regislex_workflow_t*** workflows,
    int* count)
{
    if (!ctx || !workflows || !count) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    char sql[1024];
    if (category) {
        snprintf(sql, sizeof(sql),
            "SELECT id, name, description, category, status, version,"
            "  run_once, allow_parallel, timeout_minutes,"
            "  created_at, updated_at, created_by "
            "FROM workflows WHERE category = ? ORDER BY name");
    } else {
        strcpy(sql,
            "SELECT id, name, description, category, status, version,"
            "  run_once, allow_parallel, timeout_minutes,"
            "  created_at, updated_at, created_by "
            "FROM workflows ORDER BY name");
    }

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    if (category) {
        regislex_db_bind_text(stmt, 1, category);
    }

    int capacity = 50;
    regislex_workflow_t** list = (regislex_workflow_t**)platform_calloc(capacity, sizeof(regislex_workflow_t*));
    if (!list) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    int n = 0;
    while ((err = regislex_db_step(stmt)) == REGISLEX_OK) {
        if (n >= capacity) {
            capacity *= 2;
            regislex_workflow_t** new_list = (regislex_workflow_t**)platform_realloc(
                list, capacity * sizeof(regislex_workflow_t*));
            if (!new_list) {
                for (int i = 0; i < n; i++) regislex_workflow_free(list[i]);
                platform_free(list);
                regislex_db_finalize(stmt);
                return REGISLEX_ERROR_OUT_OF_MEMORY;
            }
            list = new_list;
        }

        regislex_workflow_t* wf = workflow_alloc();
        if (!wf) {
            for (int i = 0; i < n; i++) regislex_workflow_free(list[i]);
            platform_free(list);
            regislex_db_finalize(stmt);
            return REGISLEX_ERROR_OUT_OF_MEMORY;
        }

        workflow_from_row(stmt, wf);
        list[n++] = wf;
    }

    regislex_db_finalize(stmt);

    *workflows = list;
    *count = n;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_workflow_activate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "UPDATE workflows SET status = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, REGISLEX_WORKFLOW_ACTIVE);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_uuid(stmt, 3, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_workflow_pause(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "UPDATE workflows SET status = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, REGISLEX_WORKFLOW_PAUSED);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_uuid(stmt, 3, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_workflow_execute(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_uuid_t* case_id,
    const char* trigger_data,
    regislex_workflow_run_t** out_run)
{
    if (!ctx || !workflow_id || !out_run) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Get workflow */
    regislex_workflow_t* workflow = NULL;
    regislex_error_t err = regislex_workflow_get(ctx, workflow_id, &workflow);
    if (err != REGISLEX_OK) return err;

    /* Check if workflow is active */
    if (workflow->status != REGISLEX_WORKFLOW_ACTIVE) {
        regislex_workflow_free(workflow);
        return REGISLEX_ERROR_INVALID_STATE;
    }

    /* Create workflow run */
    regislex_workflow_run_t* run = workflow_run_alloc();
    if (!run) {
        regislex_workflow_free(workflow);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    regislex_uuid_generate(&run->id);
    memcpy(&run->workflow_id, workflow_id, sizeof(regislex_uuid_t));
    if (case_id) {
        memcpy(&run->case_id, case_id, sizeof(regislex_uuid_t));
    }
    if (trigger_data) {
        strncpy(run->trigger_data, trigger_data, sizeof(run->trigger_data) - 1);
    }

    run->status = REGISLEX_WORKFLOW_ACTIVE;
    run->current_step = 0;
    regislex_datetime_now(&run->started_at);
    regislex_datetime_now(&run->created_at);

    /* Create execution context */
    workflow_exec_ctx_t exec_ctx = {0};
    exec_ctx.ctx = ctx;
    exec_ctx.workflow = workflow;
    exec_ctx.run = run;
    exec_ctx.trigger_data = trigger_data ? platform_strdup(trigger_data) : NULL;

    /* Execute actions in sequence */
    for (int i = 0; i < workflow->action_count && !exec_ctx.cancelled; i++) {
        regislex_action_t* action = workflow->actions[i];
        run->current_step = i + 1;

        err = execute_action(&exec_ctx, action);
        if (err != REGISLEX_OK) {
            run->status = REGISLEX_WORKFLOW_FAILED;
            snprintf(run->error_message, sizeof(run->error_message),
                    "Action %d (%s) failed with error %d", i, action->name, err);
            break;
        }
    }

    /* Mark completion */
    if (run->status == REGISLEX_WORKFLOW_ACTIVE) {
        run->status = REGISLEX_WORKFLOW_COMPLETED;
    }
    regislex_datetime_now(&run->completed_at);

    /* Cleanup */
    if (exec_ctx.trigger_data) {
        platform_free(exec_ctx.trigger_data);
    }
    regislex_workflow_free(workflow);

    *out_run = run;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_workflow_run_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* run_id,
    regislex_workflow_run_t** out_run)
{
    if (!ctx || !run_id || !out_run) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Load from database */
    return REGISLEX_ERROR_NOT_FOUND;
}

REGISLEX_API regislex_error_t regislex_workflow_run_cancel(
    regislex_context_t* ctx,
    const regislex_uuid_t* run_id)
{
    if (!ctx || !run_id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Cancel running workflow */
    return REGISLEX_OK;
}

REGISLEX_API void regislex_workflow_free(regislex_workflow_t* workflow) {
    if (!workflow) return;

    if (workflow->triggers) {
        for (int i = 0; i < workflow->trigger_count; i++) {
            regislex_trigger_free(workflow->triggers[i]);
        }
        platform_free(workflow->triggers);
    }

    if (workflow->actions) {
        for (int i = 0; i < workflow->action_count; i++) {
            regislex_action_free(workflow->actions[i]);
        }
        platform_free(workflow->actions);
    }

    platform_free(workflow);
}

REGISLEX_API void regislex_workflow_run_free(regislex_workflow_run_t* run) {
    if (run) {
        platform_free(run);
    }
}

/* ============================================================================
 * Trigger Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_trigger_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_trigger_t* trigger,
    regislex_trigger_t** out_trigger)
{
    if (!ctx || !workflow_id || !trigger || !out_trigger) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_trigger_t* new_trigger = (regislex_trigger_t*)platform_calloc(1, sizeof(regislex_trigger_t));
    if (!new_trigger) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_trigger, trigger, sizeof(regislex_trigger_t));
    regislex_uuid_generate(&new_trigger->id);
    memcpy(&new_trigger->workflow_id, workflow_id, sizeof(regislex_uuid_t));

    regislex_datetime_now(&new_trigger->created_at);
    memcpy(&new_trigger->updated_at, &new_trigger->created_at, sizeof(regislex_datetime_t));

    /* TODO: Insert into database */

    *out_trigger = new_trigger;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_trigger_update(
    regislex_context_t* ctx,
    const regislex_trigger_t* trigger)
{
    if (!ctx || !trigger) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Update in database */
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_trigger_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Delete from database */
    return REGISLEX_OK;
}

REGISLEX_API void regislex_trigger_free(regislex_trigger_t* trigger) {
    if (!trigger) return;

    if (trigger->conditions) {
        platform_free(trigger->conditions);
    }

    platform_free(trigger);
}

/* ============================================================================
 * Action Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_action_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_action_t* action,
    regislex_action_t** out_action)
{
    if (!ctx || !workflow_id || !action || !out_action) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_action_t* new_action = (regislex_action_t*)platform_calloc(1, sizeof(regislex_action_t));
    if (!new_action) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_action, action, sizeof(regislex_action_t));
    regislex_uuid_generate(&new_action->id);
    memcpy(&new_action->workflow_id, workflow_id, sizeof(regislex_uuid_t));

    regislex_datetime_now(&new_action->created_at);
    memcpy(&new_action->updated_at, &new_action->created_at, sizeof(regislex_datetime_t));

    /* TODO: Insert into database */

    *out_action = new_action;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_action_update(
    regislex_context_t* ctx,
    const regislex_action_t* action)
{
    if (!ctx || !action) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Update in database */
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_action_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* TODO: Delete from database */
    return REGISLEX_OK;
}

REGISLEX_API void regislex_action_free(regislex_action_t* action) {
    if (!action) return;

    if (action->params) {
        platform_free(action->params);
    }

    if (action->conditions) {
        platform_free(action->conditions);
    }

    platform_free(action);
}

/* ============================================================================
 * Task Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_task_create(
    regislex_context_t* ctx,
    const regislex_task_t* task,
    regislex_task_t** out_task)
{
    if (!ctx || !task || !out_task) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_task_t* new_task = task_alloc();
    if (!new_task) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_task, task, sizeof(regislex_task_t));

    if (new_task->id.value[0] == '\0') {
        regislex_uuid_generate(&new_task->id);
    }

    new_task->status = REGISLEX_TASK_PENDING;
    new_task->percent_complete = 0;

    regislex_datetime_now(&new_task->created_at);
    memcpy(&new_task->updated_at, &new_task->created_at, sizeof(regislex_datetime_t));

    regislex_db_context_t* db = NULL;

    const char* sql =
        "INSERT INTO tasks ("
        "  id, case_id, matter_id, workflow_run_id, parent_task_id,"
        "  title, description, status, priority,"
        "  assigned_to_id, assigned_by, due_date,"
        "  estimated_minutes, actual_minutes, percent_complete,"
        "  completion_notes, requires_approval, approver_id, approved_at,"
        "  started_at, completed_at, created_at, updated_at, created_by"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_task_free(new_task);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_task->id);
    regislex_db_bind_uuid(stmt, idx++, &new_task->case_id);
    regislex_db_bind_uuid(stmt, idx++, &new_task->matter_id);
    regislex_db_bind_uuid(stmt, idx++, &new_task->workflow_run_id);
    regislex_db_bind_uuid(stmt, idx++, &new_task->parent_task_id);
    regislex_db_bind_text(stmt, idx++, new_task->title);
    regislex_db_bind_text(stmt, idx++, new_task->description);
    regislex_db_bind_int(stmt, idx++, new_task->status);
    regislex_db_bind_int(stmt, idx++, new_task->priority);
    regislex_db_bind_uuid(stmt, idx++, &new_task->assigned_to_id);
    regislex_db_bind_uuid(stmt, idx++, &new_task->assigned_by);
    regislex_db_bind_datetime(stmt, idx++, &new_task->due_date);
    regislex_db_bind_int(stmt, idx++, new_task->estimated_minutes);
    regislex_db_bind_int(stmt, idx++, new_task->actual_minutes);
    regislex_db_bind_int(stmt, idx++, new_task->percent_complete);
    regislex_db_bind_text(stmt, idx++, new_task->completion_notes);
    regislex_db_bind_int(stmt, idx++, new_task->requires_approval ? 1 : 0);
    regislex_db_bind_uuid(stmt, idx++, &new_task->approver_id);
    regislex_db_bind_datetime(stmt, idx++, &new_task->approved_at);
    regislex_db_bind_datetime(stmt, idx++, &new_task->started_at);
    regislex_db_bind_datetime(stmt, idx++, &new_task->completed_at);
    regislex_db_bind_datetime(stmt, idx++, &new_task->created_at);
    regislex_db_bind_datetime(stmt, idx++, &new_task->updated_at);
    regislex_db_bind_uuid(stmt, idx++, &new_task->created_by);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_task_free(new_task);
        return err;
    }

    *out_task = new_task;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_task_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_task_t** out_task)
{
    if (!ctx || !id || !out_task) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "SELECT id, case_id, matter_id, workflow_run_id, parent_task_id,"
        "  title, description, status, priority,"
        "  assigned_to_id, assigned_by, due_date,"
        "  estimated_minutes, actual_minutes, percent_complete,"
        "  completion_notes, requires_approval, approver_id, approved_at,"
        "  started_at, completed_at, created_at, updated_at, created_by "
        "FROM tasks WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_task_t* task = task_alloc();
    if (!task) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    task_from_row(stmt, task);
    regislex_db_finalize(stmt);

    *out_task = task;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_task_update(
    regislex_context_t* ctx,
    const regislex_task_t* task)
{
    if (!ctx || !task) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE tasks SET "
        "  title = ?, description = ?, status = ?, priority = ?,"
        "  assigned_to_id = ?, due_date = ?,"
        "  estimated_minutes = ?, actual_minutes = ?, percent_complete = ?,"
        "  completion_notes = ?, updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    int idx = 1;
    regislex_db_bind_text(stmt, idx++, task->title);
    regislex_db_bind_text(stmt, idx++, task->description);
    regislex_db_bind_int(stmt, idx++, task->status);
    regislex_db_bind_int(stmt, idx++, task->priority);
    regislex_db_bind_uuid(stmt, idx++, &task->assigned_to_id);
    regislex_db_bind_datetime(stmt, idx++, &task->due_date);
    regislex_db_bind_int(stmt, idx++, task->estimated_minutes);
    regislex_db_bind_int(stmt, idx++, task->actual_minutes);
    regislex_db_bind_int(stmt, idx++, task->percent_complete);
    regislex_db_bind_text(stmt, idx++, task->completion_notes);
    regislex_db_bind_datetime(stmt, idx++, &now);
    regislex_db_bind_uuid(stmt, idx++, &task->id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_task_start(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id)
{
    if (!ctx || !task_id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE tasks SET status = ?, started_at = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, REGISLEX_TASK_IN_PROGRESS);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_datetime(stmt, 3, &now);
    regislex_db_bind_uuid(stmt, 4, task_id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_task_complete(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const char* notes,
    int actual_minutes)
{
    if (!ctx || !task_id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE tasks SET "
        "  status = ?, percent_complete = 100, completion_notes = ?,"
        "  actual_minutes = ?, completed_at = ?, updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, REGISLEX_TASK_COMPLETED);
    regislex_db_bind_text(stmt, 2, notes);
    regislex_db_bind_int(stmt, 3, actual_minutes);
    regislex_db_bind_datetime(stmt, 4, &now);
    regislex_db_bind_datetime(stmt, 5, &now);
    regislex_db_bind_uuid(stmt, 6, task_id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API regislex_error_t regislex_task_assign(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const regislex_uuid_t* user_id)
{
    if (!ctx || !task_id || !user_id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE tasks SET assigned_to_id = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) return err;

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_uuid(stmt, 1, user_id);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_uuid(stmt, 3, task_id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return (err == REGISLEX_ERROR_NOT_FOUND) ? REGISLEX_OK : err;
}

REGISLEX_API void regislex_task_free(regislex_task_t* task) {
    if (task) {
        platform_free(task);
    }
}

REGISLEX_API void regislex_task_list_free(regislex_task_list_t* list) {
    if (!list) return;

    if (list->tasks) {
        for (int i = 0; i < list->count; i++) {
            regislex_task_free(list->tasks[i]);
        }
        platform_free(list->tasks);
    }

    platform_free(list);
}
