/**
 * RegisLex - Enterprise Legal Software Suite
 * Legislative Tracking Implementation
 *
 * Provides tracking for federal, state, and local legislation,
 * regulation monitoring, stakeholder management, and position tracking.
 */

#include "regislex/modules/legislative_tracking/legislative.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Legislation Management
 * ========================================================================== */

regislex_error_t regislex_legislation_create(
    regislex_context_t* ctx,
    regislex_legislation_t* leg_data,
    regislex_legislation_t** out_legislation)
{
    if (!ctx || !leg_data || !out_legislation) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_legislation = NULL;

    regislex_uuid_t id;
    if (leg_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = leg_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char intro_date_str[32] = {0};
    regislex_datetime_format(&leg_data->introduced_date, intro_date_str, sizeof(intro_date_str));

    const char* sql =
        "INSERT INTO legislation (id, bill_number, title, summary, type, "
        "government_level, jurisdiction, status, introduced_date, session, "
        "primary_sponsor, source_url, full_text_url, our_position, "
        "position_rationale, priority, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, leg_data->bill_number);
    regislex_db_bind_text(stmt, 3, leg_data->title);
    regislex_db_bind_text(stmt, 4, leg_data->summary);
    regislex_db_bind_int(stmt, 5, leg_data->type);
    regislex_db_bind_int(stmt, 6, leg_data->government_level);
    regislex_db_bind_text(stmt, 7, leg_data->jurisdiction);
    regislex_db_bind_int(stmt, 8, leg_data->status);
    regislex_db_bind_text(stmt, 9, intro_date_str);
    regislex_db_bind_text(stmt, 10, leg_data->session);
    regislex_db_bind_text(stmt, 11, leg_data->primary_sponsor);
    regislex_db_bind_text(stmt, 12, leg_data->source_url);
    regislex_db_bind_text(stmt, 13, leg_data->full_text_url);
    regislex_db_bind_int(stmt, 14, leg_data->our_position);
    regislex_db_bind_text(stmt, 15, leg_data->position_rationale);
    regislex_db_bind_int(stmt, 16, leg_data->priority);
    regislex_db_bind_text(stmt, 17, now_str);
    regislex_db_bind_text(stmt, 18, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_legislation_get(ctx, &id, out_legislation);
}

regislex_error_t regislex_legislation_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_legislation_t** out_legislation)
{
    if (!ctx || !id || !out_legislation) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_legislation = NULL;

    const char* sql =
        "SELECT id, bill_number, title, summary, type, government_level, "
        "jurisdiction, status, introduced_date, session, primary_sponsor, "
        "source_url, full_text_url, our_position, position_rationale, priority, "
        "created_at, updated_at "
        "FROM legislation WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_legislation_t* leg = (regislex_legislation_t*)calloc(1, sizeof(regislex_legislation_t));
    if (!leg) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(leg->id.value, str, sizeof(leg->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(leg->bill_number, str, sizeof(leg->bill_number) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(leg->title, str, sizeof(leg->title) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(leg->summary, str, sizeof(leg->summary) - 1);

    leg->type = (regislex_legislation_type_t)regislex_db_column_int(stmt, 4);
    leg->government_level = (regislex_gov_level_t)regislex_db_column_int(stmt, 5);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(leg->jurisdiction, str, sizeof(leg->jurisdiction) - 1);

    leg->status = (regislex_leg_status_t)regislex_db_column_int(stmt, 7);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &leg->introduced_date);

    str = regislex_db_column_text(stmt, 9);
    if (str) strncpy(leg->session, str, sizeof(leg->session) - 1);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(leg->primary_sponsor, str, sizeof(leg->primary_sponsor) - 1);

    str = regislex_db_column_text(stmt, 11);
    if (str) strncpy(leg->source_url, str, sizeof(leg->source_url) - 1);

    str = regislex_db_column_text(stmt, 12);
    if (str) strncpy(leg->full_text_url, str, sizeof(leg->full_text_url) - 1);

    leg->our_position = (regislex_position_t)regislex_db_column_int(stmt, 13);

    str = regislex_db_column_text(stmt, 14);
    if (str) strncpy(leg->position_rationale, str, sizeof(leg->position_rationale) - 1);

    leg->priority = regislex_db_column_int(stmt, 15);

    str = regislex_db_column_text(stmt, 16);
    if (str) regislex_datetime_parse(str, &leg->created_at);

    str = regislex_db_column_text(stmt, 17);
    if (str) regislex_datetime_parse(str, &leg->updated_at);

    regislex_db_finalize(stmt);

    *out_legislation = leg;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_legislation_update(
    regislex_context_t* ctx,
    regislex_legislation_t* legislation)
{
    if (!ctx || !legislation || legislation->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char intro_date_str[32] = {0};
    regislex_datetime_format(&legislation->introduced_date, intro_date_str, sizeof(intro_date_str));

    const char* sql =
        "UPDATE legislation SET bill_number = ?, title = ?, summary = ?, "
        "type = ?, government_level = ?, jurisdiction = ?, status = ?, "
        "introduced_date = ?, session = ?, primary_sponsor = ?, source_url = ?, "
        "full_text_url = ?, our_position = ?, position_rationale = ?, "
        "priority = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, legislation->bill_number);
    regislex_db_bind_text(stmt, 2, legislation->title);
    regislex_db_bind_text(stmt, 3, legislation->summary);
    regislex_db_bind_int(stmt, 4, legislation->type);
    regislex_db_bind_int(stmt, 5, legislation->government_level);
    regislex_db_bind_text(stmt, 6, legislation->jurisdiction);
    regislex_db_bind_int(stmt, 7, legislation->status);
    regislex_db_bind_text(stmt, 8, intro_date_str);
    regislex_db_bind_text(stmt, 9, legislation->session);
    regislex_db_bind_text(stmt, 10, legislation->primary_sponsor);
    regislex_db_bind_text(stmt, 11, legislation->source_url);
    regislex_db_bind_text(stmt, 12, legislation->full_text_url);
    regislex_db_bind_int(stmt, 13, legislation->our_position);
    regislex_db_bind_text(stmt, 14, legislation->position_rationale);
    regislex_db_bind_int(stmt, 15, legislation->priority);
    regislex_db_bind_text(stmt, 16, now_str);
    regislex_db_bind_text(stmt, 17, legislation->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_legislation_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Delete related records first */
    const char* history_sql = "DELETE FROM legislation_history WHERE legislation_id = ?";
    regislex_db_stmt_t* stmt = NULL;
    if (regislex_db_prepare(ctx, history_sql, &stmt) == REGISLEX_SUCCESS) {
        regislex_db_bind_text(stmt, 1, id->value);
        regislex_db_step(stmt);
        regislex_db_finalize(stmt);
    }

    const char* sql = "DELETE FROM legislation WHERE id = ?";
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);
    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_legislation_list(
    regislex_context_t* ctx,
    regislex_legislation_filter_t* filter,
    regislex_legislation_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[2048];
    char where_clause[1024] = "WHERE 1=1";

    if (filter) {
        if (filter->government_level >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND government_level = %d", filter->government_level);
            strcat(where_clause, buf);
        }

        if (filter->jurisdiction[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND jurisdiction = '%s'", filter->jurisdiction);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        }

        if (filter->type >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND type = %d", filter->type);
            strcat(where_clause, buf);
        }

        if (filter->our_position >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND our_position = %d", filter->our_position);
            strcat(where_clause, buf);
        }

        if (filter->session[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND session = '%s'", filter->session);
            strcat(where_clause, buf);
        }

        if (filter->search_text[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     " AND (title LIKE '%%%s%%' OR bill_number LIKE '%%%s%%' OR summary LIKE '%%%s%%')",
                     filter->search_text, filter->search_text, filter->search_text);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, bill_number, title, type, government_level, jurisdiction, "
             "status, session, our_position, priority, introduced_date "
             "FROM legislation %s ORDER BY priority DESC, introduced_date DESC",
             where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_legislation_list_t* list = (regislex_legislation_list_t*)calloc(1,
        sizeof(regislex_legislation_list_t) + count * sizeof(regislex_legislation_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_legislation_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].bill_number, str, sizeof(list->items[i].bill_number) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->items[i].title, str, sizeof(list->items[i].title) - 1);

        list->items[i].type = (regislex_legislation_type_t)regislex_db_result_get_int(result, 3);
        list->items[i].government_level = (regislex_gov_level_t)regislex_db_result_get_int(result, 4);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->items[i].jurisdiction, str, sizeof(list->items[i].jurisdiction) - 1);

        list->items[i].status = (regislex_leg_status_t)regislex_db_result_get_int(result, 6);

        str = regislex_db_result_get_text(result, 7);
        if (str) strncpy(list->items[i].session, str, sizeof(list->items[i].session) - 1);

        list->items[i].our_position = (regislex_position_t)regislex_db_result_get_int(result, 8);
        list->items[i].priority = regislex_db_result_get_int(result, 9);

        str = regislex_db_result_get_text(result, 10);
        if (str) regislex_datetime_parse(str, &list->items[i].introduced_date);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_legislation_free(regislex_legislation_t* legislation) {
    free(legislation);
}

void regislex_legislation_list_free(regislex_legislation_list_t* list) {
    free(list);
}

/* ============================================================================
 * Legislation History
 * ========================================================================== */

regislex_error_t regislex_legislation_add_history(
    regislex_context_t* ctx,
    const regislex_uuid_t* legislation_id,
    regislex_leg_status_t new_status,
    const char* action_description,
    const regislex_datetime_t* action_date)
{
    if (!ctx || !legislation_id || !action_description) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_uuid_t id;
    regislex_uuid_generate(&id);

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char action_date_str[32] = {0};
    if (action_date) {
        regislex_datetime_format(action_date, action_date_str, sizeof(action_date_str));
    } else {
        strcpy(action_date_str, now_str);
    }

    const char* sql =
        "INSERT INTO legislation_history (id, legislation_id, action_date, "
        "status, action_description, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, legislation_id->value);
    regislex_db_bind_text(stmt, 3, action_date_str);
    regislex_db_bind_int(stmt, 4, new_status);
    regislex_db_bind_text(stmt, 5, action_description);
    regislex_db_bind_text(stmt, 6, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    /* Update legislation status */
    const char* update_sql = "UPDATE legislation SET status = ?, updated_at = ? WHERE id = ?";
    err = regislex_db_prepare(ctx, update_sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_int(stmt, 1, new_status);
    regislex_db_bind_text(stmt, 2, now_str);
    regislex_db_bind_text(stmt, 3, legislation_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_legislation_get_history(
    regislex_context_t* ctx,
    const regislex_uuid_t* legislation_id,
    regislex_leg_history_list_t** out_list)
{
    if (!ctx || !legislation_id || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, legislation_id, action_date, status, action_description "
             "FROM legislation_history WHERE legislation_id = '%s' "
             "ORDER BY action_date DESC", legislation_id->value);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_leg_history_list_t* list = (regislex_leg_history_list_t*)calloc(1,
        sizeof(regislex_leg_history_list_t) + count * sizeof(regislex_leg_history_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_leg_history_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].legislation_id.value, str, sizeof(list->items[i].legislation_id.value) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) regislex_datetime_parse(str, &list->items[i].action_date);

        list->items[i].status = (regislex_leg_status_t)regislex_db_result_get_int(result, 3);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->items[i].action_description, str, sizeof(list->items[i].action_description) - 1);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_leg_history_list_free(regislex_leg_history_list_t* list) {
    free(list);
}

/* ============================================================================
 * Regulation Management
 * ========================================================================== */

regislex_error_t regislex_regulation_create(
    regislex_context_t* ctx,
    regislex_regulation_t* reg_data,
    regislex_regulation_t** out_regulation)
{
    if (!ctx || !reg_data || !out_regulation) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_regulation = NULL;

    regislex_uuid_t id;
    if (reg_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = reg_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char pub_date_str[32] = {0};
    regislex_datetime_format(&reg_data->publication_date, pub_date_str, sizeof(pub_date_str));

    char eff_date_str[32] = {0};
    regislex_datetime_format(&reg_data->effective_date, eff_date_str, sizeof(eff_date_str));

    char comment_date_str[32] = {0};
    regislex_datetime_format(&reg_data->comment_deadline, comment_date_str, sizeof(comment_date_str));

    const char* sql =
        "INSERT INTO regulations (id, docket_number, title, summary, agency, "
        "cfr_citation, status, publication_date, effective_date, comment_deadline, "
        "source_url, our_position, position_rationale, priority, "
        "created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, reg_data->docket_number);
    regislex_db_bind_text(stmt, 3, reg_data->title);
    regislex_db_bind_text(stmt, 4, reg_data->summary);
    regislex_db_bind_text(stmt, 5, reg_data->agency);
    regislex_db_bind_text(stmt, 6, reg_data->cfr_citation);
    regislex_db_bind_int(stmt, 7, reg_data->status);
    regislex_db_bind_text(stmt, 8, pub_date_str);
    regislex_db_bind_text(stmt, 9, eff_date_str);
    regislex_db_bind_text(stmt, 10, comment_date_str);
    regislex_db_bind_text(stmt, 11, reg_data->source_url);
    regislex_db_bind_int(stmt, 12, reg_data->our_position);
    regislex_db_bind_text(stmt, 13, reg_data->position_rationale);
    regislex_db_bind_int(stmt, 14, reg_data->priority);
    regislex_db_bind_text(stmt, 15, now_str);
    regislex_db_bind_text(stmt, 16, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_regulation_get(ctx, &id, out_regulation);
}

regislex_error_t regislex_regulation_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_regulation_t** out_regulation)
{
    if (!ctx || !id || !out_regulation) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_regulation = NULL;

    const char* sql =
        "SELECT id, docket_number, title, summary, agency, cfr_citation, status, "
        "publication_date, effective_date, comment_deadline, source_url, "
        "our_position, position_rationale, priority, created_at, updated_at "
        "FROM regulations WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_regulation_t* reg = (regislex_regulation_t*)calloc(1, sizeof(regislex_regulation_t));
    if (!reg) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(reg->id.value, str, sizeof(reg->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(reg->docket_number, str, sizeof(reg->docket_number) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(reg->title, str, sizeof(reg->title) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(reg->summary, str, sizeof(reg->summary) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(reg->agency, str, sizeof(reg->agency) - 1);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(reg->cfr_citation, str, sizeof(reg->cfr_citation) - 1);

    reg->status = (regislex_reg_status_t)regislex_db_column_int(stmt, 6);

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &reg->publication_date);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &reg->effective_date);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &reg->comment_deadline);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(reg->source_url, str, sizeof(reg->source_url) - 1);

    reg->our_position = (regislex_position_t)regislex_db_column_int(stmt, 11);

    str = regislex_db_column_text(stmt, 12);
    if (str) strncpy(reg->position_rationale, str, sizeof(reg->position_rationale) - 1);

    reg->priority = regislex_db_column_int(stmt, 13);

    str = regislex_db_column_text(stmt, 14);
    if (str) regislex_datetime_parse(str, &reg->created_at);

    str = regislex_db_column_text(stmt, 15);
    if (str) regislex_datetime_parse(str, &reg->updated_at);

    regislex_db_finalize(stmt);

    *out_regulation = reg;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_regulation_list(
    regislex_context_t* ctx,
    regislex_regulation_filter_t* filter,
    regislex_regulation_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[2048];
    char where_clause[1024] = "WHERE 1=1";

    if (filter) {
        if (filter->agency[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf), " AND agency = '%s'", filter->agency);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        }

        if (filter->our_position >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND our_position = %d", filter->our_position);
            strcat(where_clause, buf);
        }

        if (filter->comment_deadline_before.year > 0) {
            char date_str[32];
            regislex_datetime_format(&filter->comment_deadline_before, date_str, sizeof(date_str));
            char buf[128];
            snprintf(buf, sizeof(buf), " AND comment_deadline <= '%s'", date_str);
            strcat(where_clause, buf);
        }

        if (filter->search_text[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     " AND (title LIKE '%%%s%%' OR docket_number LIKE '%%%s%%' OR summary LIKE '%%%s%%')",
                     filter->search_text, filter->search_text, filter->search_text);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, docket_number, title, agency, cfr_citation, status, "
             "publication_date, comment_deadline, our_position, priority "
             "FROM regulations %s ORDER BY priority DESC, comment_deadline ASC",
             where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_regulation_list_t* list = (regislex_regulation_list_t*)calloc(1,
        sizeof(regislex_regulation_list_t) + count * sizeof(regislex_regulation_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_regulation_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].docket_number, str, sizeof(list->items[i].docket_number) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->items[i].title, str, sizeof(list->items[i].title) - 1);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->items[i].agency, str, sizeof(list->items[i].agency) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->items[i].cfr_citation, str, sizeof(list->items[i].cfr_citation) - 1);

        list->items[i].status = (regislex_reg_status_t)regislex_db_result_get_int(result, 5);

        str = regislex_db_result_get_text(result, 6);
        if (str) regislex_datetime_parse(str, &list->items[i].publication_date);

        str = regislex_db_result_get_text(result, 7);
        if (str) regislex_datetime_parse(str, &list->items[i].comment_deadline);

        list->items[i].our_position = (regislex_position_t)regislex_db_result_get_int(result, 8);
        list->items[i].priority = regislex_db_result_get_int(result, 9);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_regulation_free(regislex_regulation_t* regulation) {
    free(regulation);
}

void regislex_regulation_list_free(regislex_regulation_list_t* list) {
    free(list);
}

/* ============================================================================
 * Stakeholder Management
 * ========================================================================== */

regislex_error_t regislex_stakeholder_create(
    regislex_context_t* ctx,
    regislex_stakeholder_t* stake_data,
    regislex_stakeholder_t** out_stakeholder)
{
    if (!ctx || !stake_data || !out_stakeholder) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_stakeholder = NULL;

    regislex_uuid_t id;
    if (stake_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = stake_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO stakeholders (id, name, title, organization, type, "
        "email, phone, address, notes, influence_level, relationship_status, "
        "created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, stake_data->name);
    regislex_db_bind_text(stmt, 3, stake_data->title);
    regislex_db_bind_text(stmt, 4, stake_data->organization);
    regislex_db_bind_int(stmt, 5, stake_data->type);
    regislex_db_bind_text(stmt, 6, stake_data->email);
    regislex_db_bind_text(stmt, 7, stake_data->phone);
    regislex_db_bind_text(stmt, 8, stake_data->address);
    regislex_db_bind_text(stmt, 9, stake_data->notes);
    regislex_db_bind_int(stmt, 10, stake_data->influence_level);
    regislex_db_bind_int(stmt, 11, stake_data->relationship_status);
    regislex_db_bind_text(stmt, 12, now_str);
    regislex_db_bind_text(stmt, 13, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_stakeholder_get(ctx, &id, out_stakeholder);
}

regislex_error_t regislex_stakeholder_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_stakeholder_t** out_stakeholder)
{
    if (!ctx || !id || !out_stakeholder) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_stakeholder = NULL;

    const char* sql =
        "SELECT id, name, title, organization, type, email, phone, address, "
        "notes, influence_level, relationship_status, created_at, updated_at "
        "FROM stakeholders WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_stakeholder_t* stake = (regislex_stakeholder_t*)calloc(1, sizeof(regislex_stakeholder_t));
    if (!stake) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(stake->id.value, str, sizeof(stake->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(stake->name, str, sizeof(stake->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(stake->title, str, sizeof(stake->title) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(stake->organization, str, sizeof(stake->organization) - 1);

    stake->type = (regislex_stakeholder_type_t)regislex_db_column_int(stmt, 4);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(stake->email, str, sizeof(stake->email) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(stake->phone, str, sizeof(stake->phone) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) strncpy(stake->address, str, sizeof(stake->address) - 1);

    str = regislex_db_column_text(stmt, 8);
    if (str) strncpy(stake->notes, str, sizeof(stake->notes) - 1);

    stake->influence_level = regislex_db_column_int(stmt, 9);
    stake->relationship_status = (regislex_relationship_status_t)regislex_db_column_int(stmt, 10);

    str = regislex_db_column_text(stmt, 11);
    if (str) regislex_datetime_parse(str, &stake->created_at);

    str = regislex_db_column_text(stmt, 12);
    if (str) regislex_datetime_parse(str, &stake->updated_at);

    regislex_db_finalize(stmt);

    *out_stakeholder = stake;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_stakeholder_update(
    regislex_context_t* ctx,
    regislex_stakeholder_t* stakeholder)
{
    if (!ctx || !stakeholder || stakeholder->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "UPDATE stakeholders SET name = ?, title = ?, organization = ?, "
        "type = ?, email = ?, phone = ?, address = ?, notes = ?, "
        "influence_level = ?, relationship_status = ?, updated_at = ? "
        "WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, stakeholder->name);
    regislex_db_bind_text(stmt, 2, stakeholder->title);
    regislex_db_bind_text(stmt, 3, stakeholder->organization);
    regislex_db_bind_int(stmt, 4, stakeholder->type);
    regislex_db_bind_text(stmt, 5, stakeholder->email);
    regislex_db_bind_text(stmt, 6, stakeholder->phone);
    regislex_db_bind_text(stmt, 7, stakeholder->address);
    regislex_db_bind_text(stmt, 8, stakeholder->notes);
    regislex_db_bind_int(stmt, 9, stakeholder->influence_level);
    regislex_db_bind_int(stmt, 10, stakeholder->relationship_status);
    regislex_db_bind_text(stmt, 11, now_str);
    regislex_db_bind_text(stmt, 12, stakeholder->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_stakeholder_list(
    regislex_context_t* ctx,
    regislex_stakeholder_filter_t* filter,
    regislex_stakeholder_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[1024];
    char where_clause[512] = "WHERE 1=1";

    if (filter) {
        if (filter->type >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND type = %d", filter->type);
            strcat(where_clause, buf);
        }

        if (filter->organization[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf), " AND organization LIKE '%%%s%%'", filter->organization);
            strcat(where_clause, buf);
        }

        if (filter->search_text[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     " AND (name LIKE '%%%s%%' OR organization LIKE '%%%s%%')",
                     filter->search_text, filter->search_text);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, name, title, organization, type, email, influence_level, "
             "relationship_status FROM stakeholders %s ORDER BY influence_level DESC, name",
             where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_stakeholder_list_t* list = (regislex_stakeholder_list_t*)calloc(1,
        sizeof(regislex_stakeholder_list_t) + count * sizeof(regislex_stakeholder_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_stakeholder_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].name, str, sizeof(list->items[i].name) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->items[i].title, str, sizeof(list->items[i].title) - 1);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->items[i].organization, str, sizeof(list->items[i].organization) - 1);

        list->items[i].type = (regislex_stakeholder_type_t)regislex_db_result_get_int(result, 4);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->items[i].email, str, sizeof(list->items[i].email) - 1);

        list->items[i].influence_level = regislex_db_result_get_int(result, 6);
        list->items[i].relationship_status = (regislex_relationship_status_t)regislex_db_result_get_int(result, 7);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_stakeholder_free(regislex_stakeholder_t* stakeholder) {
    free(stakeholder);
}

void regislex_stakeholder_list_free(regislex_stakeholder_list_t* list) {
    free(list);
}

/* ============================================================================
 * Engagement Tracking
 * ========================================================================== */

regislex_error_t regislex_engagement_create(
    regislex_context_t* ctx,
    regislex_engagement_t* eng_data,
    regislex_engagement_t** out_engagement)
{
    if (!ctx || !eng_data || !out_engagement) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_engagement = NULL;

    regislex_uuid_t id;
    if (eng_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = eng_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char eng_date_str[32];
    regislex_datetime_format(&eng_data->engagement_date, eng_date_str, sizeof(eng_date_str));

    char follow_date_str[32] = {0};
    if (eng_data->follow_up_date.year > 0) {
        regislex_datetime_format(&eng_data->follow_up_date, follow_date_str, sizeof(follow_date_str));
    }

    const char* sql =
        "INSERT INTO engagements (id, stakeholder_id, legislation_id, regulation_id, "
        "engagement_type, engagement_date, subject, description, outcome, "
        "follow_up_required, follow_up_date, conducted_by_id, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, eng_data->stakeholder_id.value);

    if (eng_data->legislation_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 3, eng_data->legislation_id.value);
    } else {
        regislex_db_bind_null(stmt, 3);
    }

    if (eng_data->regulation_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 4, eng_data->regulation_id.value);
    } else {
        regislex_db_bind_null(stmt, 4);
    }

    regislex_db_bind_int(stmt, 5, eng_data->engagement_type);
    regislex_db_bind_text(stmt, 6, eng_date_str);
    regislex_db_bind_text(stmt, 7, eng_data->subject);
    regislex_db_bind_text(stmt, 8, eng_data->description);
    regislex_db_bind_text(stmt, 9, eng_data->outcome);
    regislex_db_bind_int(stmt, 10, eng_data->follow_up_required ? 1 : 0);

    if (follow_date_str[0] != '\0') {
        regislex_db_bind_text(stmt, 11, follow_date_str);
    } else {
        regislex_db_bind_null(stmt, 11);
    }

    regislex_db_bind_text(stmt, 12, eng_data->conducted_by_id.value);
    regislex_db_bind_text(stmt, 13, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_engagement_get(ctx, &id, out_engagement);
}

regislex_error_t regislex_engagement_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_engagement_t** out_engagement)
{
    if (!ctx || !id || !out_engagement) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_engagement = NULL;

    const char* sql =
        "SELECT id, stakeholder_id, legislation_id, regulation_id, engagement_type, "
        "engagement_date, subject, description, outcome, follow_up_required, "
        "follow_up_date, conducted_by_id, created_at "
        "FROM engagements WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_engagement_t* eng = (regislex_engagement_t*)calloc(1, sizeof(regislex_engagement_t));
    if (!eng) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(eng->id.value, str, sizeof(eng->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(eng->stakeholder_id.value, str, sizeof(eng->stakeholder_id.value) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(eng->legislation_id.value, str, sizeof(eng->legislation_id.value) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(eng->regulation_id.value, str, sizeof(eng->regulation_id.value) - 1);

    eng->engagement_type = (regislex_engagement_type_t)regislex_db_column_int(stmt, 4);

    str = regislex_db_column_text(stmt, 5);
    if (str) regislex_datetime_parse(str, &eng->engagement_date);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(eng->subject, str, sizeof(eng->subject) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) strncpy(eng->description, str, sizeof(eng->description) - 1);

    str = regislex_db_column_text(stmt, 8);
    if (str) strncpy(eng->outcome, str, sizeof(eng->outcome) - 1);

    eng->follow_up_required = regislex_db_column_int(stmt, 9) != 0;

    str = regislex_db_column_text(stmt, 10);
    if (str) regislex_datetime_parse(str, &eng->follow_up_date);

    str = regislex_db_column_text(stmt, 11);
    if (str) strncpy(eng->conducted_by_id.value, str, sizeof(eng->conducted_by_id.value) - 1);

    str = regislex_db_column_text(stmt, 12);
    if (str) regislex_datetime_parse(str, &eng->created_at);

    regislex_db_finalize(stmt);

    *out_engagement = eng;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_engagement_list_by_stakeholder(
    regislex_context_t* ctx,
    const regislex_uuid_t* stakeholder_id,
    regislex_engagement_list_t** out_list)
{
    if (!ctx || !stakeholder_id || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, stakeholder_id, legislation_id, regulation_id, engagement_type, "
             "engagement_date, subject, outcome, follow_up_required "
             "FROM engagements WHERE stakeholder_id = '%s' "
             "ORDER BY engagement_date DESC", stakeholder_id->value);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_engagement_list_t* list = (regislex_engagement_list_t*)calloc(1,
        sizeof(regislex_engagement_list_t) + count * sizeof(regislex_engagement_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_engagement_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].stakeholder_id.value, str, sizeof(list->items[i].stakeholder_id.value) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->items[i].legislation_id.value, str, sizeof(list->items[i].legislation_id.value) - 1);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->items[i].regulation_id.value, str, sizeof(list->items[i].regulation_id.value) - 1);

        list->items[i].engagement_type = (regislex_engagement_type_t)regislex_db_result_get_int(result, 4);

        str = regislex_db_result_get_text(result, 5);
        if (str) regislex_datetime_parse(str, &list->items[i].engagement_date);

        str = regislex_db_result_get_text(result, 6);
        if (str) strncpy(list->items[i].subject, str, sizeof(list->items[i].subject) - 1);

        str = regislex_db_result_get_text(result, 7);
        if (str) strncpy(list->items[i].outcome, str, sizeof(list->items[i].outcome) - 1);

        list->items[i].follow_up_required = regislex_db_result_get_int(result, 8) != 0;

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_engagement_free(regislex_engagement_t* engagement) {
    free(engagement);
}

void regislex_engagement_list_free(regislex_engagement_list_t* list) {
    free(list);
}

/* ============================================================================
 * Legislative Alerts
 * ========================================================================== */

regislex_error_t regislex_leg_alert_create(
    regislex_context_t* ctx,
    regislex_leg_alert_t* alert_data,
    regislex_leg_alert_t** out_alert)
{
    if (!ctx || !alert_data || !out_alert) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_alert = NULL;

    regislex_uuid_t id;
    if (alert_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = alert_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO legislative_alerts (id, name, alert_type, keywords, "
        "government_levels, jurisdictions, notification_emails, is_active, "
        "created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, alert_data->name);
    regislex_db_bind_int(stmt, 3, alert_data->alert_type);
    regislex_db_bind_text(stmt, 4, alert_data->keywords);
    regislex_db_bind_text(stmt, 5, alert_data->government_levels);
    regislex_db_bind_text(stmt, 6, alert_data->jurisdictions);
    regislex_db_bind_text(stmt, 7, alert_data->notification_emails);
    regislex_db_bind_int(stmt, 8, alert_data->is_active ? 1 : 0);
    regislex_db_bind_text(stmt, 9, now_str);
    regislex_db_bind_text(stmt, 10, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_leg_alert_get(ctx, &id, out_alert);
}

regislex_error_t regislex_leg_alert_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_leg_alert_t** out_alert)
{
    if (!ctx || !id || !out_alert) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_alert = NULL;

    const char* sql =
        "SELECT id, name, alert_type, keywords, government_levels, jurisdictions, "
        "notification_emails, is_active, last_triggered, created_at, updated_at "
        "FROM legislative_alerts WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_leg_alert_t* alert = (regislex_leg_alert_t*)calloc(1, sizeof(regislex_leg_alert_t));
    if (!alert) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(alert->id.value, str, sizeof(alert->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(alert->name, str, sizeof(alert->name) - 1);

    alert->alert_type = (regislex_leg_alert_type_t)regislex_db_column_int(stmt, 2);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(alert->keywords, str, sizeof(alert->keywords) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(alert->government_levels, str, sizeof(alert->government_levels) - 1);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(alert->jurisdictions, str, sizeof(alert->jurisdictions) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(alert->notification_emails, str, sizeof(alert->notification_emails) - 1);

    alert->is_active = regislex_db_column_int(stmt, 7) != 0;

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &alert->last_triggered);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &alert->created_at);

    str = regislex_db_column_text(stmt, 10);
    if (str) regislex_datetime_parse(str, &alert->updated_at);

    regislex_db_finalize(stmt);

    *out_alert = alert;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_leg_alert_list(
    regislex_context_t* ctx,
    bool active_only,
    regislex_leg_alert_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    if (active_only) {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, alert_type, keywords, is_active, last_triggered "
                 "FROM legislative_alerts WHERE is_active = 1 ORDER BY name");
    } else {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, alert_type, keywords, is_active, last_triggered "
                 "FROM legislative_alerts ORDER BY name");
    }

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_leg_alert_list_t* list = (regislex_leg_alert_list_t*)calloc(1,
        sizeof(regislex_leg_alert_list_t) + count * sizeof(regislex_leg_alert_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_leg_alert_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].name, str, sizeof(list->items[i].name) - 1);

        list->items[i].alert_type = (regislex_leg_alert_type_t)regislex_db_result_get_int(result, 2);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->items[i].keywords, str, sizeof(list->items[i].keywords) - 1);

        list->items[i].is_active = regislex_db_result_get_int(result, 4) != 0;

        str = regislex_db_result_get_text(result, 5);
        if (str) regislex_datetime_parse(str, &list->items[i].last_triggered);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_leg_alert_free(regislex_leg_alert_t* alert) {
    free(alert);
}

void regislex_leg_alert_list_free(regislex_leg_alert_list_t* list) {
    free(list);
}
