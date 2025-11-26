/**
 * @file time_utils.c
 * @brief RegisLex Time Utilities Implementation
 */

#include "regislex/regislex.h"
#include <time.h>
#include <string.h>

static const int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int get_days_in_month(int year, int month) {
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return days_in_month[month];
}

regislex_error_t regislex_datetime_add_days(regislex_datetime_t* dt, int days) {
    if (!dt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    /* Convert to time_t, add days, convert back */
    struct tm tm_info = {0};
    tm_info.tm_year = dt->year - 1900;
    tm_info.tm_mon = dt->month - 1;
    tm_info.tm_mday = dt->day;
    tm_info.tm_hour = dt->hour;
    tm_info.tm_min = dt->minute;
    tm_info.tm_sec = dt->second;
    tm_info.tm_isdst = -1;

    time_t t = mktime(&tm_info);
    t += days * 24 * 60 * 60;

#ifdef REGISLEX_PLATFORM_WINDOWS
    struct tm result;
    gmtime_s(&result, &t);
    dt->year = result.tm_year + 1900;
    dt->month = result.tm_mon + 1;
    dt->day = result.tm_mday;
    dt->hour = result.tm_hour;
    dt->minute = result.tm_min;
    dt->second = result.tm_sec;
#else
    struct tm* result = gmtime(&t);
    dt->year = result->tm_year + 1900;
    dt->month = result->tm_mon + 1;
    dt->day = result->tm_mday;
    dt->hour = result->tm_hour;
    dt->minute = result->tm_min;
    dt->second = result->tm_sec;
#endif

    return REGISLEX_OK;
}

regislex_error_t regislex_datetime_add_months(regislex_datetime_t* dt, int months) {
    if (!dt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int total_months = (dt->year * 12 + dt->month - 1) + months;
    dt->year = total_months / 12;
    dt->month = (total_months % 12) + 1;

    /* Adjust day if it exceeds days in new month */
    int max_day = get_days_in_month(dt->year, dt->month);
    if (dt->day > max_day) dt->day = max_day;

    return REGISLEX_OK;
}

int regislex_datetime_diff_days(const regislex_datetime_t* dt1, const regislex_datetime_t* dt2) {
    if (!dt1 || !dt2) return 0;

    struct tm tm1 = {0}, tm2 = {0};
    tm1.tm_year = dt1->year - 1900;
    tm1.tm_mon = dt1->month - 1;
    tm1.tm_mday = dt1->day;
    tm1.tm_isdst = -1;

    tm2.tm_year = dt2->year - 1900;
    tm2.tm_mon = dt2->month - 1;
    tm2.tm_mday = dt2->day;
    tm2.tm_isdst = -1;

    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);

    return (int)((t2 - t1) / (24 * 60 * 60));
}

int regislex_datetime_compare(const regislex_datetime_t* dt1, const regislex_datetime_t* dt2) {
    if (!dt1 || !dt2) return 0;

    if (dt1->year != dt2->year) return dt1->year - dt2->year;
    if (dt1->month != dt2->month) return dt1->month - dt2->month;
    if (dt1->day != dt2->day) return dt1->day - dt2->day;
    if (dt1->hour != dt2->hour) return dt1->hour - dt2->hour;
    if (dt1->minute != dt2->minute) return dt1->minute - dt2->minute;
    return dt1->second - dt2->second;
}

int regislex_datetime_day_of_week(const regislex_datetime_t* dt) {
    if (!dt) return -1;

    struct tm tm_info = {0};
    tm_info.tm_year = dt->year - 1900;
    tm_info.tm_mon = dt->month - 1;
    tm_info.tm_mday = dt->day;
    tm_info.tm_isdst = -1;

    mktime(&tm_info);
    return tm_info.tm_wday;  /* 0 = Sunday */
}

bool regislex_datetime_is_weekend(const regislex_datetime_t* dt) {
    int dow = regislex_datetime_day_of_week(dt);
    return dow == 0 || dow == 6;
}

regislex_error_t regislex_datetime_next_business_day(regislex_datetime_t* dt) {
    if (!dt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    do {
        regislex_datetime_add_days(dt, 1);
    } while (regislex_datetime_is_weekend(dt));

    return REGISLEX_OK;
}

regislex_error_t regislex_datetime_add_business_days(regislex_datetime_t* dt, int days) {
    if (!dt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int direction = days >= 0 ? 1 : -1;
    int remaining = days >= 0 ? days : -days;

    while (remaining > 0) {
        regislex_datetime_add_days(dt, direction);
        if (!regislex_datetime_is_weekend(dt)) {
            remaining--;
        }
    }

    return REGISLEX_OK;
}
