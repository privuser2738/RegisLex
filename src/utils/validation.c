/**
 * @file validation.c
 * @brief Input Validation Utilities
 */

#include "regislex/regislex.h"
#include <string.h>
#include <ctype.h>

bool regislex_validate_email(const char* email) {
    if (!email) return false;
    const char* at = strchr(email, '@');
    if (!at || at == email) return false;
    const char* dot = strchr(at, '.');
    if (!dot || dot == at + 1 || *(dot + 1) == '\0') return false;
    return true;
}

bool regislex_validate_phone(const char* phone) {
    if (!phone) return false;
    int digits = 0;
    for (const char* p = phone; *p; p++) {
        if (isdigit((unsigned char)*p)) digits++;
        else if (*p != '-' && *p != ' ' && *p != '(' && *p != ')' && *p != '+') return false;
    }
    return digits >= 7 && digits <= 15;
}

bool regislex_validate_uuid(const char* uuid) {
    if (!uuid || strlen(uuid) != 36) return false;
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid[i] != '-') return false;
        } else if (!isxdigit((unsigned char)uuid[i])) return false;
    }
    return true;
}

bool regislex_validate_date(int year, int month, int day) {
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (year < 1900 || year > 2100) return false;
    int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) days[2] = 29;
    return day <= days[month];
}

bool regislex_validate_required(const char* value) {
    if (!value) return false;
    while (*value == ' ' || *value == '\t') value++;
    return *value != '\0';
}

bool regislex_validate_length(const char* value, size_t min, size_t max) {
    if (!value) return min == 0;
    size_t len = strlen(value);
    return len >= min && len <= max;
}
