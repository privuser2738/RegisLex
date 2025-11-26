/**
 * RegisLex - Enterprise Legal Software Suite
 * Authentication and Authorization Implementation
 *
 * Provides user authentication, session management, JWT tokens,
 * role-based access control, and permission checking.
 */

#include "regislex/regislex.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * Constants and Types
 * ========================================================================== */

#define REGISLEX_HASH_ITERATIONS 10000
#define REGISLEX_SALT_SIZE 32
#define REGISLEX_TOKEN_SIZE 64
#define REGISLEX_SESSION_EXPIRY_HOURS 24
#define REGISLEX_TOKEN_EXPIRY_HOURS 8

typedef enum {
    REGISLEX_ROLE_ADMIN = 0,
    REGISLEX_ROLE_ATTORNEY,
    REGISLEX_ROLE_PARALEGAL,
    REGISLEX_ROLE_CLERK,
    REGISLEX_ROLE_GUEST,
    REGISLEX_ROLE_COUNT
} regislex_role_t;

typedef enum {
    REGISLEX_PERM_CASE_READ = (1 << 0),
    REGISLEX_PERM_CASE_CREATE = (1 << 1),
    REGISLEX_PERM_CASE_UPDATE = (1 << 2),
    REGISLEX_PERM_CASE_DELETE = (1 << 3),
    REGISLEX_PERM_DEADLINE_READ = (1 << 4),
    REGISLEX_PERM_DEADLINE_CREATE = (1 << 5),
    REGISLEX_PERM_DEADLINE_UPDATE = (1 << 6),
    REGISLEX_PERM_DEADLINE_DELETE = (1 << 7),
    REGISLEX_PERM_DOCUMENT_READ = (1 << 8),
    REGISLEX_PERM_DOCUMENT_CREATE = (1 << 9),
    REGISLEX_PERM_DOCUMENT_UPDATE = (1 << 10),
    REGISLEX_PERM_DOCUMENT_DELETE = (1 << 11),
    REGISLEX_PERM_WORKFLOW_READ = (1 << 12),
    REGISLEX_PERM_WORKFLOW_MANAGE = (1 << 13),
    REGISLEX_PERM_REPORT_READ = (1 << 14),
    REGISLEX_PERM_REPORT_CREATE = (1 << 15),
    REGISLEX_PERM_USER_READ = (1 << 16),
    REGISLEX_PERM_USER_MANAGE = (1 << 17),
    REGISLEX_PERM_BILLING_READ = (1 << 18),
    REGISLEX_PERM_BILLING_MANAGE = (1 << 19),
    REGISLEX_PERM_ADMIN = (1 << 31)
} regislex_permission_t;

typedef struct {
    regislex_uuid_t id;
    char username[64];
    char email[128];
    char full_name[128];
    regislex_role_t role;
    uint32_t permissions;
    bool is_active;
    bool is_locked;
    int failed_login_attempts;
    regislex_datetime_t last_login;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_user_t;

typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t user_id;
    char token[REGISLEX_TOKEN_SIZE * 2 + 1];
    char refresh_token[REGISLEX_TOKEN_SIZE * 2 + 1];
    regislex_datetime_t created_at;
    regislex_datetime_t expires_at;
    char ip_address[64];
    char user_agent[256];
    bool is_valid;
} regislex_session_t;

/* ============================================================================
 * Role Permission Mappings
 * ========================================================================== */

