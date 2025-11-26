/**
 * @file case.h
 * @brief Case Management Module
 *
 * Provides functionality for managing legal cases, matters,
 * parties, and related case information.
 */

#ifndef REGISLEX_CASE_H
#define REGISLEX_CASE_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Case Types and Enums
 * ============================================================================ */

/**
 * @brief Case type classification
 */
typedef enum {
    REGISLEX_CASE_TYPE_CIVIL = 0,
    REGISLEX_CASE_TYPE_CRIMINAL,
    REGISLEX_CASE_TYPE_ADMINISTRATIVE,
    REGISLEX_CASE_TYPE_REGULATORY,
    REGISLEX_CASE_TYPE_APPELLATE,
    REGISLEX_CASE_TYPE_BANKRUPTCY,
    REGISLEX_CASE_TYPE_FAMILY,
    REGISLEX_CASE_TYPE_PROBATE,
    REGISLEX_CASE_TYPE_TAX,
    REGISLEX_CASE_TYPE_IMMIGRATION,
    REGISLEX_CASE_TYPE_INTELLECTUAL_PROPERTY,
    REGISLEX_CASE_TYPE_EMPLOYMENT,
    REGISLEX_CASE_TYPE_ENVIRONMENTAL,
    REGISLEX_CASE_TYPE_CONTRACT,
    REGISLEX_CASE_TYPE_TORT,
    REGISLEX_CASE_TYPE_OTHER
} regislex_case_type_t;

/**
 * @brief Party role in a case
 */
typedef enum {
    REGISLEX_PARTY_PLAINTIFF = 0,
    REGISLEX_PARTY_DEFENDANT,
    REGISLEX_PARTY_PETITIONER,
    REGISLEX_PARTY_RESPONDENT,
    REGISLEX_PARTY_APPELLANT,
    REGISLEX_PARTY_APPELLEE,
    REGISLEX_PARTY_INTERVENOR,
    REGISLEX_PARTY_WITNESS,
    REGISLEX_PARTY_EXPERT_WITNESS,
    REGISLEX_PARTY_THIRD_PARTY,
    REGISLEX_PARTY_COUNSEL,
    REGISLEX_PARTY_JUDGE,
    REGISLEX_PARTY_MEDIATOR,
    REGISLEX_PARTY_ARBITRATOR,
    REGISLEX_PARTY_GUARDIAN,
    REGISLEX_PARTY_OTHER
} regislex_party_role_t;

/**
 * @brief Party type
 */
typedef enum {
    REGISLEX_PARTY_TYPE_INDIVIDUAL = 0,
    REGISLEX_PARTY_TYPE_CORPORATION,
    REGISLEX_PARTY_TYPE_LLC,
    REGISLEX_PARTY_TYPE_PARTNERSHIP,
    REGISLEX_PARTY_TYPE_GOVERNMENT,
    REGISLEX_PARTY_TYPE_NONPROFIT,
    REGISLEX_PARTY_TYPE_TRUST,
    REGISLEX_PARTY_TYPE_ESTATE,
    REGISLEX_PARTY_TYPE_OTHER
} regislex_party_type_t;

/**
 * @brief Case outcome
 */
typedef enum {
    REGISLEX_OUTCOME_PENDING = 0,
    REGISLEX_OUTCOME_SETTLED,
    REGISLEX_OUTCOME_JUDGMENT_PLAINTIFF,
    REGISLEX_OUTCOME_JUDGMENT_DEFENDANT,
    REGISLEX_OUTCOME_DISMISSED,
    REGISLEX_OUTCOME_DISMISSED_WITH_PREJUDICE,
    REGISLEX_OUTCOME_DISMISSED_WITHOUT_PREJUDICE,
    REGISLEX_OUTCOME_DEFAULT_JUDGMENT,
    REGISLEX_OUTCOME_SUMMARY_JUDGMENT,
    REGISLEX_OUTCOME_VERDICT,
    REGISLEX_OUTCOME_APPEAL_AFFIRMED,
    REGISLEX_OUTCOME_APPEAL_REVERSED,
    REGISLEX_OUTCOME_APPEAL_REMANDED,
    REGISLEX_OUTCOME_WITHDRAWN,
    REGISLEX_OUTCOME_OTHER
} regislex_case_outcome_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Contact information
 */
