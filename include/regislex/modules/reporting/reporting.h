/**
 * @file reporting.h
 * @brief Reporting Module
 *
 * Provides functionality for generating data-driven reports on caseloads,
 * performance, and resource allocation for transparency and decision-making.
 */

#ifndef REGISLEX_REPORTING_H
#define REGISLEX_REPORTING_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Report Types and Enums
 * ============================================================================ */

/**
 * @brief Report type classification
 */
typedef enum {
    REGISLEX_REPORT_CASELOAD = 0,
    REGISLEX_REPORT_PERFORMANCE,
    REGISLEX_REPORT_FINANCIAL,
    REGISLEX_REPORT_DEADLINE,
    REGISLEX_REPORT_PRODUCTIVITY,
    REGISLEX_REPORT_COMPLIANCE,
    REGISLEX_REPORT_AUDIT,
    REGISLEX_REPORT_AGING,
    REGISLEX_REPORT_TREND,
    REGISLEX_REPORT_CUSTOM
} regislex_report_type_t;

/**
 * @brief Report output format
 */
typedef enum {
    REGISLEX_FORMAT_PDF = 0,
    REGISLEX_FORMAT_HTML,
    REGISLEX_FORMAT_EXCEL,
    REGISLEX_FORMAT_CSV,
    REGISLEX_FORMAT_JSON,
    REGISLEX_FORMAT_XML,
    REGISLEX_FORMAT_WORD
} regislex_report_format_t;

/**
 * @brief Date range preset
 */
typedef enum {
    REGISLEX_DATE_RANGE_TODAY = 0,
    REGISLEX_DATE_RANGE_YESTERDAY,
    REGISLEX_DATE_RANGE_THIS_WEEK,
    REGISLEX_DATE_RANGE_LAST_WEEK,
    REGISLEX_DATE_RANGE_THIS_MONTH,
    REGISLEX_DATE_RANGE_LAST_MONTH,
    REGISLEX_DATE_RANGE_THIS_QUARTER,
    REGISLEX_DATE_RANGE_LAST_QUARTER,
    REGISLEX_DATE_RANGE_THIS_YEAR,
    REGISLEX_DATE_RANGE_LAST_YEAR,
    REGISLEX_DATE_RANGE_CUSTOM
} regislex_date_range_t;

/**
 * @brief Aggregation type
 */
typedef enum {
    REGISLEX_AGG_COUNT = 0,
    REGISLEX_AGG_SUM,
    REGISLEX_AGG_AVG,
    REGISLEX_AGG_MIN,
    REGISLEX_AGG_MAX,
    REGISLEX_AGG_DISTINCT
} regislex_aggregation_t;

/**
 * @brief Chart type
 */
typedef enum {
    REGISLEX_CHART_NONE = 0,
    REGISLEX_CHART_BAR,
    REGISLEX_CHART_LINE,
    REGISLEX_CHART_PIE,
    REGISLEX_CHART_DONUT,
    REGISLEX_CHART_AREA,
    REGISLEX_CHART_STACKED_BAR,
    REGISLEX_CHART_TABLE,
    REGISLEX_CHART_HEATMAP,
    REGISLEX_CHART_GAUGE,
    REGISLEX_CHART_FUNNEL
} regislex_chart_type_t;

/**
 * @brief Report schedule frequency
 */
typedef enum {
    REGISLEX_SCHEDULE_NONE = 0,
    REGISLEX_SCHEDULE_DAILY,
    REGISLEX_SCHEDULE_WEEKLY,
    REGISLEX_SCHEDULE_BIWEEKLY,
    REGISLEX_SCHEDULE_MONTHLY,
    REGISLEX_SCHEDULE_QUARTERLY,
    REGISLEX_SCHEDULE_YEARLY,
    REGISLEX_SCHEDULE_CUSTOM
} regislex_schedule_freq_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Report column definition
 */
typedef struct {
    char name[128];
    char display_name[256];
    char data_type[32];         /* "string", "number", "date", "currency", "percentage" */
    regislex_aggregation_t aggregation;
    char format_string[64];
    int width;
    bool sortable;
    bool visible;
    char formula[512];          /* For calculated columns */
} regislex_report_column_t;

/**
 * @brief Report filter
 */
typedef struct {
    char field[128];
    char operator_str[32];      /* "equals", "contains", "between", "in", etc. */
    char value[1024];
    char logical_op[8];         /* "AND", "OR" */
} regislex_report_filter_t;

/**
 * @brief Report grouping
 */
typedef struct {
    char field[128];
    char interval[32];          /* For dates: "day", "week", "month", "quarter", "year" */
    bool show_subtotals;
    char sort_order[8];         /* "asc", "desc" */
} regislex_report_group_t;

/**
 * @brief Chart configuration
 */
typedef struct {
    regislex_chart_type_t type;
    char title[256];
    char x_axis_field[128];
    char y_axis_field[128];
    char series_field[128];
    char color_scheme[64];
    bool show_legend;
    bool show_labels;
    bool show_grid;
    int width;
    int height;
} regislex_chart_config_t;

