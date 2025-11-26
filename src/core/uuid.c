/**
 * @file uuid.c
 * @brief RegisLex UUID Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

regislex_error_t regislex_uuid_parse(const char* str, regislex_uuid_t* uuid) {
    if (!str || !uuid) return REGISLEX_ERROR_INVALID_ARGUMENT;

    /* Validate format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    size_t len = strlen(str);
    if (len != 36) return REGISLEX_ERROR_VALIDATION;

    for (size_t i = 0; i < len; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (str[i] != '-') return REGISLEX_ERROR_VALIDATION;
        } else {
            if (!isxdigit((unsigned char)str[i])) return REGISLEX_ERROR_VALIDATION;
        }
    }

    memcpy(uuid->value, str, 36);
    uuid->value[36] = '\0';

    return REGISLEX_OK;
}

bool regislex_uuid_equal(const regislex_uuid_t* a, const regislex_uuid_t* b) {
    if (!a || !b) return false;
    return strcmp(a->value, b->value) == 0;
}

bool regislex_uuid_is_null(const regislex_uuid_t* uuid) {
    if (!uuid) return true;
    return uuid->value[0] == '\0' ||
           strcmp(uuid->value, "00000000-0000-0000-0000-000000000000") == 0;
}

void regislex_uuid_clear(regislex_uuid_t* uuid) {
    if (uuid) {
        memset(uuid->value, 0, sizeof(uuid->value));
    }
}

regislex_error_t regislex_uuid_to_string(const regislex_uuid_t* uuid, char* buffer, size_t size) {
    if (!uuid || !buffer || size < 37) return REGISLEX_ERROR_INVALID_ARGUMENT;
    memcpy(buffer, uuid->value, 36);
    buffer[36] = '\0';
    return REGISLEX_OK;
}
