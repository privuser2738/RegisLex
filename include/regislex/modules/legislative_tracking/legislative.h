/**
 * @file legislative.h
 * @brief Legislative Tracking Module
 *
 * Provides functionality for tracking federal, state, and local legislation
 * and regulations, managing government relations and stakeholder engagement.
 */

#ifndef REGISLEX_LEGISLATIVE_H
#define REGISLEX_LEGISLATIVE_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Legislative Types and Enums
 * ============================================================================ */

/**
 * @brief Legislation type
 */
typedef enum {
    REGISLEX_LEG_BILL = 0,
    REGISLEX_LEG_RESOLUTION,
    REGISLEX_LEG_JOINT_RESOLUTION,
    REGISLEX_LEG_CONCURRENT_RESOLUTION,
    REGISLEX_LEG_AMENDMENT,
    REGISLEX_LEG_ACT,
    REGISLEX_LEG_STATUTE,
    REGISLEX_LEG_ORDINANCE,
    REGISLEX_LEG_REGULATION,
    REGISLEX_LEG_EXECUTIVE_ORDER,
    REGISLEX_LEG_ADMINISTRATIVE_RULE,
    REGISLEX_LEG_PROPOSED_RULE
} regislex_legislation_type_t;

/**
 * @brief Government level
 */
typedef enum {
    REGISLEX_GOV_FEDERAL = 0,
    REGISLEX_GOV_STATE,
    REGISLEX_GOV_COUNTY,
    REGISLEX_GOV_MUNICIPAL,
    REGISLEX_GOV_SPECIAL_DISTRICT,
    REGISLEX_GOV_INTERNATIONAL
} regislex_gov_level_t;

/**
 * @brief Legislative status
 */
typedef enum {
    REGISLEX_LEG_STATUS_INTRODUCED = 0,
    REGISLEX_LEG_STATUS_IN_COMMITTEE,
    REGISLEX_LEG_STATUS_COMMITTEE_PASSED,
    REGISLEX_LEG_STATUS_FLOOR_VOTE_PENDING,
    REGISLEX_LEG_STATUS_PASSED_CHAMBER,
    REGISLEX_LEG_STATUS_IN_CONFERENCE,
    REGISLEX_LEG_STATUS_PASSED_BOTH,
    REGISLEX_LEG_STATUS_SENT_TO_EXECUTIVE,
    REGISLEX_LEG_STATUS_SIGNED,
    REGISLEX_LEG_STATUS_VETOED,
    REGISLEX_LEG_STATUS_VETO_OVERRIDDEN,
    REGISLEX_LEG_STATUS_ENACTED,
    REGISLEX_LEG_STATUS_FAILED,
    REGISLEX_LEG_STATUS_WITHDRAWN,
    REGISLEX_LEG_STATUS_TABLED,
    REGISLEX_LEG_STATUS_EXPIRED
} regislex_leg_status_t;

/**
 * @brief Position on legislation
 */
typedef enum {
    REGISLEX_POSITION_NEUTRAL = 0,
    REGISLEX_POSITION_SUPPORT,
    REGISLEX_POSITION_OPPOSE,
    REGISLEX_POSITION_SUPPORT_IF_AMENDED,
    REGISLEX_POSITION_OPPOSE_UNLESS_AMENDED,
    REGISLEX_POSITION_WATCH
} regislex_position_t;

/**
 * @brief Alert type
 */
typedef enum {
    REGISLEX_ALERT_NEW_LEGISLATION = 0,
    REGISLEX_ALERT_STATUS_CHANGE,
    REGISLEX_ALERT_COMMITTEE_ACTION,
    REGISLEX_ALERT_FLOOR_ACTION,
    REGISLEX_ALERT_EXECUTIVE_ACTION,
    REGISLEX_ALERT_AMENDMENT_FILED,
    REGISLEX_ALERT_HEARING_SCHEDULED,
    REGISLEX_ALERT_VOTE_SCHEDULED,
    REGISLEX_ALERT_DEADLINE_APPROACHING,
    REGISLEX_ALERT_COMMENT_PERIOD,
    REGISLEX_ALERT_EFFECTIVE_DATE,
    REGISLEX_ALERT_KEYWORD_MATCH,
    REGISLEX_ALERT_CUSTOM
} regislex_alert_type_t;

/**
 * @brief Stakeholder type
 */