typedef struct {
    char address_line1[256];
    char address_line2[256];
    char city[128];
    char state[64];
    char postal_code[32];
    char country[64];
    char phone[32];
    char fax[32];
    char email[256];
    char website[256];
} regislex_contact_t;

/**
 * @brief Party in a legal case
 */
struct regislex_party {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char display_name[REGISLEX_MAX_NAME_LENGTH];
    regislex_party_type_t type;
    regislex_party_role_t role;
    regislex_contact_t contact;
    char attorney_name[REGISLEX_MAX_NAME_LENGTH];
    char attorney_firm[REGISLEX_MAX_NAME_LENGTH];
    char bar_number[64];
    char notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    bool is_primary;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Court information
 */
typedef struct {
    char name[REGISLEX_MAX_NAME_LENGTH];
    char division[128];
    char jurisdiction[128];
    char address[512];
    char phone[32];
    char judge_name[REGISLEX_MAX_NAME_LENGTH];
    char clerk_name[REGISLEX_MAX_NAME_LENGTH];
    char courtroom[64];
} regislex_court_t;

/**
 * @brief Legal case/matter
 */
struct regislex_case {
    regislex_uuid_t id;
    char case_number[64];
    char title[REGISLEX_MAX_NAME_LENGTH];
    char short_title[128];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_case_type_t type;
    regislex_status_t status;
    regislex_priority_t priority;
    regislex_case_outcome_t outcome;

    /* Court information */
    regislex_court_t court;
    char docket_number[64];

    /* Case identifiers */
    char internal_reference[64];
    char client_reference[64];

    /* Financial */
    regislex_money_t estimated_value;
    regislex_money_t settlement_amount;
    regislex_money_t fees_billed;
    regislex_money_t fees_collected;

    /* Dates */
    regislex_datetime_t filed_date;
    regislex_datetime_t service_date;
    regislex_datetime_t discovery_cutoff;
    regislex_datetime_t trial_date;
    regislex_datetime_t closed_date;
    regislex_datetime_t statute_of_limitations;

    /* Assignments */
    regislex_uuid_t lead_attorney_id;
    regislex_uuid_t assigned_to_id;
    regislex_uuid_t client_id;

    /* Relationships */
    regislex_uuid_t parent_case_id;
    int party_count;
    regislex_party_t** parties;

