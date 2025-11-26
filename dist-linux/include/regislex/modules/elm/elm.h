/**
 * @file elm.h
 * @brief Enterprise Legal Management (ELM) Module
 *
 * Provides functionality for managing legal operations including
 * outside counsel spending (eBilling), contract management, and risk mitigation.
 */

#ifndef REGISLEX_ELM_H
#define REGISLEX_ELM_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ELM Types and Enums
 * ============================================================================ */

/**
 * @brief Contract type
 */
typedef enum {
    REGISLEX_CONTRACT_SERVICE = 0,
    REGISLEX_CONTRACT_EMPLOYMENT,
    REGISLEX_CONTRACT_NDA,
    REGISLEX_CONTRACT_LICENSE,
    REGISLEX_CONTRACT_LEASE,
    REGISLEX_CONTRACT_VENDOR,
    REGISLEX_CONTRACT_PARTNERSHIP,
    REGISLEX_CONTRACT_SETTLEMENT,
    REGISLEX_CONTRACT_RETAINER,
    REGISLEX_CONTRACT_SLA,
    REGISLEX_CONTRACT_MASTER,
    REGISLEX_CONTRACT_AMENDMENT,
    REGISLEX_CONTRACT_SOW,
    REGISLEX_CONTRACT_OTHER
} regislex_contract_type_t;

/**
 * @brief Contract status
 */
typedef enum {
    REGISLEX_CONTRACT_DRAFT = 0,
    REGISLEX_CONTRACT_NEGOTIATION,
    REGISLEX_CONTRACT_PENDING_APPROVAL,
    REGISLEX_CONTRACT_APPROVED,
    REGISLEX_CONTRACT_EXECUTED,
    REGISLEX_CONTRACT_ACTIVE,
    REGISLEX_CONTRACT_EXPIRED,
    REGISLEX_CONTRACT_TERMINATED,
    REGISLEX_CONTRACT_RENEWED,
    REGISLEX_CONTRACT_CANCELLED
} regislex_contract_status_t;

/**
 * @brief Invoice status
 */
typedef enum {
    REGISLEX_INVOICE_DRAFT = 0,
    REGISLEX_INVOICE_SUBMITTED,
    REGISLEX_INVOICE_UNDER_REVIEW,
    REGISLEX_INVOICE_APPROVED,
    REGISLEX_INVOICE_DISPUTED,
    REGISLEX_INVOICE_ADJUSTED,
    REGISLEX_INVOICE_PAID,
    REGISLEX_INVOICE_REJECTED,
    REGISLEX_INVOICE_VOID
} regislex_invoice_status_t;

/**
 * @brief Billing type
 */
typedef enum {
    REGISLEX_BILLING_HOURLY = 0,
    REGISLEX_BILLING_FLAT_FEE,
    REGISLEX_BILLING_CONTINGENCY,
    REGISLEX_BILLING_RETAINER,
    REGISLEX_BILLING_CAPPED,
    REGISLEX_BILLING_SUCCESS_FEE,
    REGISLEX_BILLING_BLENDED,
    REGISLEX_BILLING_TASK_BASED
} regislex_billing_type_t;

/**
 * @brief Risk category
 */
typedef enum {
    REGISLEX_RISK_COMPLIANCE = 0,
    REGISLEX_RISK_REGULATORY,
    REGISLEX_RISK_LITIGATION,
    REGISLEX_RISK_CONTRACTUAL,
    REGISLEX_RISK_IP,
    REGISLEX_RISK_DATA_PRIVACY,
    REGISLEX_RISK_EMPLOYMENT,
    REGISLEX_RISK_ENVIRONMENTAL,
    REGISLEX_RISK_FINANCIAL,
    REGISLEX_RISK_REPUTATIONAL,
    REGISLEX_RISK_OPERATIONAL,
    REGISLEX_RISK_OTHER
} regislex_risk_category_t;

/**
 * @brief Risk level
 */
typedef enum {
    REGISLEX_RISK_LOW = 0,
    REGISLEX_RISK_MEDIUM,
    REGISLEX_RISK_HIGH,
    REGISLEX_RISK_CRITICAL
} regislex_risk_level_t;

