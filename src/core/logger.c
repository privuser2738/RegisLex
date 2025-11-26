/**
 * @file logger.c
 * @brief RegisLex Logging Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

static const char* level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
static log_level_t current_level = LOG_LEVEL_INFO;
static FILE* log_file = NULL;
static platform_mutex_t* log_mutex = NULL;
static bool log_to_console = true;

static log_level_t parse_level(const char* level_str) {
    if (!level_str) return LOG_LEVEL_INFO;
    if (strcmp(level_str, "debug") == 0) return LOG_LEVEL_DEBUG;
    if (strcmp(level_str, "info") == 0) return LOG_LEVEL_INFO;
    if (strcmp(level_str, "warn") == 0 || strcmp(level_str, "warning") == 0) return LOG_LEVEL_WARN;
    if (strcmp(level_str, "error") == 0) return LOG_LEVEL_ERROR;
    if (strcmp(level_str, "fatal") == 0) return LOG_LEVEL_FATAL;
    return LOG_LEVEL_INFO;
}

regislex_error_t regislex_log_init(const char* log_path, const char* level) {
    if (log_mutex == NULL) {
        if (platform_mutex_create(&log_mutex) != PLATFORM_OK) {
            return REGISLEX_ERROR;
        }
    }

    current_level = parse_level(level);

    if (log_path && log_path[0] != '\0') {
        log_file = fopen(log_path, "a");
        if (!log_file) {
            return REGISLEX_ERROR_IO;
        }
    }

    return REGISLEX_OK;
}

void regislex_log_shutdown(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    if (log_mutex) {
        platform_mutex_destroy(log_mutex);
        log_mutex = NULL;
    }
}

void regislex_log_set_level(const char* level) {
    current_level = parse_level(level);
}

void regislex_log_set_console(bool enabled) {
    log_to_console = enabled;
}

static void log_write(log_level_t level, const char* format, va_list args) {
    if (level < current_level) return;

    if (log_mutex) platform_mutex_lock(log_mutex);

    time_t now = time(NULL);
    struct tm* tm_info;
    char timestamp[32];

#ifdef REGISLEX_PLATFORM_WINDOWS
    struct tm tm_buf;
    localtime_s(&tm_buf, &now);
    tm_info = &tm_buf;
#else
    tm_info = localtime(&now);
#endif

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    char message[4096];
    vsnprintf(message, sizeof(message), format, args);

    if (log_to_console) {
        FILE* out = (level >= LOG_LEVEL_WARN) ? stderr : stdout;
        fprintf(out, "[%s] [%s] %s\n", timestamp, level_names[level], message);
        fflush(out);
    }

    if (log_file) {
        fprintf(log_file, "[%s] [%s] %s\n", timestamp, level_names[level], message);
        fflush(log_file);
    }

    if (log_mutex) platform_mutex_unlock(log_mutex);
}

void regislex_log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void regislex_log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void regislex_log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void regislex_log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void regislex_log_fatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_write(LOG_LEVEL_FATAL, format, args);
    va_end(args);
}
