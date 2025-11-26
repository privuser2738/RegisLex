/**
 * @file string_utils.c
 * @brief RegisLex String Utilities Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* regislex_str_trim(char* str) {
    if (!str) return NULL;

    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) str++;

    if (*str == '\0') return str;

    /* Trim trailing whitespace */
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    return str;
}

char* regislex_str_lower(char* str) {
    if (!str) return NULL;
    for (char* p = str; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
    return str;
}

char* regislex_str_upper(char* str) {
    if (!str) return NULL;
    for (char* p = str; *p; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return str;
}

bool regislex_str_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    size_t prefix_len = strlen(prefix);
    return strncmp(str, prefix, prefix_len) == 0;
}

bool regislex_str_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char* regislex_str_replace(const char* str, const char* old_sub, const char* new_sub) {
    if (!str || !old_sub || !new_sub) return NULL;

    size_t old_len = strlen(old_sub);
    size_t new_len = strlen(new_sub);

    /* Count occurrences */
    int count = 0;
    const char* p = str;
    while ((p = strstr(p, old_sub)) != NULL) {
        count++;
        p += old_len;
    }

    if (count == 0) {
        return platform_strdup(str);
    }

    /* Allocate result */
    size_t result_len = strlen(str) + count * (new_len - old_len) + 1;
    char* result = (char*)platform_malloc(result_len);
    if (!result) return NULL;

    /* Perform replacement */
    char* dest = result;
    p = str;
    const char* next;
    while ((next = strstr(p, old_sub)) != NULL) {
        size_t prefix_len = next - p;
        memcpy(dest, p, prefix_len);
        dest += prefix_len;
        memcpy(dest, new_sub, new_len);
        dest += new_len;
        p = next + old_len;
    }
    strcpy(dest, p);

    return result;
}

int regislex_str_split(const char* str, char delimiter, char*** parts, int* count) {
    if (!str || !parts || !count) return -1;

    /* Count parts */
    int n = 1;
    for (const char* p = str; *p; p++) {
        if (*p == delimiter) n++;
    }

    *parts = (char**)platform_calloc(n, sizeof(char*));
    if (!*parts) return -1;

    /* Split */
    const char* start = str;
    int idx = 0;
    for (const char* p = str; ; p++) {
        if (*p == delimiter || *p == '\0') {
            size_t len = p - start;
            (*parts)[idx] = (char*)platform_malloc(len + 1);
            if ((*parts)[idx]) {
                memcpy((*parts)[idx], start, len);
                (*parts)[idx][len] = '\0';
            }
            idx++;
            if (*p == '\0') break;
            start = p + 1;
        }
    }

    *count = n;
    return 0;
}

void regislex_str_free_parts(char** parts, int count) {
    if (!parts) return;
    for (int i = 0; i < count; i++) {
        platform_free(parts[i]);
    }
    platform_free(parts);
}

char* regislex_str_join(char** parts, int count, const char* separator) {
    if (!parts || count <= 0) return platform_strdup("");

    size_t sep_len = separator ? strlen(separator) : 0;
    size_t total_len = 0;

    for (int i = 0; i < count; i++) {
        if (parts[i]) total_len += strlen(parts[i]);
        if (i < count - 1) total_len += sep_len;
    }

    char* result = (char*)platform_malloc(total_len + 1);
    if (!result) return NULL;

    char* p = result;
    for (int i = 0; i < count; i++) {
        if (parts[i]) {
            size_t len = strlen(parts[i]);
            memcpy(p, parts[i], len);
            p += len;
        }
        if (i < count - 1 && separator) {
            memcpy(p, separator, sep_len);
            p += sep_len;
        }
    }
    *p = '\0';

    return result;
}
