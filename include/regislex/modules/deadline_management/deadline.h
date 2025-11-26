/**
 * @file deadline.h
 * @brief Deadline Management Module
 *
 * Provides functionality for tracking court dates, statutes of limitations,
 * filing deadlines, reminders, and calendar management.
 */

#ifndef REGISLEX_DEADLINE_H
#define REGISLEX_DEADLINE_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Deadline Types and Enums
 * ============================================================================ */

/**
 * @brief Deadline type classification
 */
typedef enum {
    REGISLEX_DEADLINE_COURT_DATE = 0,
    REGISLEX_DEADLINE_FILING,
    REGISLEX_DEADLINE_DISCOVERY,
    REGISLEX_DEADLINE_RESPONSE,
    REGISLEX_DEADLINE_STATUTE_OF_LIMITATIONS,
    REGISLEX_DEADLINE_APPEAL,
    REGISLEX_DEADLINE_HEARING,
    REGISLEX_DEADLINE_TRIAL,
    REGISLEX_DEADLINE_DEPOSITION,
    REGISLEX_DEADLINE_MOTION,
    REGISLEX_DEADLINE_CONFERENCE,
    REGISLEX_DEADLINE_MEDIATION,
    REGISLEX_DEADLINE_ARBITRATION,
    REGISLEX_DEADLINE_COMPLIANCE,
    REGISLEX_DEADLINE_PAYMENT,
    REGISLEX_DEADLINE_REVIEW,
    REGISLEX_DEADLINE_CUSTOM
} regislex_deadline_type_t;

/**
 * @brief Reminder type
 */
typedef enum {
    REGISLEX_REMINDER_EMAIL = 0,
    REGISLEX_REMINDER_SMS,
    REGISLEX_REMINDER_PUSH,
    REGISLEX_REMINDER_IN_APP,
    REGISLEX_REMINDER_CALENDAR
} regislex_reminder_type_t;

/**
 * @brief Recurrence pattern
 */
typedef enum {
    REGISLEX_RECUR_NONE = 0,
    REGISLEX_RECUR_DAILY,
    REGISLEX_RECUR_WEEKLY,
    REGISLEX_RECUR_BIWEEKLY,
    REGISLEX_RECUR_MONTHLY,
    REGISLEX_RECUR_QUARTERLY,
    REGISLEX_RECUR_YEARLY,
    REGISLEX_RECUR_CUSTOM
} regislex_recurrence_t;

/**
 * @brief Calendar event type
 */
typedef enum {
    REGISLEX_CALENDAR_DEADLINE = 0,
    REGISLEX_CALENDAR_APPOINTMENT,
    REGISLEX_CALENDAR_MEETING,
    REGISLEX_CALENDAR_TASK,
    REGISLEX_CALENDAR_REMINDER,
    REGISLEX_CALENDAR_BLOCKED_TIME,
    REGISLEX_CALENDAR_OUT_OF_OFFICE
} regislex_calendar_event_type_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Deadline/due date
 */
struct regislex_deadline {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    char title[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_deadline_type_t type;
    regislex_status_t status;
    regislex_priority_t priority;

    /* Timing */
    regislex_datetime_t due_date;
    regislex_datetime_t start_date;
    bool is_all_day;
    int duration_minutes;
    char timezone[64];

    /* Recurrence */
    regislex_recurrence_t recurrence;
    int recurrence_interval;
    regislex_datetime_t recurrence_end;

    /* Assignment */
    regislex_uuid_t assigned_to_id;
    regislex_uuid_t created_by;

    /* Court rules reference */
    char rule_reference[256];
    int days_from_trigger;
    bool count_business_days;
    bool exclude_holidays;

    /* Status tracking */
    regislex_datetime_t completed_at;
    regislex_uuid_t completed_by;
    char completion_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];

