/**
 * @file config.c
 * @brief RegisLex Configuration Management
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

regislex_error_t regislex_config_save(const char* path, const regislex_config_t* config) {
    if (!path || !config) return REGISLEX_ERROR_INVALID_ARGUMENT;

    FILE* fp = fopen(path, "w");
    if (!fp) return REGISLEX_ERROR_IO;

    fprintf(fp, "# RegisLex Configuration File\n\n");

    fprintf(fp, "[app]\n");
    fprintf(fp, "name=%s\n", config->app_name);
    fprintf(fp, "data_dir=%s\n", config->data_dir);
    fprintf(fp, "log_dir=%s\n", config->log_dir);
    fprintf(fp, "log_level=%s\n", config->log_level);
    fprintf(fp, "\n");

    fprintf(fp, "[database]\n");
    fprintf(fp, "type=%s\n", config->database.type);
    if (config->database.host[0]) fprintf(fp, "host=%s\n", config->database.host);
    if (config->database.port > 0) fprintf(fp, "port=%d\n", config->database.port);
    if (config->database.database[0]) fprintf(fp, "database=%s\n", config->database.database);
    if (config->database.username[0]) fprintf(fp, "username=%s\n", config->database.username);
    fprintf(fp, "pool_size=%d\n", config->database.pool_size);
    fprintf(fp, "\n");

    fprintf(fp, "[server]\n");
    fprintf(fp, "host=%s\n", config->server.host);
    fprintf(fp, "port=%d\n", config->server.port);
    fprintf(fp, "use_ssl=%s\n", config->server.use_ssl ? "true" : "false");
    if (config->server.ssl_cert_path[0]) fprintf(fp, "ssl_cert_path=%s\n", config->server.ssl_cert_path);
    if (config->server.ssl_key_path[0]) fprintf(fp, "ssl_key_path=%s\n", config->server.ssl_key_path);
    fprintf(fp, "max_connections=%d\n", config->server.max_connections);
    fprintf(fp, "\n");

    fprintf(fp, "[storage]\n");
    fprintf(fp, "type=%s\n", config->storage.type);
    if (config->storage.base_path[0]) fprintf(fp, "base_path=%s\n", config->storage.base_path);
    fprintf(fp, "encryption_enabled=%s\n", config->storage.encryption_enabled ? "true" : "false");
    fprintf(fp, "\n");

    fprintf(fp, "[security]\n");
    fprintf(fp, "audit_logging=%s\n", config->audit_logging_enabled ? "true" : "false");
    fprintf(fp, "encryption_at_rest=%s\n", config->encryption_at_rest ? "true" : "false");
    fprintf(fp, "session_timeout=%d\n", config->session_timeout_minutes);

    fclose(fp);
    return REGISLEX_OK;
}

regislex_error_t regislex_config_validate(const regislex_config_t* config) {
    if (!config) return REGISLEX_ERROR_INVALID_ARGUMENT;

    /* Validate database type */
    if (strcmp(config->database.type, "sqlite") != 0 &&
        strcmp(config->database.type, "postgres") != 0 &&
        strcmp(config->database.type, "mysql") != 0) {
        return REGISLEX_ERROR_VALIDATION;
    }

    /* Validate port ranges */
    if (config->server.port < 1 || config->server.port > 65535) {
        return REGISLEX_ERROR_VALIDATION;
    }

    /* Validate SSL configuration */
    if (config->server.use_ssl) {
        if (config->server.ssl_cert_path[0] == '\0' ||
            config->server.ssl_key_path[0] == '\0') {
            return REGISLEX_ERROR_VALIDATION;
        }
    }

    return REGISLEX_OK;
}
