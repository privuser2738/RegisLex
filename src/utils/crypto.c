/**
 * @file crypto.c
 * @brief Cryptographic Utilities
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>
#include <stdio.h>

/* Simple SHA-256 stub (use OpenSSL in production) */
regislex_error_t regislex_sha256(const void* data, size_t len, char* hash_out) {
    if (!data || !hash_out) return REGISLEX_ERROR_INVALID_ARGUMENT;
    /* Placeholder - returns fake hash */
    const unsigned char* bytes = (const unsigned char*)data;
    unsigned int h = 0x811c9dc5;
    for (size_t i = 0; i < len; i++) h = (h ^ bytes[i]) * 0x01000193;
    snprintf(hash_out, 65, "%08x%08x%08x%08x%08x%08x%08x%08x", h, h^0x12345678, h^0x9abcdef0, h^0xfedcba98, h^0x76543210, h^0x01234567, h^0x89abcdef, h^0xdeadbeef);
    return REGISLEX_OK;
}

regislex_error_t regislex_hash_password(const char* password, const char* salt, char* hash_out, size_t size) {
    if (!password || !hash_out || size < 65) return REGISLEX_ERROR_INVALID_ARGUMENT;
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", salt ? salt : "", password);
    return regislex_sha256(combined, strlen(combined), hash_out);
}

regislex_error_t regislex_generate_salt(char* salt, size_t size) {
    if (!salt || size < 32) return REGISLEX_ERROR_INVALID_ARGUMENT;
    uint8_t bytes[16];
    if (platform_random_bytes(bytes, sizeof(bytes)) != PLATFORM_OK) return REGISLEX_ERROR;
    for (int i = 0; i < 16; i++) snprintf(salt + i*2, 3, "%02x", bytes[i]);
    return REGISLEX_OK;
}

bool regislex_verify_password(const char* password, const char* salt, const char* stored_hash) {
    if (!password || !stored_hash) return false;
    char computed[65];
    if (regislex_hash_password(password, salt, computed, sizeof(computed)) != REGISLEX_OK) return false;
    return strcmp(computed, stored_hash) == 0;
}
