/**
 * @file regislex.c
 * @brief RegisLex Core Implementation
 *
 * Main entry point and core functionality for the RegisLex legal software suite.
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include "database/database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Internal Context Structure
 * ============================================================================ */

struct regislex_context {
    regislex_config_t config;
    regislex_db_context_t* db;
    char last_error[REGISLEX_MAX_DESCRIPTION_LENGTH];
    bool initialized;
    platform_mutex_t* mutex;
    regislex_user_t* current_user;
};

/* ============================================================================
 * Version Information
 * ============================================================================ */

REGISLEX_API const char* regislex_version(void) {
    return REGISLEX_VERSION_STRING;
}

/* ============================================================================
 * Error Handling
 * ============================================================================ */

REGISLEX_API const char* regislex_get_error(regislex_context_t* ctx) {
    if (!ctx) {
        return "Invalid context";
    }
    return ctx->last_error;
}

static void set_error(regislex_context_t* ctx, const char* format, ...) {
    if (!ctx) return;

    va_list args;
    va_start(args, format);
    vsnprintf(ctx->last_error, sizeof(ctx->last_error), format, args);
    va_end(args);
}

/* ============================================================================
 * Configuration
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_config_default(regislex_config_t* config) {
    if (!config) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    memset(config, 0, sizeof(regislex_config_t));

    /* Application settings */
    strncpy(config->app_name, "RegisLex", sizeof(config->app_name) - 1);
    strncpy(config->log_level, "info", sizeof(config->log_level) - 1);

    /* Get application data directory */
    char app_data[REGISLEX_MAX_PATH_LENGTH];
    if (platform_get_app_data_dir("RegisLex", app_data, sizeof(app_data)) == PLATFORM_OK) {
        strncpy(config->data_dir, app_data, sizeof(config->data_dir) - 1);

        char log_dir[REGISLEX_MAX_PATH_LENGTH];
        platform_path_join(log_dir, sizeof(log_dir), app_data, "logs");
        strncpy(config->log_dir, log_dir, sizeof(config->log_dir) - 1);
    }

    /* Database defaults (SQLite) */
    strncpy(config->database.type, "sqlite", sizeof(config->database.type) - 1);
    config->database.pool_size = 5;
    config->database.timeout_seconds = 30;

    /* Server defaults */
    strncpy(config->server.host, "127.0.0.1", sizeof(config->server.host) - 1);
    config->server.port = 8080;
    config->server.use_ssl = false;
    config->server.max_connections = 100;
    config->server.request_timeout_seconds = 60;

    /* Storage defaults */
    strncpy(config->storage.type, "filesystem", sizeof(config->storage.type) - 1);
    config->storage.max_file_size = 100 * 1024 * 1024;  /* 100 MB */
    config->storage.encryption_enabled = true;

    /* Security settings */
    config->audit_logging_enabled = true;
    config->encryption_at_rest = true;
    config->session_timeout_minutes = 60;

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_config_load(const char* path, regislex_config_t* config) {
    if (!path || !config) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Start with defaults */
    regislex_error_t err = regislex_config_default(config);
    if (err != REGISLEX_OK) {
        return err;
    }

    /* Check if file exists */
    if (!platform_file_exists(path)) {
        return REGISLEX_ERROR_NOT_FOUND;
    }

    /* Load and parse configuration file */
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return REGISLEX_ERROR_IO;
    }

    char line[1024];
    char section[128] = "";

    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '#' || *trimmed == ';' || *trimmed == '\n' || *trimmed == '\0') {
            continue;
        }

        /* Section header */
        if (*trimmed == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
            }
            continue;
        }

        /* Key=Value pair */
        char* eq = strchr(trimmed, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = trimmed;
        char* value = eq + 1;

        /* Trim whitespace */
        while (*key && (key[strlen(key) - 1] == ' ' || key[strlen(key) - 1] == '\t')) {
            key[strlen(key) - 1] = '\0';
        }
        while (*value == ' ' || *value == '\t') value++;
        char* newline = strchr(value, '\n');
        if (newline) *newline = '\0';
        char* cr = strchr(value, '\r');
        if (cr) *cr = '\0';

        /* Apply configuration */
        if (strcmp(section, "app") == 0 || strcmp(section, "") == 0) {
            if (strcmp(key, "name") == 0) {
                strncpy(config->app_name, value, sizeof(config->app_name) - 1);
            } else if (strcmp(key, "data_dir") == 0) {
                strncpy(config->data_dir, value, sizeof(config->data_dir) - 1);
            } else if (strcmp(key, "log_dir") == 0) {
                strncpy(config->log_dir, value, sizeof(config->log_dir) - 1);
            } else if (strcmp(key, "log_level") == 0) {
                strncpy(config->log_level, value, sizeof(config->log_level) - 1);
            }
        } else if (strcmp(section, "database") == 0) {
            if (strcmp(key, "type") == 0) {
                strncpy(config->database.type, value, sizeof(config->database.type) - 1);
            } else if (strcmp(key, "host") == 0) {
                strncpy(config->database.host, value, sizeof(config->database.host) - 1);
            } else if (strcmp(key, "port") == 0) {
                config->database.port = atoi(value);
            } else if (strcmp(key, "database") == 0) {
                strncpy(config->database.database, value, sizeof(config->database.database) - 1);
            } else if (strcmp(key, "username") == 0) {
                strncpy(config->database.username, value, sizeof(config->database.username) - 1);
            } else if (strcmp(key, "password") == 0) {
                strncpy(config->database.password, value, sizeof(config->database.password) - 1);
            } else if (strcmp(key, "pool_size") == 0) {
                config->database.pool_size = atoi(value);
            }
        } else if (strcmp(section, "server") == 0) {
            if (strcmp(key, "host") == 0) {
                strncpy(config->server.host, value, sizeof(config->server.host) - 1);
            } else if (strcmp(key, "port") == 0) {
                config->server.port = atoi(value);
            } else if (strcmp(key, "use_ssl") == 0) {
                config->server.use_ssl = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "ssl_cert_path") == 0) {
                strncpy(config->server.ssl_cert_path, value, sizeof(config->server.ssl_cert_path) - 1);
            } else if (strcmp(key, "ssl_key_path") == 0) {
                strncpy(config->server.ssl_key_path, value, sizeof(config->server.ssl_key_path) - 1);
            }
        } else if (strcmp(section, "storage") == 0) {
            if (strcmp(key, "type") == 0) {
                strncpy(config->storage.type, value, sizeof(config->storage.type) - 1);
            } else if (strcmp(key, "base_path") == 0) {
                strncpy(config->storage.base_path, value, sizeof(config->storage.base_path) - 1);
            } else if (strcmp(key, "encryption_enabled") == 0) {
                config->storage.encryption_enabled = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            }
        }
    }

    fclose(fp);
    return REGISLEX_OK;
}

