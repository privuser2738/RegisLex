/**
 * RegisLex - Enterprise Legal Software Suite
 * Reporting Engine Implementation
 *
 * Provides comprehensive reporting and analytics capabilities including
 * caseload summaries, attorney performance metrics, deadline compliance,
 * financial reports, and custom report generation.
 */

#include "regislex/modules/reporting/reporting.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Internal Structures
 * ========================================================================== */

typedef struct report_context {
    regislex_context_t* ctx;
    regislex_report_t* report;
    regislex_report_params_t* params;
    char* output_buffer;
    size_t output_size;
    size_t output_capacity;
} report_context_t;

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

static void append_output(report_context_t* rctx, const char* text) {
    if (!rctx || !text) return;

    size_t len = strlen(text);
    if (rctx->output_size + len + 1 > rctx->output_capacity) {
        size_t new_capacity = (rctx->output_capacity == 0) ? 4096 : rctx->output_capacity * 2;
        while (new_capacity < rctx->output_size + len + 1) {
            new_capacity *= 2;
        }
        char* new_buffer = (char*)realloc(rctx->output_buffer, new_capacity);
        if (!new_buffer) return;
        rctx->output_buffer = new_buffer;
        rctx->output_capacity = new_capacity;
    }

    memcpy(rctx->output_buffer + rctx->output_size, text, len);
    rctx->output_size += len;
    rctx->output_buffer[rctx->output_size] = '\0';
}

static void append_output_line(report_context_t* rctx, const char* text) {
    append_output(rctx, text);
    append_output(rctx, "\n");
}

static void append_output_fmt(report_context_t* rctx, const char* fmt, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    append_output(rctx, buffer);
}

static const char* format_money(regislex_money_t* money, char* buffer, size_t size) {
    if (!money || !buffer) return "";
    snprintf(buffer, size, "%s %.2f", money->currency, money->amount / 100.0);
    return buffer;
}

/* ============================================================================
 * Report Generation - Caseload Summary
 * ========================================================================== */

static regislex_error_t generate_caseload_summary(report_context_t* rctx) {
    regislex_db_result_t* result = NULL;
    char date_from[32] = {0};
    char date_to[32] = {0};

    if (rctx->params) {
        regislex_datetime_format(&rctx->params->date_from, date_from, sizeof(date_from));
        regislex_datetime_format(&rctx->params->date_to, date_to, sizeof(date_to));
    }

    append_output_line(rctx, "================================================================================");
    append_output_line(rctx, "                         CASELOAD SUMMARY REPORT");
    append_output_line(rctx, "================================================================================");
    append_output_fmt(rctx, "Report Period: %s to %s\n\n",
                      date_from[0] ? date_from : "All Time",
                      date_to[0] ? date_to : "Present");

    /* Total cases by status */
    const char* status_sql =
        "SELECT status, COUNT(*) as count FROM cases "
        "GROUP BY status ORDER BY count DESC";

    regislex_error_t err = regislex_db_query(rctx->ctx, status_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "CASES BY STATUS");
        append_output_line(rctx, "---------------");

        int total = 0;
        while (regislex_db_result_next(result)) {
            int status = regislex_db_result_get_int(result, 0);
            int count = regislex_db_result_get_int(result, 1);
            total += count;

            const char* status_name = "Unknown";
            switch (status) {
                case 0: status_name = "Draft"; break;
                case 1: status_name = "Active"; break;
                case 2: status_name = "Pending"; break;
                case 3: status_name = "On Hold"; break;
                case 4: status_name = "Closed"; break;
                case 5: status_name = "Archived"; break;
            }
            append_output_fmt(rctx, "  %-15s: %d\n", status_name, count);
        }
        append_output_fmt(rctx, "  %-15s: %d\n\n", "TOTAL", total);
        regislex_db_result_free(result);
    }

    /* Cases by type */
    const char* type_sql =
        "SELECT type, COUNT(*) as count FROM cases "
        "GROUP BY type ORDER BY count DESC";

    err = regislex_db_query(rctx->ctx, type_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "CASES BY TYPE");
        append_output_line(rctx, "-------------");

        while (regislex_db_result_next(result)) {
            int type = regislex_db_result_get_int(result, 0);
            int count = regislex_db_result_get_int(result, 1);

            const char* type_name = "Other";
            switch (type) {
                case 0: type_name = "Civil"; break;
                case 1: type_name = "Criminal"; break;
                case 2: type_name = "Administrative"; break;
                case 3: type_name = "Regulatory"; break;
                case 4: type_name = "Contract"; break;
                case 5: type_name = "Employment"; break;
                case 6: type_name = "IP"; break;
                case 7: type_name = "Real Estate"; break;
                case 8: type_name = "Tax"; break;
                case 9: type_name = "Bankruptcy"; break;
            }
            append_output_fmt(rctx, "  %-15s: %d\n", type_name, count);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    /* Cases opened this period */
    if (date_from[0] && date_to[0]) {
        char query[512];
        snprintf(query, sizeof(query),
                 "SELECT COUNT(*) FROM cases WHERE created_at >= '%s' AND created_at <= '%s'",
                 date_from, date_to);

        err = regislex_db_query(rctx->ctx, query, &result);
        if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
            int opened = regislex_db_result_get_int(result, 0);
            append_output_fmt(rctx, "Cases Opened This Period: %d\n", opened);
            regislex_db_result_free(result);
        }

        snprintf(query, sizeof(query),
                 "SELECT COUNT(*) FROM cases WHERE closed_at >= '%s' AND closed_at <= '%s'",
                 date_from, date_to);

        err = regislex_db_query(rctx->ctx, query, &result);
        if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
            int closed = regislex_db_result_get_int(result, 0);
            append_output_fmt(rctx, "Cases Closed This Period: %d\n", closed);
            regislex_db_result_free(result);
        }
    }

    append_output_line(rctx, "\n================================================================================");

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report Generation - Attorney Performance
 * ========================================================================== */

