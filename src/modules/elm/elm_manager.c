/**
 * RegisLex - Enterprise Legal Software Suite
 * Enterprise Legal Management (ELM) Implementation
 *
 * Provides outside counsel management, eBilling with LEDES support,
 * matter budgeting, contract lifecycle management, and risk tracking.
 */

#include "regislex/modules/elm/elm.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Vendor (Outside Counsel) Management
 * ========================================================================== */

regislex_error_t regislex_vendor_create(
    regislex_context_t* ctx,
    regislex_vendor_t* vendor_data,
    regislex_vendor_t** out_vendor)
{
    if (!ctx || !vendor_data || !out_vendor) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_vendor = NULL;

    regislex_uuid_t id;
    if (vendor_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = vendor_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO vendors (id, name, vendor_type, tax_id, address, "
        "city, state, postal_code, country, primary_contact_name, "
        "primary_contact_email, primary_contact_phone, billing_contact_email, "
        "payment_terms, default_rate_currency, status, notes, "
        "diversity_certified, diversity_categories, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, vendor_data->name);
    regislex_db_bind_int(stmt, 3, vendor_data->vendor_type);
    regislex_db_bind_text(stmt, 4, vendor_data->tax_id);
    regislex_db_bind_text(stmt, 5, vendor_data->address);
    regislex_db_bind_text(stmt, 6, vendor_data->city);
    regislex_db_bind_text(stmt, 7, vendor_data->state);
    regislex_db_bind_text(stmt, 8, vendor_data->postal_code);
    regislex_db_bind_text(stmt, 9, vendor_data->country);
    regislex_db_bind_text(stmt, 10, vendor_data->primary_contact_name);
    regislex_db_bind_text(stmt, 11, vendor_data->primary_contact_email);
    regislex_db_bind_text(stmt, 12, vendor_data->primary_contact_phone);
    regislex_db_bind_text(stmt, 13, vendor_data->billing_contact_email);
    regislex_db_bind_int(stmt, 14, vendor_data->payment_terms);
    regislex_db_bind_text(stmt, 15, vendor_data->default_rate.currency);
    regislex_db_bind_int(stmt, 16, vendor_data->status);
    regislex_db_bind_text(stmt, 17, vendor_data->notes);
    regislex_db_bind_int(stmt, 18, vendor_data->diversity_certified ? 1 : 0);
    regislex_db_bind_text(stmt, 19, vendor_data->diversity_categories);
    regislex_db_bind_text(stmt, 20, now_str);
    regislex_db_bind_text(stmt, 21, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_vendor_get(ctx, &id, out_vendor);
}

regislex_error_t regislex_vendor_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_vendor_t** out_vendor)
{
    if (!ctx || !id || !out_vendor) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_vendor = NULL;

    const char* sql =
        "SELECT id, name, vendor_type, tax_id, address, city, state, "
        "postal_code, country, primary_contact_name, primary_contact_email, "
        "primary_contact_phone, billing_contact_email, payment_terms, "
        "default_rate_currency, status, notes, diversity_certified, "
        "diversity_categories, created_at, updated_at "
        "FROM vendors WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_vendor_t* vendor = (regislex_vendor_t*)calloc(1, sizeof(regislex_vendor_t));
    if (!vendor) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(vendor->id.value, str, sizeof(vendor->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(vendor->name, str, sizeof(vendor->name) - 1);

    vendor->vendor_type = (regislex_vendor_type_t)regislex_db_column_int(stmt, 2);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(vendor->tax_id, str, sizeof(vendor->tax_id) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(vendor->address, str, sizeof(vendor->address) - 1);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(vendor->city, str, sizeof(vendor->city) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(vendor->state, str, sizeof(vendor->state) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) strncpy(vendor->postal_code, str, sizeof(vendor->postal_code) - 1);

    str = regislex_db_column_text(stmt, 8);
    if (str) strncpy(vendor->country, str, sizeof(vendor->country) - 1);

    str = regislex_db_column_text(stmt, 9);
    if (str) strncpy(vendor->primary_contact_name, str, sizeof(vendor->primary_contact_name) - 1);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(vendor->primary_contact_email, str, sizeof(vendor->primary_contact_email) - 1);

    str = regislex_db_column_text(stmt, 11);
    if (str) strncpy(vendor->primary_contact_phone, str, sizeof(vendor->primary_contact_phone) - 1);

    str = regislex_db_column_text(stmt, 12);
    if (str) strncpy(vendor->billing_contact_email, str, sizeof(vendor->billing_contact_email) - 1);

    vendor->payment_terms = regislex_db_column_int(stmt, 13);

    str = regislex_db_column_text(stmt, 14);
    if (str) strncpy(vendor->default_rate.currency, str, sizeof(vendor->default_rate.currency) - 1);

    vendor->status = (regislex_vendor_status_t)regislex_db_column_int(stmt, 15);

    str = regislex_db_column_text(stmt, 16);
    if (str) strncpy(vendor->notes, str, sizeof(vendor->notes) - 1);

    vendor->diversity_certified = regislex_db_column_int(stmt, 17) != 0;

    str = regislex_db_column_text(stmt, 18);
    if (str) strncpy(vendor->diversity_categories, str, sizeof(vendor->diversity_categories) - 1);

    str = regislex_db_column_text(stmt, 19);
    if (str) regislex_datetime_parse(str, &vendor->created_at);

    str = regislex_db_column_text(stmt, 20);
    if (str) regislex_datetime_parse(str, &vendor->updated_at);

    regislex_db_finalize(stmt);

    *out_vendor = vendor;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_vendor_update(
    regislex_context_t* ctx,
    regislex_vendor_t* vendor)
{
    if (!ctx || !vendor || vendor->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "UPDATE vendors SET name = ?, vendor_type = ?, tax_id = ?, "
        "address = ?, city = ?, state = ?, postal_code = ?, country = ?, "
        "primary_contact_name = ?, primary_contact_email = ?, "
        "primary_contact_phone = ?, billing_contact_email = ?, "
        "payment_terms = ?, status = ?, notes = ?, diversity_certified = ?, "
        "diversity_categories = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, vendor->name);
    regislex_db_bind_int(stmt, 2, vendor->vendor_type);
    regislex_db_bind_text(stmt, 3, vendor->tax_id);
    regislex_db_bind_text(stmt, 4, vendor->address);
    regislex_db_bind_text(stmt, 5, vendor->city);
    regislex_db_bind_text(stmt, 6, vendor->state);
    regislex_db_bind_text(stmt, 7, vendor->postal_code);
    regislex_db_bind_text(stmt, 8, vendor->country);
    regislex_db_bind_text(stmt, 9, vendor->primary_contact_name);
    regislex_db_bind_text(stmt, 10, vendor->primary_contact_email);
    regislex_db_bind_text(stmt, 11, vendor->primary_contact_phone);
    regislex_db_bind_text(stmt, 12, vendor->billing_contact_email);
    regislex_db_bind_int(stmt, 13, vendor->payment_terms);
    regislex_db_bind_int(stmt, 14, vendor->status);
    regislex_db_bind_text(stmt, 15, vendor->notes);
    regislex_db_bind_int(stmt, 16, vendor->diversity_certified ? 1 : 0);
    regislex_db_bind_text(stmt, 17, vendor->diversity_categories);
    regislex_db_bind_text(stmt, 18, now_str);
    regislex_db_bind_text(stmt, 19, vendor->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_vendor_list(
    regislex_context_t* ctx,
    regislex_vendor_filter_t* filter,
    regislex_vendor_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[1024];
    char where_clause[512] = "WHERE 1=1";

    if (filter) {
        if (filter->vendor_type >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND vendor_type = %d", filter->vendor_type);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        }

        if (filter->diversity_certified) {
            strcat(where_clause, " AND diversity_certified = 1");
        }

        if (filter->search_text[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf), " AND (name LIKE '%%%s%%' OR city LIKE '%%%s%%')",
                     filter->search_text, filter->search_text);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, name, vendor_type, city, state, country, status, "
             "diversity_certified FROM vendors %s ORDER BY name", where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_vendor_list_t* list = (regislex_vendor_list_t*)calloc(1,
        sizeof(regislex_vendor_list_t) + count * sizeof(regislex_vendor_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->vendors = (regislex_vendor_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->vendors[i].id.value, str, sizeof(list->vendors[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->vendors[i].name, str, sizeof(list->vendors[i].name) - 1);

        list->vendors[i].vendor_type = (regislex_vendor_type_t)regislex_db_result_get_int(result, 2);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->vendors[i].city, str, sizeof(list->vendors[i].city) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->vendors[i].state, str, sizeof(list->vendors[i].state) - 1);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->vendors[i].country, str, sizeof(list->vendors[i].country) - 1);

        list->vendors[i].status = (regislex_vendor_status_t)regislex_db_result_get_int(result, 6);
        list->vendors[i].diversity_certified = regislex_db_result_get_int(result, 7) != 0;

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_vendor_free(regislex_vendor_t* vendor) {
    free(vendor);
}

void regislex_vendor_list_free(regislex_vendor_list_t* list) {
    free(list);
}

/* ============================================================================
 * Invoice Management
 * ========================================================================== */

regislex_error_t regislex_invoice_create(
    regislex_context_t* ctx,
    regislex_invoice_t* invoice_data,
    regislex_invoice_t** out_invoice)
{
    if (!ctx || !invoice_data || !out_invoice) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_invoice = NULL;

    regislex_uuid_t id;
    if (invoice_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = invoice_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char invoice_date_str[32];
    regislex_datetime_format(&invoice_data->invoice_date, invoice_date_str, sizeof(invoice_date_str));

    char period_start_str[32];
    regislex_datetime_format(&invoice_data->period_start, period_start_str, sizeof(period_start_str));

    char period_end_str[32];
    regislex_datetime_format(&invoice_data->period_end, period_end_str, sizeof(period_end_str));

    char due_date_str[32];
    regislex_datetime_format(&invoice_data->due_date, due_date_str, sizeof(due_date_str));

    const char* sql =
        "INSERT INTO invoices (id, invoice_number, vendor_id, case_id, "
        "invoice_date, period_start, period_end, due_date, status, "
        "currency, fees_amount, expenses_amount, adjustments_amount, "
        "tax_amount, total_amount, notes, ledes_format, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, invoice_data->invoice_number);
    regislex_db_bind_text(stmt, 3, invoice_data->vendor_id.value);

    if (invoice_data->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 4, invoice_data->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 4);
    }

    regislex_db_bind_text(stmt, 5, invoice_date_str);
    regislex_db_bind_text(stmt, 6, period_start_str);
    regislex_db_bind_text(stmt, 7, period_end_str);
    regislex_db_bind_text(stmt, 8, due_date_str);
    regislex_db_bind_int(stmt, 9, invoice_data->status);
    regislex_db_bind_text(stmt, 10, invoice_data->currency);
    regislex_db_bind_int64(stmt, 11, invoice_data->fees_amount);
    regislex_db_bind_int64(stmt, 12, invoice_data->expenses_amount);
    regislex_db_bind_int64(stmt, 13, invoice_data->adjustments_amount);
    regislex_db_bind_int64(stmt, 14, invoice_data->tax_amount);
    regislex_db_bind_int64(stmt, 15, invoice_data->total_amount);
    regislex_db_bind_text(stmt, 16, invoice_data->notes);
    regislex_db_bind_int(stmt, 17, invoice_data->ledes_format);
    regislex_db_bind_text(stmt, 18, now_str);
    regislex_db_bind_text(stmt, 19, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_invoice_get(ctx, &id, out_invoice);
}

regislex_error_t regislex_invoice_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_invoice_t** out_invoice)
{
    if (!ctx || !id || !out_invoice) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_invoice = NULL;

    const char* sql =
        "SELECT id, invoice_number, vendor_id, case_id, invoice_date, "
        "period_start, period_end, due_date, status, currency, fees_amount, "
        "expenses_amount, adjustments_amount, tax_amount, total_amount, "
        "notes, ledes_format, submitted_at, approved_at, paid_at, "
        "created_at, updated_at "
        "FROM invoices WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_invoice_t* invoice = (regislex_invoice_t*)calloc(1, sizeof(regislex_invoice_t));
    if (!invoice) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(invoice->id.value, str, sizeof(invoice->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(invoice->invoice_number, str, sizeof(invoice->invoice_number) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(invoice->vendor_id.value, str, sizeof(invoice->vendor_id.value) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(invoice->case_id.value, str, sizeof(invoice->case_id.value) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) regislex_datetime_parse(str, &invoice->invoice_date);

    str = regislex_db_column_text(stmt, 5);
    if (str) regislex_datetime_parse(str, &invoice->period_start);

    str = regislex_db_column_text(stmt, 6);
    if (str) regislex_datetime_parse(str, &invoice->period_end);

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &invoice->due_date);

    invoice->status = (regislex_invoice_status_t)regislex_db_column_int(stmt, 8);

    str = regislex_db_column_text(stmt, 9);
    if (str) strncpy(invoice->currency, str, sizeof(invoice->currency) - 1);

    invoice->fees_amount = regislex_db_column_int64(stmt, 10);
    invoice->expenses_amount = regislex_db_column_int64(stmt, 11);
    invoice->adjustments_amount = regislex_db_column_int64(stmt, 12);
    invoice->tax_amount = regislex_db_column_int64(stmt, 13);
    invoice->total_amount = regislex_db_column_int64(stmt, 14);

    str = regislex_db_column_text(stmt, 15);
    if (str) strncpy(invoice->notes, str, sizeof(invoice->notes) - 1);

    invoice->ledes_format = (regislex_ledes_format_t)regislex_db_column_int(stmt, 16);

    str = regislex_db_column_text(stmt, 17);
    if (str) regislex_datetime_parse(str, &invoice->submitted_at);

    str = regislex_db_column_text(stmt, 18);
    if (str) regislex_datetime_parse(str, &invoice->approved_at);

    str = regislex_db_column_text(stmt, 19);
    if (str) regislex_datetime_parse(str, &invoice->paid_at);

    str = regislex_db_column_text(stmt, 20);
    if (str) regislex_datetime_parse(str, &invoice->created_at);

    str = regislex_db_column_text(stmt, 21);
    if (str) regislex_datetime_parse(str, &invoice->updated_at);

    regislex_db_finalize(stmt);

    *out_invoice = invoice;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_invoice_update_status(
    regislex_context_t* ctx,
    const regislex_uuid_t* invoice_id,
    regislex_invoice_status_t new_status,
    const char* reviewer_notes)
{
    if (!ctx || !invoice_id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    /* Determine which timestamp to update */
    const char* timestamp_col = NULL;
    switch (new_status) {
        case REGISLEX_INVOICE_SUBMITTED: timestamp_col = "submitted_at"; break;
        case REGISLEX_INVOICE_APPROVED: timestamp_col = "approved_at"; break;
        case REGISLEX_INVOICE_PAID: timestamp_col = "paid_at"; break;
        default: break;
    }

    char sql[512];
    if (timestamp_col) {
        snprintf(sql, sizeof(sql),
                 "UPDATE invoices SET status = ?, %s = ?, reviewer_notes = ?, updated_at = ? WHERE id = ?",
                 timestamp_col);
    } else {
        snprintf(sql, sizeof(sql),
                 "UPDATE invoices SET status = ?, reviewer_notes = ?, updated_at = ? WHERE id = ?");
    }

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    int param = 1;
    regislex_db_bind_int(stmt, param++, new_status);
    if (timestamp_col) {
        regislex_db_bind_text(stmt, param++, now_str);
    }
    if (reviewer_notes) {
        regislex_db_bind_text(stmt, param++, reviewer_notes);
    } else {
        regislex_db_bind_null(stmt, param++);
    }
    regislex_db_bind_text(stmt, param++, now_str);
    regislex_db_bind_text(stmt, param++, invoice_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_invoice_list(
    regislex_context_t* ctx,
    regislex_invoice_filter_t* filter,
    regislex_invoice_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[2048];
    char where_clause[1024] = "WHERE 1=1";

    if (filter) {
        if (filter->vendor_id.value[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND vendor_id = '%s'", filter->vendor_id.value);
            strcat(where_clause, buf);
        }

        if (filter->case_id.value[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND case_id = '%s'", filter->case_id.value);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        }

        if (filter->date_from.year > 0) {
            char date_str[32];
            regislex_datetime_format(&filter->date_from, date_str, sizeof(date_str));
            char buf[128];
            snprintf(buf, sizeof(buf), " AND invoice_date >= '%s'", date_str);
            strcat(where_clause, buf);
        }

        if (filter->date_to.year > 0) {
            char date_str[32];
            regislex_datetime_format(&filter->date_to, date_str, sizeof(date_str));
            char buf[128];
            snprintf(buf, sizeof(buf), " AND invoice_date <= '%s'", date_str);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT i.id, i.invoice_number, i.vendor_id, v.name as vendor_name, "
             "i.case_id, i.invoice_date, i.due_date, i.status, i.currency, "
             "i.total_amount, i.created_at "
             "FROM invoices i "
             "LEFT JOIN vendors v ON i.vendor_id = v.id "
             "%s ORDER BY i.invoice_date DESC", where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_invoice_list_t* list = (regislex_invoice_list_t*)calloc(1,
        sizeof(regislex_invoice_list_t) + count * sizeof(regislex_invoice_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->invoices = (regislex_invoice_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->invoices[i].id.value, str, sizeof(list->invoices[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->invoices[i].invoice_number, str, sizeof(list->invoices[i].invoice_number) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->invoices[i].vendor_id.value, str, sizeof(list->invoices[i].vendor_id.value) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->invoices[i].case_id.value, str, sizeof(list->invoices[i].case_id.value) - 1);

        str = regislex_db_result_get_text(result, 5);
        if (str) regislex_datetime_parse(str, &list->invoices[i].invoice_date);

        str = regislex_db_result_get_text(result, 6);
        if (str) regislex_datetime_parse(str, &list->invoices[i].due_date);

        list->invoices[i].status = (regislex_invoice_status_t)regislex_db_result_get_int(result, 7);

        str = regislex_db_result_get_text(result, 8);
        if (str) strncpy(list->invoices[i].currency, str, sizeof(list->invoices[i].currency) - 1);

        list->invoices[i].total_amount = regislex_db_result_get_int64(result, 9);

        str = regislex_db_result_get_text(result, 10);
        if (str) regislex_datetime_parse(str, &list->invoices[i].created_at);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_invoice_free(regislex_invoice_t* invoice) {
    if (invoice) {
        free(invoice->line_items);
        free(invoice);
    }
}

void regislex_invoice_list_free(regislex_invoice_list_t* list) {
    free(list);
}

/* ============================================================================
 * Invoice Line Items
 * ========================================================================== */

regislex_error_t regislex_invoice_add_line_item(
    regislex_context_t* ctx,
    const regislex_uuid_t* invoice_id,
    regislex_invoice_line_t* line_item)
{
    if (!ctx || !invoice_id || !line_item) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_uuid_t id;
    if (line_item->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = line_item->id;
    }

    char work_date_str[32];
    regislex_datetime_format(&line_item->work_date, work_date_str, sizeof(work_date_str));

    const char* sql =
        "INSERT INTO invoice_lines (id, invoice_id, line_type, utbms_code, "
        "description, timekeeper_id, timekeeper_name, work_date, hours, "
        "rate, amount) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, invoice_id->value);
    regislex_db_bind_int(stmt, 3, line_item->line_type);
    regislex_db_bind_text(stmt, 4, line_item->utbms_code);
    regislex_db_bind_text(stmt, 5, line_item->description);

    if (line_item->timekeeper_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 6, line_item->timekeeper_id.value);
    } else {
        regislex_db_bind_null(stmt, 6);
    }

    regislex_db_bind_text(stmt, 7, line_item->timekeeper_name);
    regislex_db_bind_text(stmt, 8, work_date_str);
    regislex_db_bind_double(stmt, 9, line_item->hours);
    regislex_db_bind_int64(stmt, 10, line_item->rate);
    regislex_db_bind_int64(stmt, 11, line_item->amount);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err == REGISLEX_SUCCESS) {
        line_item->id = id;
    }

    return err;
}

regislex_error_t regislex_invoice_get_line_items(
    regislex_context_t* ctx,
    const regislex_uuid_t* invoice_id,
    regislex_invoice_line_list_t** out_list)
{
    if (!ctx || !invoice_id || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, invoice_id, line_type, utbms_code, description, "
             "timekeeper_id, timekeeper_name, work_date, hours, rate, amount "
             "FROM invoice_lines WHERE invoice_id = '%s' ORDER BY work_date, id",
             invoice_id->value);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_invoice_line_list_t* list = (regislex_invoice_line_list_t*)calloc(1,
        sizeof(regislex_invoice_line_list_t) + count * sizeof(regislex_invoice_line_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->items = (regislex_invoice_line_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->items[i].id.value, str, sizeof(list->items[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->items[i].invoice_id.value, str, sizeof(list->items[i].invoice_id.value) - 1);

        list->items[i].line_type = (regislex_line_type_t)regislex_db_result_get_int(result, 2);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->items[i].utbms_code, str, sizeof(list->items[i].utbms_code) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->items[i].description, str, sizeof(list->items[i].description) - 1);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->items[i].timekeeper_id.value, str, sizeof(list->items[i].timekeeper_id.value) - 1);

        str = regislex_db_result_get_text(result, 6);
        if (str) strncpy(list->items[i].timekeeper_name, str, sizeof(list->items[i].timekeeper_name) - 1);

        str = regislex_db_result_get_text(result, 7);
        if (str) regislex_datetime_parse(str, &list->items[i].work_date);

        list->items[i].hours = regislex_db_result_get_double(result, 8);
        list->items[i].rate = regislex_db_result_get_int64(result, 9);
        list->items[i].amount = regislex_db_result_get_int64(result, 10);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_invoice_line_list_free(regislex_invoice_line_list_t* list) {
    free(list);
}

/* ============================================================================
 * LEDES Import/Export
 * ========================================================================== */

regislex_error_t regislex_invoice_import_ledes(
    regislex_context_t* ctx,
    const char* file_path,
    regislex_ledes_format_t format,
    regislex_invoice_t** out_invoice)
{
    if (!ctx || !file_path || !out_invoice) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_invoice = NULL;

    /* Read file */
    FILE* f = fopen(file_path, "r");
    if (!f) {
        return REGISLEX_ERROR_IO;
    }

    /* Parse based on format */
    regislex_invoice_t invoice_data = {0};

    switch (format) {
        case REGISLEX_LEDES_1998B:
            /* LEDES 1998B is pipe-delimited */
            {
                char line[4096];
                bool header_parsed = false;

                while (fgets(line, sizeof(line), f)) {
                    /* Remove newline */
                    char* nl = strchr(line, '\n');
                    if (nl) *nl = '\0';

                    if (!header_parsed) {
                        /* Skip header line */
                        header_parsed = true;
                        continue;
                    }

                    /* Parse pipe-delimited fields */
                    /* LEDES 1998B format: INVOICE_DATE|INVOICE_NUMBER|... */
                    char* fields[30];
                    int field_count = 0;
                    char* token = strtok(line, "|");
                    while (token && field_count < 30) {
                        fields[field_count++] = token;
                        token = strtok(NULL, "|");
                    }

                    if (field_count >= 10) {
                        /* Extract key fields */
                        if (fields[1]) strncpy(invoice_data.invoice_number, fields[1], sizeof(invoice_data.invoice_number) - 1);
                        /* ... more field parsing would go here */
                    }
                }
            }
            break;

        case REGISLEX_LEDES_XML_20:
        case REGISLEX_LEDES_XML_21:
            /* XML parsing would require an XML library */
            fclose(f);
            return REGISLEX_ERROR_NOT_IMPLEMENTED;

        default:
            fclose(f);
            return REGISLEX_ERROR_INVALID_PARAM;
    }

    fclose(f);

    invoice_data.ledes_format = format;

    return regislex_invoice_create(ctx, &invoice_data, out_invoice);
}

regislex_error_t regislex_invoice_export_ledes(
    regislex_context_t* ctx,
    const regislex_uuid_t* invoice_id,
    regislex_ledes_format_t format,
    const char* output_path)
{
    if (!ctx || !invoice_id || !output_path) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Get invoice */
    regislex_invoice_t* invoice = NULL;
    regislex_error_t err = regislex_invoice_get(ctx, invoice_id, &invoice);
    if (err != REGISLEX_SUCCESS) return err;

    /* Get line items */
    regislex_invoice_line_list_t* lines = NULL;
    err = regislex_invoice_get_line_items(ctx, invoice_id, &lines);
    if (err != REGISLEX_SUCCESS) {
        regislex_invoice_free(invoice);
        return err;
    }

    /* Get vendor */
    regislex_vendor_t* vendor = NULL;
    regislex_vendor_get(ctx, &invoice->vendor_id, &vendor);

    FILE* f = fopen(output_path, "w");
    if (!f) {
        regislex_invoice_free(invoice);
        regislex_invoice_line_list_free(lines);
        regislex_vendor_free(vendor);
        return REGISLEX_ERROR_IO;
    }

    switch (format) {
        case REGISLEX_LEDES_1998B:
            /* Write LEDES 1998B header */
            fprintf(f, "INVOICE_DATE|INVOICE_NUMBER|CLIENT_ID|CLIENT_MATTER_ID|LAW_FIRM_MATTER_ID|"
                       "INVOICE_TOTAL|BILLING_START_DATE|BILLING_END_DATE|INVOICE_DESCRIPTION|"
                       "LINE_ITEM_NUMBER|EXP/FEE/INV_ADJ_TYPE|LINE_ITEM_NUMBER_OF_UNITS|"
                       "LINE_ITEM_ADJUSTMENT_AMOUNT|LINE_ITEM_TOTAL|LINE_ITEM_DATE|LINE_ITEM_TASK_CODE|"
                       "LINE_ITEM_EXPENSE_CODE|LINE_ITEM_ACTIVITY_CODE|TIMEKEEPER_ID|LINE_ITEM_DESCRIPTION|"
                       "LAW_FIRM_ID|LINE_ITEM_UNIT_COST|TIMEKEEPER_NAME|TIMEKEEPER_CLASSIFICATION|[]\n");

            /* Write line items */
            char invoice_date_str[32];
            regislex_datetime_format(&invoice->invoice_date, invoice_date_str, sizeof(invoice_date_str));

            char period_start_str[32];
            regislex_datetime_format(&invoice->period_start, period_start_str, sizeof(period_start_str));

            char period_end_str[32];
            regislex_datetime_format(&invoice->period_end, period_end_str, sizeof(period_end_str));

            for (size_t i = 0; lines && i < lines->count; i++) {
                regislex_invoice_line_t* line = &lines->items[i];

                char work_date_str[32];
                regislex_datetime_format(&line->work_date, work_date_str, sizeof(work_date_str));

                const char* line_type_code = "F"; /* Fee */
                if (line->line_type == REGISLEX_LINE_EXPENSE) line_type_code = "E";
                else if (line->line_type == REGISLEX_LINE_ADJUSTMENT) line_type_code = "IF";

                fprintf(f, "%s|%s||%s||%.2f|%s|%s||%zu|%s|%.2f|0|%.2f|%s|%s|||%s|%s|%s|%.2f|%s||[]\n",
                        invoice_date_str,
                        invoice->invoice_number,
                        invoice->case_id.value,
                        invoice->total_amount / 100.0,
                        period_start_str,
                        period_end_str,
                        i + 1,
                        line_type_code,
                        line->hours,
                        line->amount / 100.0,
                        work_date_str,
                        line->utbms_code,
                        line->timekeeper_id.value,
                        line->description,
                        vendor ? vendor->tax_id : "",
                        line->rate / 100.0,
                        line->timekeeper_name);
            }
            break;

        case REGISLEX_LEDES_XML_20:
        case REGISLEX_LEDES_XML_21:
            /* XML export would go here */
            fclose(f);
            regislex_invoice_free(invoice);
            regislex_invoice_line_list_free(lines);
            regislex_vendor_free(vendor);
            return REGISLEX_ERROR_NOT_IMPLEMENTED;

        default:
            fclose(f);
            regislex_invoice_free(invoice);
            regislex_invoice_line_list_free(lines);
            regislex_vendor_free(vendor);
            return REGISLEX_ERROR_INVALID_PARAM;
    }

    fclose(f);
    regislex_invoice_free(invoice);
    regislex_invoice_line_list_free(lines);
    regislex_vendor_free(vendor);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Budget Management
 * ========================================================================== */

regislex_error_t regislex_budget_create(
    regislex_context_t* ctx,
    regislex_budget_t* budget_data,
    regislex_budget_t** out_budget)
{
    if (!ctx || !budget_data || !out_budget) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_budget = NULL;

    regislex_uuid_t id;
    if (budget_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = budget_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char period_start_str[32];
    regislex_datetime_format(&budget_data->period_start, period_start_str, sizeof(period_start_str));

    char period_end_str[32];
    regislex_datetime_format(&budget_data->period_end, period_end_str, sizeof(period_end_str));

    const char* sql =
        "INSERT INTO budgets (id, case_id, name, budget_type, currency, "
        "total_budget, fees_budget, expenses_budget, period_start, period_end, "
        "notes, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, budget_data->case_id.value);
    regislex_db_bind_text(stmt, 3, budget_data->name);
    regislex_db_bind_int(stmt, 4, budget_data->budget_type);
    regislex_db_bind_text(stmt, 5, budget_data->currency);
    regislex_db_bind_int64(stmt, 6, budget_data->total_budget);
    regislex_db_bind_int64(stmt, 7, budget_data->fees_budget);
    regislex_db_bind_int64(stmt, 8, budget_data->expenses_budget);
    regislex_db_bind_text(stmt, 9, period_start_str);
    regislex_db_bind_text(stmt, 10, period_end_str);
    regislex_db_bind_text(stmt, 11, budget_data->notes);
    regislex_db_bind_text(stmt, 12, now_str);
    regislex_db_bind_text(stmt, 13, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_budget_get(ctx, &id, out_budget);
}

regislex_error_t regislex_budget_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_budget_t** out_budget)
{
    if (!ctx || !id || !out_budget) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_budget = NULL;

    const char* sql =
        "SELECT b.id, b.case_id, b.name, b.budget_type, b.currency, "
        "b.total_budget, b.fees_budget, b.expenses_budget, "
        "b.period_start, b.period_end, b.notes, "
        "COALESCE(SUM(CASE WHEN i.status = 'paid' THEN i.fees_amount ELSE 0 END), 0) as actual_fees, "
        "COALESCE(SUM(CASE WHEN i.status = 'paid' THEN i.expenses_amount ELSE 0 END), 0) as actual_expenses, "
        "b.created_at, b.updated_at "
        "FROM budgets b "
        "LEFT JOIN invoices i ON i.case_id = b.case_id "
        "WHERE b.id = ? "
        "GROUP BY b.id";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_budget_t* budget = (regislex_budget_t*)calloc(1, sizeof(regislex_budget_t));
    if (!budget) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(budget->id.value, str, sizeof(budget->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(budget->case_id.value, str, sizeof(budget->case_id.value) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(budget->name, str, sizeof(budget->name) - 1);

    budget->budget_type = (regislex_budget_type_t)regislex_db_column_int(stmt, 3);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(budget->currency, str, sizeof(budget->currency) - 1);

    budget->total_budget = regislex_db_column_int64(stmt, 5);
    budget->fees_budget = regislex_db_column_int64(stmt, 6);
    budget->expenses_budget = regislex_db_column_int64(stmt, 7);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &budget->period_start);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &budget->period_end);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(budget->notes, str, sizeof(budget->notes) - 1);

    budget->actual_fees = regislex_db_column_int64(stmt, 11);
    budget->actual_expenses = regislex_db_column_int64(stmt, 12);
    budget->actual_total = budget->actual_fees + budget->actual_expenses;

    /* Calculate variance */
    budget->variance_fees = budget->actual_fees - budget->fees_budget;
    budget->variance_expenses = budget->actual_expenses - budget->expenses_budget;
    budget->variance_total = budget->actual_total - budget->total_budget;

    str = regislex_db_column_text(stmt, 13);
    if (str) regislex_datetime_parse(str, &budget->created_at);

    str = regislex_db_column_text(stmt, 14);
    if (str) regislex_datetime_parse(str, &budget->updated_at);

    regislex_db_finalize(stmt);

    *out_budget = budget;
    return REGISLEX_SUCCESS;
}

void regislex_budget_free(regislex_budget_t* budget) {
    free(budget);
}

/* ============================================================================
 * Contract Management
 * ========================================================================== */

regislex_error_t regislex_contract_create(
    regislex_context_t* ctx,
    regislex_contract_t* contract_data,
    regislex_contract_t** out_contract)
{
    if (!ctx || !contract_data || !out_contract) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_contract = NULL;

    regislex_uuid_t id;
    if (contract_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = contract_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char eff_date_str[32];
    regislex_datetime_format(&contract_data->effective_date, eff_date_str, sizeof(eff_date_str));

    char exp_date_str[32];
    regislex_datetime_format(&contract_data->expiration_date, exp_date_str, sizeof(exp_date_str));

    const char* sql =
        "INSERT INTO contracts (id, contract_number, title, description, "
        "contract_type, vendor_id, case_id, status, effective_date, "
        "expiration_date, auto_renew, renewal_notice_days, currency, "
        "total_value, payment_terms, document_id, notes, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, contract_data->contract_number);
    regislex_db_bind_text(stmt, 3, contract_data->title);
    regislex_db_bind_text(stmt, 4, contract_data->description);
    regislex_db_bind_int(stmt, 5, contract_data->contract_type);

    if (contract_data->vendor_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 6, contract_data->vendor_id.value);
    } else {
        regislex_db_bind_null(stmt, 6);
    }

    if (contract_data->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 7, contract_data->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 7);
    }

    regislex_db_bind_int(stmt, 8, contract_data->status);
    regislex_db_bind_text(stmt, 9, eff_date_str);
    regislex_db_bind_text(stmt, 10, exp_date_str);
    regislex_db_bind_int(stmt, 11, contract_data->auto_renew ? 1 : 0);
    regislex_db_bind_int(stmt, 12, contract_data->renewal_notice_days);
    regislex_db_bind_text(stmt, 13, contract_data->total_value.currency);
    regislex_db_bind_int64(stmt, 14, contract_data->total_value.amount);
    regislex_db_bind_text(stmt, 15, contract_data->payment_terms);

    if (contract_data->document_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 16, contract_data->document_id.value);
    } else {
        regislex_db_bind_null(stmt, 16);
    }

    regislex_db_bind_text(stmt, 17, contract_data->notes);
    regislex_db_bind_text(stmt, 18, now_str);
    regislex_db_bind_text(stmt, 19, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_contract_get(ctx, &id, out_contract);
}

regislex_error_t regislex_contract_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_contract_t** out_contract)
{
    if (!ctx || !id || !out_contract) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_contract = NULL;

    const char* sql =
        "SELECT id, contract_number, title, description, contract_type, "
        "vendor_id, case_id, status, effective_date, expiration_date, "
        "auto_renew, renewal_notice_days, currency, total_value, "
        "payment_terms, document_id, notes, created_at, updated_at "
        "FROM contracts WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_contract_t* contract = (regislex_contract_t*)calloc(1, sizeof(regislex_contract_t));
    if (!contract) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(contract->id.value, str, sizeof(contract->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(contract->contract_number, str, sizeof(contract->contract_number) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(contract->title, str, sizeof(contract->title) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(contract->description, str, sizeof(contract->description) - 1);

    contract->contract_type = (regislex_contract_type_t)regislex_db_column_int(stmt, 4);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(contract->vendor_id.value, str, sizeof(contract->vendor_id.value) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(contract->case_id.value, str, sizeof(contract->case_id.value) - 1);

    contract->status = (regislex_contract_status_t)regislex_db_column_int(stmt, 7);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &contract->effective_date);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &contract->expiration_date);

    contract->auto_renew = regislex_db_column_int(stmt, 10) != 0;
    contract->renewal_notice_days = regislex_db_column_int(stmt, 11);

    str = regislex_db_column_text(stmt, 12);
    if (str) strncpy(contract->total_value.currency, str, sizeof(contract->total_value.currency) - 1);

    contract->total_value.amount = regislex_db_column_int64(stmt, 13);

    str = regislex_db_column_text(stmt, 14);
    if (str) strncpy(contract->payment_terms, str, sizeof(contract->payment_terms) - 1);

    str = regislex_db_column_text(stmt, 15);
    if (str) strncpy(contract->document_id.value, str, sizeof(contract->document_id.value) - 1);

    str = regislex_db_column_text(stmt, 16);
    if (str) strncpy(contract->notes, str, sizeof(contract->notes) - 1);

    str = regislex_db_column_text(stmt, 17);
    if (str) regislex_datetime_parse(str, &contract->created_at);

    str = regislex_db_column_text(stmt, 18);
    if (str) regislex_datetime_parse(str, &contract->updated_at);

    regislex_db_finalize(stmt);

    *out_contract = contract;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_contract_list_expiring(
    regislex_context_t* ctx,
    int days_ahead,
    regislex_contract_list_t** out_list)
{
    if (!ctx || !out_list || days_ahead < 0) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, contract_number, title, contract_type, vendor_id, "
             "status, effective_date, expiration_date, auto_renew, total_value "
             "FROM contracts "
             "WHERE status = %d AND expiration_date <= date('now', '+%d days') "
             "ORDER BY expiration_date ASC",
             REGISLEX_CONTRACT_ACTIVE, days_ahead);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_contract_list_t* list = (regislex_contract_list_t*)calloc(1,
        sizeof(regislex_contract_list_t) + count * sizeof(regislex_contract_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->contracts = (regislex_contract_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->contracts[i].id.value, str, sizeof(list->contracts[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->contracts[i].contract_number, str, sizeof(list->contracts[i].contract_number) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->contracts[i].title, str, sizeof(list->contracts[i].title) - 1);

        list->contracts[i].contract_type = (regislex_contract_type_t)regislex_db_result_get_int(result, 3);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->contracts[i].vendor_id.value, str, sizeof(list->contracts[i].vendor_id.value) - 1);

        list->contracts[i].status = (regislex_contract_status_t)regislex_db_result_get_int(result, 5);

        str = regislex_db_result_get_text(result, 6);
        if (str) regislex_datetime_parse(str, &list->contracts[i].effective_date);

        str = regislex_db_result_get_text(result, 7);
        if (str) regislex_datetime_parse(str, &list->contracts[i].expiration_date);

        list->contracts[i].auto_renew = regislex_db_result_get_int(result, 8) != 0;
        list->contracts[i].total_value.amount = regislex_db_result_get_int64(result, 9);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_contract_free(regislex_contract_t* contract) {
    free(contract);
}

void regislex_contract_list_free(regislex_contract_list_t* list) {
    free(list);
}

/* ============================================================================
 * Risk Management
 * ========================================================================== */

regislex_error_t regislex_risk_create(
    regislex_context_t* ctx,
    regislex_risk_t* risk_data,
    regislex_risk_t** out_risk)
{
    if (!ctx || !risk_data || !out_risk) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_risk = NULL;

    regislex_uuid_t id;
    if (risk_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = risk_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char identified_date_str[32];
    regislex_datetime_format(&risk_data->identified_date, identified_date_str, sizeof(identified_date_str));

    const char* sql =
        "INSERT INTO risks (id, title, description, risk_category, "
        "case_id, likelihood, impact, risk_level, status, identified_date, "
        "identified_by_id, owner_id, mitigation_plan, contingency_plan, "
        "currency, potential_exposure, notes, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, risk_data->title);
    regislex_db_bind_text(stmt, 3, risk_data->description);
    regislex_db_bind_int(stmt, 4, risk_data->risk_category);

    if (risk_data->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 5, risk_data->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 5);
    }

    regislex_db_bind_int(stmt, 6, risk_data->likelihood);
    regislex_db_bind_int(stmt, 7, risk_data->impact);
    regislex_db_bind_int(stmt, 8, risk_data->risk_level);
    regislex_db_bind_int(stmt, 9, risk_data->status);
    regislex_db_bind_text(stmt, 10, identified_date_str);
    regislex_db_bind_text(stmt, 11, risk_data->identified_by_id.value);
    regislex_db_bind_text(stmt, 12, risk_data->owner_id.value);
    regislex_db_bind_text(stmt, 13, risk_data->mitigation_plan);
    regislex_db_bind_text(stmt, 14, risk_data->contingency_plan);
    regislex_db_bind_text(stmt, 15, risk_data->potential_exposure.currency);
    regislex_db_bind_int64(stmt, 16, risk_data->potential_exposure.amount);
    regislex_db_bind_text(stmt, 17, risk_data->notes);
    regislex_db_bind_text(stmt, 18, now_str);
    regislex_db_bind_text(stmt, 19, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_risk_get(ctx, &id, out_risk);
}

regislex_error_t regislex_risk_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_risk_t** out_risk)
{
    if (!ctx || !id || !out_risk) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_risk = NULL;

    const char* sql =
        "SELECT id, title, description, risk_category, case_id, "
        "likelihood, impact, risk_level, status, identified_date, "
        "identified_by_id, owner_id, mitigation_plan, contingency_plan, "
        "currency, potential_exposure, notes, review_date, "
        "created_at, updated_at "
        "FROM risks WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_risk_t* risk = (regislex_risk_t*)calloc(1, sizeof(regislex_risk_t));
    if (!risk) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(risk->id.value, str, sizeof(risk->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(risk->title, str, sizeof(risk->title) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(risk->description, str, sizeof(risk->description) - 1);

    risk->risk_category = (regislex_risk_category_t)regislex_db_column_int(stmt, 3);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(risk->case_id.value, str, sizeof(risk->case_id.value) - 1);

    risk->likelihood = regislex_db_column_int(stmt, 5);
    risk->impact = regislex_db_column_int(stmt, 6);
    risk->risk_level = (regislex_risk_level_t)regislex_db_column_int(stmt, 7);
    risk->status = (regislex_risk_status_t)regislex_db_column_int(stmt, 8);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &risk->identified_date);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(risk->identified_by_id.value, str, sizeof(risk->identified_by_id.value) - 1);

    str = regislex_db_column_text(stmt, 11);
    if (str) strncpy(risk->owner_id.value, str, sizeof(risk->owner_id.value) - 1);

    str = regislex_db_column_text(stmt, 12);
    if (str) strncpy(risk->mitigation_plan, str, sizeof(risk->mitigation_plan) - 1);

    str = regislex_db_column_text(stmt, 13);
    if (str) strncpy(risk->contingency_plan, str, sizeof(risk->contingency_plan) - 1);

    str = regislex_db_column_text(stmt, 14);
    if (str) strncpy(risk->potential_exposure.currency, str, sizeof(risk->potential_exposure.currency) - 1);

    risk->potential_exposure.amount = regislex_db_column_int64(stmt, 15);

    str = regislex_db_column_text(stmt, 16);
    if (str) strncpy(risk->notes, str, sizeof(risk->notes) - 1);

    str = regislex_db_column_text(stmt, 17);
    if (str) regislex_datetime_parse(str, &risk->review_date);

    str = regislex_db_column_text(stmt, 18);
    if (str) regislex_datetime_parse(str, &risk->created_at);

    str = regislex_db_column_text(stmt, 19);
    if (str) regislex_datetime_parse(str, &risk->updated_at);

    regislex_db_finalize(stmt);

    *out_risk = risk;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_risk_list(
    regislex_context_t* ctx,
    regislex_risk_filter_t* filter,
    regislex_risk_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[1024];
    char where_clause[512] = "WHERE 1=1";

    if (filter) {
        if (filter->case_id.value[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND case_id = '%s'", filter->case_id.value);
            strcat(where_clause, buf);
        }

        if (filter->risk_level >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND risk_level = %d", filter->risk_level);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        }

        if (filter->category >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND risk_category = %d", filter->category);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, title, risk_category, case_id, likelihood, impact, "
             "risk_level, status, potential_exposure, identified_date "
             "FROM risks %s ORDER BY risk_level DESC, potential_exposure DESC",
             where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_risk_list_t* list = (regislex_risk_list_t*)calloc(1,
        sizeof(regislex_risk_list_t) + count * sizeof(regislex_risk_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->risks = (regislex_risk_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->risks[i].id.value, str, sizeof(list->risks[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->risks[i].title, str, sizeof(list->risks[i].title) - 1);

        list->risks[i].risk_category = (regislex_risk_category_t)regislex_db_result_get_int(result, 2);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->risks[i].case_id.value, str, sizeof(list->risks[i].case_id.value) - 1);

        list->risks[i].likelihood = regislex_db_result_get_int(result, 4);
        list->risks[i].impact = regislex_db_result_get_int(result, 5);
        list->risks[i].risk_level = (regislex_risk_level_t)regislex_db_result_get_int(result, 6);
        list->risks[i].status = (regislex_risk_status_t)regislex_db_result_get_int(result, 7);
        list->risks[i].potential_exposure.amount = regislex_db_result_get_int64(result, 8);

        str = regislex_db_result_get_text(result, 9);
        if (str) regislex_datetime_parse(str, &list->risks[i].identified_date);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_risk_free(regislex_risk_t* risk) {
    free(risk);
}

void regislex_risk_list_free(regislex_risk_list_t* list) {
    free(list);
}