static const uint32_t role_permissions[REGISLEX_ROLE_COUNT] = {
    /* ADMIN - All permissions */
    [REGISLEX_ROLE_ADMIN] = 0xFFFFFFFF,

    /* ATTORNEY - Full case, deadline, document, report access */
    [REGISLEX_ROLE_ATTORNEY] =
        REGISLEX_PERM_CASE_READ | REGISLEX_PERM_CASE_CREATE | REGISLEX_PERM_CASE_UPDATE |
        REGISLEX_PERM_DEADLINE_READ | REGISLEX_PERM_DEADLINE_CREATE | REGISLEX_PERM_DEADLINE_UPDATE |
        REGISLEX_PERM_DOCUMENT_READ | REGISLEX_PERM_DOCUMENT_CREATE | REGISLEX_PERM_DOCUMENT_UPDATE |
        REGISLEX_PERM_WORKFLOW_READ |
        REGISLEX_PERM_REPORT_READ | REGISLEX_PERM_REPORT_CREATE |
        REGISLEX_PERM_BILLING_READ,

    /* PARALEGAL - Case, deadline, document access (no delete) */
    [REGISLEX_ROLE_PARALEGAL] =
        REGISLEX_PERM_CASE_READ | REGISLEX_PERM_CASE_CREATE | REGISLEX_PERM_CASE_UPDATE |
        REGISLEX_PERM_DEADLINE_READ | REGISLEX_PERM_DEADLINE_CREATE | REGISLEX_PERM_DEADLINE_UPDATE |
        REGISLEX_PERM_DOCUMENT_READ | REGISLEX_PERM_DOCUMENT_CREATE | REGISLEX_PERM_DOCUMENT_UPDATE |
        REGISLEX_PERM_WORKFLOW_READ |
        REGISLEX_PERM_REPORT_READ,

    /* CLERK - Read and create only */
    [REGISLEX_ROLE_CLERK] =
        REGISLEX_PERM_CASE_READ | REGISLEX_PERM_CASE_CREATE |
        REGISLEX_PERM_DEADLINE_READ | REGISLEX_PERM_DEADLINE_CREATE |
        REGISLEX_PERM_DOCUMENT_READ | REGISLEX_PERM_DOCUMENT_CREATE,

    /* GUEST - Read only */
    [REGISLEX_ROLE_GUEST] =
        REGISLEX_PERM_CASE_READ |
        REGISLEX_PERM_DEADLINE_READ |
        REGISLEX_PERM_DOCUMENT_READ
};

/* ============================================================================
 * Password Hashing (PBKDF2-like with SHA256)
 * ========================================================================== */

/* Simple hash function (in production, use a proper crypto library) */
static void sha256_simple(const unsigned char* data, size_t len, unsigned char* hash) {
    /* This is a placeholder - in production use OpenSSL or similar */
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    /* Simplified mixing - NOT cryptographically secure */
    for (size_t i = 0; i < len; i++) {
        for (int j = 0; j < 8; j++) {
            h[j] = ((h[j] << 5) | (h[j] >> 27)) ^ data[i] ^ (h[(j+1) % 8] >> 3);
        }
    }

    for (int i = 0; i < 8; i++) {
        hash[i*4+0] = (h[i] >> 24) & 0xFF;
        hash[i*4+1] = (h[i] >> 16) & 0xFF;
        hash[i*4+2] = (h[i] >> 8) & 0xFF;
        hash[i*4+3] = h[i] & 0xFF;
    }
}

static void generate_salt(unsigned char* salt, size_t size) {
    platform_random_bytes(salt, size);
}

static void hash_password(const char* password, const unsigned char* salt, size_t salt_len,
                          unsigned char* hash, size_t hash_len) {
    unsigned char block[64];
    unsigned char temp_hash[32];

    /* Initial hash: password + salt */
    size_t pwd_len = strlen(password);
    size_t total_len = pwd_len + salt_len;
    unsigned char* combined = (unsigned char*)malloc(total_len);
    if (!combined) return;

    memcpy(combined, password, pwd_len);
    memcpy(combined + pwd_len, salt, salt_len);
    sha256_simple(combined, total_len, temp_hash);
    free(combined);

    /* Iterate */
    for (int i = 0; i < REGISLEX_HASH_ITERATIONS; i++) {
        memcpy(block, temp_hash, 32);
        memcpy(block + 32, salt, salt_len < 32 ? salt_len : 32);
        sha256_simple(block, 64, temp_hash);
    }

    memcpy(hash, temp_hash, hash_len < 32 ? hash_len : 32);
}