/**
 * @brief Risk status
 */
typedef enum {
    REGISLEX_RISK_IDENTIFIED = 0,
    REGISLEX_RISK_ASSESSING,
    REGISLEX_RISK_MITIGATING,
    REGISLEX_RISK_MONITORING,
    REGISLEX_RISK_RESOLVED,
    REGISLEX_RISK_ACCEPTED
} regislex_risk_status_t;

/**
 * @brief Vendor status
 */
typedef enum {
    REGISLEX_VENDOR_PROSPECT = 0,
    REGISLEX_VENDOR_APPROVED,
    REGISLEX_VENDOR_PREFERRED,
    REGISLEX_VENDOR_ACTIVE,
    REGISLEX_VENDOR_ON_HOLD,
    REGISLEX_VENDOR_TERMINATED,
    REGISLEX_VENDOR_BLACKLISTED
} regislex_vendor_status_t;

/**
 * @brief UTBMS/LEDES task code categories
 */
typedef enum {
    REGISLEX_TASK_ANALYSIS = 0,
    REGISLEX_TASK_PLEADINGS,
    REGISLEX_TASK_DISCOVERY,
    REGISLEX_TASK_TRIAL,
    REGISLEX_TASK_APPEAL,
    REGISLEX_TASK_PROJECT_MGMT,
    REGISLEX_TASK_NEGOTIATION,
    REGISLEX_TASK_COUNSELING,
    REGISLEX_TASK_OTHER
} regislex_task_category_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Law firm/vendor
 */
struct regislex_vendor {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char legal_name[REGISLEX_MAX_NAME_LENGTH];
    char tax_id[64];
    char duns_number[32];
    regislex_vendor_status_t status;
    regislex_contact_t contact;
    char website[256];

    /* Classification */
    char vendor_type[64];       /* "Law Firm", "eDiscovery", "Expert", etc. */
    char practice_areas[2048];
    char jurisdictions[1024];
    char diversity_certifications[512];
    bool is_minority_owned;
    bool is_woman_owned;
    bool is_veteran_owned;

    /* Rating */
    int quality_rating;         /* 1-5 */
    int responsiveness_rating;
    int value_rating;

    /* Financial */
    char payment_terms[256];
    char preferred_currency[4];
    char banking_info[1024];    /* Encrypted */

    /* Insurance */
    char malpractice_policy[256];
    regislex_money_t malpractice_coverage;
    regislex_datetime_t insurance_expiration;

    /* Contacts */
    char primary_contact[256];
    char billing_contact[256];
    char relationship_partner[256];

    /* Performance tracking */
    regislex_money_t total_spend_ytd;
    regislex_money_t total_spend_lifetime;
    int active_matter_count;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_datetime_t last_reviewed;
    regislex_uuid_t created_by;
};

/**
 * @brief Rate card entry
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t vendor_id;
    char timekeeper_name[REGISLEX_MAX_NAME_LENGTH];
    char timekeeper_id[64];
    char title[128];
    char classification[64];    /* "Partner", "Associate", "Paralegal" */
    int years_experience;
    regislex_money_t standard_rate;
    regislex_money_t negotiated_rate;
    regislex_money_t blended_rate;
    regislex_datetime_t effective_date;
    regislex_datetime_t expiration_date;
    bool is_approved;
    regislex_datetime_t created_at;
} regislex_rate_card_t;

/**
 * @brief Budget line item
 */
typedef struct {
    regislex_uuid_t id;
    char phase[128];
    char task_code[32];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_money_t budgeted_amount;
    regislex_money_t spent_amount;
    regislex_money_t committed_amount;
    int budgeted_hours;
    int spent_hours;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_budget_line_t;

/**
 * @brief Matter budget
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_money_t total_budget;
    regislex_money_t total_spent;
    regislex_money_t total_committed;
    int total_hours_budget;
    int total_hours_spent;
    int line_count;
    regislex_budget_line_t* lines;
    char fiscal_year[16];
    regislex_status_t status;
    regislex_uuid_t approved_by;
    regislex_datetime_t approved_at;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_budget_t;

/**
 * @brief Invoice line item
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t invoice_id;
    regislex_datetime_t service_date;
    char timekeeper_id[64];
    char timekeeper_name[256];
    char task_code[32];         /* UTBMS code */
    char activity_code[32];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    double hours;
    regislex_money_t rate;
    regislex_money_t amount;
    regislex_money_t adjustment;
    char adjustment_reason[512];
    bool is_expense;
    char expense_type[64];
    bool is_approved;
    char reviewer_notes[512];
} regislex_invoice_line_t;