/**
 * @brief Report data source
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char entity_type[64];       /* "cases", "deadlines", "documents", etc. */
    char query[8192];           /* SQL or query definition */
    char connection_name[128];  /* For external data sources */
    bool is_custom;
    regislex_datetime_t created_at;
} regislex_data_source_t;

/**
 * @brief Report template
 */
struct regislex_report_template {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_report_type_t type;
    char category[128];

    /* Data source */
    regislex_uuid_t data_source_id;
    char custom_query[8192];

    /* Columns */
    int column_count;
    regislex_report_column_t* columns;

    /* Filters */
    int filter_count;
    regislex_report_filter_t* filters;

    /* Grouping */
    int group_count;
    regislex_report_group_t* groups;

    /* Visualization */
    int chart_count;
    regislex_chart_config_t* charts;

    /* Layout */
    char header_html[4096];
    char footer_html[4096];
    char custom_css[8192];
    bool landscape_orientation;
    char page_size[16];         /* "A4", "Letter", etc. */

    /* Permissions */
    char allowed_roles[512];
    bool is_public;
    bool is_system;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Report parameters (runtime input)
 */
typedef struct {
    regislex_date_range_t date_range;
    regislex_datetime_t start_date;
    regislex_datetime_t end_date;
    char jurisdiction[128];
    regislex_uuid_t* user_ids;
    int user_id_count;
    regislex_uuid_t* case_ids;
    int case_id_count;
    regislex_case_type_t* case_types;
    int case_type_count;
    regislex_status_t* statuses;
    int status_count;
    int limit;
    char custom_params[4096];   /* JSON for additional parameters */
} regislex_report_params_t;

/**
 * @brief Report execution result
 */
struct regislex_report {
    regislex_uuid_t id;
    regislex_uuid_t template_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_report_type_t type;
    regislex_report_format_t format;

    /* Execution info */
    regislex_datetime_t generated_at;
    regislex_uuid_t generated_by;
    int execution_time_ms;

    /* Results */
    int row_count;
    int total_row_count;
    char* data_json;            /* JSON array of rows */
    char* summary_json;         /* JSON aggregated summary */

    /* Output */
    char output_path[REGISLEX_MAX_PATH_LENGTH];
    size_t output_size;
    char content_type[64];

    /* Status */
    regislex_status_t status;
    char error_message[REGISLEX_MAX_DESCRIPTION_LENGTH];
};

/**
 * @brief Dashboard widget
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t dashboard_id;
    char title[REGISLEX_MAX_NAME_LENGTH];
    regislex_uuid_t report_template_id;
    regislex_chart_type_t display_type;
    int position_x;
    int position_y;
    int width;
    int height;
    int refresh_interval_seconds;
    char custom_config[4096];   /* JSON for widget-specific settings */
    regislex_datetime_t created_at;
} regislex_dashboard_widget_t;

/**
 * @brief Dashboard definition
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    int widget_count;
    regislex_dashboard_widget_t* widgets;
    bool is_default;
    char allowed_roles[512];
    regislex_uuid_t owner_id;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_dashboard_t;

/**
 * @brief Scheduled report configuration
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t template_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_schedule_freq_t frequency;
    char cron_expression[128];
    regislex_datetime_t next_run;
    regislex_datetime_t last_run;
    regislex_report_format_t format;
    regislex_report_params_t params;
    char recipients[2048];      /* Email addresses, comma-separated */
    char subject_template[256];
    char body_template[4096];
    bool include_attachment;
    bool is_active;
    regislex_datetime_t created_at;
    regislex_uuid_t created_by;
} regislex_scheduled_report_t;

/* ============================================================================
 * Report Template Functions
 * ============================================================================ */

/**
 * @brief Create report template
 * @param ctx Context
 * @param template_data Template data
 * @param out_template Output created template
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_template_create(
    regislex_context_t* ctx,
    const regislex_report_template_t* template_data,
    regislex_report_template_t** out_template
);

/**
 * @brief Get report template by ID
 * @param ctx Context
 * @param id Template ID
 * @param out_template Output template
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_template_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_report_template_t** out_template
);

/**
 * @brief Update report template
 * @param ctx Context
 * @param template_data Updated template data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_template_update(
    regislex_context_t* ctx,
    const regislex_report_template_t* template_data
);

/**
 * @brief Delete report template
 * @param ctx Context
 * @param id Template ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_template_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List report templates
 * @param ctx Context
 * @param type Filter by type (NULL for all)
 * @param category Filter by category (NULL for all)
 * @param templates Output template array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_template_list(
    regislex_context_t* ctx,
    const regislex_report_type_t* type,
    const char* category,
    regislex_report_template_t*** templates,
    int* count
);

/**
 * @brief Free report template
 * @param template_ptr Template to free
 */
REGISLEX_API void regislex_report_template_free(regislex_report_template_t* template_ptr);

/* ============================================================================
 * Report Generation Functions
 * ============================================================================ */