static void bytes_to_hex(const unsigned char* bytes, size_t len, char* hex) {
    const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        hex[i*2] = hex_chars[(bytes[i] >> 4) & 0x0F];
        hex[i*2+1] = hex_chars[bytes[i] & 0x0F];
    }
    hex[len*2] = '\0';
}

static void hex_to_bytes(const char* hex, unsigned char* bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        int hi = hex[i*2];
        int lo = hex[i*2+1];
        hi = (hi >= 'a') ? (hi - 'a' + 10) : (hi - '0');
        lo = (lo >= 'a') ? (lo - 'a' + 10) : (lo - '0');
        bytes[i] = (hi << 4) | lo;
    }
}

/* ============================================================================
 * Token Generation
 * ========================================================================== */

static void generate_token(char* token, size_t token_size) {
    unsigned char random_bytes[REGISLEX_TOKEN_SIZE];
    platform_random_bytes(random_bytes, sizeof(random_bytes));
    bytes_to_hex(random_bytes, sizeof(random_bytes), token);
}

/* ============================================================================
 * User Management
 * ========================================================================== */

regislex_error_t regislex_user_create(
    regislex_context_t* ctx,
    const char* username,
    const char* email,
    const char* password,
    const char* full_name,
    regislex_role_t role,
    regislex_user_t** out_user)
{
    if (!ctx || !username || !email || !password || !out_user) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_user = NULL;

    /* Check if username or email already exists */
    char check_sql[256];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT COUNT(*) FROM users WHERE username = '%s' OR email = '%s'",
             username, email);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, check_sql, &result);
    if (err == REGISLEX_SUCCESS && result && regislex_db_result_next(result)) {
        int count = regislex_db_result_get_int(result, 0);
        regislex_db_result_free(result);
        if (count > 0) {
            return REGISLEX_ERROR_ALREADY_EXISTS;
        }
    }

    /* Generate salt and hash password */
    unsigned char salt[REGISLEX_SALT_SIZE];
    unsigned char hash[32];
    generate_salt(salt, sizeof(salt));
    hash_password(password, salt, sizeof(salt), hash, sizeof(hash));

    char salt_hex[REGISLEX_SALT_SIZE * 2 + 1];
    char hash_hex[64 + 1];
    bytes_to_hex(salt, sizeof(salt), salt_hex);
    bytes_to_hex(hash, sizeof(hash), hash_hex);

    /* Generate user ID */
    regislex_uuid_t id;
    regislex_uuid_generate(&id);

    /* Get timestamps */
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    /* Calculate permissions */
    uint32_t permissions = role_permissions[role];

    const char* sql =
        "INSERT INTO users (id, username, email, password_hash, password_salt, "
        "full_name, role, permissions, is_active, is_locked, "
        "failed_login_attempts, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, username);
    regislex_db_bind_text(stmt, 3, email);
    regislex_db_bind_text(stmt, 4, hash_hex);
    regislex_db_bind_text(stmt, 5, salt_hex);
    regislex_db_bind_text(stmt, 6, full_name ? full_name : "");
    regislex_db_bind_int(stmt, 7, role);
    regislex_db_bind_int(stmt, 8, permissions);
    regislex_db_bind_int(stmt, 9, 1);  /* is_active */
    regislex_db_bind_int(stmt, 10, 0); /* is_locked */
    regislex_db_bind_int(stmt, 11, 0); /* failed_login_attempts */
    regislex_db_bind_text(stmt, 12, now_str);
    regislex_db_bind_text(stmt, 13, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_user_get(ctx, &id, out_user);
}