    /* Metadata */
    int metadata_count;
    regislex_metadata_t* metadata;
    char tags[1024];

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
    regislex_uuid_t updated_by;
};

/**
 * @brief Matter/sub-case for complex litigation
 */
struct regislex_matter {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    char matter_number[64];
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_status_t status;
    regislex_priority_t priority;
    regislex_datetime_t start_date;
    regislex_datetime_t end_date;
    regislex_money_t budget;
    regislex_uuid_t assigned_to_id;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Case search/filter criteria
 */
typedef struct {
    const char* case_number;
    const char* title_contains;
    regislex_case_type_t* type;
    regislex_status_t* status;
    regislex_priority_t* priority;
    regislex_uuid_t* assigned_to_id;
    regislex_uuid_t* client_id;
    regislex_datetime_t* filed_after;
    regislex_datetime_t* filed_before;
    const char* court_name;
    const char* tags_contain;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_case_filter_t;

/**
 * @brief Case list result
 */
typedef struct {
    regislex_case_t** cases;
    int count;
    int total_count;
    int offset;
    int limit;
} regislex_case_list_t;

/* ============================================================================
 * Case Management Functions
 * ============================================================================ */

/**
 * @brief Create a new case
 * @param ctx Context
 * @param case_data Case data to create
 * @param out_case Output created case
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_create(
    regislex_context_t* ctx,
    const regislex_case_t* case_data,
    regislex_case_t** out_case
);

/**
 * @brief Get a case by ID
 * @param ctx Context
 * @param id Case ID
 * @param out_case Output case
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_case_t** out_case
);

/**
 * @brief Get a case by case number
 * @param ctx Context
 * @param case_number Case number
 * @param out_case Output case
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_get_by_number(
    regislex_context_t* ctx,
    const char* case_number,
    regislex_case_t** out_case
);

/**
 * @brief Update a case
 * @param ctx Context
 * @param case_data Updated case data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_update(
    regislex_context_t* ctx,
    const regislex_case_t* case_data
);

/**
 * @brief Delete a case
 * @param ctx Context
 * @param id Case ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List cases with filtering
 * @param ctx Context
 * @param filter Filter criteria (NULL for all)
 * @param out_list Output case list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_list(
    regislex_context_t* ctx,
    const regislex_case_filter_t* filter,
    regislex_case_list_t** out_list
);

/**
 * @brief Free a case structure
 * @param case_ptr Case to free
 */
REGISLEX_API void regislex_case_free(regislex_case_t* case_ptr);

/**
 * @brief Free a case list
 * @param list List to free
 */
REGISLEX_API void regislex_case_list_free(regislex_case_list_t* list);

/**
 * @brief Change case status
 * @param ctx Context
 * @param id Case ID
 * @param new_status New status
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_change_status(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_status_t new_status
);

/**
 * @brief Assign case to user
 * @param ctx Context
 * @param case_id Case ID
 * @param user_id User ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_case_assign(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    const regislex_uuid_t* user_id
);

/* ============================================================================
 * Party Management Functions
 * ============================================================================ */

/**
 * @brief Add a party to a case
 * @param ctx Context
 * @param case_id Case ID
 * @param party Party data
 * @param out_party Output created party
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_party_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    const regislex_party_t* party,
    regislex_party_t** out_party
);

/**
 * @brief Update a party
 * @param ctx Context
 * @param party Updated party data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_party_update(
    regislex_context_t* ctx,
    const regislex_party_t* party
);

/**
 * @brief Remove a party from a case
 * @param ctx Context
 * @param case_id Case ID
 * @param party_id Party ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_party_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    const regislex_uuid_t* party_id
);

/**
 * @brief List parties for a case
 * @param ctx Context
 * @param case_id Case ID
 * @param parties Output party array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_party_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    regislex_party_t*** parties,
    int* count
);

/**
 * @brief Free a party structure
 * @param party Party to free
 */
REGISLEX_API void regislex_party_free(regislex_party_t* party);

/* ============================================================================
 * Matter Management Functions
 * ============================================================================ */

/**
 * @brief Create a matter under a case
 * @param ctx Context
 * @param matter Matter data
 * @param out_matter Output created matter
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_matter_create(
    regislex_context_t* ctx,
    const regislex_matter_t* matter,
    regislex_matter_t** out_matter
);

/**
 * @brief Get a matter by ID
 * @param ctx Context
 * @param id Matter ID
 * @param out_matter Output matter
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_matter_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_matter_t** out_matter
);

/**
 * @brief List matters for a case
 * @param ctx Context
 * @param case_id Case ID
 * @param matters Output matter array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_matter_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* case_id,
    regislex_matter_t*** matters,
    int* count
);

/**
 * @brief Update a matter
 * @param ctx Context
 * @param matter Updated matter data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_matter_update(
    regislex_context_t* ctx,
    const regislex_matter_t* matter
);

/**
 * @brief Delete a matter
 * @param ctx Context
 * @param id Matter ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_matter_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Free a matter structure
 * @param matter Matter to free
 */
REGISLEX_API void regislex_matter_free(regislex_matter_t* matter);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_CASE_H */
