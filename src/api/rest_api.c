/**
 * RegisLex - Enterprise Legal Software Suite
 * REST API Implementation
 *
 * Provides HTTP REST endpoints for all RegisLex functionality.
 * Uses a lightweight embedded HTTP server approach.
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include "database/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * HTTP Types and Constants
 * ========================================================================== */

#define HTTP_MAX_HEADERS 64
#define HTTP_MAX_BODY_SIZE (16 * 1024 * 1024) /* 16MB max */
#define HTTP_BUFFER_SIZE 8192

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_UNKNOWN
} http_method_t;

typedef enum {
    HTTP_STATUS_OK = 200,
    HTTP_STATUS_CREATED = 201,
    HTTP_STATUS_NO_CONTENT = 204,
    HTTP_STATUS_BAD_REQUEST = 400,
    HTTP_STATUS_UNAUTHORIZED = 401,
    HTTP_STATUS_FORBIDDEN = 403,
    HTTP_STATUS_NOT_FOUND = 404,
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
    HTTP_STATUS_CONFLICT = 409,
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,
    HTTP_STATUS_INTERNAL_ERROR = 500,
    HTTP_STATUS_NOT_IMPLEMENTED = 501
} http_status_t;

typedef struct {
    char name[64];
    char value[256];
} http_header_t;

typedef struct {
    http_method_t method;
    char path[512];
    char query_string[1024];
    http_header_t headers[HTTP_MAX_HEADERS];
    int header_count;
    char* body;
    size_t body_length;
    char content_type[128];
    char auth_token[256];
} http_request_t;

typedef struct {
    http_status_t status;
    http_header_t headers[HTTP_MAX_HEADERS];
    int header_count;
    char* body;
    size_t body_length;
    char content_type[128];
} http_response_t;

typedef struct {
    regislex_context_t* ctx;
    http_request_t* request;
    http_response_t* response;
    regislex_uuid_t user_id;  /* Authenticated user */
} api_context_t;

typedef regislex_error_t (*api_handler_t)(api_context_t* api_ctx);

typedef struct {
    const char* path_pattern;
    http_method_t method;
    api_handler_t handler;
    bool requires_auth;
} api_route_t;

/* ============================================================================
 * JSON Helpers
 * ========================================================================== */