regislex_error_t regislex_user_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_user_t** out_user)
{
    if (!ctx || !id || !out_user) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_user = NULL;

    const char* sql =
        "SELECT id, username, email, full_name, role, permissions, "
        "is_active, is_locked, failed_login_attempts, last_login, "
        "created_at, updated_at FROM users WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_user_t* user = (regislex_user_t*)calloc(1, sizeof(regislex_user_t));
    if (!user) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(user->id.value, str, sizeof(user->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(user->username, str, sizeof(user->username) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(user->email, str, sizeof(user->email) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(user->full_name, str, sizeof(user->full_name) - 1);

    user->role = (regislex_role_t)regislex_db_column_int(stmt, 4);
    user->permissions = (uint32_t)regislex_db_column_int(stmt, 5);
    user->is_active = regislex_db_column_int(stmt, 6) != 0;
    user->is_locked = regislex_db_column_int(stmt, 7) != 0;
    user->failed_login_attempts = regislex_db_column_int(stmt, 8);

    str = regislex_db_column_text(stmt, 9);
    if (str) regislex_datetime_parse(str, &user->last_login);

    str = regislex_db_column_text(stmt, 10);
    if (str) regislex_datetime_parse(str, &user->created_at);

    str = regislex_db_column_text(stmt, 11);
    if (str) regislex_datetime_parse(str, &user->updated_at);

    regislex_db_finalize(stmt);

    *out_user = user;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_user_get_by_username(
    regislex_context_t* ctx,
    const char* username,
    regislex_user_t** out_user)
{
    if (!ctx || !username || !out_user) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_user = NULL;

    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id FROM users WHERE username = '%s'", username);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    if (!regislex_db_result_next(result)) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    const char* id_str = regislex_db_result_get_text(result, 0);
    regislex_uuid_t id;
    strncpy(id.value, id_str, sizeof(id.value) - 1);
    regislex_db_result_free(result);

    return regislex_user_get(ctx, &id, out_user);
}

void regislex_user_free(regislex_user_t* user) {
    free(user);
}

/* ============================================================================
 * Authentication
 * ========================================================================== */

regislex_error_t regislex_auth_login(
    regislex_context_t* ctx,
    const char* username,
    const char* password,
    const char* ip_address,
    const char* user_agent,
    regislex_session_t** out_session)
{
    if (!ctx || !username || !password || !out_session) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_session = NULL;

    /* Get user by username */
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, password_hash, password_salt, is_active, is_locked, "
             "failed_login_attempts FROM users WHERE username = '%s'", username);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    if (!regislex_db_result_next(result)) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_AUTH_FAILED;
    }

    const char* user_id = regislex_db_result_get_text(result, 0);
    const char* stored_hash = regislex_db_result_get_text(result, 1);
    const char* stored_salt = regislex_db_result_get_text(result, 2);
    int is_active = regislex_db_result_get_int(result, 3);
    int is_locked = regislex_db_result_get_int(result, 4);
    int failed_attempts = regislex_db_result_get_int(result, 5);

    if (!is_active) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_AUTH_FAILED;
    }

    if (is_locked) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_ACCOUNT_LOCKED;
    }

    /* Verify password */
    unsigned char salt[REGISLEX_SALT_SIZE];
    unsigned char computed_hash[32];
    hex_to_bytes(stored_salt, salt, sizeof(salt));
    hash_password(password, salt, sizeof(salt), computed_hash, sizeof(computed_hash));

    char computed_hash_hex[65];
    bytes_to_hex(computed_hash, sizeof(computed_hash), computed_hash_hex);

    if (strcmp(computed_hash_hex, stored_hash) != 0) {
        regislex_db_result_free(result);

        /* Increment failed login attempts */
        failed_attempts++;
        snprintf(sql, sizeof(sql),
                 "UPDATE users SET failed_login_attempts = %d%s WHERE username = '%s'",
                 failed_attempts,
                 failed_attempts >= 5 ? ", is_locked = 1" : "",
                 username);
        regislex_db_exec(ctx, sql, NULL, NULL);

        return REGISLEX_ERROR_AUTH_FAILED;
    }

    regislex_uuid_t uid;
    strncpy(uid.value, user_id, sizeof(uid.value) - 1);
    regislex_db_result_free(result);

    /* Create session */
    regislex_session_t* session = (regislex_session_t*)calloc(1, sizeof(regislex_session_t));
    if (!session) {
        return REGISLEX_ERROR_NO_MEMORY;
    }

    regislex_uuid_generate(&session->id);
    session->user_id = uid;
    generate_token(session->token, sizeof(session->token));
    generate_token(session->refresh_token, sizeof(session->refresh_token));
    session->is_valid = true;

    if (ip_address) strncpy(session->ip_address, ip_address, sizeof(session->ip_address) - 1);
    if (user_agent) strncpy(session->user_agent, user_agent, sizeof(session->user_agent) - 1);

    regislex_datetime_now(&session->created_at);
    session->expires_at = session->created_at;
    session->expires_at.hour += REGISLEX_TOKEN_EXPIRY_HOURS;
    /* Normalize datetime (simplified - in production handle day/month overflow) */

    char created_str[32], expires_str[32];
    regislex_datetime_format(&session->created_at, created_str, sizeof(created_str));
    regislex_datetime_format(&session->expires_at, expires_str, sizeof(expires_str));

    /* Store session in database */
    const char* session_sql =
        "INSERT INTO sessions (id, user_id, token, refresh_token, ip_address, "
        "user_agent, is_valid, created_at, expires_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, session_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        free(session);
        return err;
    }

    regislex_db_bind_text(stmt, 1, session->id.value);
    regislex_db_bind_text(stmt, 2, session->user_id.value);
    regislex_db_bind_text(stmt, 3, session->token);
    regislex_db_bind_text(stmt, 4, session->refresh_token);
    regislex_db_bind_text(stmt, 5, session->ip_address);
    regislex_db_bind_text(stmt, 6, session->user_agent);
    regislex_db_bind_int(stmt, 7, 1);
    regislex_db_bind_text(stmt, 8, created_str);
    regislex_db_bind_text(stmt, 9, expires_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) {
        free(session);
        return err;
    }

    /* Update user: reset failed attempts, update last_login */
    snprintf(sql, sizeof(sql),
             "UPDATE users SET failed_login_attempts = 0, last_login = '%s' WHERE id = '%s'",
             created_str, uid.value);
    regislex_db_exec(ctx, sql, NULL, NULL);

    *out_session = session;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_auth_validate_token(
    regislex_context_t* ctx,
    const char* token,
    regislex_user_t** out_user)
{
    if (!ctx || !token || !out_user) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_user = NULL;

    /* Find session by token */
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT user_id, expires_at, is_valid FROM sessions WHERE token = '%s'",
             token);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    if (!regislex_db_result_next(result)) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_AUTH_FAILED;
    }

    const char* user_id = regislex_db_result_get_text(result, 0);
    const char* expires_at = regislex_db_result_get_text(result, 1);
    int is_valid = regislex_db_result_get_int(result, 2);

    if (!is_valid) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_TOKEN_EXPIRED;
    }

    /* Check expiration */
    regislex_datetime_t expires;
    regislex_datetime_parse(expires_at, &expires);

    regislex_datetime_t now;
    regislex_datetime_now(&now);

    /* Simple comparison (year, month, day, hour, minute) */
    bool expired = false;
    if (now.year > expires.year) expired = true;
    else if (now.year == expires.year && now.month > expires.month) expired = true;
    else if (now.year == expires.year && now.month == expires.month && now.day > expires.day) expired = true;
    else if (now.year == expires.year && now.month == expires.month && now.day == expires.day &&
             now.hour > expires.hour) expired = true;

    if (expired) {
        regislex_db_result_free(result);
        /* Invalidate session */
        snprintf(sql, sizeof(sql), "UPDATE sessions SET is_valid = 0 WHERE token = '%s'", token);
        regislex_db_exec(ctx, sql, NULL, NULL);
        return REGISLEX_ERROR_TOKEN_EXPIRED;
    }

    regislex_uuid_t uid;
    strncpy(uid.value, user_id, sizeof(uid.value) - 1);
    regislex_db_result_free(result);

    return regislex_user_get(ctx, &uid, out_user);
}

