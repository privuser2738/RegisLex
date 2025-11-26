/**
 * @file csv.c
 * @brief CSV Parser/Writer
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

regislex_error_t regislex_csv_escape(const char* input, char* output, size_t size) {
    if (!input || !output) return REGISLEX_ERROR_INVALID_ARGUMENT;
    bool needs_quotes = strchr(input, ',') || strchr(input, '"') || strchr(input, '\n');
    if (!needs_quotes) { strncpy(output, input, size - 1); return REGISLEX_OK; }
    size_t j = 0;
    output[j++] = '"';
    for (size_t i = 0; input[i] && j < size - 2; i++) {
        if (input[i] == '"') output[j++] = '"';
        output[j++] = input[i];
    }
    output[j++] = '"';
    output[j] = '\0';
    return REGISLEX_OK;
}

regislex_error_t regislex_csv_write_row(FILE* fp, const char** fields, int count) {
    if (!fp || !fields) return REGISLEX_ERROR_INVALID_ARGUMENT;
    char escaped[1024];
    for (int i = 0; i < count; i++) {
        if (i > 0) fputc(',', fp);
        if (fields[i]) {
            regislex_csv_escape(fields[i], escaped, sizeof(escaped));
            fputs(escaped, fp);
        }
    }
    fputc('\n', fp);
    return REGISLEX_OK;
}