    /* Metadata */
    char location[512];
    char tags[1024];
    int metadata_count;
    regislex_metadata_t* metadata;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Reminder configuration
 */
struct regislex_reminder {
    regislex_uuid_t id;
    regislex_uuid_t deadline_id;
    regislex_uuid_t user_id;
    regislex_reminder_type_t type;
    int minutes_before;
    bool is_sent;
    regislex_datetime_t send_at;
    regislex_datetime_t sent_at;
    char message[REGISLEX_MAX_DESCRIPTION_LENGTH];
    bool is_active;
    regislex_datetime_t created_at;
};

/**
 * @brief Statute of limitations rule
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char jurisdiction[128];
    regislex_case_type_t case_type;
    int limitation_days;
    bool tolling_allowed;
    char tolling_conditions[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char statute_reference[256];
    char notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_datetime_t effective_date;
    regislex_datetime_t expiration_date;
    bool is_active;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_statute_rule_t;

/**
 * @brief Calendar entry
 */
struct regislex_calendar {
    regislex_uuid_t id;
    regislex_uuid_t user_id;
    regislex_uuid_t case_id;
    regislex_uuid_t deadline_id;
    char title[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_calendar_event_type_t event_type;

    /* Timing */
    regislex_datetime_t start_time;
    regislex_datetime_t end_time;
    bool is_all_day;
    char timezone[64];

    /* Recurrence */
    regislex_recurrence_t recurrence;
    int recurrence_interval;
    regislex_datetime_t recurrence_end;

    /* Location */
    char location[512];
    char virtual_meeting_url[512];
    char virtual_meeting_id[128];

    /* Attendees */
    char attendees[2048];
    char organizer[256];

    /* Sync */
    char external_calendar_id[256];
    char external_event_id[256];
    regislex_datetime_t synced_at;

    /* Status */
    bool is_private;
    bool is_cancelled;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Holiday definition
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_datetime_t date;
    char jurisdiction[128];
    bool is_court_holiday;
    bool is_federal;
    bool is_recurring;
    int recurrence_month;
    int recurrence_day;
    int recurrence_week;      /* For floating holidays (e.g., 4th Thursday) */
    int recurrence_weekday;
    regislex_datetime_t created_at;
} regislex_holiday_t;

/**
 * @brief Deadline filter criteria
 */
typedef struct {
    regislex_uuid_t* case_id;
    regislex_uuid_t* matter_id;
    regislex_uuid_t* assigned_to_id;
    regislex_deadline_type_t* type;
    regislex_status_t* status;
    regislex_priority_t* priority;
    regislex_datetime_t* due_after;
    regislex_datetime_t* due_before;
    bool include_completed;
    bool overdue_only;
    const char* tags_contain;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_deadline_filter_t;

/**
 * @brief Deadline list result
 */
typedef struct {
    regislex_deadline_t** deadlines;
    int count;
    int total_count;
    int offset;
    int limit;
} regislex_deadline_list_t;

/**
 * @brief Calendar filter criteria
 */
typedef struct {
    regislex_uuid_t* user_id;
    regislex_uuid_t* case_id;
    regislex_calendar_event_type_t* event_type;
    regislex_datetime_t* start_after;
    regislex_datetime_t* start_before;
    bool include_cancelled;
    int offset;
    int limit;
} regislex_calendar_filter_t;

/* ============================================================================
 * Deadline Management Functions
 * ============================================================================ */

/**
 * @brief Create a new deadline
 * @param ctx Context
 * @param deadline Deadline data
 * @param out_deadline Output created deadline
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_create(
    regislex_context_t* ctx,
    const regislex_deadline_t* deadline,
    regislex_deadline_t** out_deadline
);

/**
 * @brief Get a deadline by ID
 * @param ctx Context
 * @param id Deadline ID
 * @param out_deadline Output deadline
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_deadline_t** out_deadline
);

/**
 * @brief Update a deadline
 * @param ctx Context
 * @param deadline Updated deadline data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_update(
    regislex_context_t* ctx,
    const regislex_deadline_t* deadline
);

/**
 * @brief Delete a deadline
 * @param ctx Context
 * @param id Deadline ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List deadlines with filtering
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output deadline list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_list(
    regislex_context_t* ctx,
    const regislex_deadline_filter_t* filter,
    regislex_deadline_list_t** out_list
);

/**
 * @brief Mark deadline as complete
 * @param ctx Context
 * @param id Deadline ID
 * @param notes Completion notes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_complete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* notes
);

/**
 * @brief Get upcoming deadlines
 * @param ctx Context
 * @param days_ahead Number of days to look ahead
 * @param out_list Output deadline list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_upcoming(
    regislex_context_t* ctx,
    int days_ahead,
    regislex_deadline_list_t** out_list
);

/**
 * @brief Get overdue deadlines
 * @param ctx Context
 * @param out_list Output deadline list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_overdue(
    regislex_context_t* ctx,
    regislex_deadline_list_t** out_list
);

/**
 * @brief Calculate deadline date from rules
 * @param ctx Context
 * @param trigger_date Trigger date (e.g., filing date)
 * @param days Days from trigger
 * @param count_business_days Whether to count business days only
 * @param jurisdiction Jurisdiction for holiday calculation
 * @param out_date Output calculated date
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_deadline_calculate(
    regislex_context_t* ctx,
    const regislex_datetime_t* trigger_date,
    int days,
    bool count_business_days,
    const char* jurisdiction,
    regislex_datetime_t* out_date
);

/**
 * @brief Free deadline structure
 * @param deadline Deadline to free
 */
REGISLEX_API void regislex_deadline_free(regislex_deadline_t* deadline);

/**
 * @brief Free deadline list
 * @param list List to free
 */
REGISLEX_API void regislex_deadline_list_free(regislex_deadline_list_t* list);

/* ============================================================================
 * Reminder Functions
 * ============================================================================ */

/**
 * @brief Add reminder to deadline
 * @param ctx Context
 * @param deadline_id Deadline ID
 * @param reminder Reminder data
 * @param out_reminder Output created reminder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_reminder_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* deadline_id,
    const regislex_reminder_t* reminder,
    regislex_reminder_t** out_reminder
);

/**
 * @brief Remove reminder
 * @param ctx Context
 * @param id Reminder ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_reminder_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List reminders for deadline
 * @param ctx Context
 * @param deadline_id Deadline ID
 * @param reminders Output reminder array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_reminder_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* deadline_id,
    regislex_reminder_t*** reminders,
    int* count
);

/**
 * @brief Get pending reminders to send
 * @param ctx Context
 * @param reminders Output reminder array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_reminder_pending(
    regislex_context_t* ctx,
    regislex_reminder_t*** reminders,
    int* count
);

/**
 * @brief Mark reminder as sent
 * @param ctx Context
 * @param id Reminder ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_reminder_mark_sent(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Free reminder structure
 * @param reminder Reminder to free
 */
REGISLEX_API void regislex_reminder_free(regislex_reminder_t* reminder);

/* ============================================================================
 * Statute of Limitations Functions
 * ============================================================================ */

/**
 * @brief Create statute rule
 * @param ctx Context
 * @param rule Rule data
 * @param out_rule Output created rule
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_statute_rule_create(
    regislex_context_t* ctx,
    const regislex_statute_rule_t* rule,
    regislex_statute_rule_t** out_rule
);

/**
 * @brief Get applicable statute rules
 * @param ctx Context
 * @param jurisdiction Jurisdiction
 * @param case_type Case type
 * @param rules Output rule array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_statute_rules_get(
    regislex_context_t* ctx,
    const char* jurisdiction,
    regislex_case_type_t case_type,
    regislex_statute_rule_t*** rules,
    int* count
);

/**
 * @brief Calculate statute expiration
 * @param ctx Context
 * @param rule_id Rule ID
 * @param accrual_date Date cause of action accrued
 * @param out_expiration Output expiration date
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_statute_calculate(
    regislex_context_t* ctx,
    const regislex_uuid_t* rule_id,
    const regislex_datetime_t* accrual_date,
    regislex_datetime_t* out_expiration
);

/**
 * @brief Free statute rule
 * @param rule Rule to free
 */
REGISLEX_API void regislex_statute_rule_free(regislex_statute_rule_t* rule);

/* ============================================================================
 * Calendar Functions
 * ============================================================================ */

/**
 * @brief Create calendar entry
 * @param ctx Context
 * @param entry Calendar entry data
 * @param out_entry Output created entry
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_calendar_create(
    regislex_context_t* ctx,
    const regislex_calendar_t* entry,
    regislex_calendar_t** out_entry
);

/**
 * @brief Get calendar entries
 * @param ctx Context
 * @param filter Filter criteria
 * @param entries Output entry array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_calendar_list(
    regislex_context_t* ctx,
    const regislex_calendar_filter_t* filter,
    regislex_calendar_t*** entries,
    int* count
);

/**
 * @brief Update calendar entry
 * @param ctx Context
 * @param entry Updated entry data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_calendar_update(
    regislex_context_t* ctx,
    const regislex_calendar_t* entry
);

/**
 * @brief Delete calendar entry
 * @param ctx Context
 * @param id Entry ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_calendar_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Sync with external calendar
 * @param ctx Context
 * @param user_id User ID
 * @param calendar_type Calendar type ("google", "outlook", "ical")
 * @param credentials Calendar credentials
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_calendar_sync(
    regislex_context_t* ctx,
    const regislex_uuid_t* user_id,
    const char* calendar_type,
    const char* credentials
);

/**
 * @brief Free calendar entry
 * @param entry Entry to free
 */
REGISLEX_API void regislex_calendar_free(regislex_calendar_t* entry);

/* ============================================================================
 * Holiday Functions
 * ============================================================================ */

/**
 * @brief Add holiday
 * @param ctx Context
 * @param holiday Holiday data
 * @param out_holiday Output created holiday
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_holiday_add(
    regislex_context_t* ctx,
    const regislex_holiday_t* holiday,
    regislex_holiday_t** out_holiday
);

/**
 * @brief Get holidays for date range
 * @param ctx Context
 * @param jurisdiction Jurisdiction
 * @param start_date Start date
 * @param end_date End date
 * @param holidays Output holiday array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_holiday_list(
    regislex_context_t* ctx,
    const char* jurisdiction,
    const regislex_datetime_t* start_date,
    const regislex_datetime_t* end_date,
    regislex_holiday_t*** holidays,
    int* count
);

/**
 * @brief Check if date is holiday
 * @param ctx Context
 * @param date Date to check
 * @param jurisdiction Jurisdiction
 * @param is_holiday Output boolean
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_holiday_check(
    regislex_context_t* ctx,
    const regislex_datetime_t* date,
    const char* jurisdiction,
    bool* is_holiday
);

/**
 * @brief Check if date is business day
 * @param ctx Context
 * @param date Date to check
 * @param jurisdiction Jurisdiction
 * @param is_business_day Output boolean
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_is_business_day(
    regislex_context_t* ctx,
    const regislex_datetime_t* date,
    const char* jurisdiction,
    bool* is_business_day
);

/**
 * @brief Free holiday
 * @param holiday Holiday to free
 */
REGISLEX_API void regislex_holiday_free(regislex_holiday_t* holiday);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_DEADLINE_H */
