/**
 * @file auth.c
 * @brief API Authentication Middleware
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

static char api_key[256] = "";

regislex_error_t regislex_api_auth_init(const char* key) {
    if (key) strncpy(api_key, key, sizeof(api_key) - 1);
    return REGISLEX_OK;
}

bool regislex_api_auth_validate_key(const char* key) {
    if (!key || api_key[0] == '\0') return false;
    return strcmp(key, api_key) == 0;
}

bool regislex_api_auth_validate_token(const char* token) {
    (void)token;
    /* TODO: Validate JWT token */
    return token != NULL && strlen(token) > 0;
}

regislex_error_t regislex_api_auth_extract_user(const char* token, char* user_id, size_t size) {
    (void)token;
    if (!user_id) return REGISLEX_ERROR_INVALID_ARGUMENT;
    user_id[0] = '\0';
    return REGISLEX_ERROR_UNSUPPORTED;
}
