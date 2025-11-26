/**
 * @file case.c
 * @brief Case Management Implementation
 *
 * Provides functionality for managing legal cases, matters, and parties.
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

static regislex_case_t* case_alloc(void) {
    regislex_case_t* c = (regislex_case_t*)platform_calloc(1, sizeof(regislex_case_t));
    return c;
}

static regislex_error_t case_from_row(regislex_db_stmt_t* stmt, regislex_case_t* case_out) {
    if (!stmt || !case_out) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int col = 0;

    regislex_db_column_uuid(stmt, col++, &case_out->id);

    const char* case_number = regislex_db_column_text(stmt, col++);
    if (case_number) strncpy(case_out->case_number, case_number, sizeof(case_out->case_number) - 1);

    const char* title = regislex_db_column_text(stmt, col++);
    if (title) strncpy(case_out->title, title, sizeof(case_out->title) - 1);

    const char* short_title = regislex_db_column_text(stmt, col++);
    if (short_title) strncpy(case_out->short_title, short_title, sizeof(case_out->short_title) - 1);

    const char* description = regislex_db_column_text(stmt, col++);
    if (description) strncpy(case_out->description, description, sizeof(case_out->description) - 1);

    case_out->type = (regislex_case_type_t)regislex_db_column_int(stmt, col++);
    case_out->status = (regislex_status_t)regislex_db_column_int(stmt, col++);
    case_out->priority = (regislex_priority_t)regislex_db_column_int(stmt, col++);
    case_out->outcome = (regislex_case_outcome_t)regislex_db_column_int(stmt, col++);

    const char* court_name = regislex_db_column_text(stmt, col++);
    if (court_name) strncpy(case_out->court.name, court_name, sizeof(case_out->court.name) - 1);

    const char* court_division = regislex_db_column_text(stmt, col++);
    if (court_division) strncpy(case_out->court.division, court_division, sizeof(case_out->court.division) - 1);

    const char* docket_number = regislex_db_column_text(stmt, col++);
    if (docket_number) strncpy(case_out->docket_number, docket_number, sizeof(case_out->docket_number) - 1);

    const char* internal_ref = regislex_db_column_text(stmt, col++);
    if (internal_ref) strncpy(case_out->internal_reference, internal_ref, sizeof(case_out->internal_reference) - 1);

    const char* client_ref = regislex_db_column_text(stmt, col++);
    if (client_ref) strncpy(case_out->client_reference, client_ref, sizeof(case_out->client_reference) - 1);

    case_out->estimated_value.amount = regislex_db_column_int(stmt, col++);
    strcpy(case_out->estimated_value.currency, "USD");

    case_out->settlement_amount.amount = regislex_db_column_int(stmt, col++);
    strcpy(case_out->settlement_amount.currency, "USD");

    regislex_db_column_datetime(stmt, col++, &case_out->filed_date);
    regislex_db_column_datetime(stmt, col++, &case_out->trial_date);
    regislex_db_column_datetime(stmt, col++, &case_out->closed_date);
    regislex_db_column_datetime(stmt, col++, &case_out->statute_of_limitations);

    regislex_db_column_uuid(stmt, col++, &case_out->lead_attorney_id);
    regislex_db_column_uuid(stmt, col++, &case_out->assigned_to_id);
    regislex_db_column_uuid(stmt, col++, &case_out->parent_case_id);

    const char* tags = regislex_db_column_text(stmt, col++);
    if (tags) strncpy(case_out->tags, tags, sizeof(case_out->tags) - 1);

    regislex_db_column_datetime(stmt, col++, &case_out->created_at);
    regislex_db_column_datetime(stmt, col++, &case_out->updated_at);
    regislex_db_column_uuid(stmt, col++, &case_out->created_by);
    regislex_db_column_uuid(stmt, col++, &case_out->updated_by);

    return REGISLEX_OK;
}

/* ============================================================================
 * Case Management Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_case_create(
    regislex_context_t* ctx,
    const regislex_case_t* case_data,
    regislex_case_t** out_case)
{
    if (!ctx || !case_data || !out_case) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_case_t* new_case = case_alloc();
    if (!new_case) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    /* Copy input data */
    memcpy(new_case, case_data, sizeof(regislex_case_t));

    /* Generate UUID if not provided */
    if (new_case->id.value[0] == '\0') {
        regislex_uuid_generate(&new_case->id);
    }

    /* Set timestamps */
    regislex_datetime_now(&new_case->created_at);
    memcpy(&new_case->updated_at, &new_case->created_at, sizeof(regislex_datetime_t));

    /* Get database context */
    regislex_db_context_t* db = NULL;
    /* Note: In real implementation, we'd get db from ctx internal structure */

    const char* sql =
        "INSERT INTO cases ("
        "  id, case_number, title, short_title, description, type, status, priority, outcome,"
        "  court_name, court_division, docket_number, internal_reference, client_reference,"
        "  estimated_value, settlement_amount, filed_date, trial_date, closed_date,"
        "  statute_of_limitations, lead_attorney_id, assigned_to_id, parent_case_id,"
        "  tags, created_at, updated_at, created_by, updated_by"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_case_free(new_case);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_case->id);
    regislex_db_bind_text(stmt, idx++, new_case->case_number);
    regislex_db_bind_text(stmt, idx++, new_case->title);
    regislex_db_bind_text(stmt, idx++, new_case->short_title);
    regislex_db_bind_text(stmt, idx++, new_case->description);
    regislex_db_bind_int(stmt, idx++, new_case->type);
    regislex_db_bind_int(stmt, idx++, new_case->status);
    regislex_db_bind_int(stmt, idx++, new_case->priority);
    regislex_db_bind_int(stmt, idx++, new_case->outcome);
    regislex_db_bind_text(stmt, idx++, new_case->court.name);
    regislex_db_bind_text(stmt, idx++, new_case->court.division);
    regislex_db_bind_text(stmt, idx++, new_case->docket_number);
    regislex_db_bind_text(stmt, idx++, new_case->internal_reference);
    regislex_db_bind_text(stmt, idx++, new_case->client_reference);
    regislex_db_bind_money(stmt, idx++, &new_case->estimated_value);
    regislex_db_bind_money(stmt, idx++, &new_case->settlement_amount);
    regislex_db_bind_datetime(stmt, idx++, &new_case->filed_date);
    regislex_db_bind_datetime(stmt, idx++, &new_case->trial_date);
    regislex_db_bind_datetime(stmt, idx++, &new_case->closed_date);
    regislex_db_bind_datetime(stmt, idx++, &new_case->statute_of_limitations);
    regislex_db_bind_uuid(stmt, idx++, &new_case->lead_attorney_id);
    regislex_db_bind_uuid(stmt, idx++, &new_case->assigned_to_id);
    regislex_db_bind_uuid(stmt, idx++, &new_case->parent_case_id);
    regislex_db_bind_text(stmt, idx++, new_case->tags);
    regislex_db_bind_datetime(stmt, idx++, &new_case->created_at);
    regislex_db_bind_datetime(stmt, idx++, &new_case->updated_at);
    regislex_db_bind_uuid(stmt, idx++, &new_case->created_by);
    regislex_db_bind_uuid(stmt, idx++, &new_case->updated_by);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_case_free(new_case);
        return err;
    }

    *out_case = new_case;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_case_t** out_case)
{
    if (!ctx || !id || !out_case) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;
    /* Note: In real implementation, we'd get db from ctx internal structure */

    const char* sql =
        "SELECT id, case_number, title, short_title, description, type, status, priority, outcome,"
        "  court_name, court_division, docket_number, internal_reference, client_reference,"
        "  estimated_value, settlement_amount, filed_date, trial_date, closed_date,"
        "  statute_of_limitations, lead_attorney_id, assigned_to_id, parent_case_id,"
        "  tags, created_at, updated_at, created_by, updated_by "
        "FROM cases WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    } else if (err != REGISLEX_OK) {
        regislex_db_finalize(stmt);
        return err;
    }

    regislex_case_t* case_out = case_alloc();
    if (!case_out) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    case_from_row(stmt, case_out);
    regislex_db_finalize(stmt);

    *out_case = case_out;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_get_by_number(
    regislex_context_t* ctx,
    const char* case_number,
    regislex_case_t** out_case)
{
    if (!ctx || !case_number || !out_case) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "SELECT id, case_number, title, short_title, description, type, status, priority, outcome,"
        "  court_name, court_division, docket_number, internal_reference, client_reference,"
        "  estimated_value, settlement_amount, filed_date, trial_date, closed_date,"
        "  statute_of_limitations, lead_attorney_id, assigned_to_id, parent_case_id,"
        "  tags, created_at, updated_at, created_by, updated_by "
        "FROM cases WHERE case_number = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_db_bind_text(stmt, 1, case_number);

    err = regislex_db_step(stmt);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    } else if (err != REGISLEX_OK) {
        regislex_db_finalize(stmt);
        return err;
    }

    regislex_case_t* case_out = case_alloc();
    if (!case_out) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    case_from_row(stmt, case_out);
    regislex_db_finalize(stmt);

    *out_case = case_out;
    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_update(
    regislex_context_t* ctx,
    const regislex_case_t* case_data)
{
    if (!ctx || !case_data) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql =
        "UPDATE cases SET "
        "  case_number = ?, title = ?, short_title = ?, description = ?,"
        "  type = ?, status = ?, priority = ?, outcome = ?,"
        "  court_name = ?, court_division = ?, docket_number = ?,"
        "  internal_reference = ?, client_reference = ?,"
        "  estimated_value = ?, settlement_amount = ?,"
        "  filed_date = ?, trial_date = ?, closed_date = ?,"
        "  statute_of_limitations = ?, lead_attorney_id = ?,"
        "  assigned_to_id = ?, parent_case_id = ?, tags = ?,"
        "  updated_at = ?, updated_by = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    int idx = 1;
    regislex_db_bind_text(stmt, idx++, case_data->case_number);
    regislex_db_bind_text(stmt, idx++, case_data->title);
    regislex_db_bind_text(stmt, idx++, case_data->short_title);
    regislex_db_bind_text(stmt, idx++, case_data->description);
    regislex_db_bind_int(stmt, idx++, case_data->type);
    regislex_db_bind_int(stmt, idx++, case_data->status);
    regislex_db_bind_int(stmt, idx++, case_data->priority);
    regislex_db_bind_int(stmt, idx++, case_data->outcome);
    regislex_db_bind_text(stmt, idx++, case_data->court.name);
    regislex_db_bind_text(stmt, idx++, case_data->court.division);
    regislex_db_bind_text(stmt, idx++, case_data->docket_number);
    regislex_db_bind_text(stmt, idx++, case_data->internal_reference);
    regislex_db_bind_text(stmt, idx++, case_data->client_reference);
    regislex_db_bind_money(stmt, idx++, &case_data->estimated_value);
    regislex_db_bind_money(stmt, idx++, &case_data->settlement_amount);
    regislex_db_bind_datetime(stmt, idx++, &case_data->filed_date);
    regislex_db_bind_datetime(stmt, idx++, &case_data->trial_date);
    regislex_db_bind_datetime(stmt, idx++, &case_data->closed_date);
    regislex_db_bind_datetime(stmt, idx++, &case_data->statute_of_limitations);
    regislex_db_bind_uuid(stmt, idx++, &case_data->lead_attorney_id);
    regislex_db_bind_uuid(stmt, idx++, &case_data->assigned_to_id);
    regislex_db_bind_uuid(stmt, idx++, &case_data->parent_case_id);
    regislex_db_bind_text(stmt, idx++, case_data->tags);
    regislex_db_bind_datetime(stmt, idx++, &now);
    regislex_db_bind_uuid(stmt, idx++, &case_data->updated_by);
    regislex_db_bind_uuid(stmt, idx++, &case_data->id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        return err;
    }

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "DELETE FROM cases WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_db_bind_uuid(stmt, 1, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        return err;
    }

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_list(
    regislex_context_t* ctx,
    const regislex_case_filter_t* filter,
    regislex_case_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    /* Build query with filters */
    char sql[4096];
    char where_clause[2048] = "";
    char order_clause[256] = "ORDER BY created_at DESC";
    int param_idx = 1;

    strcpy(sql,
        "SELECT id, case_number, title, short_title, description, type, status, priority, outcome,"
        "  court_name, court_division, docket_number, internal_reference, client_reference,"
        "  estimated_value, settlement_amount, filed_date, trial_date, closed_date,"
        "  statute_of_limitations, lead_attorney_id, assigned_to_id, parent_case_id,"
        "  tags, created_at, updated_at, created_by, updated_by "
        "FROM cases");

    /* Apply filters */
    if (filter) {
        int first_filter = 1;

        if (filter->case_number) {
            strcat(where_clause, first_filter ? " WHERE " : " AND ");
            strcat(where_clause, "case_number = ?");
            first_filter = 0;
        }

        if (filter->title_contains) {
            strcat(where_clause, first_filter ? " WHERE " : " AND ");
            strcat(where_clause, "title LIKE ?");
            first_filter = 0;
        }

        if (filter->status) {
            strcat(where_clause, first_filter ? " WHERE " : " AND ");
            strcat(where_clause, "status = ?");
            first_filter = 0;
        }

        if (filter->type) {
            strcat(where_clause, first_filter ? " WHERE " : " AND ");
            strcat(where_clause, "type = ?");
            first_filter = 0;
        }

        if (filter->assigned_to_id) {
            strcat(where_clause, first_filter ? " WHERE " : " AND ");
            strcat(where_clause, "assigned_to_id = ?");
            first_filter = 0;
        }

        if (filter->order_by) {
            snprintf(order_clause, sizeof(order_clause), "ORDER BY %s %s",
                    filter->order_by, filter->order_desc ? "DESC" : "ASC");
        }
    }

    strcat(sql, where_clause);
    strcat(sql, " ");
    strcat(sql, order_clause);

    /* Add pagination */
    int limit = (filter && filter->limit > 0) ? filter->limit : 100;
    int offset = (filter && filter->offset > 0) ? filter->offset : 0;
    char pagination[64];
    snprintf(pagination, sizeof(pagination), " LIMIT %d OFFSET %d", limit, offset);
    strcat(sql, pagination);

    /* Execute query */
    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    /* Bind filter parameters */
    if (filter) {
        if (filter->case_number) {
            regislex_db_bind_text(stmt, param_idx++, filter->case_number);
        }
        if (filter->title_contains) {
            char like_pattern[512];
            snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", filter->title_contains);
            regislex_db_bind_text(stmt, param_idx++, like_pattern);
        }
        if (filter->status) {
            regislex_db_bind_int(stmt, param_idx++, *filter->status);
        }
        if (filter->type) {
            regislex_db_bind_int(stmt, param_idx++, *filter->type);
        }
        if (filter->assigned_to_id) {
            regislex_db_bind_uuid(stmt, param_idx++, filter->assigned_to_id);
        }
    }

    /* Allocate result list */
    regislex_case_list_t* list = (regislex_case_list_t*)platform_calloc(1, sizeof(regislex_case_list_t));
    if (!list) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    /* Collect results */
    int capacity = 100;
    list->cases = (regislex_case_t**)platform_calloc(capacity, sizeof(regislex_case_t*));
    if (!list->cases) {
        platform_free(list);
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    while ((err = regislex_db_step(stmt)) == REGISLEX_OK) {
        if (list->count >= capacity) {
            capacity *= 2;
            regislex_case_t** new_cases = (regislex_case_t**)platform_realloc(
                list->cases, capacity * sizeof(regislex_case_t*));
            if (!new_cases) {
                regislex_case_list_free(list);
                regislex_db_finalize(stmt);
                return REGISLEX_ERROR_OUT_OF_MEMORY;
            }
            list->cases = new_cases;
        }

        regislex_case_t* case_item = case_alloc();
        if (!case_item) {
            regislex_case_list_free(list);
            regislex_db_finalize(stmt);
            return REGISLEX_ERROR_OUT_OF_MEMORY;
        }

        case_from_row(stmt, case_item);
        list->cases[list->count++] = case_item;
    }

    regislex_db_finalize(stmt);

    list->offset = offset;
    list->limit = limit;

    *out_list = list;
    return REGISLEX_OK;
}

REGISLEX_API void regislex_case_free(regislex_case_t* case_ptr) {
    if (!case_ptr) return;

    /* Free parties if allocated */
    if (case_ptr->parties) {
        for (int i = 0; i < case_ptr->party_count; i++) {
            regislex_party_free(case_ptr->parties[i]);
        }
        platform_free(case_ptr->parties);
    }

    /* Free metadata if allocated */
    if (case_ptr->metadata) {
        for (int i = 0; i < case_ptr->metadata_count; i++) {
            platform_free(case_ptr->metadata[i].key);
            platform_free(case_ptr->metadata[i].value);
        }
        platform_free(case_ptr->metadata);
    }

    platform_free(case_ptr);
}

REGISLEX_API void regislex_case_list_free(regislex_case_list_t* list) {
    if (!list) return;

    if (list->cases) {
        for (int i = 0; i < list->count; i++) {
            regislex_case_free(list->cases[i]);
        }
        platform_free(list->cases);
    }

    platform_free(list);
}

REGISLEX_API regislex_error_t regislex_case_change_status(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_status_t new_status)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "UPDATE cases SET status = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_int(stmt, 1, new_status);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_uuid(stmt, 3, id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        return err;
    }

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_case_assign(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    const regislex_uuid_t* user_id)
{
    if (!ctx || !case_id || !user_id) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_db_context_t* db = NULL;

    const char* sql = "UPDATE cases SET assigned_to_id = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        return err;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    regislex_db_bind_uuid(stmt, 1, user_id);
    regislex_db_bind_datetime(stmt, 2, &now);
    regislex_db_bind_uuid(stmt, 3, case_id);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        return err;
    }

    return REGISLEX_OK;
}