regislex_error_t regislex_auth_logout(
    regislex_context_t* ctx,
    const char* token)
{
    if (!ctx || !token) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE sessions SET is_valid = 0 WHERE token = '%s'", token);
    return regislex_db_exec(ctx, sql, NULL, NULL);
}

regislex_error_t regislex_auth_refresh_token(
    regislex_context_t* ctx,
    const char* refresh_token,
    regislex_session_t** out_session)
{
    if (!ctx || !refresh_token || !out_session) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_session = NULL;

    /* Find session by refresh token */
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, user_id, ip_address, user_agent, is_valid FROM sessions "
             "WHERE refresh_token = '%s'", refresh_token);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    if (!regislex_db_result_next(result)) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_AUTH_FAILED;
    }

    const char* session_id = regislex_db_result_get_text(result, 0);
    const char* user_id = regislex_db_result_get_text(result, 1);
    const char* ip_address = regislex_db_result_get_text(result, 2);
    const char* user_agent = regislex_db_result_get_text(result, 3);
    int is_valid = regislex_db_result_get_int(result, 4);

    if (!is_valid) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_TOKEN_EXPIRED;
    }

    /* Invalidate old session */
    snprintf(sql, sizeof(sql), "UPDATE sessions SET is_valid = 0 WHERE id = '%s'", session_id);
    regislex_db_exec(ctx, sql, NULL, NULL);

    /* Create new session */
    regislex_session_t* session = (regislex_session_t*)calloc(1, sizeof(regislex_session_t));
    if (!session) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    regislex_uuid_generate(&session->id);
    strncpy(session->user_id.value, user_id, sizeof(session->user_id.value) - 1);
    generate_token(session->token, sizeof(session->token));
    generate_token(session->refresh_token, sizeof(session->refresh_token));
    session->is_valid = true;

    if (ip_address) strncpy(session->ip_address, ip_address, sizeof(session->ip_address) - 1);
    if (user_agent) strncpy(session->user_agent, user_agent, sizeof(session->user_agent) - 1);

    regislex_db_result_free(result);

    regislex_datetime_now(&session->created_at);
    session->expires_at = session->created_at;
    session->expires_at.hour += REGISLEX_TOKEN_EXPIRY_HOURS;

    char created_str[32], expires_str[32];
    regislex_datetime_format(&session->created_at, created_str, sizeof(created_str));
    regislex_datetime_format(&session->expires_at, expires_str, sizeof(expires_str));

    /* Store new session */
    const char* session_sql =
        "INSERT INTO sessions (id, user_id, token, refresh_token, ip_address, "
        "user_agent, is_valid, created_at, expires_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, session_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        free(session);
        return err;
    }

    regislex_db_bind_text(stmt, 1, session->id.value);
    regislex_db_bind_text(stmt, 2, session->user_id.value);
    regislex_db_bind_text(stmt, 3, session->token);
    regislex_db_bind_text(stmt, 4, session->refresh_token);
    regislex_db_bind_text(stmt, 5, session->ip_address);
    regislex_db_bind_text(stmt, 6, session->user_agent);
    regislex_db_bind_int(stmt, 7, 1);
    regislex_db_bind_text(stmt, 8, created_str);
    regislex_db_bind_text(stmt, 9, expires_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) {
        free(session);
        return err;
    }

    *out_session = session;
    return REGISLEX_SUCCESS;
}

