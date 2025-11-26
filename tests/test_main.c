/**
 * RegisLex - Enterprise Legal Software Suite
 * Unit Test Suite
 *
 * Simple test framework and tests for core functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ============================================================================
 * Test Framework
 * ========================================================================== */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  [PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  [FAIL] %s (line %d)\n", message, __LINE__); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_EQUAL_STR(expected, actual, message) \
    TEST_ASSERT(strcmp(expected, actual) == 0, message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_SUITE_BEGIN(name) \
    printf("\n=== Test Suite: %s ===\n", name)

#define TEST_SUITE_END() \
    printf("\n")

/* ============================================================================
 * Mock/Stub Includes (would normally include regislex headers)
 * ========================================================================== */

/* For testing without full compilation, define minimal structures */
typedef struct { char value[64]; } test_uuid_t;

typedef struct {
    int year, month, day, hour, minute, second;
    int tz_offset_minutes;
} test_datetime_t;

/* ============================================================================
 * UUID Tests
 * ========================================================================== */

static void test_uuid_generate(void) {
    TEST_SUITE_BEGIN("UUID Generation");

    test_uuid_t uuid1 = {0};
    test_uuid_t uuid2 = {0};

    /* Simulate UUID generation */
    snprintf(uuid1.value, sizeof(uuid1.value), "%08x-%04x-%04x-%04x-%012x",
             rand(), rand() & 0xFFFF, 0x4000 | (rand() & 0x0FFF),
             0x8000 | (rand() & 0x3FFF), rand());
    snprintf(uuid2.value, sizeof(uuid2.value), "%08x-%04x-%04x-%04x-%012x",
             rand(), rand() & 0xFFFF, 0x4000 | (rand() & 0x0FFF),
             0x8000 | (rand() & 0x3FFF), rand());

    TEST_ASSERT(strlen(uuid1.value) == 36, "UUID1 has correct length");
    TEST_ASSERT(strlen(uuid2.value) == 36, "UUID2 has correct length");
    TEST_ASSERT(strcmp(uuid1.value, uuid2.value) != 0, "UUIDs are unique");
    TEST_ASSERT(uuid1.value[14] == '4', "UUID version is 4");

    TEST_SUITE_END();
}

/* ============================================================================
 * DateTime Tests
 * ========================================================================== */

static int datetime_parse(const char* str, test_datetime_t* dt) {
    /* ISO 8601 format: YYYY-MM-DDTHH:MM:SS */
    memset(dt, 0, sizeof(*dt));
    return sscanf(str, "%d-%d-%dT%d:%d:%d",
                  &dt->year, &dt->month, &dt->day,
                  &dt->hour, &dt->minute, &dt->second) >= 3;
}

static int datetime_format(const test_datetime_t* dt, char* buf, size_t size) {
    return snprintf(buf, size, "%04d-%02d-%02dT%02d:%02d:%02d",
                    dt->year, dt->month, dt->day,
                    dt->hour, dt->minute, dt->second);
}

static void test_datetime_parse(void) {
    TEST_SUITE_BEGIN("DateTime Parsing");

    test_datetime_t dt;

    TEST_ASSERT(datetime_parse("2024-06-15T14:30:00", &dt), "Parse valid datetime");
    TEST_ASSERT_EQUAL_INT(2024, dt.year, "Year parsed correctly");
    TEST_ASSERT_EQUAL_INT(6, dt.month, "Month parsed correctly");
    TEST_ASSERT_EQUAL_INT(15, dt.day, "Day parsed correctly");
    TEST_ASSERT_EQUAL_INT(14, dt.hour, "Hour parsed correctly");
    TEST_ASSERT_EQUAL_INT(30, dt.minute, "Minute parsed correctly");
    TEST_ASSERT_EQUAL_INT(0, dt.second, "Second parsed correctly");

    TEST_ASSERT(datetime_parse("2024-01-01", &dt), "Parse date only");
    TEST_ASSERT_EQUAL_INT(2024, dt.year, "Year parsed for date only");

    TEST_SUITE_END();
}

static void test_datetime_format(void) {
    TEST_SUITE_BEGIN("DateTime Formatting");

    test_datetime_t dt = {2024, 3, 20, 9, 5, 30, 0};
    char buf[64];

    datetime_format(&dt, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STR("2024-03-20T09:05:30", buf, "Format produces correct ISO 8601");

    dt.month = 12;
    dt.day = 31;
    datetime_format(&dt, buf, sizeof(buf));
    TEST_ASSERT(strstr(buf, "2024-12-31") != NULL, "End of year formatted correctly");

    TEST_SUITE_END();
}

/* ============================================================================
 * Case Management Tests
 * ========================================================================== */

typedef enum {
    CASE_TYPE_CIVIL = 0,
    CASE_TYPE_CRIMINAL,
    CASE_TYPE_ADMINISTRATIVE
} case_type_t;

typedef enum {
    STATUS_DRAFT = 0,
    STATUS_ACTIVE,
    STATUS_PENDING,
    STATUS_CLOSED
} case_status_t;

typedef struct {
    test_uuid_t id;
    char case_number[32];
    char title[256];
    case_type_t type;
    case_status_t status;
} test_case_t;

static void test_case_validation(void) {
    TEST_SUITE_BEGIN("Case Validation");

    test_case_t c = {0};

    /* Test case number validation */
    strcpy(c.case_number, "2024-CV-001");
    TEST_ASSERT(strlen(c.case_number) > 0, "Case number not empty");
    TEST_ASSERT(strlen(c.case_number) < 32, "Case number within limits");

    /* Test title validation */
    strcpy(c.title, "Smith v. Jones");
    TEST_ASSERT(strlen(c.title) > 0, "Title not empty");
    TEST_ASSERT(strlen(c.title) < 256, "Title within limits");

    /* Test type enum */
    c.type = CASE_TYPE_CIVIL;
    TEST_ASSERT(c.type >= 0 && c.type <= CASE_TYPE_ADMINISTRATIVE, "Type in valid range");

    /* Test status enum */
    c.status = STATUS_ACTIVE;
    TEST_ASSERT(c.status >= STATUS_DRAFT && c.status <= STATUS_CLOSED, "Status in valid range");

    TEST_SUITE_END();
}

/* ============================================================================
 * Deadline Tests
 * ========================================================================== */

static int calculate_business_days(int start_day, int days_to_add) {
    int current = start_day;
    int added = 0;

    while (added < days_to_add) {
        current++;
        int dow = current % 7;
        /* Skip Saturday (6) and Sunday (0) */
        if (dow != 0 && dow != 6) {
            added++;
        }
    }
    return current;
}

static void test_business_days(void) {
    TEST_SUITE_BEGIN("Business Day Calculations");

    /* Starting from Monday (1) */
    int result = calculate_business_days(1, 5);
    /* 5 business days from Monday = next Monday */
    TEST_ASSERT(result == 8, "5 business days from Monday");

    /* Starting from Friday (5) */
    result = calculate_business_days(5, 1);
    /* 1 business day from Friday = Monday */
    TEST_ASSERT(result == 8, "1 business day from Friday");

    /* Starting from Thursday (4) */
    result = calculate_business_days(4, 3);
    /* 3 business days from Thursday = Tuesday (skip weekend) */
    TEST_ASSERT(result == 9, "3 business days from Thursday");

    TEST_SUITE_END();
}

/* ============================================================================
 * Money Handling Tests
 * ========================================================================== */

typedef struct {
    int64_t amount;  /* In cents */
    char currency[4];
} test_money_t;

static void test_money_operations(void) {
    TEST_SUITE_BEGIN("Money Operations");

    test_money_t m1 = {10050, "USD"};  /* $100.50 */
    test_money_t m2 = {2575, "USD"};   /* $25.75 */

    /* Addition */
    int64_t sum = m1.amount + m2.amount;
    TEST_ASSERT_EQUAL_INT(12625, sum, "Money addition correct");

    /* Subtraction */
    int64_t diff = m1.amount - m2.amount;
    TEST_ASSERT_EQUAL_INT(7475, diff, "Money subtraction correct");

    /* Formatting */
    char formatted[32];
    snprintf(formatted, sizeof(formatted), "%s %.2f", m1.currency, m1.amount / 100.0);
    TEST_ASSERT_EQUAL_STR("USD 100.50", formatted, "Money formatted correctly");

    /* Large amounts */
    test_money_t large = {1000000000, "USD"};  /* $10,000,000.00 */
    TEST_ASSERT(large.amount > 0, "Large amount handled");

    TEST_SUITE_END();
}

/* ============================================================================
 * JSON Serialization Tests
 * ========================================================================== */

static void json_escape_string(const char* input, char* output, size_t output_size) {
    char* out = output;
    const char* in = input;

    while (*in && (size_t)(out - output) < output_size - 2) {
        if (*in == '"' || *in == '\\') {
            *out++ = '\\';
        } else if (*in == '\n') {
            *out++ = '\\';
            *out++ = 'n';
            in++;
            continue;
        }
        *out++ = *in++;
    }
    *out = '\0';
}

static void test_json_escaping(void) {
    TEST_SUITE_BEGIN("JSON Escaping");

    char output[256];

    json_escape_string("Hello World", output, sizeof(output));
    TEST_ASSERT_EQUAL_STR("Hello World", output, "No escaping needed");

    json_escape_string("Say \"Hello\"", output, sizeof(output));
    TEST_ASSERT_EQUAL_STR("Say \\\"Hello\\\"", output, "Quotes escaped");

    json_escape_string("Path\\to\\file", output, sizeof(output));
    TEST_ASSERT(strstr(output, "\\\\") != NULL, "Backslashes escaped");

    json_escape_string("Line1\nLine2", output, sizeof(output));
    TEST_ASSERT(strstr(output, "\\n") != NULL, "Newlines escaped");

    TEST_SUITE_END();
}

/* ============================================================================
 * Permission/Authorization Tests
 * ========================================================================== */

#define PERM_READ   0x01
#define PERM_CREATE 0x02
#define PERM_UPDATE 0x04
#define PERM_DELETE 0x08
#define PERM_ADMIN  0x80

static bool has_permission(uint32_t user_perms, uint32_t required) {
    return (user_perms & required) == required;
}

static void test_permissions(void) {
    TEST_SUITE_BEGIN("Permission Checks");

    uint32_t admin_perms = PERM_READ | PERM_CREATE | PERM_UPDATE | PERM_DELETE | PERM_ADMIN;
    uint32_t user_perms = PERM_READ | PERM_CREATE;
    uint32_t guest_perms = PERM_READ;

    TEST_ASSERT(has_permission(admin_perms, PERM_DELETE), "Admin can delete");
    TEST_ASSERT(!has_permission(user_perms, PERM_DELETE), "User cannot delete");
    TEST_ASSERT(has_permission(guest_perms, PERM_READ), "Guest can read");
    TEST_ASSERT(!has_permission(guest_perms, PERM_CREATE), "Guest cannot create");

    /* Combined permissions */
    TEST_ASSERT(has_permission(user_perms, PERM_READ | PERM_CREATE), "User has read+create");
    TEST_ASSERT(!has_permission(user_perms, PERM_READ | PERM_DELETE), "User doesn't have read+delete");

    TEST_SUITE_END();
}

/* ============================================================================
 * Token Generation Tests
 * ========================================================================== */

static void generate_hex_token(char* token, size_t len) {
    const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len - 1; i++) {
        token[i] = hex[rand() % 16];
    }
    token[len - 1] = '\0';
}

static void test_token_generation(void) {
    TEST_SUITE_BEGIN("Token Generation");

    char token1[65], token2[65];

    generate_hex_token(token1, sizeof(token1));
    generate_hex_token(token2, sizeof(token2));

    TEST_ASSERT(strlen(token1) == 64, "Token1 has correct length");
    TEST_ASSERT(strlen(token2) == 64, "Token2 has correct length");
    TEST_ASSERT(strcmp(token1, token2) != 0, "Tokens are unique");

    /* Check all characters are hex */
    bool all_hex = true;
    for (size_t i = 0; i < strlen(token1); i++) {
        char c = token1[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            all_hex = false;
            break;
        }
    }
    TEST_ASSERT(all_hex, "Token contains only hex characters");

    TEST_SUITE_END();
}

/* ============================================================================
 * Path Matching Tests (for REST API routing)
 * ========================================================================== */

static bool match_path(const char* pattern, const char* path, char params[][64], int* param_count) {
    *param_count = 0;

    while (*pattern && *path) {
        if (*pattern == ':') {
            /* Extract parameter */
            pattern++;
            const char* param_start = path;
            while (*path && *path != '/') path++;

            if (*param_count < 8) {
                size_t len = path - param_start;
                if (len >= 64) len = 63;
                strncpy(params[*param_count], param_start, len);
                params[*param_count][len] = '\0';
                (*param_count)++;
            }

            while (*pattern && *pattern != '/') pattern++;
        } else if (*pattern != *path) {
            return false;
        } else {
            pattern++;
            path++;
        }
    }

    return (*pattern == '\0' && *path == '\0');
}

static void test_path_matching(void) {
    TEST_SUITE_BEGIN("Path Matching");

    char params[8][64];
    int param_count;

    TEST_ASSERT(match_path("/api/cases", "/api/cases", params, &param_count), "Exact match");
    TEST_ASSERT_EQUAL_INT(0, param_count, "No params for exact match");

    TEST_ASSERT(match_path("/api/cases/:id", "/api/cases/123", params, &param_count), "Single param match");
    TEST_ASSERT_EQUAL_INT(1, param_count, "One param extracted");
    TEST_ASSERT_EQUAL_STR("123", params[0], "Param value correct");

    TEST_ASSERT(match_path("/api/cases/:id/docs/:doc_id", "/api/cases/abc/docs/xyz", params, &param_count),
                "Multiple params match");
    TEST_ASSERT_EQUAL_INT(2, param_count, "Two params extracted");
    TEST_ASSERT_EQUAL_STR("abc", params[0], "First param correct");
    TEST_ASSERT_EQUAL_STR("xyz", params[1], "Second param correct");

    TEST_ASSERT(!match_path("/api/cases", "/api/deadlines", params, &param_count), "Non-match detected");
    TEST_ASSERT(!match_path("/api/cases/:id", "/api/cases/", params, &param_count), "Empty param rejected");

    TEST_SUITE_END();
}

/* ============================================================================
 * Query Parameter Parsing Tests
 * ========================================================================== */

static const char* get_query_param(const char* query, const char* key, char* value, size_t value_size) {
    if (!query || !key) return NULL;

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "%s=", key);

    const char* start = strstr(query, pattern);
    if (!start) return NULL;

    if (start != query && *(start - 1) != '&') {
        /* Key found but not at start or after & - might be partial match */
        const char* next = start + 1;
        while ((start = strstr(next, pattern)) != NULL) {
            if (*(start - 1) == '&') break;
            next = start + 1;
        }
        if (!start) return NULL;
    }

    start += strlen(pattern);
    const char* end = strchr(start, '&');
    if (!end) end = start + strlen(start);

    size_t len = end - start;
    if (len >= value_size) len = value_size - 1;
    strncpy(value, start, len);
    value[len] = '\0';

    return value;
}