/* ============================================================================
 * Party Management Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_party_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    const regislex_party_t* party,
    regislex_party_t** out_party)
{
    if (!ctx || !case_id || !party || !out_party) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    regislex_party_t* new_party = (regislex_party_t*)platform_calloc(1, sizeof(regislex_party_t));
    if (!new_party) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    memcpy(new_party, party, sizeof(regislex_party_t));

    if (new_party->id.value[0] == '\0') {
        regislex_uuid_generate(&new_party->id);
    }

    regislex_datetime_now(&new_party->created_at);
    memcpy(&new_party->updated_at, &new_party->created_at, sizeof(regislex_datetime_t));

    regislex_db_context_t* db = NULL;

    const char* sql =
        "INSERT INTO parties ("
        "  id, case_id, name, display_name, type, role,"
        "  address_line1, address_line2, city, state, postal_code, country,"
        "  phone, email, attorney_name, attorney_firm, bar_number,"
        "  is_primary, notes, created_at, updated_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(db, sql, &stmt);
    if (err != REGISLEX_OK) {
        regislex_party_free(new_party);
        return err;
    }

    int idx = 1;
    regislex_db_bind_uuid(stmt, idx++, &new_party->id);
    regislex_db_bind_uuid(stmt, idx++, case_id);
    regislex_db_bind_text(stmt, idx++, new_party->name);
    regislex_db_bind_text(stmt, idx++, new_party->display_name);
    regislex_db_bind_int(stmt, idx++, new_party->type);
    regislex_db_bind_int(stmt, idx++, new_party->role);
    regislex_db_bind_text(stmt, idx++, new_party->contact.address_line1);
    regislex_db_bind_text(stmt, idx++, new_party->contact.address_line2);
    regislex_db_bind_text(stmt, idx++, new_party->contact.city);
    regislex_db_bind_text(stmt, idx++, new_party->contact.state);
    regislex_db_bind_text(stmt, idx++, new_party->contact.postal_code);
    regislex_db_bind_text(stmt, idx++, new_party->contact.country);
    regislex_db_bind_text(stmt, idx++, new_party->contact.phone);
    regislex_db_bind_text(stmt, idx++, new_party->contact.email);
    regislex_db_bind_text(stmt, idx++, new_party->attorney_name);
    regislex_db_bind_text(stmt, idx++, new_party->attorney_firm);
    regislex_db_bind_text(stmt, idx++, new_party->bar_number);
    regislex_db_bind_int(stmt, idx++, new_party->is_primary ? 1 : 0);
    regislex_db_bind_text(stmt, idx++, new_party->notes);
    regislex_db_bind_datetime(stmt, idx++, &new_party->created_at);
    regislex_db_bind_datetime(stmt, idx++, &new_party->updated_at);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_ERROR_NOT_FOUND && err != REGISLEX_OK) {
        regislex_party_free(new_party);
        return err;
    }

    *out_party = new_party;
    return REGISLEX_OK;
}

REGISLEX_API void regislex_party_free(regislex_party_t* party) {
    if (party) {
        platform_free(party);
    }
}

/* ============================================================================
 * Matter Management Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_matter_create(
    regislex_context_t* ctx,
    const regislex_matter_t* matter,
    regislex_matter_t** out_matter)
{
    if (!ctx || !matter || !out_matter) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Implementation similar to case_create */
    /* Placeholder for full implementation */

    return REGISLEX_ERROR_UNSUPPORTED;
}

REGISLEX_API void regislex_matter_free(regislex_matter_t* matter) {
    if (matter) {
        platform_free(matter);
    }
}