void regislex_session_free(regislex_session_t* session) {
    if (session) {
        /* Zero out tokens before freeing */
        memset(session->token, 0, sizeof(session->token));
        memset(session->refresh_token, 0, sizeof(session->refresh_token));
        free(session);
    }
}

/* ============================================================================
 * Authorization (Permission Checking)
 * ========================================================================== */

bool regislex_auth_has_permission(const regislex_user_t* user, regislex_permission_t permission) {
    if (!user) return false;
    return (user->permissions & permission) != 0;
}

bool regislex_auth_can_access_case(
    regislex_context_t* ctx,
    const regislex_user_t* user,
    const regislex_uuid_t* case_id,
    regislex_permission_t required_permission)
{
    if (!user || !case_id) return false;

    /* Admins can access everything */
    if (user->role == REGISLEX_ROLE_ADMIN) return true;

    /* Check base permission */
    if (!regislex_auth_has_permission(user, required_permission)) return false;

    /* For granular access control, check if user is assigned to case */
    /* This would involve checking case assignments table */
    /* For now, return true if they have the permission */

    return true;
}

regislex_error_t regislex_auth_change_password(
    regislex_context_t* ctx,
    const regislex_uuid_t* user_id,
    const char* old_password,
    const char* new_password)
{
    if (!ctx || !user_id || !old_password || !new_password) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Verify old password */
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT password_hash, password_salt FROM users WHERE id = '%s'",
             user_id->value);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    if (!regislex_db_result_next(result)) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    const char* stored_hash = regislex_db_result_get_text(result, 0);
    const char* stored_salt = regislex_db_result_get_text(result, 1);

    unsigned char salt[REGISLEX_SALT_SIZE];
    unsigned char computed_hash[32];
    hex_to_bytes(stored_salt, salt, sizeof(salt));
    hash_password(old_password, salt, sizeof(salt), computed_hash, sizeof(computed_hash));

    char computed_hash_hex[65];
    bytes_to_hex(computed_hash, sizeof(computed_hash), computed_hash_hex);

    if (strcmp(computed_hash_hex, stored_hash) != 0) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_AUTH_FAILED;
    }

    regislex_db_result_free(result);

    /* Generate new salt and hash */
    unsigned char new_salt[REGISLEX_SALT_SIZE];
    unsigned char new_hash[32];
    generate_salt(new_salt, sizeof(new_salt));
    hash_password(new_password, new_salt, sizeof(new_salt), new_hash, sizeof(new_hash));

    char new_salt_hex[REGISLEX_SALT_SIZE * 2 + 1];
    char new_hash_hex[65];
    bytes_to_hex(new_salt, sizeof(new_salt), new_salt_hex);
    bytes_to_hex(new_hash, sizeof(new_hash), new_hash_hex);

    /* Update password */
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    snprintf(sql, sizeof(sql),
             "UPDATE users SET password_hash = '%s', password_salt = '%s', "
             "updated_at = '%s' WHERE id = '%s'",
             new_hash_hex, new_salt_hex, now_str, user_id->value);

    return regislex_db_exec(ctx, sql, NULL, NULL);
}

/* ============================================================================
 * Audit Logging
 * ========================================================================== */

regislex_error_t regislex_audit_log(
    regislex_context_t* ctx,
    const regislex_uuid_t* user_id,
    const char* action,
    const char* entity_type,
    const regislex_uuid_t* entity_id,
    const char* details)
{
    if (!ctx || !action) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_uuid_t log_id;
    regislex_uuid_generate(&log_id);

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO audit_log (id, user_id, action, entity_type, entity_id, "
        "details, created_at) VALUES (?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, log_id.value);

    if (user_id) {
        regislex_db_bind_text(stmt, 2, user_id->value);
    } else {
        regislex_db_bind_null(stmt, 2);
    }

    regislex_db_bind_text(stmt, 3, action);
    regislex_db_bind_text(stmt, 4, entity_type ? entity_type : "");

    if (entity_id) {
        regislex_db_bind_text(stmt, 5, entity_id->value);
    } else {
        regislex_db_bind_null(stmt, 5);
    }

    regislex_db_bind_text(stmt, 6, details ? details : "");
    regislex_db_bind_text(stmt, 7, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}