/* ============================================================================
 * Initialization and Shutdown
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_init(const regislex_config_t* config,
                                            regislex_context_t** ctx) {
    if (!ctx) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    /* Allocate context */
    regislex_context_t* new_ctx = (regislex_context_t*)platform_calloc(1, sizeof(regislex_context_t));
    if (!new_ctx) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    /* Load configuration */
    if (config) {
        memcpy(&new_ctx->config, config, sizeof(regislex_config_t));
    } else {
        regislex_error_t err = regislex_config_default(&new_ctx->config);
        if (err != REGISLEX_OK) {
            platform_free(new_ctx);
            return err;
        }
    }

    /* Create mutex */
    if (platform_mutex_create(&new_ctx->mutex) != PLATFORM_OK) {
        platform_free(new_ctx);
        return REGISLEX_ERROR;
    }

    /* Create data directory if needed */
    if (new_ctx->config.data_dir[0] != '\0' && !platform_file_exists(new_ctx->config.data_dir)) {
        platform_error_t perr = platform_mkdir(new_ctx->config.data_dir, true);
        if (perr != PLATFORM_OK && perr != PLATFORM_ERROR_ALREADY_EXISTS) {
            set_error(new_ctx, "Failed to create data directory: %s", new_ctx->config.data_dir);
            platform_mutex_destroy(new_ctx->mutex);
            platform_free(new_ctx);
            return REGISLEX_ERROR_IO;
        }
    }

    /* Create log directory if needed */
    if (new_ctx->config.log_dir[0] != '\0' && !platform_file_exists(new_ctx->config.log_dir)) {
        platform_mkdir(new_ctx->config.log_dir, true);
    }

    /* Initialize database */
    char db_path[REGISLEX_MAX_PATH_LENGTH];
    if (strcmp(new_ctx->config.database.type, "sqlite") == 0) {
        if (new_ctx->config.database.database[0] == '\0') {
            platform_path_join(db_path, sizeof(db_path),
                              new_ctx->config.data_dir, "regislex.db");
            strncpy(new_ctx->config.database.database, db_path,
                   sizeof(new_ctx->config.database.database) - 1);
        }
    }

    regislex_error_t db_err = regislex_db_init(&new_ctx->config.database, &new_ctx->db);
    if (db_err != REGISLEX_OK) {
        set_error(new_ctx, "Failed to initialize database");
        platform_mutex_destroy(new_ctx->mutex);
        platform_free(new_ctx);
        return db_err;
    }

    /* Run database migrations */
    db_err = regislex_db_migrate(new_ctx->db);
    if (db_err != REGISLEX_OK) {
        set_error(new_ctx, "Failed to run database migrations");
        regislex_db_shutdown(new_ctx->db);
        platform_mutex_destroy(new_ctx->mutex);
        platform_free(new_ctx);
        return db_err;
    }

    /* Create document storage directory */
    if (strcmp(new_ctx->config.storage.type, "filesystem") == 0) {
        if (new_ctx->config.storage.base_path[0] == '\0') {
            char storage_path[REGISLEX_MAX_PATH_LENGTH];
            platform_path_join(storage_path, sizeof(storage_path),
                              new_ctx->config.data_dir, "documents");
            strncpy(new_ctx->config.storage.base_path, storage_path,
                   sizeof(new_ctx->config.storage.base_path) - 1);
        }

        if (!platform_file_exists(new_ctx->config.storage.base_path)) {
            platform_mkdir(new_ctx->config.storage.base_path, true);
        }
    }

    new_ctx->initialized = true;
    *ctx = new_ctx;

    return REGISLEX_OK;
}