static regislex_error_t generate_attorney_performance(report_context_t* rctx) {
    regislex_db_result_t* result = NULL;

    append_output_line(rctx, "================================================================================");
    append_output_line(rctx, "                      ATTORNEY PERFORMANCE REPORT");
    append_output_line(rctx, "================================================================================\n");

    /* Cases per attorney */
    const char* attorney_cases_sql =
        "SELECT u.name, COUNT(DISTINCT c.id) as case_count "
        "FROM users u "
        "LEFT JOIN cases c ON c.assigned_attorney_id = u.id "
        "WHERE u.role = 'attorney' "
        "GROUP BY u.id ORDER BY case_count DESC";

    regislex_error_t err = regislex_db_query(rctx->ctx, attorney_cases_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "ACTIVE CASELOAD BY ATTORNEY");
        append_output_line(rctx, "---------------------------");
        append_output_fmt(rctx, "%-30s %10s\n", "Attorney", "Cases");
        append_output_line(rctx, "----------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* name = regislex_db_result_get_text(result, 0);
            int count = regislex_db_result_get_int(result, 1);
            append_output_fmt(rctx, "%-30s %10d\n", name ? name : "Unassigned", count);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    /* Tasks completed per attorney */
    const char* tasks_sql =
        "SELECT u.name, "
        "  SUM(CASE WHEN t.status = 'completed' THEN 1 ELSE 0 END) as completed, "
        "  SUM(CASE WHEN t.status = 'in_progress' THEN 1 ELSE 0 END) as in_progress, "
        "  SUM(CASE WHEN t.status = 'pending' THEN 1 ELSE 0 END) as pending "
        "FROM users u "
        "LEFT JOIN tasks t ON t.assigned_to_id = u.id "
        "WHERE u.role = 'attorney' "
        "GROUP BY u.id";

    err = regislex_db_query(rctx->ctx, tasks_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "TASK STATUS BY ATTORNEY");
        append_output_line(rctx, "-----------------------");
        append_output_fmt(rctx, "%-30s %10s %12s %10s\n",
                          "Attorney", "Completed", "In Progress", "Pending");
        append_output_line(rctx, "----------------------------------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* name = regislex_db_result_get_text(result, 0);
            int completed = regislex_db_result_get_int(result, 1);
            int in_progress = regislex_db_result_get_int(result, 2);
            int pending = regislex_db_result_get_int(result, 3);
            append_output_fmt(rctx, "%-30s %10d %12d %10d\n",
                              name ? name : "Unknown", completed, in_progress, pending);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    append_output_line(rctx, "================================================================================");

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report Generation - Deadline Compliance
 * ========================================================================== */

static regislex_error_t generate_deadline_compliance(report_context_t* rctx) {
    regislex_db_result_t* result = NULL;

    append_output_line(rctx, "================================================================================");
    append_output_line(rctx, "                      DEADLINE COMPLIANCE REPORT");
    append_output_line(rctx, "================================================================================\n");

    /* Overall compliance stats */
    const char* compliance_sql =
        "SELECT "
        "  SUM(CASE WHEN status = 'completed' AND completed_at <= due_date THEN 1 ELSE 0 END) as on_time, "
        "  SUM(CASE WHEN status = 'completed' AND completed_at > due_date THEN 1 ELSE 0 END) as late, "
        "  SUM(CASE WHEN status = 'missed' THEN 1 ELSE 0 END) as missed, "
        "  SUM(CASE WHEN status IN ('pending', 'in_progress') THEN 1 ELSE 0 END) as pending "
        "FROM deadlines";

    regislex_error_t err = regislex_db_query(rctx->ctx, compliance_sql, &result);
    if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
        int on_time = regislex_db_result_get_int(result, 0);
        int late = regislex_db_result_get_int(result, 1);
        int missed = regislex_db_result_get_int(result, 2);
        int pending = regislex_db_result_get_int(result, 3);

        int total_completed = on_time + late;
        double compliance_rate = total_completed > 0 ?
            (double)on_time / total_completed * 100.0 : 0.0;

        append_output_line(rctx, "OVERALL COMPLIANCE STATISTICS");
        append_output_line(rctx, "-----------------------------");
        append_output_fmt(rctx, "Deadlines Met On Time:  %d\n", on_time);
        append_output_fmt(rctx, "Deadlines Met Late:     %d\n", late);
        append_output_fmt(rctx, "Deadlines Missed:       %d\n", missed);
        append_output_fmt(rctx, "Deadlines Pending:      %d\n", pending);
        append_output_fmt(rctx, "Compliance Rate:        %.1f%%\n\n", compliance_rate);

        regislex_db_result_free(result);
    }

    /* Upcoming deadlines */
    const char* upcoming_sql =
        "SELECT d.title, d.due_date, d.type, c.case_number "
        "FROM deadlines d "
        "LEFT JOIN cases c ON d.case_id = c.id "
        "WHERE d.status IN ('pending', 'in_progress') "
        "AND d.due_date >= date('now') "
        "AND d.due_date <= date('now', '+14 days') "
        "ORDER BY d.due_date ASC LIMIT 20";

    err = regislex_db_query(rctx->ctx, upcoming_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "UPCOMING DEADLINES (Next 14 Days)");
        append_output_line(rctx, "---------------------------------");
        append_output_fmt(rctx, "%-12s %-15s %-30s %s\n",
                          "Due Date", "Case", "Deadline", "Type");
        append_output_line(rctx, "------------------------------------------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* title = regislex_db_result_get_text(result, 0);
            const char* due_date = regislex_db_result_get_text(result, 1);
            int type = regislex_db_result_get_int(result, 2);
            const char* case_num = regislex_db_result_get_text(result, 3);

            const char* type_name = "Other";
            switch (type) {
                case 0: type_name = "Filing"; break;
                case 1: type_name = "Court Date"; break;
                case 2: type_name = "Discovery"; break;
                case 3: type_name = "Response"; break;
                case 4: type_name = "SOL"; break;
            }

            append_output_fmt(rctx, "%-12s %-15s %-30.30s %s\n",
                              due_date ? due_date : "",
                              case_num ? case_num : "N/A",
                              title ? title : "",
                              type_name);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    /* Overdue deadlines */
    const char* overdue_sql =
        "SELECT d.title, d.due_date, d.type, c.case_number, "
        "  julianday('now') - julianday(d.due_date) as days_overdue "
        "FROM deadlines d "
        "LEFT JOIN cases c ON d.case_id = c.id "
        "WHERE d.status IN ('pending', 'in_progress') "
        "AND d.due_date < date('now') "
        "ORDER BY d.due_date ASC";

    err = regislex_db_query(rctx->ctx, overdue_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "OVERDUE DEADLINES");
        append_output_line(rctx, "-----------------");
        append_output_fmt(rctx, "%-12s %-15s %-30s %s\n",
                          "Due Date", "Case", "Deadline", "Days Over");
        append_output_line(rctx, "------------------------------------------------------------------------");

        int count = 0;
        while (regislex_db_result_next(result)) {
            const char* title = regislex_db_result_get_text(result, 0);
            const char* due_date = regislex_db_result_get_text(result, 1);
            const char* case_num = regislex_db_result_get_text(result, 3);
            int days_overdue = regislex_db_result_get_int(result, 4);

            append_output_fmt(rctx, "%-12s %-15s %-30.30s %d days\n",
                              due_date ? due_date : "",
                              case_num ? case_num : "N/A",
                              title ? title : "",
                              days_overdue);
            count++;
        }

        if (count == 0) {
            append_output_line(rctx, "  No overdue deadlines - Great job!");
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    append_output_line(rctx, "================================================================================");

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report Generation - Case Aging
 * ========================================================================== */

static regislex_error_t generate_case_aging(report_context_t* rctx) {
    regislex_db_result_t* result = NULL;

    append_output_line(rctx, "================================================================================");
    append_output_line(rctx, "                          CASE AGING REPORT");
    append_output_line(rctx, "================================================================================\n");

    /* Aging buckets */
    const char* aging_sql =
        "SELECT "
        "  SUM(CASE WHEN julianday('now') - julianday(created_at) <= 30 THEN 1 ELSE 0 END) as under_30, "
        "  SUM(CASE WHEN julianday('now') - julianday(created_at) > 30 "
        "           AND julianday('now') - julianday(created_at) <= 90 THEN 1 ELSE 0 END) as d30_90, "
        "  SUM(CASE WHEN julianday('now') - julianday(created_at) > 90 "
        "           AND julianday('now') - julianday(created_at) <= 180 THEN 1 ELSE 0 END) as d90_180, "
        "  SUM(CASE WHEN julianday('now') - julianday(created_at) > 180 "
        "           AND julianday('now') - julianday(created_at) <= 365 THEN 1 ELSE 0 END) as d180_365, "
        "  SUM(CASE WHEN julianday('now') - julianday(created_at) > 365 THEN 1 ELSE 0 END) as over_365 "
        "FROM cases WHERE status IN (0, 1, 2, 3)"; /* Draft, Active, Pending, On Hold */

    regislex_error_t err = regislex_db_query(rctx->ctx, aging_sql, &result);
    if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
        int under_30 = regislex_db_result_get_int(result, 0);
        int d30_90 = regislex_db_result_get_int(result, 1);
        int d90_180 = regislex_db_result_get_int(result, 2);
        int d180_365 = regislex_db_result_get_int(result, 3);
        int over_365 = regislex_db_result_get_int(result, 4);
        int total = under_30 + d30_90 + d90_180 + d180_365 + over_365;

        append_output_line(rctx, "CASE AGING DISTRIBUTION (Active Cases Only)");
        append_output_line(rctx, "-------------------------------------------");
        append_output_fmt(rctx, "%-20s %10s %10s\n", "Age Bucket", "Count", "Percent");
        append_output_line(rctx, "--------------------------------------------");

        if (total > 0) {
            append_output_fmt(rctx, "%-20s %10d %9.1f%%\n", "0-30 days", under_30, (double)under_30/total*100);
            append_output_fmt(rctx, "%-20s %10d %9.1f%%\n", "31-90 days", d30_90, (double)d30_90/total*100);
            append_output_fmt(rctx, "%-20s %10d %9.1f%%\n", "91-180 days", d90_180, (double)d90_180/total*100);
            append_output_fmt(rctx, "%-20s %10d %9.1f%%\n", "181-365 days", d180_365, (double)d180_365/total*100);
            append_output_fmt(rctx, "%-20s %10d %9.1f%%\n", "Over 1 year", over_365, (double)over_365/total*100);
        }
        append_output_line(rctx, "--------------------------------------------");
        append_output_fmt(rctx, "%-20s %10d %10s\n\n", "TOTAL", total, "100%");

        regislex_db_result_free(result);
    }

    /* Oldest active cases */
    const char* oldest_sql =
        "SELECT case_number, title, created_at, "
        "  CAST(julianday('now') - julianday(created_at) AS INTEGER) as age_days "
        "FROM cases WHERE status IN (0, 1, 2, 3) "
        "ORDER BY created_at ASC LIMIT 10";

    err = regislex_db_query(rctx->ctx, oldest_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "TOP 10 OLDEST ACTIVE CASES");
        append_output_line(rctx, "--------------------------");
        append_output_fmt(rctx, "%-15s %-35s %-12s %s\n",
                          "Case Number", "Title", "Opened", "Age");
        append_output_line(rctx, "------------------------------------------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* case_num = regislex_db_result_get_text(result, 0);
            const char* title = regislex_db_result_get_text(result, 1);
            const char* created = regislex_db_result_get_text(result, 2);
            int age_days = regislex_db_result_get_int(result, 3);

            char age_str[32];
            if (age_days >= 365) {
                snprintf(age_str, sizeof(age_str), "%d yr %d mo", age_days/365, (age_days%365)/30);
            } else if (age_days >= 30) {
                snprintf(age_str, sizeof(age_str), "%d mo %d d", age_days/30, age_days%30);
            } else {
                snprintf(age_str, sizeof(age_str), "%d days", age_days);
            }

            append_output_fmt(rctx, "%-15s %-35.35s %-12.10s %s\n",
                              case_num ? case_num : "",
                              title ? title : "",
                              created ? created : "",
                              age_str);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    append_output_line(rctx, "================================================================================");

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report Generation - Financial Summary
 * ========================================================================== */

static regislex_error_t generate_financial_summary(report_context_t* rctx) {
    regislex_db_result_t* result = NULL;
    char money_buf[64];

    append_output_line(rctx, "================================================================================");
    append_output_line(rctx, "                        FINANCIAL SUMMARY REPORT");
    append_output_line(rctx, "================================================================================\n");

    /* Invoice summary */
    const char* invoice_sql =
        "SELECT "
        "  SUM(CASE WHEN status = 'paid' THEN total_amount ELSE 0 END) as paid, "
        "  SUM(CASE WHEN status = 'approved' THEN total_amount ELSE 0 END) as approved, "
        "  SUM(CASE WHEN status = 'pending' THEN total_amount ELSE 0 END) as pending, "
        "  SUM(CASE WHEN status = 'disputed' THEN total_amount ELSE 0 END) as disputed, "
        "  COUNT(*) as total_invoices "
        "FROM invoices";

    regislex_error_t err = regislex_db_query(rctx->ctx, invoice_sql, &result);
    if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
        int64_t paid = regislex_db_result_get_int64(result, 0);
        int64_t approved = regislex_db_result_get_int64(result, 1);
        int64_t pending = regislex_db_result_get_int64(result, 2);
        int64_t disputed = regislex_db_result_get_int64(result, 3);
        int total_invoices = regislex_db_result_get_int(result, 4);

        append_output_line(rctx, "INVOICE STATUS SUMMARY");
        append_output_line(rctx, "----------------------");
        append_output_fmt(rctx, "Total Invoices:      %d\n", total_invoices);
        append_output_fmt(rctx, "Paid:                $%'.2f\n", paid / 100.0);
        append_output_fmt(rctx, "Approved (Unpaid):   $%'.2f\n", approved / 100.0);
        append_output_fmt(rctx, "Pending Review:      $%'.2f\n", pending / 100.0);
        append_output_fmt(rctx, "Disputed:            $%'.2f\n\n", disputed / 100.0);

        regislex_db_result_free(result);
    }

    /* Spend by vendor */
    const char* vendor_sql =
        "SELECT v.name, SUM(i.total_amount) as total_spend, COUNT(i.id) as invoice_count "
        "FROM vendors v "
        "JOIN invoices i ON i.vendor_id = v.id "
        "WHERE i.status = 'paid' "
        "GROUP BY v.id ORDER BY total_spend DESC LIMIT 10";

    err = regislex_db_query(rctx->ctx, vendor_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "TOP 10 VENDORS BY SPEND");
        append_output_line(rctx, "-----------------------");
        append_output_fmt(rctx, "%-30s %15s %10s\n", "Vendor", "Total Spend", "Invoices");
        append_output_line(rctx, "--------------------------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* name = regislex_db_result_get_text(result, 0);
            int64_t spend = regislex_db_result_get_int64(result, 1);
            int count = regislex_db_result_get_int(result, 2);

            append_output_fmt(rctx, "%-30.30s $%14.2f %10d\n",
                              name ? name : "Unknown", spend / 100.0, count);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    /* Budget vs actual by matter */
    const char* budget_sql =
        "SELECT c.case_number, c.title, "
        "  COALESCE(b.total_budget, 0) as budget, "
        "  COALESCE(SUM(i.total_amount), 0) as actual "
        "FROM cases c "
        "LEFT JOIN budgets b ON b.case_id = c.id "
        "LEFT JOIN invoices i ON i.case_id = c.id AND i.status = 'paid' "
        "WHERE c.status IN (0, 1, 2, 3) "
        "GROUP BY c.id "
        "HAVING budget > 0 "
        "ORDER BY (actual - budget) DESC LIMIT 10";

    err = regislex_db_query(rctx->ctx, budget_sql, &result);
    if (err == REGISLEX_SUCCESS && result) {
        append_output_line(rctx, "BUDGET VS ACTUAL (Top Variances)");
        append_output_line(rctx, "--------------------------------");
        append_output_fmt(rctx, "%-15s %-25s %12s %12s %12s\n",
                          "Case", "Title", "Budget", "Actual", "Variance");
        append_output_line(rctx, "------------------------------------------------------------------------");

        while (regislex_db_result_next(result)) {
            const char* case_num = regislex_db_result_get_text(result, 0);
            const char* title = regislex_db_result_get_text(result, 1);
            int64_t budget = regislex_db_result_get_int64(result, 2);
            int64_t actual = regislex_db_result_get_int64(result, 3);
            int64_t variance = actual - budget;

            append_output_fmt(rctx, "%-15s %-25.25s $%11.2f $%11.2f $%11.2f\n",
                              case_num ? case_num : "",
                              title ? title : "",
                              budget / 100.0, actual / 100.0, variance / 100.0);
        }
        append_output_line(rctx, "");
        regislex_db_result_free(result);
    }

    append_output_line(rctx, "================================================================================");

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report Template Management
 * ========================================================================== */

regislex_error_t regislex_report_template_create(
    regislex_context_t* ctx,
    regislex_report_template_t* template_data,
    regislex_report_template_t** out_template)
{
    if (!ctx || !template_data || !out_template) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_template = NULL;

    /* Generate ID if not provided */
    regislex_uuid_t id;
    if (template_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = template_data->id;
    }

    /* Get current timestamp */
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO report_templates (id, name, description, type, query_sql, "
        "output_format, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, template_data->name);
    regislex_db_bind_text(stmt, 3, template_data->description);
    regislex_db_bind_int(stmt, 4, template_data->type);
    regislex_db_bind_text(stmt, 5, template_data->query_sql);
    regislex_db_bind_int(stmt, 6, template_data->output_format);
    regislex_db_bind_text(stmt, 7, now_str);
    regislex_db_bind_text(stmt, 8, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    /* Return created template */
    return regislex_report_template_get(ctx, &id, out_template);
}

regislex_error_t regislex_report_template_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_report_template_t** out_template)
{
    if (!ctx || !id || !out_template) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_template = NULL;

    const char* sql =
        "SELECT id, name, description, type, query_sql, output_format, "
        "column_mappings, created_at, updated_at "
        "FROM report_templates WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_report_template_t* tmpl = (regislex_report_template_t*)calloc(1, sizeof(regislex_report_template_t));
    if (!tmpl) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(tmpl->id.value, str, sizeof(tmpl->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(tmpl->name, str, sizeof(tmpl->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(tmpl->description, str, sizeof(tmpl->description) - 1);

    tmpl->type = (regislex_report_type_t)regislex_db_column_int(stmt, 3);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(tmpl->query_sql, str, sizeof(tmpl->query_sql) - 1);

    tmpl->output_format = (regislex_report_format_t)regislex_db_column_int(stmt, 5);

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &tmpl->created_at);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &tmpl->updated_at);

    regislex_db_finalize(stmt);

    *out_template = tmpl;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_report_template_list(
    regislex_context_t* ctx,
    regislex_report_type_t type_filter,
    regislex_report_template_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    if (type_filter >= 0) {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, description, type, output_format FROM report_templates "
                 "WHERE type = %d ORDER BY name", type_filter);
    } else {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, description, type, output_format FROM report_templates "
                 "ORDER BY type, name");
    }

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_report_template_list_t* list = (regislex_report_template_list_t*)calloc(1,
        sizeof(regislex_report_template_list_t) + count * sizeof(regislex_report_template_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->templates = (regislex_report_template_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->templates[i].id.value, str, sizeof(list->templates[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->templates[i].name, str, sizeof(list->templates[i].name) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->templates[i].description, str, sizeof(list->templates[i].description) - 1);

        list->templates[i].type = (regislex_report_type_t)regislex_db_result_get_int(result, 3);
        list->templates[i].output_format = (regislex_report_format_t)regislex_db_result_get_int(result, 4);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_report_template_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    const char* sql = "DELETE FROM report_templates WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

void regislex_report_template_free(regislex_report_template_t* tmpl) {
    free(tmpl);
}

void regislex_report_template_list_free(regislex_report_template_list_t* list) {
    free(list);
}

/* ============================================================================
 * Report Generation
 * ========================================================================== */

regislex_error_t regislex_report_generate(
    regislex_context_t* ctx,
    regislex_report_type_t type,
    regislex_report_params_t* params,
    regislex_report_t** out_report)
{
    if (!ctx || !out_report) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_report = NULL;

    /* Create report structure */
    regislex_report_t* report = (regislex_report_t*)calloc(1, sizeof(regislex_report_t));
    if (!report) {
        return REGISLEX_ERROR_NO_MEMORY;
    }

    regislex_uuid_generate(&report->id);
    report->type = type;
    regislex_datetime_now(&report->generated_at);

    if (params) {
        report->params = *params;
    }

    /* Set report name based on type */
    const char* report_name = "Custom Report";
    switch (type) {
        case REGISLEX_REPORT_CASELOAD_SUMMARY: report_name = "Caseload Summary"; break;
        case REGISLEX_REPORT_ATTORNEY_PERFORMANCE: report_name = "Attorney Performance"; break;
        case REGISLEX_REPORT_DEADLINE_COMPLIANCE: report_name = "Deadline Compliance"; break;
        case REGISLEX_REPORT_CASE_AGING: report_name = "Case Aging"; break;
        case REGISLEX_REPORT_FINANCIAL_SUMMARY: report_name = "Financial Summary"; break;
        case REGISLEX_REPORT_MATTER_BUDGET: report_name = "Matter Budget"; break;
        case REGISLEX_REPORT_VENDOR_SPEND: report_name = "Vendor Spend"; break;
        case REGISLEX_REPORT_CUSTOM: report_name = "Custom Report"; break;
    }
    strncpy(report->name, report_name, sizeof(report->name) - 1);

    /* Initialize report context */
    report_context_t rctx = {0};
    rctx.ctx = ctx;
    rctx.report = report;
    rctx.params = params;

    /* Generate report based on type */
    regislex_error_t err = REGISLEX_SUCCESS;
    switch (type) {
        case REGISLEX_REPORT_CASELOAD_SUMMARY:
            err = generate_caseload_summary(&rctx);
            break;
        case REGISLEX_REPORT_ATTORNEY_PERFORMANCE:
            err = generate_attorney_performance(&rctx);
            break;
        case REGISLEX_REPORT_DEADLINE_COMPLIANCE:
            err = generate_deadline_compliance(&rctx);
            break;
        case REGISLEX_REPORT_CASE_AGING:
            err = generate_case_aging(&rctx);
            break;
        case REGISLEX_REPORT_FINANCIAL_SUMMARY:
            err = generate_financial_summary(&rctx);
            break;
        default:
            err = REGISLEX_ERROR_NOT_IMPLEMENTED;
            break;
    }

    if (err != REGISLEX_SUCCESS) {
        free(rctx.output_buffer);
        free(report);
        return err;
    }

    /* Store output in report */
    report->output_data = rctx.output_buffer;
    report->output_size = rctx.output_size;

    *out_report = report;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_report_generate_from_template(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    regislex_report_params_t* params,
    regislex_report_t** out_report)
{
    if (!ctx || !template_id || !out_report) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Get template */
    regislex_report_template_t* tmpl = NULL;
    regislex_error_t err = regislex_report_template_get(ctx, template_id, &tmpl);
    if (err != REGISLEX_SUCCESS) return err;

    /* Create report */
    regislex_report_t* report = (regislex_report_t*)calloc(1, sizeof(regislex_report_t));
    if (!report) {
        regislex_report_template_free(tmpl);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    regislex_uuid_generate(&report->id);
    report->type = tmpl->type;
    strncpy(report->name, tmpl->name, sizeof(report->name) - 1);
    report->template_id = *template_id;
    regislex_datetime_now(&report->generated_at);

    if (params) {
        report->params = *params;
    }

    /* Execute custom SQL if present */
    if (tmpl->query_sql[0] != '\0') {
        report_context_t rctx = {0};
        rctx.ctx = ctx;
        rctx.report = report;
        rctx.params = params;

        append_output_fmt(&rctx, "Report: %s\n", tmpl->name);
        append_output_line(&rctx, "================================================================================\n");

        regislex_db_result_t* result = NULL;
        err = regislex_db_query(ctx, tmpl->query_sql, &result);
        if (err == REGISLEX_SUCCESS && result) {
            /* Output column headers */
            int col_count = regislex_db_result_column_count(result);
            for (int i = 0; i < col_count; i++) {
                const char* col_name = regislex_db_result_column_name(result, i);
                append_output_fmt(&rctx, "%-20s", col_name ? col_name : "");
            }
            append_output_line(&rctx, "");

            /* Output separator */
            for (int i = 0; i < col_count * 20; i++) {
                append_output(&rctx, "-");
            }
            append_output_line(&rctx, "");

            /* Output rows */
            while (regislex_db_result_next(result)) {
                for (int i = 0; i < col_count; i++) {
                    const char* val = regislex_db_result_get_text(result, i);
                    append_output_fmt(&rctx, "%-20s", val ? val : "NULL");
                }
                append_output_line(&rctx, "");
            }

            regislex_db_result_free(result);
        }

        report->output_data = rctx.output_buffer;
        report->output_size = rctx.output_size;
    }

    regislex_report_template_free(tmpl);

    *out_report = report;
    return REGISLEX_SUCCESS;
}

void regislex_report_free(regislex_report_t* report) {
    if (report) {
        free(report->output_data);
        free(report);
    }
}

/* ============================================================================
 * Report Export
 * ========================================================================== */

regislex_error_t regislex_report_export(
    regislex_report_t* report,
    regislex_report_format_t format,
    const char* file_path)
{
    if (!report || !file_path) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    if (!report->output_data || report->output_size == 0) {
        return REGISLEX_ERROR_INVALID_STATE;
    }

    FILE* f = fopen(file_path, "wb");
    if (!f) {
        return REGISLEX_ERROR_IO;
    }

    switch (format) {
        case REGISLEX_FORMAT_TEXT:
        case REGISLEX_FORMAT_CSV:
            /* Write raw output */
            fwrite(report->output_data, 1, report->output_size, f);
            break;

        case REGISLEX_FORMAT_HTML:
            /* Wrap in basic HTML */
            fprintf(f, "<!DOCTYPE html>\n<html>\n<head>\n");
            fprintf(f, "<title>%s</title>\n", report->name);
            fprintf(f, "<style>body{font-family:monospace;white-space:pre;}</style>\n");
            fprintf(f, "</head>\n<body>\n");
            fwrite(report->output_data, 1, report->output_size, f);
            fprintf(f, "</body>\n</html>\n");
            break;

        case REGISLEX_FORMAT_PDF:
            /* PDF generation would require a library like libharu */
            /* For now, just write text */
            fwrite(report->output_data, 1, report->output_size, f);
            break;

        case REGISLEX_FORMAT_EXCEL:
        case REGISLEX_FORMAT_JSON:
            /* These formats require more complex serialization */
            fwrite(report->output_data, 1, report->output_size, f);
            break;
    }

    fclose(f);
    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Dashboard Management
 * ========================================================================== */

regislex_error_t regislex_dashboard_create(
    regislex_context_t* ctx,
    regislex_dashboard_t* dashboard_data,
    regislex_dashboard_t** out_dashboard)
{
    if (!ctx || !dashboard_data || !out_dashboard) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_dashboard = NULL;

    regislex_uuid_t id;
    if (dashboard_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = dashboard_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO dashboards (id, name, description, owner_id, is_shared, "
        "layout_config, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, dashboard_data->name);
    regislex_db_bind_text(stmt, 3, dashboard_data->description);
    regislex_db_bind_text(stmt, 4, dashboard_data->owner_id.value);
    regislex_db_bind_int(stmt, 5, dashboard_data->is_shared ? 1 : 0);
    regislex_db_bind_text(stmt, 6, dashboard_data->layout_config);
    regislex_db_bind_text(stmt, 7, now_str);
    regislex_db_bind_text(stmt, 8, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_dashboard_get(ctx, &id, out_dashboard);
}

regislex_error_t regislex_dashboard_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_dashboard_t** out_dashboard)
{
    if (!ctx || !id || !out_dashboard) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_dashboard = NULL;

    const char* sql =
        "SELECT id, name, description, owner_id, is_shared, layout_config, "
        "created_at, updated_at FROM dashboards WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_dashboard_t* dashboard = (regislex_dashboard_t*)calloc(1, sizeof(regislex_dashboard_t));
    if (!dashboard) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(dashboard->id.value, str, sizeof(dashboard->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(dashboard->name, str, sizeof(dashboard->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(dashboard->description, str, sizeof(dashboard->description) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(dashboard->owner_id.value, str, sizeof(dashboard->owner_id.value) - 1);

    dashboard->is_shared = regislex_db_column_int(stmt, 4) != 0;

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(dashboard->layout_config, str, sizeof(dashboard->layout_config) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) regislex_datetime_parse(str, &dashboard->created_at);

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &dashboard->updated_at);

    regislex_db_finalize(stmt);

    *out_dashboard = dashboard;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_dashboard_update(
    regislex_context_t* ctx,
    regislex_dashboard_t* dashboard)
{
    if (!ctx || !dashboard || dashboard->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "UPDATE dashboards SET name = ?, description = ?, is_shared = ?, "
        "layout_config = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, dashboard->name);
    regislex_db_bind_text(stmt, 2, dashboard->description);
    regislex_db_bind_int(stmt, 3, dashboard->is_shared ? 1 : 0);
    regislex_db_bind_text(stmt, 4, dashboard->layout_config);
    regislex_db_bind_text(stmt, 5, now_str);
    regislex_db_bind_text(stmt, 6, dashboard->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_dashboard_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Delete widgets first */
    const char* widget_sql = "DELETE FROM dashboard_widgets WHERE dashboard_id = ?";
    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, widget_sql, &stmt);
    if (err == REGISLEX_SUCCESS) {
        regislex_db_bind_text(stmt, 1, id->value);
        regislex_db_step(stmt);
        regislex_db_finalize(stmt);
    }

    /* Delete dashboard */
    const char* sql = "DELETE FROM dashboards WHERE id = ?";
    err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);
    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

void regislex_dashboard_free(regislex_dashboard_t* dashboard) {
    if (dashboard) {
        /* Free widgets if allocated */
        free(dashboard->widgets);
        free(dashboard);
    }
}

/* ============================================================================
 * Dashboard Widget Management
 * ========================================================================== */

regislex_error_t regislex_dashboard_widget_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* dashboard_id,
    regislex_dashboard_widget_t* widget)
{
    if (!ctx || !dashboard_id || !widget) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_uuid_t id;
    if (widget->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = widget->id;
    }

    const char* sql =
        "INSERT INTO dashboard_widgets (id, dashboard_id, widget_type, title, "
        "report_type, config_json, position_x, position_y, width, height, refresh_interval) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, dashboard_id->value);
    regislex_db_bind_int(stmt, 3, widget->widget_type);
    regislex_db_bind_text(stmt, 4, widget->title);
    regislex_db_bind_int(stmt, 5, widget->report_type);
    regislex_db_bind_text(stmt, 6, widget->config_json);
    regislex_db_bind_int(stmt, 7, widget->position_x);
    regislex_db_bind_int(stmt, 8, widget->position_y);
    regislex_db_bind_int(stmt, 9, widget->width);
    regislex_db_bind_int(stmt, 10, widget->height);
    regislex_db_bind_int(stmt, 11, widget->refresh_interval);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err == REGISLEX_SUCCESS) {
        widget->id = id;
    }

    return err;
}

regislex_error_t regislex_dashboard_widget_update(
    regislex_context_t* ctx,
    regislex_dashboard_widget_t* widget)
{
    if (!ctx || !widget || widget->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    const char* sql =
        "UPDATE dashboard_widgets SET widget_type = ?, title = ?, report_type = ?, "
        "config_json = ?, position_x = ?, position_y = ?, width = ?, height = ?, "
        "refresh_interval = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_int(stmt, 1, widget->widget_type);
    regislex_db_bind_text(stmt, 2, widget->title);
    regislex_db_bind_int(stmt, 3, widget->report_type);
    regislex_db_bind_text(stmt, 4, widget->config_json);
    regislex_db_bind_int(stmt, 5, widget->position_x);
    regislex_db_bind_int(stmt, 6, widget->position_y);
    regislex_db_bind_int(stmt, 7, widget->width);
    regislex_db_bind_int(stmt, 8, widget->height);
    regislex_db_bind_int(stmt, 9, widget->refresh_interval);
    regislex_db_bind_text(stmt, 10, widget->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_dashboard_widget_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* widget_id)
{
    if (!ctx || !widget_id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    const char* sql = "DELETE FROM dashboard_widgets WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, widget_id->value);
    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

/* ============================================================================
 * Scheduled Reports
 * ========================================================================== */

regislex_error_t regislex_scheduled_report_create(
    regislex_context_t* ctx,
    regislex_scheduled_report_t* schedule_data,
    regislex_scheduled_report_t** out_schedule)
{
    if (!ctx || !schedule_data || !out_schedule) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_schedule = NULL;

    regislex_uuid_t id;
    if (schedule_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = schedule_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    char next_run_str[32] = {0};
    regislex_datetime_format(&schedule_data->next_run, next_run_str, sizeof(next_run_str));

    const char* sql =
        "INSERT INTO scheduled_reports (id, name, template_id, schedule_cron, "
        "output_format, recipients, is_enabled, next_run, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, schedule_data->name);
    regislex_db_bind_text(stmt, 3, schedule_data->template_id.value);
    regislex_db_bind_text(stmt, 4, schedule_data->schedule_cron);
    regislex_db_bind_int(stmt, 5, schedule_data->output_format);
    regislex_db_bind_text(stmt, 6, schedule_data->recipients);
    regislex_db_bind_int(stmt, 7, schedule_data->is_enabled ? 1 : 0);
    regislex_db_bind_text(stmt, 8, next_run_str);
    regislex_db_bind_text(stmt, 9, now_str);
    regislex_db_bind_text(stmt, 10, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_scheduled_report_get(ctx, &id, out_schedule);
}

regislex_error_t regislex_scheduled_report_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_scheduled_report_t** out_schedule)
{
    if (!ctx || !id || !out_schedule) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_schedule = NULL;

    const char* sql =
        "SELECT id, name, template_id, schedule_cron, output_format, recipients, "
        "is_enabled, next_run, last_run, created_at, updated_at "
        "FROM scheduled_reports WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_scheduled_report_t* schedule = (regislex_scheduled_report_t*)calloc(1, sizeof(regislex_scheduled_report_t));
    if (!schedule) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(schedule->id.value, str, sizeof(schedule->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(schedule->name, str, sizeof(schedule->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(schedule->template_id.value, str, sizeof(schedule->template_id.value) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(schedule->schedule_cron, str, sizeof(schedule->schedule_cron) - 1);

    schedule->output_format = (regislex_report_format_t)regislex_db_column_int(stmt, 4);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(schedule->recipients, str, sizeof(schedule->recipients) - 1);

    schedule->is_enabled = regislex_db_column_int(stmt, 6) != 0;

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &schedule->next_run);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &schedule->last_run);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &schedule->created_at);

    str = regislex_db_column_text(stmt, 10);
    if (str) regislex_datetime_parse(str, &schedule->updated_at);

    regislex_db_finalize(stmt);

    *out_schedule = schedule;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_scheduled_report_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    const char* sql = "DELETE FROM scheduled_reports WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);
    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

void regislex_scheduled_report_free(regislex_scheduled_report_t* schedule) {
    free(schedule);
}
