/**
 * @file json_utils.c
 * @brief JSON Utility Functions for API
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Simple JSON string escaping */
regislex_error_t regislex_json_escape(const char* input, char* output, size_t size) {
    if (!input || !output) return REGISLEX_ERROR_INVALID_ARGUMENT;
    size_t j = 0;
    for (size_t i = 0; input[i] && j < size - 2; i++) {
        char c = input[i];
        if (c == '"' || c == '\\') {
            output[j++] = '\\';
        } else if (c == '\n') {
            output[j++] = '\\'; output[j++] = 'n'; continue;
        } else if (c == '\r') {
            output[j++] = '\\'; output[j++] = 'r'; continue;
        } else if (c == '\t') {
            output[j++] = '\\'; output[j++] = 't'; continue;
        }
        output[j++] = c;
    }
    output[j] = '\0';
    return REGISLEX_OK;
}

/* Simple JSON object builder */
regislex_error_t regislex_json_object_start(char* buffer, size_t size) {
    if (!buffer || size < 2) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strcpy(buffer, "{");
    return REGISLEX_OK;
}

regislex_error_t regislex_json_object_end(char* buffer, size_t size) {
    size_t len = strlen(buffer);
    if (len + 2 > size) return REGISLEX_ERROR_INVALID_ARGUMENT;
    if (len > 1 && buffer[len-1] == ',') buffer[len-1] = '\0';
    strcat(buffer, "}");
    return REGISLEX_OK;
}

regislex_error_t regislex_json_add_string(char* buffer, size_t size, const char* key, const char* value) {
    if (!buffer || !key) return REGISLEX_ERROR_INVALID_ARGUMENT;
    size_t len = strlen(buffer);
    char escaped[1024] = "";
    if (value) regislex_json_escape(value, escaped, sizeof(escaped));
    int written = snprintf(buffer + len, size - len, "\"%s\":\"%s\",", key, escaped);
    return written > 0 ? REGISLEX_OK : REGISLEX_ERROR;
}

regislex_error_t regislex_json_add_int(char* buffer, size_t size, const char* key, int value) {
    if (!buffer || !key) return REGISLEX_ERROR_INVALID_ARGUMENT;
    size_t len = strlen(buffer);
    int written = snprintf(buffer + len, size - len, "\"%s\":%d,", key, value);
    return written > 0 ? REGISLEX_OK : REGISLEX_ERROR;
}

regislex_error_t regislex_json_add_bool(char* buffer, size_t size, const char* key, bool value) {
    if (!buffer || !key) return REGISLEX_ERROR_INVALID_ARGUMENT;
    size_t len = strlen(buffer);
    int written = snprintf(buffer + len, size - len, "\"%s\":%s,", key, value ? "true" : "false");
    return written > 0 ? REGISLEX_OK : REGISLEX_ERROR;
}