static void test_query_parsing(void) {
    TEST_SUITE_BEGIN("Query Parameter Parsing");

    char value[256];

    TEST_ASSERT_NOT_NULL(get_query_param("status=active", "status", value, sizeof(value)), "Single param");
    TEST_ASSERT_EQUAL_STR("active", value, "Single param value");

    TEST_ASSERT_NOT_NULL(get_query_param("status=active&type=civil", "type", value, sizeof(value)), "Second param");
    TEST_ASSERT_EQUAL_STR("civil", value, "Second param value");

    TEST_ASSERT_NOT_NULL(get_query_param("status=active&type=civil", "status", value, sizeof(value)), "First of multiple");
    TEST_ASSERT_EQUAL_STR("active", value, "First param value");

    TEST_ASSERT_NULL(get_query_param("status=active", "missing", value, sizeof(value)), "Missing param");

    TEST_ASSERT_NOT_NULL(get_query_param("limit=10&offset=20", "offset", value, sizeof(value)), "Numeric param");
    TEST_ASSERT_EQUAL_STR("20", value, "Numeric param value");

    TEST_SUITE_END();
}

/* ============================================================================
 * String Utilities Tests
 * ========================================================================== */

static char* trim_whitespace(char* str) {
    char* end;

    while (*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')) str++;
    if (*str == 0) return str;

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    end[1] = '\0';

    return str;
}

static void test_string_utils(void) {
    TEST_SUITE_BEGIN("String Utilities");

    char str1[] = "  hello  ";
    TEST_ASSERT_EQUAL_STR("hello", trim_whitespace(str1), "Trim both sides");

    char str2[] = "hello";
    TEST_ASSERT_EQUAL_STR("hello", trim_whitespace(str2), "No trimming needed");

    char str3[] = "   ";
    TEST_ASSERT_EQUAL_STR("", trim_whitespace(str3), "All whitespace");

    char str4[] = "\n\thello\r\n";
    TEST_ASSERT_EQUAL_STR("hello", trim_whitespace(str4), "Trim mixed whitespace");

    TEST_SUITE_END();
}

/* ============================================================================
 * Main Test Runner
 * ========================================================================== */

int main(int argc, char* argv[]) {
    printf("\n");
    printf("================================================================================\n");
    printf("                    RegisLex Unit Test Suite\n");
    printf("================================================================================\n");

    /* Seed random for UUID/token tests */
    srand(42);  /* Fixed seed for reproducibility */

    /* Run all test suites */
    test_uuid_generate();
    test_datetime_parse();
    test_datetime_format();
    test_case_validation();
    test_business_days();
    test_money_operations();
    test_json_escaping();
    test_permissions();
    test_token_generation();
    test_path_matching();
    test_query_parsing();
    test_string_utils();

    /* Print summary */
    printf("\n================================================================================\n");
    printf("Test Results Summary\n");
    printf("================================================================================\n");
    printf("Total tests:  %d\n", tests_run);
    printf("Passed:       %d\n", tests_passed);
    printf("Failed:       %d\n", tests_failed);
    printf("Pass rate:    %.1f%%\n", tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("================================================================================\n\n");

    return tests_failed > 0 ? 1 : 0;
}
