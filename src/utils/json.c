/**
 * @file json.c
 * @brief JSON Parser/Writer
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type_t;

typedef struct json_value {
    json_type_t type;
    union {
        bool boolean;
        double number;
        char* string;
        struct { struct json_value** items; int count; } array;
        struct { char** keys; struct json_value** values; int count; } object;
    } data;
} json_value_t;

json_value_t* json_parse(const char* str) {
    (void)str;
    /* TODO: Implement JSON parser */
    return NULL;
}

void json_free(json_value_t* value) {
    if (!value) return;
    if (value->type == JSON_STRING && value->data.string) platform_free(value->data.string);
    platform_free(value);
}

char* json_stringify(const json_value_t* value) {
    if (!value) return platform_strdup("null");
    switch (value->type) {
        case JSON_NULL: return platform_strdup("null");
        case JSON_BOOL: return platform_strdup(value->data.boolean ? "true" : "false");
        case JSON_STRING: {
            size_t len = strlen(value->data.string) + 3;
            char* s = (char*)platform_malloc(len);
            if (s) snprintf(s, len, "\"%s\"", value->data.string);
            return s;
        }
        default: return platform_strdup("null");
    }
}
