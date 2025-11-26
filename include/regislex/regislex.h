/**
 * @file regislex.h
 * @brief RegisLex - Enterprise Legal Software Suite
 *
 * Main header file for the RegisLex legal software suite.
 * This provides a unified API for all legal case management,
 * document management, and enterprise legal operations.
 *
 * @copyright Copyright (c) 2024 RegisLex Project
 * @license MIT
 */

#ifndef REGISLEX_H
#define REGISLEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/* Version information */
#define REGISLEX_VERSION_MAJOR 1
#define REGISLEX_VERSION_MINOR 0
#define REGISLEX_VERSION_PATCH 0
#define REGISLEX_VERSION_STRING "1.0.0"

/* Export/Import macros for shared library */
#ifdef REGISLEX_PLATFORM_WINDOWS
    #ifdef REGISLEX_BUILD_SHARED
        #ifdef REGISLEX_EXPORTS
            #define REGISLEX_API __declspec(dllexport)
        #else
            #define REGISLEX_API __declspec(dllimport)
        #endif
    #else
        #define REGISLEX_API
    #endif
#else
    #ifdef REGISLEX_BUILD_SHARED
        #define REGISLEX_API __attribute__((visibility("default")))
    #else
        #define REGISLEX_API
    #endif
#endif

/* Maximum string lengths */
#define REGISLEX_MAX_NAME_LENGTH        256
#define REGISLEX_MAX_PATH_LENGTH        4096
#define REGISLEX_MAX_DESCRIPTION_LENGTH 8192
#define REGISLEX_MAX_UUID_LENGTH        37

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef enum {
    REGISLEX_OK = 0,
    REGISLEX_ERROR = -1,
    REGISLEX_ERROR_INVALID_ARGUMENT = -2,
    REGISLEX_ERROR_OUT_OF_MEMORY = -3,
    REGISLEX_ERROR_NOT_FOUND = -4,
    REGISLEX_ERROR_ALREADY_EXISTS = -5,
    REGISLEX_ERROR_PERMISSION_DENIED = -6,
    REGISLEX_ERROR_DATABASE = -7,
    REGISLEX_ERROR_IO = -8,
    REGISLEX_ERROR_NETWORK = -9,
    REGISLEX_ERROR_TIMEOUT = -10,
    REGISLEX_ERROR_INVALID_STATE = -11,
    REGISLEX_ERROR_NOT_INITIALIZED = -12,
    REGISLEX_ERROR_ENCRYPTION = -13,
    REGISLEX_ERROR_VALIDATION = -14,
    REGISLEX_ERROR_DEADLINE_MISSED = -15,
    REGISLEX_ERROR_WORKFLOW_FAILED = -16,
    REGISLEX_ERROR_DOCUMENT_LOCKED = -17,
    REGISLEX_ERROR_VERSION_CONFLICT = -18,
    REGISLEX_ERROR_QUOTA_EXCEEDED = -19,
    REGISLEX_ERROR_UNSUPPORTED = -20
} regislex_error_t;

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

/* Core types */
typedef struct regislex_context regislex_context_t;
typedef struct regislex_config regislex_config_t;

/* Case Management */
typedef struct regislex_case regislex_case_t;
typedef struct regislex_party regislex_party_t;
typedef struct regislex_matter regislex_matter_t;

/* Deadline Management */
typedef struct regislex_deadline regislex_deadline_t;
typedef struct regislex_calendar regislex_calendar_t;
typedef struct regislex_reminder regislex_reminder_t;

/* Workflow */
typedef struct regislex_workflow regislex_workflow_t;
typedef struct regislex_task regislex_task_t;
typedef struct regislex_trigger regislex_trigger_t;

/* Document Management */
typedef struct regislex_document regislex_document_t;
typedef struct regislex_doc_template regislex_doc_template_t;
typedef struct regislex_doc_version regislex_doc_version_t;

/* Reporting */
typedef struct regislex_report regislex_report_t;
typedef struct regislex_report_template regislex_report_template_t;

/* Legislative Tracking */
typedef struct regislex_legislation regislex_legislation_t;
typedef struct regislex_stakeholder regislex_stakeholder_t;

/* Enterprise Legal Management */
typedef struct regislex_contract regislex_contract_t;
typedef struct regislex_invoice regislex_invoice_t;
typedef struct regislex_vendor regislex_vendor_t;
typedef struct regislex_risk regislex_risk_t;

/* ============================================================================
 * Common Types
 * ============================================================================ */

/**
 * @brief UUID structure for unique identification
 */
typedef struct {
    char value[REGISLEX_MAX_UUID_LENGTH];
} regislex_uuid_t;

/**
 * @brief Date and time structure
 */
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int timezone_offset; /* Minutes from UTC */
} regislex_datetime_t;

/**
 * @brief Money/currency structure for financial operations
 */
typedef struct {
    int64_t amount;      /* Amount in smallest currency unit (cents) */
    char currency[4];    /* ISO 4217 currency code (e.g., "USD") */
} regislex_money_t;

/**
 * @brief Generic key-value pair for metadata
 */
typedef struct {
    char* key;
    char* value;
} regislex_metadata_t;