static void json_begin_object(char** buf, size_t* remaining) {
    int n = snprintf(*buf, *remaining, "{");
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_end_object(char** buf, size_t* remaining) {
    /* Remove trailing comma if present */
    if (*(*buf - 1) == ',') (*buf)--;
    int n = snprintf(*buf, *remaining, "}");
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_begin_array(char** buf, size_t* remaining) {
    int n = snprintf(*buf, *remaining, "[");
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_end_array(char** buf, size_t* remaining) {
    /* Remove trailing comma if present */
    if (*(*buf - 1) == ',') (*buf)--;
    int n = snprintf(*buf, *remaining, "]");
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_string(char** buf, size_t* remaining, const char* key, const char* value) {
    if (!value) value = "";
    /* Escape special characters in value */
    char escaped[2048];
    char* e = escaped;
    while (*value && (e - escaped) < (int)sizeof(escaped) - 2) {
        if (*value == '"' || *value == '\\') *e++ = '\\';
        else if (*value == '\n') { *e++ = '\\'; *e++ = 'n'; value++; continue; }
        else if (*value == '\r') { *e++ = '\\'; *e++ = 'r'; value++; continue; }
        else if (*value == '\t') { *e++ = '\\'; *e++ = 't'; value++; continue; }
        *e++ = *value++;
    }
    *e = '\0';

    int n = snprintf(*buf, *remaining, "\"%s\":\"%s\",", key, escaped);
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_int(char** buf, size_t* remaining, const char* key, int value) {
    int n = snprintf(*buf, *remaining, "\"%s\":%d,", key, value);
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_int64(char** buf, size_t* remaining, const char* key, int64_t value) {
    int n = snprintf(*buf, *remaining, "\"%s\":%lld,", key, (long long)value);
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_double(char** buf, size_t* remaining, const char* key, double value) {
    int n = snprintf(*buf, *remaining, "\"%s\":%.2f,", key, value);
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_bool(char** buf, size_t* remaining, const char* key, bool value) {
    int n = snprintf(*buf, *remaining, "\"%s\":%s,", key, value ? "true" : "false");
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_null(char** buf, size_t* remaining, const char* key) {
    int n = snprintf(*buf, *remaining, "\"%s\":null,", key);
    if (n > 0) { *buf += n; *remaining -= n; }
}

static void json_add_key(char** buf, size_t* remaining, const char* key) {
    int n = snprintf(*buf, *remaining, "\"%s\":", key);
    if (n > 0) { *buf += n; *remaining -= n; }
}

/* Simple JSON parser helpers */
static const char* json_get_string(const char* json, const char* key, char* value, size_t value_size) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char* start = strstr(json, pattern);
    if (!start) return NULL;

    start += strlen(pattern);
    while (*start && isspace(*start)) start++;

    if (*start == '"') {
        start++;
        const char* end = start;
        while (*end && *end != '"') {
            if (*end == '\\' && *(end+1)) end++;
            end++;
        }
        size_t len = end - start;
        if (len >= value_size) len = value_size - 1;
        strncpy(value, start, len);
        value[len] = '\0';
        return end + 1;
    }
    return NULL;
}

static int json_get_int(const char* json, const char* key, int default_value) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    const char* start = strstr(json, pattern);
    if (!start) return default_value;

    start += strlen(pattern);
    while (*start && isspace(*start)) start++;

    return atoi(start);
}

/* ============================================================================
 * Response Helpers
 * ========================================================================== */

static void response_set_json(http_response_t* resp, http_status_t status, const char* json) {
    resp->status = status;
    strcpy(resp->content_type, "application/json");
    if (json) {
        resp->body_length = strlen(json);
        resp->body = (char*)malloc(resp->body_length + 1);
        if (resp->body) {
            strcpy(resp->body, json);
        }
    }
}

static void response_error(http_response_t* resp, http_status_t status, const char* message) {
    char json[512];
    snprintf(json, sizeof(json), "{\"error\":{\"code\":%d,\"message\":\"%s\"}}", status, message);
    response_set_json(resp, status, json);
}

/* ============================================================================
 * Route Helpers
 * ========================================================================== */

static bool path_matches(const char* pattern, const char* path, char params[][64], int* param_count) {
    *param_count = 0;

    while (*pattern && *path) {
        if (*pattern == ':') {
            /* Extract parameter */
            pattern++;
            const char* param_start = path;
            while (*path && *path != '/') path++;

            if (*param_count < 8) {
                size_t len = path - param_start;
                if (len >= 64) len = 63;
                strncpy(params[*param_count], param_start, len);
                params[*param_count][len] = '\0';
                (*param_count)++;
            }

            /* Skip pattern parameter name */
            while (*pattern && *pattern != '/') pattern++;
        } else if (*pattern != *path) {
            return false;
        } else {
            pattern++;
            path++;
        }
    }

    return (*pattern == '\0' && *path == '\0');
}

static const char* get_query_param(const char* query, const char* key, char* value, size_t value_size) {
    if (!query || !key) return NULL;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "%s=", key);

    const char* start = strstr(query, pattern);
    if (!start) return NULL;

    /* Check it's at start or after & */
    if (start != query && *(start - 1) != '&') return NULL;

    start += strlen(pattern);
    const char* end = strchr(start, '&');
    if (!end) end = start + strlen(start);

    size_t len = end - start;
    if (len >= value_size) len = value_size - 1;
    strncpy(value, start, len);
    value[len] = '\0';

    return value;
}

/* ============================================================================
 * Case API Handlers
 * ========================================================================== */

static regislex_error_t api_cases_list(api_context_t* api_ctx) {
    regislex_case_filter_t filter = {0};
    filter.status = -1;
    filter.type = -1;

    /* Parse query parameters */
    char value[256];
    if (get_query_param(api_ctx->request->query_string, "status", value, sizeof(value))) {
        filter.status = atoi(value);
    }
    if (get_query_param(api_ctx->request->query_string, "type", value, sizeof(value))) {
        filter.type = atoi(value);
    }
    if (get_query_param(api_ctx->request->query_string, "search", value, sizeof(value))) {
        strncpy(filter.search_text, value, sizeof(filter.search_text) - 1);
    }

    regislex_case_list_t* list = NULL;
    regislex_error_t err = regislex_case_list(api_ctx->ctx, &filter, &list);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to list cases");
        return err;
    }

    /* Build JSON response */
    size_t buf_size = 65536;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_case_list_free(list);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;

    json_begin_object(&buf, &remaining);
    json_add_int(&buf, &remaining, "total", (int)list->count);
    json_add_key(&buf, &remaining, "items");
    json_begin_array(&buf, &remaining);

    for (size_t i = 0; i < list->count && remaining > 512; i++) {
        json_begin_object(&buf, &remaining);
        json_add_string(&buf, &remaining, "id", list->cases[i].id.value);
        json_add_string(&buf, &remaining, "case_number", list->cases[i].case_number);
        json_add_string(&buf, &remaining, "title", list->cases[i].title);
        json_add_int(&buf, &remaining, "type", list->cases[i].type);
        json_add_int(&buf, &remaining, "status", list->cases[i].status);
        json_end_object(&buf, &remaining);
        int n = snprintf(buf, remaining, ",");
        if (n > 0) { buf += n; remaining -= n; }
    }

    json_end_array(&buf, &remaining);
    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_case_list_free(list);

    return REGISLEX_SUCCESS;
}

static regislex_error_t api_cases_get(api_context_t* api_ctx) {
    char params[8][64];
    int param_count;

    if (!path_matches("/api/v1/cases/:id", api_ctx->request->path, params, &param_count) || param_count < 1) {
        response_error(api_ctx->response, HTTP_STATUS_BAD_REQUEST, "Invalid case ID");
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_uuid_t id;
    strncpy(id.value, params[0], sizeof(id.value) - 1);

    regislex_case_t* case_data = NULL;
    regislex_error_t err = regislex_case_get(api_ctx->ctx, &id, &case_data);
    if (err == REGISLEX_ERROR_NOT_FOUND) {
        response_error(api_ctx->response, HTTP_STATUS_NOT_FOUND, "Case not found");
        return err;
    }
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to get case");
        return err;
    }

    /* Build JSON response */
    size_t buf_size = 8192;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_case_free(case_data);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;
    char datetime_str[32];

    json_begin_object(&buf, &remaining);
    json_add_string(&buf, &remaining, "id", case_data->id.value);
    json_add_string(&buf, &remaining, "case_number", case_data->case_number);
    json_add_string(&buf, &remaining, "title", case_data->title);
    json_add_string(&buf, &remaining, "description", case_data->description);
    json_add_int(&buf, &remaining, "type", case_data->type);
    json_add_int(&buf, &remaining, "status", case_data->status);
    json_add_string(&buf, &remaining, "court_name", case_data->court_name);
    json_add_string(&buf, &remaining, "court_case_number", case_data->court_case_number);
    json_add_string(&buf, &remaining, "judge_name", case_data->judge_name);

    regislex_datetime_format(&case_data->created_at, datetime_str, sizeof(datetime_str));
    json_add_string(&buf, &remaining, "created_at", datetime_str);

    regislex_datetime_format(&case_data->updated_at, datetime_str, sizeof(datetime_str));
    json_add_string(&buf, &remaining, "updated_at", datetime_str);

    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_case_free(case_data);

    return REGISLEX_SUCCESS;
}

static regislex_error_t api_cases_create(api_context_t* api_ctx) {
    if (!api_ctx->request->body) {
        response_error(api_ctx->response, HTTP_STATUS_BAD_REQUEST, "Request body required");
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_case_t case_data = {0};

    /* Parse JSON body */
    json_get_string(api_ctx->request->body, "case_number", case_data.case_number, sizeof(case_data.case_number));
    json_get_string(api_ctx->request->body, "title", case_data.title, sizeof(case_data.title));
    json_get_string(api_ctx->request->body, "description", case_data.description, sizeof(case_data.description));
    case_data.type = json_get_int(api_ctx->request->body, "type", REGISLEX_CASE_TYPE_CIVIL);
    case_data.status = REGISLEX_STATUS_DRAFT;
    json_get_string(api_ctx->request->body, "court_name", case_data.court_name, sizeof(case_data.court_name));
    json_get_string(api_ctx->request->body, "court_case_number", case_data.court_case_number, sizeof(case_data.court_case_number));
    json_get_string(api_ctx->request->body, "judge_name", case_data.judge_name, sizeof(case_data.judge_name));

    if (case_data.case_number[0] == '\0' || case_data.title[0] == '\0') {
        response_error(api_ctx->response, HTTP_STATUS_BAD_REQUEST, "case_number and title are required");
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_case_t* new_case = NULL;
    regislex_error_t err = regislex_case_create(api_ctx->ctx, &case_data, &new_case);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to create case");
        return err;
    }

    /* Build response */
    char json[1024];
    snprintf(json, sizeof(json), "{\"id\":\"%s\",\"case_number\":\"%s\",\"message\":\"Case created successfully\"}",
             new_case->id.value, new_case->case_number);

    response_set_json(api_ctx->response, HTTP_STATUS_CREATED, json);
    regislex_case_free(new_case);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Deadline API Handlers
 * ========================================================================== */

static regislex_error_t api_deadlines_list(api_context_t* api_ctx) {
    regislex_deadline_filter_t filter = {0};
    filter.status = -1;
    filter.type = -1;

    char value[256];
    if (get_query_param(api_ctx->request->query_string, "case_id", value, sizeof(value))) {
        strncpy(filter.case_id.value, value, sizeof(filter.case_id.value) - 1);
    }
    if (get_query_param(api_ctx->request->query_string, "status", value, sizeof(value))) {
        filter.status = atoi(value);
    }

    regislex_deadline_list_t* list = NULL;
    regislex_error_t err = regislex_deadline_list(api_ctx->ctx, &filter, &list);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to list deadlines");
        return err;
    }

    size_t buf_size = 65536;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_deadline_list_free(list);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;
    char datetime_str[32];

    json_begin_object(&buf, &remaining);
    json_add_int(&buf, &remaining, "total", (int)list->count);
    json_add_key(&buf, &remaining, "items");
    json_begin_array(&buf, &remaining);

    for (size_t i = 0; i < list->count && remaining > 512; i++) {
        json_begin_object(&buf, &remaining);
        json_add_string(&buf, &remaining, "id", list->deadlines[i].id.value);
        json_add_string(&buf, &remaining, "title", list->deadlines[i].title);
        json_add_string(&buf, &remaining, "case_id", list->deadlines[i].case_id.value);
        json_add_int(&buf, &remaining, "type", list->deadlines[i].type);
        json_add_int(&buf, &remaining, "status", list->deadlines[i].status);
        json_add_int(&buf, &remaining, "priority", list->deadlines[i].priority);

        regislex_datetime_format(&list->deadlines[i].due_date, datetime_str, sizeof(datetime_str));
        json_add_string(&buf, &remaining, "due_date", datetime_str);

        json_end_object(&buf, &remaining);
        int n = snprintf(buf, remaining, ",");
        if (n > 0) { buf += n; remaining -= n; }
    }

    json_end_array(&buf, &remaining);
    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_deadline_list_free(list);

    return REGISLEX_SUCCESS;
}

static regislex_error_t api_deadlines_upcoming(api_context_t* api_ctx) {
    int days = 7;
    char value[64];
    if (get_query_param(api_ctx->request->query_string, "days", value, sizeof(value))) {
        days = atoi(value);
        if (days <= 0) days = 7;
        if (days > 365) days = 365;
    }

    regislex_deadline_list_t* list = NULL;
    regislex_error_t err = regislex_deadline_get_upcoming(api_ctx->ctx, days, &list);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to get upcoming deadlines");
        return err;
    }

    size_t buf_size = 32768;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_deadline_list_free(list);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;
    char datetime_str[32];

    json_begin_object(&buf, &remaining);
    json_add_int(&buf, &remaining, "days", days);
    json_add_int(&buf, &remaining, "total", (int)list->count);
    json_add_key(&buf, &remaining, "items");
    json_begin_array(&buf, &remaining);

    for (size_t i = 0; i < list->count && remaining > 512; i++) {
        json_begin_object(&buf, &remaining);
        json_add_string(&buf, &remaining, "id", list->deadlines[i].id.value);
        json_add_string(&buf, &remaining, "title", list->deadlines[i].title);
        json_add_string(&buf, &remaining, "case_id", list->deadlines[i].case_id.value);
        json_add_int(&buf, &remaining, "type", list->deadlines[i].type);
        json_add_int(&buf, &remaining, "priority", list->deadlines[i].priority);

        regislex_datetime_format(&list->deadlines[i].due_date, datetime_str, sizeof(datetime_str));
        json_add_string(&buf, &remaining, "due_date", datetime_str);

        json_end_object(&buf, &remaining);
        int n = snprintf(buf, remaining, ",");
        if (n > 0) { buf += n; remaining -= n; }
    }

    json_end_array(&buf, &remaining);
    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_deadline_list_free(list);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Document API Handlers
 * ========================================================================== */

static regislex_error_t api_documents_list(api_context_t* api_ctx) {
    regislex_document_filter_t filter = {0};
    filter.document_type = -1;
    filter.status = -1;

    char value[256];
    if (get_query_param(api_ctx->request->query_string, "folder_id", value, sizeof(value))) {
        strncpy(filter.folder_id.value, value, sizeof(filter.folder_id.value) - 1);
    }
    if (get_query_param(api_ctx->request->query_string, "case_id", value, sizeof(value))) {
        strncpy(filter.case_id.value, value, sizeof(filter.case_id.value) - 1);
    }
    if (get_query_param(api_ctx->request->query_string, "search", value, sizeof(value))) {
        strncpy(filter.search_text, value, sizeof(filter.search_text) - 1);
    }

    regislex_document_list_t* list = NULL;
    regislex_error_t err = regislex_document_list(api_ctx->ctx, &filter, &list);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to list documents");
        return err;
    }

    size_t buf_size = 65536;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_document_list_free(list);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;

    json_begin_object(&buf, &remaining);
    json_add_int(&buf, &remaining, "total", (int)list->count);
    json_add_key(&buf, &remaining, "items");
    json_begin_array(&buf, &remaining);

    for (size_t i = 0; i < list->count && remaining > 512; i++) {
        json_begin_object(&buf, &remaining);
        json_add_string(&buf, &remaining, "id", list->documents[i].id.value);
        json_add_string(&buf, &remaining, "filename", list->documents[i].filename);
        json_add_string(&buf, &remaining, "title", list->documents[i].title);
        json_add_string(&buf, &remaining, "mime_type", list->documents[i].mime_type);
        json_add_int64(&buf, &remaining, "file_size", list->documents[i].file_size);
        json_add_int(&buf, &remaining, "document_type", list->documents[i].document_type);
        json_add_int(&buf, &remaining, "status", list->documents[i].status);
        json_add_int(&buf, &remaining, "current_version", list->documents[i].current_version);
        json_end_object(&buf, &remaining);
        int n = snprintf(buf, remaining, ",");
        if (n > 0) { buf += n; remaining -= n; }
    }

    json_end_array(&buf, &remaining);
    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_document_list_free(list);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Report API Handlers
 * ========================================================================== */

static regislex_error_t api_reports_generate(api_context_t* api_ctx) {
    if (!api_ctx->request->body) {
        response_error(api_ctx->response, HTTP_STATUS_BAD_REQUEST, "Request body required");
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_report_type_t report_type = json_get_int(api_ctx->request->body, "type", REGISLEX_REPORT_CASELOAD_SUMMARY);

    regislex_report_params_t params = {0};
    /* Parse date range if provided */
    char date_str[32];
    if (json_get_string(api_ctx->request->body, "date_from", date_str, sizeof(date_str))) {
        regislex_datetime_parse(date_str, &params.date_from);
    }
    if (json_get_string(api_ctx->request->body, "date_to", date_str, sizeof(date_str))) {
        regislex_datetime_parse(date_str, &params.date_to);
    }

    regislex_report_t* report = NULL;
    regislex_error_t err = regislex_report_generate(api_ctx->ctx, report_type, &params, &report);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to generate report");
        return err;
    }

    /* Return report output */
    size_t buf_size = report->output_size + 1024;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_report_free(report);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;
    char datetime_str[32];

    json_begin_object(&buf, &remaining);
    json_add_string(&buf, &remaining, "id", report->id.value);
    json_add_string(&buf, &remaining, "name", report->name);
    json_add_int(&buf, &remaining, "type", report->type);

    regislex_datetime_format(&report->generated_at, datetime_str, sizeof(datetime_str));
    json_add_string(&buf, &remaining, "generated_at", datetime_str);

    json_add_key(&buf, &remaining, "output");
    int n = snprintf(buf, remaining, "\"%s\"", report->output_data ? report->output_data : "");
    if (n > 0) { buf += n; remaining -= n; }

    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_report_free(report);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Vendor API Handlers
 * ========================================================================== */

static regislex_error_t api_vendors_list(api_context_t* api_ctx) {
    regislex_vendor_filter_t filter = {0};
    filter.vendor_type = -1;
    filter.status = -1;

    char value[256];
    if (get_query_param(api_ctx->request->query_string, "status", value, sizeof(value))) {
        filter.status = atoi(value);
    }
    if (get_query_param(api_ctx->request->query_string, "search", value, sizeof(value))) {
        strncpy(filter.search_text, value, sizeof(filter.search_text) - 1);
    }

    regislex_vendor_list_t* list = NULL;
    regislex_error_t err = regislex_vendor_list(api_ctx->ctx, &filter, &list);
    if (err != REGISLEX_SUCCESS) {
        response_error(api_ctx->response, HTTP_STATUS_INTERNAL_ERROR, "Failed to list vendors");
        return err;
    }

    size_t buf_size = 65536;
    char* json = (char*)malloc(buf_size);
    if (!json) {
        regislex_vendor_list_free(list);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    char* buf = json;
    size_t remaining = buf_size;

    json_begin_object(&buf, &remaining);
    json_add_int(&buf, &remaining, "total", (int)list->count);
    json_add_key(&buf, &remaining, "items");
    json_begin_array(&buf, &remaining);

    for (size_t i = 0; i < list->count && remaining > 512; i++) {
        json_begin_object(&buf, &remaining);
        json_add_string(&buf, &remaining, "id", list->vendors[i].id.value);
        json_add_string(&buf, &remaining, "name", list->vendors[i].name);
        json_add_int(&buf, &remaining, "vendor_type", list->vendors[i].vendor_type);
        json_add_string(&buf, &remaining, "city", list->vendors[i].city);
        json_add_string(&buf, &remaining, "state", list->vendors[i].state);
        json_add_int(&buf, &remaining, "status", list->vendors[i].status);
        json_add_bool(&buf, &remaining, "diversity_certified", list->vendors[i].diversity_certified);
        json_end_object(&buf, &remaining);
        int n = snprintf(buf, remaining, ",");
        if (n > 0) { buf += n; remaining -= n; }
    }

    json_end_array(&buf, &remaining);
    json_end_object(&buf, &remaining);

    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    free(json);
    regislex_vendor_list_free(list);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Health Check Handler
 * ========================================================================== */

static regislex_error_t api_health(api_context_t* api_ctx) {
    char json[256];
    snprintf(json, sizeof(json),
             "{\"status\":\"healthy\",\"version\":\"%s\",\"database\":\"connected\"}",
             REGISLEX_VERSION);
    response_set_json(api_ctx->response, HTTP_STATUS_OK, json);
    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * API Routes Table
 * ========================================================================== */

static api_route_t api_routes[] = {
    /* Health */
    { "/api/v1/health",               HTTP_METHOD_GET,    api_health,             false },

    /* Cases */
    { "/api/v1/cases",                HTTP_METHOD_GET,    api_cases_list,         true  },
    { "/api/v1/cases",                HTTP_METHOD_POST,   api_cases_create,       true  },
    { "/api/v1/cases/:id",            HTTP_METHOD_GET,    api_cases_get,          true  },

    /* Deadlines */
    { "/api/v1/deadlines",            HTTP_METHOD_GET,    api_deadlines_list,     true  },
    { "/api/v1/deadlines/upcoming",   HTTP_METHOD_GET,    api_deadlines_upcoming, true  },

    /* Documents */
    { "/api/v1/documents",            HTTP_METHOD_GET,    api_documents_list,     true  },

    /* Reports */
    { "/api/v1/reports/generate",     HTTP_METHOD_POST,   api_reports_generate,   true  },

    /* Vendors (ELM) */
    { "/api/v1/vendors",              HTTP_METHOD_GET,    api_vendors_list,       true  },

    /* Terminator */
    { NULL, HTTP_METHOD_UNKNOWN, NULL, false }
};

/* ============================================================================
 * HTTP Server Core
 * ========================================================================== */

typedef struct {
    regislex_context_t* ctx;
    int server_socket;
    int port;
    bool running;
    platform_thread_t* thread;
} http_server_t;

static http_method_t parse_method(const char* method_str) {
    if (strcmp(method_str, "GET") == 0) return HTTP_METHOD_GET;
    if (strcmp(method_str, "POST") == 0) return HTTP_METHOD_POST;
    if (strcmp(method_str, "PUT") == 0) return HTTP_METHOD_PUT;
    if (strcmp(method_str, "PATCH") == 0) return HTTP_METHOD_PATCH;
    if (strcmp(method_str, "DELETE") == 0) return HTTP_METHOD_DELETE;
    if (strcmp(method_str, "OPTIONS") == 0) return HTTP_METHOD_OPTIONS;
    if (strcmp(method_str, "HEAD") == 0) return HTTP_METHOD_HEAD;
    return HTTP_METHOD_UNKNOWN;
}

static void parse_request_line(const char* line, http_request_t* req) {
    char method[16] = {0};
    char path[512] = {0};

    sscanf(line, "%15s %511s", method, path);
    req->method = parse_method(method);

    /* Split path and query string */
    char* query = strchr(path, '?');
    if (query) {
        *query = '\0';
        strncpy(req->query_string, query + 1, sizeof(req->query_string) - 1);
    }
    strncpy(req->path, path, sizeof(req->path) - 1);
}

static void parse_header(const char* line, http_request_t* req) {
    if (req->header_count >= HTTP_MAX_HEADERS) return;

    const char* colon = strchr(line, ':');
    if (!colon) return;

    size_t name_len = colon - line;
    if (name_len >= sizeof(req->headers[0].name)) return;

    strncpy(req->headers[req->header_count].name, line, name_len);

    const char* value = colon + 1;
    while (*value && isspace(*value)) value++;
    strncpy(req->headers[req->header_count].value, value, sizeof(req->headers[0].value) - 1);

    /* Extract common headers */
    if (strcasecmp(req->headers[req->header_count].name, "Content-Type") == 0) {
        strncpy(req->content_type, req->headers[req->header_count].value, sizeof(req->content_type) - 1);
    }
    else if (strcasecmp(req->headers[req->header_count].name, "Authorization") == 0) {
        /* Extract Bearer token */
        if (strncasecmp(req->headers[req->header_count].value, "Bearer ", 7) == 0) {
            strncpy(req->auth_token, req->headers[req->header_count].value + 7, sizeof(req->auth_token) - 1);
        }
    }

    req->header_count++;
}

static void send_response(int client_socket, http_response_t* resp) {
    char header_buf[2048];

    const char* status_text = "OK";
    switch (resp->status) {
        case HTTP_STATUS_CREATED: status_text = "Created"; break;
        case HTTP_STATUS_NO_CONTENT: status_text = "No Content"; break;
        case HTTP_STATUS_BAD_REQUEST: status_text = "Bad Request"; break;
        case HTTP_STATUS_UNAUTHORIZED: status_text = "Unauthorized"; break;
        case HTTP_STATUS_FORBIDDEN: status_text = "Forbidden"; break;
        case HTTP_STATUS_NOT_FOUND: status_text = "Not Found"; break;
        case HTTP_STATUS_METHOD_NOT_ALLOWED: status_text = "Method Not Allowed"; break;
        case HTTP_STATUS_INTERNAL_ERROR: status_text = "Internal Server Error"; break;
        default: break;
    }

    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, PATCH, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Connection: close\r\n"
        "\r\n",
        resp->status, status_text,
        resp->content_type[0] ? resp->content_type : "application/json",
        resp->body_length);

    send(client_socket, header_buf, header_len, 0);

    if (resp->body && resp->body_length > 0) {
        send(client_socket, resp->body, resp->body_length, 0);
    }
}

static void handle_client(http_server_t* server, int client_socket) {
    char buffer[HTTP_BUFFER_SIZE];
    ssize_t bytes_read;

    /* Read request */
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        platform_socket_close(client_socket);
        return;
    }
    buffer[bytes_read] = '\0';

    /* Parse request */
    http_request_t request = {0};
    http_response_t response = {0};

    char* line = strtok(buffer, "\r\n");
    if (line) {
        parse_request_line(line, &request);

        while ((line = strtok(NULL, "\r\n")) != NULL && line[0] != '\0') {
            parse_header(line, &request);
        }
    }

    /* Handle CORS preflight */
    if (request.method == HTTP_METHOD_OPTIONS) {
        response.status = HTTP_STATUS_NO_CONTENT;
        send_response(client_socket, &response);
        platform_socket_close(client_socket);
        return;
    }

    /* Find matching route */
    api_route_t* route = NULL;
    char params[8][64];
    int param_count;

    for (int i = 0; api_routes[i].path_pattern != NULL; i++) {
        if (api_routes[i].method == request.method &&
            path_matches(api_routes[i].path_pattern, request.path, params, &param_count)) {
            route = &api_routes[i];
            break;
        }
    }

    if (!route) {
        response_error(&response, HTTP_STATUS_NOT_FOUND, "Endpoint not found");
    } else {
        /* TODO: Add authentication check here */

        api_context_t api_ctx = {0};
        api_ctx.ctx = server->ctx;
        api_ctx.request = &request;
        api_ctx.response = &response;

        route->handler(&api_ctx);
    }

    send_response(client_socket, &response);

    /* Cleanup */
    free(response.body);
    free(request.body);
    platform_socket_close(client_socket);
}

static void* server_thread(void* arg) {
    http_server_t* server = (http_server_t*)arg;

    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server->server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server->running) {
                platform_sleep(10);
            }
            continue;
        }

        handle_client(server, client_socket);
    }

    return NULL;
}

/* ============================================================================
 * Public API
 * ========================================================================== */

regislex_error_t regislex_api_server_start(
    regislex_context_t* ctx,
    int port,
    void** out_server)
{
    if (!ctx || !out_server || port <= 0 || port > 65535) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    http_server_t* server = (http_server_t*)calloc(1, sizeof(http_server_t));
    if (!server) {
        return REGISLEX_ERROR_NO_MEMORY;
    }

    server->ctx = ctx;
    server->port = port;

    /* Create socket */
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        free(server);
        return REGISLEX_ERROR_IO;
    }

    /* Set socket options */
    int opt = 1;
    setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    /* Bind */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server->server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        platform_socket_close(server->server_socket);
        free(server);
        return REGISLEX_ERROR_IO;
    }

    /* Listen */
    if (listen(server->server_socket, 10) < 0) {
        platform_socket_close(server->server_socket);
        free(server);
        return REGISLEX_ERROR_IO;
    }

    server->running = true;

    /* Start server thread */
    regislex_error_t err = platform_thread_create(&server->thread, server_thread, server);
    if (err != REGISLEX_SUCCESS) {
        platform_socket_close(server->server_socket);
        free(server);
        return err;
    }

    *out_server = server;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_api_server_stop(void* server_handle) {
    if (!server_handle) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    http_server_t* server = (http_server_t*)server_handle;

    server->running = false;
    platform_socket_close(server->server_socket);

    if (server->thread) {
        platform_thread_join(server->thread);
        platform_thread_destroy(server->thread);
    }

    free(server);
    return REGISLEX_SUCCESS;
}