typedef enum {
    REGISLEX_STAKEHOLDER_LEGISLATOR = 0,
    REGISLEX_STAKEHOLDER_STAFF,
    REGISLEX_STAKEHOLDER_EXECUTIVE,
    REGISLEX_STAKEHOLDER_AGENCY,
    REGISLEX_STAKEHOLDER_LOBBYIST,
    REGISLEX_STAKEHOLDER_ORGANIZATION,
    REGISLEX_STAKEHOLDER_COALITION,
    REGISLEX_STAKEHOLDER_MEDIA,
    REGISLEX_STAKEHOLDER_EXPERT,
    REGISLEX_STAKEHOLDER_OTHER
} regislex_stakeholder_type_t;

/**
 * @brief Engagement type
 */
typedef enum {
    REGISLEX_ENGAGE_MEETING = 0,
    REGISLEX_ENGAGE_PHONE_CALL,
    REGISLEX_ENGAGE_EMAIL,
    REGISLEX_ENGAGE_LETTER,
    REGISLEX_ENGAGE_TESTIMONY,
    REGISLEX_ENGAGE_COMMENT,
    REGISLEX_ENGAGE_EVENT,
    REGISLEX_ENGAGE_SITE_VISIT,
    REGISLEX_ENGAGE_SOCIAL_MEDIA,
    REGISLEX_ENGAGE_OTHER
} regislex_engagement_type_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Legislator/sponsor information
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char title[128];
    char party[64];
    char chamber[64];           /* "Senate", "House", etc. */
    char district[64];
    char state[64];
    char office_address[512];
    char phone[32];
    char email[256];
    char website[256];
    char committee_assignments[2048];
    char leadership_positions[512];
    char photo_url[512];
    regislex_datetime_t term_start;
    regislex_datetime_t term_end;
    bool is_active;
} regislex_legislator_t;

/**
 * @brief Committee information
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char chamber[64];
    char jurisdiction_desc[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char chair_name[REGISLEX_MAX_NAME_LENGTH];
    char ranking_member[REGISLEX_MAX_NAME_LENGTH];
    char members[4096];         /* JSON array of member IDs */
    char meeting_schedule[512];
    char website[256];
    regislex_gov_level_t gov_level;
    char state[64];
    bool is_active;
} regislex_committee_t;

/**
 * @brief Legislative action/history entry
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t legislation_id;
    regislex_datetime_t action_date;
    char action_type[128];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char chamber[64];
    char committee_name[256];
    char vote_result[128];
    int vote_yes;
    int vote_no;
    int vote_abstain;
    char roll_call_url[512];
    char source_url[512];
} regislex_leg_action_t;

/**
 * @brief Legislation/bill
 */