/**
 * @brief Invoice
 */
struct regislex_invoice {
    regislex_uuid_t id;
    regislex_uuid_t vendor_id;
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    char invoice_number[64];
    char vendor_invoice_number[64];
    regislex_invoice_status_t status;

    /* Dates */
    regislex_datetime_t invoice_date;
    regislex_datetime_t received_date;
    regislex_datetime_t due_date;
    regislex_datetime_t paid_date;
    regislex_datetime_t period_start;
    regislex_datetime_t period_end;

    /* Amounts */
    regislex_money_t subtotal_fees;
    regislex_money_t subtotal_expenses;
    regislex_money_t adjustments;
    regislex_money_t taxes;
    regislex_money_t total_amount;
    regislex_money_t amount_paid;
    regislex_money_t balance_due;

    /* Hours */
    double total_hours;
    double attorney_hours;
    double paralegal_hours;

    /* Line items */
    int line_count;
    regislex_invoice_line_t* lines;

    /* LEDES/eBilling */
    char ledes_version[16];
    char ledes_file_path[REGISLEX_MAX_PATH_LENGTH];
    bool is_ledes_compliant;

    /* Review */
    regislex_uuid_t reviewed_by;
    regislex_datetime_t reviewed_at;
    char review_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    int reduction_percentage;

    /* Payment */
    char payment_reference[128];
    char payment_method[64];

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Contract clause
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t contract_id;
    char clause_type[128];
    char title[REGISLEX_MAX_NAME_LENGTH];
    char content[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char standard_language[REGISLEX_MAX_DESCRIPTION_LENGTH];
    bool is_negotiated;
    char negotiation_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_risk_level_t risk_level;
    int sequence;
    regislex_datetime_t created_at;
} regislex_contract_clause_t;

/**
 * @brief Contract obligation
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t contract_id;
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char responsible_party[256];
    regislex_datetime_t due_date;
    regislex_recurrence_t recurrence;
    bool is_completed;
    regislex_datetime_t completed_at;
    char completion_notes[512];
    regislex_uuid_t assigned_to_id;
    int reminder_days_before;
    regislex_datetime_t created_at;
} regislex_obligation_t;

/**
 * @brief Contract
 */
struct regislex_contract {
    regislex_uuid_t id;
    char contract_number[64];
    char title[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_contract_type_t type;
    regislex_contract_status_t status;

    /* Parties */
    regislex_uuid_t vendor_id;
    char counterparty_name[REGISLEX_MAX_NAME_LENGTH];
    char counterparty_contact[512];
    char our_signatory[REGISLEX_MAX_NAME_LENGTH];
    char their_signatory[REGISLEX_MAX_NAME_LENGTH];

    /* Relationship to case/matter */
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    regislex_uuid_t parent_contract_id;

    /* Dates */
    regislex_datetime_t effective_date;
    regislex_datetime_t expiration_date;
    regislex_datetime_t execution_date;
    regislex_datetime_t termination_date;
    int notice_period_days;
    bool auto_renewal;
    int renewal_term_months;
    regislex_datetime_t next_renewal_date;

    /* Financial terms */
    regislex_money_t total_value;
    regislex_money_t annual_value;
    char payment_terms[512];
    char billing_frequency[64];
    regislex_billing_type_t billing_type;

    /* Document */
    regislex_uuid_t document_id;
    char document_path[REGISLEX_MAX_PATH_LENGTH];

    /* Clauses and obligations */
    int clause_count;
    regislex_contract_clause_t* clauses;
    int obligation_count;
    regislex_obligation_t* obligations;

    /* Risk */
    regislex_risk_level_t risk_level;
    char risk_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];