REGISLEX_API void regislex_shutdown(regislex_context_t* ctx) {
    if (!ctx) return;

    if (ctx->db) {
        regislex_db_shutdown(ctx->db);
        ctx->db = NULL;
    }

    if (ctx->mutex) {
        platform_mutex_destroy(ctx->mutex);
        ctx->mutex = NULL;
    }

    if (ctx->current_user) {
        platform_free(ctx->current_user);
        ctx->current_user = NULL;
    }

    ctx->initialized = false;
    platform_free(ctx);
}

/* ============================================================================
 * UUID Generation
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_uuid_generate(regislex_uuid_t* uuid) {
    if (!uuid) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    uint8_t bytes[16];
    if (platform_random_bytes(bytes, sizeof(bytes)) != PLATFORM_OK) {
        return REGISLEX_ERROR;
    }

    /* Set version 4 */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    /* Set variant */
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    snprintf(uuid->value, sizeof(uuid->value),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3],
             bytes[4], bytes[5],
             bytes[6], bytes[7],
             bytes[8], bytes[9],
             bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);

    return REGISLEX_OK;
}

/* ============================================================================
 * DateTime Functions
 * ============================================================================ */

REGISLEX_API regislex_error_t regislex_datetime_now(regislex_datetime_t* dt) {
    if (!dt) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    time_t now = time(NULL);
    struct tm* tm_info;

#ifdef REGISLEX_PLATFORM_WINDOWS
    struct tm tm_buf;
    gmtime_s(&tm_buf, &now);
    tm_info = &tm_buf;
#else
    tm_info = gmtime(&now);
#endif

    dt->year = tm_info->tm_year + 1900;
    dt->month = tm_info->tm_mon + 1;
    dt->day = tm_info->tm_mday;
    dt->hour = tm_info->tm_hour;
    dt->minute = tm_info->tm_min;
    dt->second = tm_info->tm_sec;
    dt->timezone_offset = 0;

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_datetime_parse(const char* str,
                                                      regislex_datetime_t* dt) {
    if (!str || !dt) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    memset(dt, 0, sizeof(regislex_datetime_t));

    /* Parse ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ or YYYY-MM-DD HH:MM:SS */
    int parsed = sscanf(str, "%d-%d-%dT%d:%d:%d",
                       &dt->year, &dt->month, &dt->day,
                       &dt->hour, &dt->minute, &dt->second);

    if (parsed < 3) {
        parsed = sscanf(str, "%d-%d-%d %d:%d:%d",
                       &dt->year, &dt->month, &dt->day,
                       &dt->hour, &dt->minute, &dt->second);
    }

    if (parsed < 3) {
        return REGISLEX_ERROR_VALIDATION;
    }

    /* Parse timezone offset if present */
    const char* tz = strchr(str, '+');
    if (!tz) tz = strrchr(str, '-');

    if (tz && tz > str + 10) {
        int tz_hour = 0, tz_min = 0;
        if (sscanf(tz + 1, "%d:%d", &tz_hour, &tz_min) >= 1) {
            dt->timezone_offset = (*tz == '-' ? -1 : 1) * (tz_hour * 60 + tz_min);
        }
    }

    return REGISLEX_OK;
}

REGISLEX_API regislex_error_t regislex_datetime_format(const regislex_datetime_t* dt,
                                                       char* buffer, size_t size) {
    if (!dt || !buffer || size < 26) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    if (dt->timezone_offset == 0) {
        snprintf(buffer, size, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                dt->year, dt->month, dt->day,
                dt->hour, dt->minute, dt->second);
    } else {
        int tz_hour = abs(dt->timezone_offset) / 60;
        int tz_min = abs(dt->timezone_offset) % 60;
        snprintf(buffer, size, "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
                dt->year, dt->month, dt->day,
                dt->hour, dt->minute, dt->second,
                dt->timezone_offset >= 0 ? '+' : '-',
                tz_hour, tz_min);
    }

    return REGISLEX_OK;
}