struct regislex_legislation {
    regislex_uuid_t id;
    char bill_number[64];
    char title[REGISLEX_MAX_NAME_LENGTH];
    char short_title[256];
    char summary[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char full_text_url[512];
    regislex_legislation_type_t type;
    regislex_leg_status_t status;
    regislex_gov_level_t gov_level;

    /* Jurisdiction */
    char jurisdiction[128];     /* State abbreviation or "US" */
    char chamber_of_origin[64];
    char session[64];

    /* Sponsors */
    regislex_uuid_t primary_sponsor_id;
    char sponsors[4096];        /* JSON array of sponsor IDs */
    int cosponsor_count;

    /* Committees */
    regislex_uuid_t committee_id;
    char committees[2048];      /* JSON array of committee assignments */

    /* Subject/topics */
    char subjects[2048];
    char keywords[1024];
    char affected_agencies[1024];
    char related_bills[1024];

    /* Dates */
    regislex_datetime_t introduced_date;
    regislex_datetime_t last_action_date;
    regislex_datetime_t effective_date;
    regislex_datetime_t sunset_date;

    /* History */
    int action_count;
    regislex_leg_action_t** actions;

    /* Tracking */
    bool is_tracked;
    regislex_position_t position;
    char position_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_priority_t priority;
    char internal_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_uuid_t assigned_to_id;

    /* External references */
    char external_id[128];      /* ID from external API */
    char source_url[512];
    regislex_datetime_t last_synced;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Regulatory action
 */
typedef struct {
    regislex_uuid_t id;
    char docket_number[64];
    char title[REGISLEX_MAX_NAME_LENGTH];
    char summary[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char agency[256];
    char agency_id[64];
    regislex_legislation_type_t type;
    regislex_leg_status_t status;
    regislex_gov_level_t gov_level;
    char jurisdiction[128];

    /* Federal Register info */
    char fr_citation[128];
    char cfr_citation[128];
    regislex_datetime_t fr_publication_date;

    /* Comment period */
    bool has_comment_period;
    regislex_datetime_t comment_start;
    regislex_datetime_t comment_end;
    int comment_count;
    char regulations_gov_id[64];

    /* Dates */
    regislex_datetime_t proposed_date;
    regislex_datetime_t final_date;
    regislex_datetime_t effective_date;

    /* Tracking */
    bool is_tracked;
    regislex_position_t position;
    char position_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_priority_t priority;
    regislex_uuid_t assigned_to_id;

    /* Documents */
    char document_urls[4096];   /* JSON array */

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_regulation_t;

/**
 * @brief Stakeholder
 */
struct regislex_stakeholder {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char organization[256];
    char title[128];
    regislex_stakeholder_type_t type;
    regislex_contact_t contact;

    /* Political info */
    char party_affiliation[64];
    char districts[256];
    char committees[1024];
    char policy_areas[1024];

    /* Relationships */
    char relationship_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    int influence_score;        /* 1-10 */
    char tags[1024];

    /* Social media */
    char twitter[128];
    char linkedin[256];
    char facebook[256];

    /* Tracking */
    int engagement_count;
    regislex_datetime_t last_contact;
    bool is_active;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Engagement record
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t stakeholder_id;
    regislex_uuid_t legislation_id;
    regislex_engagement_type_t type;
    char subject[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_datetime_t engagement_date;
    int duration_minutes;
    char location[256];
    char participants[2048];    /* JSON array */
    char outcome[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char follow_up_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_datetime_t follow_up_date;
    char attachments[2048];     /* JSON array of document IDs */
    regislex_uuid_t conducted_by;
    regislex_datetime_t created_at;
} regislex_engagement_t;

/**
 * @brief Legislative alert configuration
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_alert_type_t type;
    bool is_active;

    /* Filters */
    char keywords[2048];
    char subjects[1024];
    regislex_legislation_type_t* leg_types;
    int leg_type_count;
    regislex_gov_level_t* gov_levels;
    int gov_level_count;
    char jurisdictions[512];
    char committees[1024];
    char sponsors[1024];

    /* Notification settings */
    char recipients[2048];      /* Email addresses */
    bool notify_email;
    bool notify_sms;
    bool notify_in_app;
    char webhook_url[512];

    /* Schedule */
    char digest_frequency[32];  /* "immediate", "daily", "weekly" */
    int digest_hour;
    int digest_day_of_week;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
} regislex_leg_alert_t;

/**
 * @brief Legislation filter criteria
 */
typedef struct {
    const char* keyword;
    const char* bill_number;
    regislex_legislation_type_t* type;
    regislex_leg_status_t* status;
    regislex_gov_level_t* gov_level;
    const char* jurisdiction;
    const char* chamber;
    regislex_uuid_t* sponsor_id;
    regislex_uuid_t* committee_id;
    const char* subject;
    regislex_datetime_t* introduced_after;
    regislex_datetime_t* introduced_before;
    regislex_datetime_t* last_action_after;
    bool tracked_only;
    regislex_position_t* position;
    regislex_uuid_t* assigned_to_id;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_leg_filter_t;

/**
 * @brief Legislation list result
 */
typedef struct {
    regislex_legislation_t** items;
    int count;
    int total_count;
} regislex_leg_list_t;

/* ============================================================================
 * Legislation Functions
 * ============================================================================ */

/**
 * @brief Create/import legislation
 * @param ctx Context
 * @param legislation Legislation data
 * @param out_legislation Output created legislation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_create(
    regislex_context_t* ctx,
    const regislex_legislation_t* legislation,
    regislex_legislation_t** out_legislation
);

/**
 * @brief Get legislation by ID
 * @param ctx Context
 * @param id Legislation ID
 * @param out_legislation Output legislation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_legislation_t** out_legislation
);

/**
 * @brief Get legislation by bill number
 * @param ctx Context
 * @param bill_number Bill number
 * @param jurisdiction Jurisdiction
 * @param session Legislative session
 * @param out_legislation Output legislation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_get_by_number(
    regislex_context_t* ctx,
    const char* bill_number,
    const char* jurisdiction,
    const char* session,
    regislex_legislation_t** out_legislation
);

/**
 * @brief Update legislation
 * @param ctx Context
 * @param legislation Updated legislation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_update(
    regislex_context_t* ctx,
    const regislex_legislation_t* legislation
);

/**
 * @brief Delete legislation
 * @param ctx Context
 * @param id Legislation ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Search/list legislation
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output legislation list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_list(
    regislex_context_t* ctx,
    const regislex_leg_filter_t* filter,
    regislex_leg_list_t** out_list
);

/**
 * @brief Track legislation
 * @param ctx Context
 * @param id Legislation ID
 * @param position Position on legislation
 * @param priority Priority level
 * @param assigned_to_id Assigned user ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_track(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_position_t position,
    regislex_priority_t priority,
    const regislex_uuid_t* assigned_to_id
);

/**
 * @brief Untrack legislation
 * @param ctx Context
 * @param id Legislation ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_untrack(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Set position on legislation
 * @param ctx Context
 * @param id Legislation ID
 * @param position Position
 * @param notes Position notes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_set_position(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_position_t position,
    const char* notes
);

/**
 * @brief Sync legislation from external source
 * @param ctx Context
 * @param source Source API ("congress", "openstates", etc.)
 * @param jurisdiction Jurisdiction to sync
 * @param session Session to sync
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislation_sync(
    regislex_context_t* ctx,
    const char* source,
    const char* jurisdiction,
    const char* session
);

/**
 * @brief Free legislation
 * @param legislation Legislation to free
 */
REGISLEX_API void regislex_legislation_free(regislex_legislation_t* legislation);

/**
 * @brief Free legislation list
 * @param list List to free
 */
REGISLEX_API void regislex_leg_list_free(regislex_leg_list_t* list);

/* ============================================================================
 * Regulation Functions
 * ============================================================================ */

/**
 * @brief Create regulation
 * @param ctx Context
 * @param regulation Regulation data
 * @param out_regulation Output created regulation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_regulation_create(
    regislex_context_t* ctx,
    const regislex_regulation_t* regulation,
    regislex_regulation_t** out_regulation
);

/**
 * @brief Get regulation by ID
 * @param ctx Context
 * @param id Regulation ID
 * @param out_regulation Output regulation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_regulation_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_regulation_t** out_regulation
);

/**
 * @brief List regulations with comment periods open
 * @param ctx Context
 * @param regulations Output regulation array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_regulation_open_comments(
    regislex_context_t* ctx,
    regislex_regulation_t*** regulations,
    int* count
);

/**
 * @brief Sync regulations from Regulations.gov
 * @param ctx Context
 * @param agency_id Agency ID to sync (NULL for all)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_regulation_sync(
    regislex_context_t* ctx,
    const char* agency_id
);

/**
 * @brief Free regulation
 * @param regulation Regulation to free
 */
REGISLEX_API void regislex_regulation_free(regislex_regulation_t* regulation);

/* ============================================================================
 * Stakeholder Functions
 * ============================================================================ */

/**
 * @brief Create stakeholder
 * @param ctx Context
 * @param stakeholder Stakeholder data
 * @param out_stakeholder Output created stakeholder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_stakeholder_create(
    regislex_context_t* ctx,
    const regislex_stakeholder_t* stakeholder,
    regislex_stakeholder_t** out_stakeholder
);

/**
 * @brief Get stakeholder by ID
 * @param ctx Context
 * @param id Stakeholder ID
 * @param out_stakeholder Output stakeholder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_stakeholder_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_stakeholder_t** out_stakeholder
);

/**
 * @brief Update stakeholder
 * @param ctx Context
 * @param stakeholder Updated stakeholder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_stakeholder_update(
    regislex_context_t* ctx,
    const regislex_stakeholder_t* stakeholder
);

/**
 * @brief Search stakeholders
 * @param ctx Context
 * @param type Filter by type (NULL for all)
 * @param keyword Search keyword
 * @param stakeholders Output stakeholder array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_stakeholder_search(
    regislex_context_t* ctx,
    const regislex_stakeholder_type_t* type,
    const char* keyword,
    regislex_stakeholder_t*** stakeholders,
    int* count
);

/**
 * @brief Free stakeholder
 * @param stakeholder Stakeholder to free
 */
REGISLEX_API void regislex_stakeholder_free(regislex_stakeholder_t* stakeholder);

/* ============================================================================
 * Engagement Functions
 * ============================================================================ */

/**
 * @brief Log engagement with stakeholder
 * @param ctx Context
 * @param engagement Engagement data
 * @param out_engagement Output created engagement
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_engagement_log(
    regislex_context_t* ctx,
    const regislex_engagement_t* engagement,
    regislex_engagement_t** out_engagement
);

/**
 * @brief Get engagements for stakeholder
 * @param ctx Context
 * @param stakeholder_id Stakeholder ID
 * @param engagements Output engagement array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_engagement_list_by_stakeholder(
    regislex_context_t* ctx,
    const regislex_uuid_t* stakeholder_id,
    regislex_engagement_t*** engagements,
    int* count
);

/**
 * @brief Get engagements for legislation
 * @param ctx Context
 * @param legislation_id Legislation ID
 * @param engagements Output engagement array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_engagement_list_by_legislation(
    regislex_context_t* ctx,
    const regislex_uuid_t* legislation_id,
    regislex_engagement_t*** engagements,
    int* count
);

/**
 * @brief Free engagement
 * @param engagement Engagement to free
 */
REGISLEX_API void regislex_engagement_free(regislex_engagement_t* engagement);

/* ============================================================================
 * Alert Functions
 * ============================================================================ */

/**
 * @brief Create legislative alert
 * @param ctx Context
 * @param alert Alert configuration
 * @param out_alert Output created alert
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_create(
    regislex_context_t* ctx,
    const regislex_leg_alert_t* alert,
    regislex_leg_alert_t** out_alert
);

/**
 * @brief Get alert by ID
 * @param ctx Context
 * @param id Alert ID
 * @param out_alert Output alert
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_leg_alert_t** out_alert
);

/**
 * @brief Update alert
 * @param ctx Context
 * @param alert Updated alert
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_update(
    regislex_context_t* ctx,
    const regislex_leg_alert_t* alert
);

/**
 * @brief Delete alert
 * @param ctx Context
 * @param id Alert ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List alerts
 * @param ctx Context
 * @param alerts Output alert array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_list(
    regislex_context_t* ctx,
    regislex_leg_alert_t*** alerts,
    int* count
);

/**
 * @brief Activate alert
 * @param ctx Context
 * @param id Alert ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_activate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Deactivate alert
 * @param ctx Context
 * @param id Alert ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_leg_alert_deactivate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Free alert
 * @param alert Alert to free
 */
REGISLEX_API void regislex_leg_alert_free(regislex_leg_alert_t* alert);

/* ============================================================================
 * Legislator/Committee Functions
 * ============================================================================ */

/**
 * @brief Get legislator by ID
 * @param ctx Context
 * @param id Legislator ID
 * @param out_legislator Output legislator
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislator_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_legislator_t** out_legislator
);

/**
 * @brief Search legislators
 * @param ctx Context
 * @param jurisdiction Jurisdiction
 * @param name_keyword Name search
 * @param party Party filter (NULL for all)
 * @param legislators Output legislator array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislator_search(
    regislex_context_t* ctx,
    const char* jurisdiction,
    const char* name_keyword,
    const char* party,
    regislex_legislator_t*** legislators,
    int* count
);

/**
 * @brief Get committee by ID
 * @param ctx Context
 * @param id Committee ID
 * @param out_committee Output committee
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_committee_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_committee_t** out_committee
);

/**
 * @brief List committees
 * @param ctx Context
 * @param jurisdiction Jurisdiction
 * @param chamber Chamber filter (NULL for all)
 * @param committees Output committee array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_committee_list(
    regislex_context_t* ctx,
    const char* jurisdiction,
    const char* chamber,
    regislex_committee_t*** committees,
    int* count
);

/**
 * @brief Sync legislators from external source
 * @param ctx Context
 * @param source Source API
 * @param jurisdiction Jurisdiction
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_legislator_sync(
    regislex_context_t* ctx,
    const char* source,
    const char* jurisdiction
);

/**
 * @brief Free legislator
 * @param legislator Legislator to free
 */
REGISLEX_API void regislex_legislator_free(regislex_legislator_t* legislator);

/**
 * @brief Free committee
 * @param committee Committee to free
 */
REGISLEX_API void regislex_committee_free(regislex_committee_t* committee);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_LEGISLATIVE_H */