    /* Assignment */
    regislex_uuid_t owner_id;
    char department[128];
    char business_unit[128];

    /* Metadata */
    char tags[1024];
    char keywords[1024];

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
    regislex_uuid_t updated_by;
};

/**
 * @brief Risk mitigation action
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t risk_id;
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_uuid_t assigned_to_id;
    regislex_datetime_t due_date;
    regislex_status_t status;
    regislex_datetime_t completed_at;
    char completion_notes[512];
    regislex_money_t estimated_cost;
    regislex_money_t actual_cost;
    regislex_datetime_t created_at;
} regislex_risk_action_t;

/**
 * @brief Legal risk
 */
struct regislex_risk {
    regislex_uuid_t id;
    char title[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_risk_category_t category;
    regislex_risk_level_t level;
    regislex_risk_status_t status;

    /* Assessment */
    int likelihood_score;       /* 1-5 */
    int impact_score;           /* 1-5 */
    int risk_score;             /* likelihood * impact */
    regislex_money_t potential_exposure;
    char exposure_basis[512];

    /* Context */
    regislex_uuid_t case_id;
    regislex_uuid_t contract_id;
    regislex_uuid_t vendor_id;
    char jurisdiction[128];
    char regulatory_reference[512];

    /* Mitigation */
    int action_count;
    regislex_risk_action_t* actions;
    char mitigation_strategy[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char contingency_plan[REGISLEX_MAX_DESCRIPTION_LENGTH];

    /* Ownership */
    regislex_uuid_t owner_id;
    char department[128];

    /* Review */
    regislex_datetime_t identified_date;
    regislex_datetime_t last_assessed;
    regislex_datetime_t next_review;
    regislex_uuid_t assessed_by;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Legal spend summary
 */
typedef struct {
    regislex_datetime_t period_start;
    regislex_datetime_t period_end;
    regislex_money_t total_spend;
    regislex_money_t fees;
    regislex_money_t expenses;
    regislex_money_t outside_counsel;
    regislex_money_t in_house;
    double total_hours;
    int invoice_count;
    int matter_count;
    int vendor_count;
    char by_practice_area[4096];    /* JSON breakdown */
    char by_vendor[4096];           /* JSON breakdown */
    char by_matter_type[4096];      /* JSON breakdown */
} regislex_spend_summary_t;

/**
 * @brief Contract filter criteria
 */
typedef struct {
    regislex_contract_type_t* type;
    regislex_contract_status_t* status;
    regislex_uuid_t* vendor_id;
    regislex_uuid_t* owner_id;
    const char* counterparty;
    regislex_datetime_t* expiring_before;
    regislex_datetime_t* effective_after;
    bool expiring_soon;         /* Within 90 days */
    const char* keyword;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_contract_filter_t;

/**
 * @brief Contract list result
 */
typedef struct {
    regislex_contract_t** contracts;
    int count;
    int total_count;
} regislex_contract_list_t;

/**
 * @brief Invoice filter criteria
 */
typedef struct {
    regislex_uuid_t* vendor_id;
    regislex_uuid_t* case_id;
    regislex_uuid_t* matter_id;
    regislex_invoice_status_t* status;
    regislex_datetime_t* invoice_date_after;
    regislex_datetime_t* invoice_date_before;
    regislex_datetime_t* due_before;
    bool overdue_only;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_invoice_filter_t;

/**
 * @brief Invoice list result
 */
typedef struct {
    regislex_invoice_t** invoices;
    int count;
    int total_count;
    regislex_money_t total_amount;
} regislex_invoice_list_t;

/* ============================================================================
 * Vendor Functions
 * ============================================================================ */

/**
 * @brief Create vendor
 * @param ctx Context
 * @param vendor Vendor data
 * @param out_vendor Output created vendor
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_create(
    regislex_context_t* ctx,
    const regislex_vendor_t* vendor,
    regislex_vendor_t** out_vendor
);

/**
 * @brief Get vendor by ID
 * @param ctx Context
 * @param id Vendor ID
 * @param out_vendor Output vendor
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_vendor_t** out_vendor
);

/**
 * @brief Update vendor
 * @param ctx Context
 * @param vendor Updated vendor
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_update(
    regislex_context_t* ctx,
    const regislex_vendor_t* vendor
);

/**
 * @brief List vendors
 * @param ctx Context
 * @param status Filter by status (NULL for all)
 * @param vendor_type Filter by type (NULL for all)
 * @param vendors Output vendor array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_list(
    regislex_context_t* ctx,
    const regislex_vendor_status_t* status,
    const char* vendor_type,
    regislex_vendor_t*** vendors,
    int* count
);

/**
 * @brief Get vendor rate card
 * @param ctx Context
 * @param vendor_id Vendor ID
 * @param rates Output rate array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_rate_card(
    regislex_context_t* ctx,
    const regislex_uuid_t* vendor_id,
    regislex_rate_card_t*** rates,
    int* count
);

/**
 * @brief Update vendor rate card
 * @param ctx Context
 * @param rate Rate data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_vendor_rate_update(
    regislex_context_t* ctx,
    const regislex_rate_card_t* rate
);

/**
 * @brief Free vendor
 * @param vendor Vendor to free
 */
REGISLEX_API void regislex_vendor_free(regislex_vendor_t* vendor);

/* ============================================================================
 * Invoice/eBilling Functions
 * ============================================================================ */

/**
 * @brief Create invoice
 * @param ctx Context
 * @param invoice Invoice data
 * @param out_invoice Output created invoice
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_create(
    regislex_context_t* ctx,
    const regislex_invoice_t* invoice,
    regislex_invoice_t** out_invoice
);

/**
 * @brief Import LEDES invoice file
 * @param ctx Context
 * @param file_path Path to LEDES file
 * @param vendor_id Vendor ID
 * @param out_invoice Output created invoice
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_import_ledes(
    regislex_context_t* ctx,
    const char* file_path,
    const regislex_uuid_t* vendor_id,
    regislex_invoice_t** out_invoice
);

/**
 * @brief Get invoice by ID
 * @param ctx Context
 * @param id Invoice ID
 * @param out_invoice Output invoice
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_invoice_t** out_invoice
);

/**
 * @brief Update invoice
 * @param ctx Context
 * @param invoice Updated invoice
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_update(
    regislex_context_t* ctx,
    const regislex_invoice_t* invoice
);

/**
 * @brief List invoices
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output invoice list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_list(
    regislex_context_t* ctx,
    const regislex_invoice_filter_t* filter,
    regislex_invoice_list_t** out_list
);

/**
 * @brief Submit invoice for review
 * @param ctx Context
 * @param id Invoice ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_submit(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Approve invoice
 * @param ctx Context
 * @param id Invoice ID
 * @param notes Approval notes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_approve(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* notes
);

/**
 * @brief Reject invoice
 * @param ctx Context
 * @param id Invoice ID
 * @param reason Rejection reason
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_reject(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* reason
);

/**
 * @brief Apply adjustment to invoice line
 * @param ctx Context
 * @param line_id Line item ID
 * @param adjustment Adjustment amount (negative for reduction)
 * @param reason Adjustment reason
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_adjust_line(
    regislex_context_t* ctx,
    const regislex_uuid_t* line_id,
    regislex_money_t adjustment,
    const char* reason
);

/**
 * @brief Mark invoice as paid
 * @param ctx Context
 * @param id Invoice ID
 * @param payment_reference Payment reference
 * @param payment_date Payment date
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_mark_paid(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* payment_reference,
    const regislex_datetime_t* payment_date
);

/**
 * @brief Validate invoice against billing guidelines
 * @param ctx Context
 * @param id Invoice ID
 * @param issues Output validation issues (JSON)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_validate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    char** issues
);

/**
 * @brief Export invoice to LEDES format
 * @param ctx Context
 * @param id Invoice ID
 * @param version LEDES version ("1998B", "2000", "XML")
 * @param out_path Output file path
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_invoice_export_ledes(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const char* version,
    char* out_path
);

/**
 * @brief Free invoice
 * @param invoice Invoice to free
 */
REGISLEX_API void regislex_invoice_free(regislex_invoice_t* invoice);

/**
 * @brief Free invoice list
 * @param list List to free
 */
REGISLEX_API void regislex_invoice_list_free(regislex_invoice_list_t* list);

/* ============================================================================
 * Budget Functions
 * ============================================================================ */

/**
 * @brief Create budget
 * @param ctx Context
 * @param budget Budget data
 * @param out_budget Output created budget
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_create(
    regislex_context_t* ctx,
    const regislex_budget_t* budget,
    regislex_budget_t** out_budget
);

/**
 * @brief Get budget by ID
 * @param ctx Context
 * @param id Budget ID
 * @param out_budget Output budget
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_budget_t** out_budget
);

/**
 * @brief Get budget for matter
 * @param ctx Context
 * @param matter_id Matter ID
 * @param out_budget Output budget
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_get_by_matter(
    regislex_context_t* ctx,
    const regislex_uuid_t* matter_id,
    regislex_budget_t** out_budget
);

/**
 * @brief Update budget
 * @param ctx Context
 * @param budget Updated budget
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_update(
    regislex_context_t* ctx,
    const regislex_budget_t* budget
);

/**
 * @brief Approve budget
 * @param ctx Context
 * @param id Budget ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_approve(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Check budget vs actual
 * @param ctx Context
 * @param id Budget ID
 * @param variance Output variance analysis (JSON)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_budget_variance(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    char** variance
);

/**
 * @brief Free budget
 * @param budget Budget to free
 */
REGISLEX_API void regislex_budget_free(regislex_budget_t* budget);

/* ============================================================================
 * Contract Functions
 * ============================================================================ */

/**
 * @brief Create contract
 * @param ctx Context
 * @param contract Contract data
 * @param out_contract Output created contract
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_create(
    regislex_context_t* ctx,
    const regislex_contract_t* contract,
    regislex_contract_t** out_contract
);

/**
 * @brief Get contract by ID
 * @param ctx Context
 * @param id Contract ID
 * @param out_contract Output contract
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_contract_t** out_contract
);

/**
 * @brief Update contract
 * @param ctx Context
 * @param contract Updated contract
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_update(
    regislex_context_t* ctx,
    const regislex_contract_t* contract
);

/**
 * @brief Delete contract
 * @param ctx Context
 * @param id Contract ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List contracts
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output contract list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_list(
    regislex_context_t* ctx,
    const regislex_contract_filter_t* filter,
    regislex_contract_list_t** out_list
);

/**
 * @brief Get expiring contracts
 * @param ctx Context
 * @param days_ahead Days to look ahead
 * @param out_list Output contract list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_expiring(
    regislex_context_t* ctx,
    int days_ahead,
    regislex_contract_list_t** out_list
);

/**
 * @brief Add obligation to contract
 * @param ctx Context
 * @param contract_id Contract ID
 * @param obligation Obligation data
 * @param out_obligation Output created obligation
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_add_obligation(
    regislex_context_t* ctx,
    const regislex_uuid_t* contract_id,
    const regislex_obligation_t* obligation,
    regislex_obligation_t** out_obligation
);

/**
 * @brief Complete obligation
 * @param ctx Context
 * @param obligation_id Obligation ID
 * @param notes Completion notes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_obligation_complete(
    regislex_context_t* ctx,
    const regislex_uuid_t* obligation_id,
    const char* notes
);

/**
 * @brief Get upcoming obligations
 * @param ctx Context
 * @param days_ahead Days to look ahead
 * @param obligations Output obligation array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_obligation_upcoming(
    regislex_context_t* ctx,
    int days_ahead,
    regislex_obligation_t*** obligations,
    int* count
);

/**
 * @brief Renew contract
 * @param ctx Context
 * @param id Contract ID
 * @param new_expiration New expiration date
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_renew(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const regislex_datetime_t* new_expiration
);

/**
 * @brief Terminate contract
 * @param ctx Context
 * @param id Contract ID
 * @param termination_date Termination date
 * @param reason Termination reason
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_contract_terminate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const regislex_datetime_t* termination_date,
    const char* reason
);

/**
 * @brief Free contract
 * @param contract Contract to free
 */
REGISLEX_API void regislex_contract_free(regislex_contract_t* contract);

/**
 * @brief Free contract list
 * @param list List to free
 */
REGISLEX_API void regislex_contract_list_free(regislex_contract_list_t* list);

/* ============================================================================
 * Risk Functions
 * ============================================================================ */

/**
 * @brief Create risk
 * @param ctx Context
 * @param risk Risk data
 * @param out_risk Output created risk
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_create(
    regislex_context_t* ctx,
    const regislex_risk_t* risk,
    regislex_risk_t** out_risk
);

/**
 * @brief Get risk by ID
 * @param ctx Context
 * @param id Risk ID
 * @param out_risk Output risk
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_risk_t** out_risk
);

/**
 * @brief Update risk
 * @param ctx Context
 * @param risk Updated risk
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_update(
    regislex_context_t* ctx,
    const regislex_risk_t* risk
);

/**
 * @brief List risks
 * @param ctx Context
 * @param category Filter by category (NULL for all)
 * @param level Filter by level (NULL for all)
 * @param status Filter by status (NULL for all)
 * @param risks Output risk array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_list(
    regislex_context_t* ctx,
    const regislex_risk_category_t* category,
    const regislex_risk_level_t* level,
    const regislex_risk_status_t* status,
    regislex_risk_t*** risks,
    int* count
);

/**
 * @brief Add mitigation action to risk
 * @param ctx Context
 * @param risk_id Risk ID
 * @param action Action data
 * @param out_action Output created action
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_add_action(
    regislex_context_t* ctx,
    const regislex_uuid_t* risk_id,
    const regislex_risk_action_t* action,
    regislex_risk_action_t** out_action
);

/**
 * @brief Complete mitigation action
 * @param ctx Context
 * @param action_id Action ID
 * @param notes Completion notes
 * @param actual_cost Actual cost
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_complete_action(
    regislex_context_t* ctx,
    const regislex_uuid_t* action_id,
    const char* notes,
    regislex_money_t actual_cost
);

/**
 * @brief Assess risk
 * @param ctx Context
 * @param id Risk ID
 * @param likelihood Likelihood score (1-5)
 * @param impact Impact score (1-5)
 * @param potential_exposure Potential financial exposure
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_assess(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    int likelihood,
    int impact,
    regislex_money_t potential_exposure
);

/**
 * @brief Get risk heat map data
 * @param ctx Context
 * @param heatmap_json Output JSON heat map data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_risk_heatmap(
    regislex_context_t* ctx,
    char** heatmap_json
);

/**
 * @brief Free risk
 * @param risk Risk to free
 */
REGISLEX_API void regislex_risk_free(regislex_risk_t* risk);

/* ============================================================================
 * Spend Analytics Functions
 * ============================================================================ */

/**
 * @brief Get legal spend summary
 * @param ctx Context
 * @param start_date Period start
 * @param end_date Period end
 * @param out_summary Output spend summary
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_spend_summary(
    regislex_context_t* ctx,
    const regislex_datetime_t* start_date,
    const regislex_datetime_t* end_date,
    regislex_spend_summary_t** out_summary
);

/**
 * @brief Get spend by vendor
 * @param ctx Context
 * @param start_date Period start
 * @param end_date Period end
 * @param spend_json Output JSON spend breakdown
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_spend_by_vendor(
    regislex_context_t* ctx,
    const regislex_datetime_t* start_date,
    const regislex_datetime_t* end_date,
    char** spend_json
);

/**
 * @brief Get spend by matter
 * @param ctx Context
 * @param start_date Period start
 * @param end_date Period end
 * @param spend_json Output JSON spend breakdown
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_spend_by_matter(
    regislex_context_t* ctx,
    const regislex_datetime_t* start_date,
    const regislex_datetime_t* end_date,
    char** spend_json
);

/**
 * @brief Get spend trend
 * @param ctx Context
 * @param months Number of months
 * @param trend_json Output JSON trend data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_spend_trend(
    regislex_context_t* ctx,
    int months,
    char** trend_json
);

/**
 * @brief Free spend summary
 * @param summary Summary to free
 */
REGISLEX_API void regislex_spend_summary_free(regislex_spend_summary_t* summary);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_ELM_H */