/**
 * @brief Priority levels for cases, tasks, deadlines
 */
typedef enum {
    REGISLEX_PRIORITY_LOW = 0,
    REGISLEX_PRIORITY_NORMAL = 1,
    REGISLEX_PRIORITY_HIGH = 2,
    REGISLEX_PRIORITY_URGENT = 3,
    REGISLEX_PRIORITY_CRITICAL = 4
} regislex_priority_t;

/**
 * @brief Status types for various entities
 */
typedef enum {
    REGISLEX_STATUS_DRAFT = 0,
    REGISLEX_STATUS_ACTIVE = 1,
    REGISLEX_STATUS_PENDING = 2,
    REGISLEX_STATUS_ON_HOLD = 3,
    REGISLEX_STATUS_COMPLETED = 4,
    REGISLEX_STATUS_CLOSED = 5,
    REGISLEX_STATUS_ARCHIVED = 6,
    REGISLEX_STATUS_CANCELLED = 7
} regislex_status_t;

/**
 * @brief User structure
 */
typedef struct {
    regislex_uuid_t id;
    char username[REGISLEX_MAX_NAME_LENGTH];
    char email[REGISLEX_MAX_NAME_LENGTH];
    char full_name[REGISLEX_MAX_NAME_LENGTH];
    char role[64];
    bool is_active;
    regislex_datetime_t created_at;
    regislex_datetime_t last_login;
} regislex_user_t;

/* ============================================================================
 * Configuration
 * ============================================================================ */

/**
 * @brief Database configuration
 */
typedef struct {
    char type[32];          /* "sqlite", "postgresql", "mysql" */
    char host[256];
    int port;
    char database[256];
    char username[128];
    char password[256];
    char connection_string[1024];
    int pool_size;
    int timeout_seconds;
} regislex_db_config_t;

/**
 * @brief Server configuration
 */
typedef struct {
    char host[256];
    int port;
    bool use_ssl;
    char ssl_cert_path[REGISLEX_MAX_PATH_LENGTH];
    char ssl_key_path[REGISLEX_MAX_PATH_LENGTH];
    int max_connections;
    int request_timeout_seconds;
} regislex_server_config_t;

/**
 * @brief Storage configuration for documents
 */
typedef struct {
    char type[32];          /* "filesystem", "s3", "azure", "gcs" */
    char base_path[REGISLEX_MAX_PATH_LENGTH];
    char bucket[256];
    char region[64];
    char access_key[256];
    char secret_key[256];
    int64_t max_file_size;
    bool encryption_enabled;
} regislex_storage_config_t;

/**
 * @brief Main configuration structure
 */
struct regislex_config {
    char app_name[REGISLEX_MAX_NAME_LENGTH];
    char data_dir[REGISLEX_MAX_PATH_LENGTH];
    char log_dir[REGISLEX_MAX_PATH_LENGTH];
    char log_level[16];
    regislex_db_config_t database;
    regislex_server_config_t server;
    regislex_storage_config_t storage;
    bool audit_logging_enabled;
    bool encryption_at_rest;
    int session_timeout_minutes;
};

/* ============================================================================
 * Core Functions
 * ============================================================================ */

/**
 * @brief Get version string
 * @return Version string
 */
REGISLEX_API const char* regislex_version(void);

/**
 * @brief Get last error message
 * @param ctx Context
 * @return Error message string
 */
REGISLEX_API const char* regislex_get_error(regislex_context_t* ctx);

/**
 * @brief Initialize the RegisLex system
 * @param config Configuration (NULL for defaults)
 * @param ctx Output context pointer
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_init(
    const regislex_config_t* config,
    regislex_context_t** ctx
);

/**
 * @brief Shutdown the RegisLex system
 * @param ctx Context to shutdown
 */
REGISLEX_API void regislex_shutdown(regislex_context_t* ctx);

/**
 * @brief Load configuration from file
 * @param path Path to configuration file
 * @param config Output configuration
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_config_load(
    const char* path,
    regislex_config_t* config
);

/**
 * @brief Create default configuration
 * @param config Output configuration
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_config_default(regislex_config_t* config);

/**
 * @brief Generate a new UUID
 * @param uuid Output UUID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_uuid_generate(regislex_uuid_t* uuid);

/**
 * @brief Get current datetime
 * @param dt Output datetime
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_datetime_now(regislex_datetime_t* dt);

/**
 * @brief Parse datetime from ISO 8601 string
 * @param str Input string
 * @param dt Output datetime
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_datetime_parse(
    const char* str,
    regislex_datetime_t* dt
);

/**
 * @brief Format datetime to ISO 8601 string
 * @param dt Datetime
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_datetime_format(
    const regislex_datetime_t* dt,
    char* buffer,
    size_t size
);

/* ============================================================================
 * Include Module Headers
 * ============================================================================ */

#include "modules/case_management/case.h"
#include "modules/deadline_management/deadline.h"
#include "modules/workflow/workflow.h"
#include "modules/reporting/reporting.h"
#include "modules/document_management/document.h"
#include "modules/legislative_tracking/legislative.h"
#include "modules/elm/elm.h"

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_H */