/**
 * @brief Generate report from template
 * @param ctx Context
 * @param template_id Template ID
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_generate(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/**
 * @brief Generate report asynchronously
 * @param ctx Context
 * @param template_id Template ID
 * @param params Report parameters
 * @param format Output format
 * @param out_job_id Output job ID for tracking
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_generate_async(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_uuid_t* out_job_id
);

/**
 * @brief Get report generation status
 * @param ctx Context
 * @param job_id Job ID
 * @param out_report Output report (if complete)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_status(
    regislex_context_t* ctx,
    const regislex_uuid_t* job_id,
    regislex_report_t** out_report
);

/**
 * @brief Export report to file
 * @param ctx Context
 * @param report Report to export
 * @param path Output file path
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_export(
    regislex_context_t* ctx,
    const regislex_report_t* report,
    const char* path
);

/**
 * @brief Get report content as buffer
 * @param ctx Context
 * @param report Report
 * @param buffer Output buffer (caller must free)
 * @param size Output size
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_get_content(
    regislex_context_t* ctx,
    const regislex_report_t* report,
    char** buffer,
    size_t* size
);

/**
 * @brief Free report
 * @param report Report to free
 */
REGISLEX_API void regislex_report_free(regislex_report_t* report);

/* ============================================================================
 * Pre-built Report Functions (Common Government Reports)
 * ============================================================================ */

/**
 * @brief Generate caseload summary report
 * @param ctx Context
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_caseload_summary(
    regislex_context_t* ctx,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/**
 * @brief Generate attorney performance report
 * @param ctx Context
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_attorney_performance(
    regislex_context_t* ctx,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/**
 * @brief Generate deadline compliance report
 * @param ctx Context
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_deadline_compliance(
    regislex_context_t* ctx,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/**
 * @brief Generate case aging report
 * @param ctx Context
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_case_aging(
    regislex_context_t* ctx,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/**
 * @brief Generate financial summary report
 * @param ctx Context
 * @param params Report parameters
 * @param format Output format
 * @param out_report Output report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_report_financial_summary(
    regislex_context_t* ctx,
    const regislex_report_params_t* params,
    regislex_report_format_t format,
    regislex_report_t** out_report
);

/* ============================================================================
 * Dashboard Functions
 * ============================================================================ */

/**
 * @brief Create dashboard
 * @param ctx Context
 * @param dashboard Dashboard data
 * @param out_dashboard Output created dashboard
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_create(
    regislex_context_t* ctx,
    const regislex_dashboard_t* dashboard,
    regislex_dashboard_t** out_dashboard
);

/**
 * @brief Get dashboard by ID
 * @param ctx Context
 * @param id Dashboard ID
 * @param out_dashboard Output dashboard
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_dashboard_t** out_dashboard
);

/**
 * @brief Get default dashboard for user
 * @param ctx Context
 * @param user_id User ID
 * @param out_dashboard Output dashboard
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_get_default(
    regislex_context_t* ctx,
    const regislex_uuid_t* user_id,
    regislex_dashboard_t** out_dashboard
);

/**
 * @brief Update dashboard
 * @param ctx Context
 * @param dashboard Updated dashboard data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_update(
    regislex_context_t* ctx,
    const regislex_dashboard_t* dashboard
);

/**
 * @brief Delete dashboard
 * @param ctx Context
 * @param id Dashboard ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List dashboards for user
 * @param ctx Context
 * @param user_id User ID (NULL for all accessible)
 * @param dashboards Output dashboard array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* user_id,
    regislex_dashboard_t*** dashboards,
    int* count
);

/**
 * @brief Refresh dashboard data
 * @param ctx Context
 * @param dashboard_id Dashboard ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_dashboard_refresh(
    regislex_context_t* ctx,
    const regislex_uuid_t* dashboard_id
);

/**
 * @brief Free dashboard
 * @param dashboard Dashboard to free
 */
REGISLEX_API void regislex_dashboard_free(regislex_dashboard_t* dashboard);

/* ============================================================================
 * Scheduled Report Functions
 * ============================================================================ */

/**
 * @brief Create scheduled report
 * @param ctx Context
 * @param scheduled Scheduled report data
 * @param out_scheduled Output created scheduled report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_scheduled_report_create(
    regislex_context_t* ctx,
    const regislex_scheduled_report_t* scheduled,
    regislex_scheduled_report_t** out_scheduled
);

/**
 * @brief Update scheduled report
 * @param ctx Context
 * @param scheduled Updated scheduled report data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_scheduled_report_update(
    regislex_context_t* ctx,
    const regislex_scheduled_report_t* scheduled
);

/**
 * @brief Delete scheduled report
 * @param ctx Context
 * @param id Scheduled report ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_scheduled_report_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List scheduled reports
 * @param ctx Context
 * @param scheduled Output scheduled report array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_scheduled_report_list(
    regislex_context_t* ctx,
    regislex_scheduled_report_t*** scheduled,
    int* count
);

/**
 * @brief Run scheduled report now
 * @param ctx Context
 * @param id Scheduled report ID
 * @param out_report Output generated report
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_scheduled_report_run_now(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_report_t** out_report
);

/**
 * @brief Free scheduled report
 * @param scheduled Scheduled report to free
 */
REGISLEX_API void regislex_scheduled_report_free(regislex_scheduled_report_t* scheduled);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_REPORTING_H */
