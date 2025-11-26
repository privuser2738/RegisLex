/**
 * @file calendar.c
 * @brief Calendar Management for Legal Deadlines
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <string.h>

typedef struct {
    regislex_datetime_t date;
    char name[128];
    bool is_court_holiday;
    char jurisdiction[64];
} regislex_holiday_t;

static regislex_holiday_t* holidays = NULL;
static int holiday_count = 0;

regislex_error_t regislex_calendar_add_holiday(const regislex_datetime_t* date,
                                                const char* name,
                                                const char* jurisdiction) {
    if (!date || !name) return REGISLEX_ERROR_INVALID_ARGUMENT;

    regislex_holiday_t* new_holidays = (regislex_holiday_t*)platform_realloc(
        holidays, (holiday_count + 1) * sizeof(regislex_holiday_t));
    if (!new_holidays) return REGISLEX_ERROR_OUT_OF_MEMORY;

    holidays = new_holidays;
    memcpy(&holidays[holiday_count].date, date, sizeof(regislex_datetime_t));
    strncpy(holidays[holiday_count].name, name, sizeof(holidays[holiday_count].name) - 1);
    if (jurisdiction) {
        strncpy(holidays[holiday_count].jurisdiction, jurisdiction,
                sizeof(holidays[holiday_count].jurisdiction) - 1);
    }
    holidays[holiday_count].is_court_holiday = true;
    holiday_count++;

    return REGISLEX_OK;
}

bool regislex_calendar_is_holiday(const regislex_datetime_t* date, const char* jurisdiction) {
    if (!date) return false;

    for (int i = 0; i < holiday_count; i++) {
        if (holidays[i].date.year == date->year &&
            holidays[i].date.month == date->month &&
            holidays[i].date.day == date->day) {
            if (!jurisdiction || !holidays[i].jurisdiction[0] ||
                strcmp(holidays[i].jurisdiction, jurisdiction) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool regislex_calendar_is_court_day(const regislex_datetime_t* date, const char* jurisdiction) {
    if (!date) return false;
    if (regislex_datetime_is_weekend(date)) return false;
    if (regislex_calendar_is_holiday(date, jurisdiction)) return false;
    return true;
}

regislex_error_t regislex_calendar_next_court_day(regislex_datetime_t* date,
                                                   const char* jurisdiction) {
    if (!date) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int max_iterations = 30;  /* Safety limit */
    while (max_iterations-- > 0) {
        regislex_datetime_add_days(date, 1);
        if (regislex_calendar_is_court_day(date, jurisdiction)) {
            return REGISLEX_OK;
        }
    }

    return REGISLEX_ERROR;
}

void regislex_calendar_clear_holidays(void) {
    platform_free(holidays);
    holidays = NULL;
    holiday_count = 0;
}
