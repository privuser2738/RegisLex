/**
 * @file workflow.h
 * @brief Workflow Automation Module
 *
 * Provides functionality for automating repetitive tasks,
 * document generation, intake processes, and status updates.
 */

#ifndef REGISLEX_WORKFLOW_H
#define REGISLEX_WORKFLOW_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Workflow Types and Enums
 * ============================================================================ */

/**
 * @brief Workflow status
 */
typedef enum {
    REGISLEX_WORKFLOW_DRAFT = 0,
    REGISLEX_WORKFLOW_ACTIVE,
    REGISLEX_WORKFLOW_PAUSED,
    REGISLEX_WORKFLOW_COMPLETED,
    REGISLEX_WORKFLOW_FAILED,
    REGISLEX_WORKFLOW_CANCELLED
} regislex_workflow_status_t;

/**
 * @brief Trigger type for workflow execution
 */
typedef enum {
    REGISLEX_TRIGGER_MANUAL = 0,
    REGISLEX_TRIGGER_SCHEDULED,
    REGISLEX_TRIGGER_EVENT,
    REGISLEX_TRIGGER_CONDITION,
    REGISLEX_TRIGGER_WEBHOOK,
    REGISLEX_TRIGGER_API
} regislex_trigger_type_t;

/**
 * @brief Event types that can trigger workflows
 */
typedef enum {
    REGISLEX_EVENT_CASE_CREATED = 0,
    REGISLEX_EVENT_CASE_UPDATED,
    REGISLEX_EVENT_CASE_STATUS_CHANGED,
    REGISLEX_EVENT_CASE_ASSIGNED,
    REGISLEX_EVENT_DEADLINE_APPROACHING,
    REGISLEX_EVENT_DEADLINE_PASSED,
    REGISLEX_EVENT_DOCUMENT_UPLOADED,
    REGISLEX_EVENT_DOCUMENT_SIGNED,
    REGISLEX_EVENT_PARTY_ADDED,
    REGISLEX_EVENT_PAYMENT_RECEIVED,
    REGISLEX_EVENT_TASK_COMPLETED,
    REGISLEX_EVENT_CUSTOM
} regislex_event_type_t;

/**
 * @brief Action type in workflow
 */
typedef enum {
    REGISLEX_ACTION_SEND_EMAIL = 0,
    REGISLEX_ACTION_SEND_SMS,
    REGISLEX_ACTION_CREATE_TASK,
    REGISLEX_ACTION_CREATE_DEADLINE,
    REGISLEX_ACTION_CREATE_DOCUMENT,
    REGISLEX_ACTION_UPDATE_STATUS,
    REGISLEX_ACTION_ASSIGN_USER,
    REGISLEX_ACTION_ADD_NOTE,
    REGISLEX_ACTION_WEBHOOK,
    REGISLEX_ACTION_DELAY,
    REGISLEX_ACTION_CONDITION,
    REGISLEX_ACTION_APPROVAL,
    REGISLEX_ACTION_NOTIFY,
    REGISLEX_ACTION_GENERATE_REPORT,
    REGISLEX_ACTION_CUSTOM_SCRIPT
} regislex_action_type_t;

/**
 * @brief Task status
 */
typedef enum {
    REGISLEX_TASK_PENDING = 0,
    REGISLEX_TASK_IN_PROGRESS,
    REGISLEX_TASK_WAITING_APPROVAL,
    REGISLEX_TASK_APPROVED,
    REGISLEX_TASK_REJECTED,
    REGISLEX_TASK_COMPLETED,
    REGISLEX_TASK_CANCELLED,
    REGISLEX_TASK_FAILED
} regislex_task_status_t;

/**
 * @brief Condition operator
 */
typedef enum {
    REGISLEX_COND_EQUALS = 0,
    REGISLEX_COND_NOT_EQUALS,
    REGISLEX_COND_GREATER_THAN,
    REGISLEX_COND_LESS_THAN,
    REGISLEX_COND_GREATER_OR_EQUAL,
    REGISLEX_COND_LESS_OR_EQUAL,
    REGISLEX_COND_CONTAINS,
    REGISLEX_COND_NOT_CONTAINS,
    REGISLEX_COND_STARTS_WITH,
    REGISLEX_COND_ENDS_WITH,
    REGISLEX_COND_IS_EMPTY,
    REGISLEX_COND_IS_NOT_EMPTY,
    REGISLEX_COND_IN_LIST,
    REGISLEX_COND_NOT_IN_LIST,
    REGISLEX_COND_MATCHES_REGEX
} regislex_condition_op_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Condition for workflow branching
 */
typedef struct {
    char field[256];
    regislex_condition_op_t operator;
    char value[1024];
    char logical_op[8];  /* "AND", "OR" */
} regislex_condition_t;

/**
 * @brief Workflow trigger definition
 */
struct regislex_trigger {
    regislex_uuid_t id;
    regislex_uuid_t workflow_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    regislex_trigger_type_t type;
    regislex_event_type_t event_type;
    char event_filter[1024];     /* JSON filter criteria */
    char schedule_cron[128];     /* Cron expression for scheduled triggers */
    char webhook_secret[256];
    int condition_count;
    regislex_condition_t* conditions;
    bool is_active;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
};

/**
 * @brief Action parameters (JSON-based for flexibility)
 */
typedef struct {
    char type[64];
    char value[4096];
} regislex_action_param_t;

/**
 * @brief Workflow action/step
 */
typedef struct regislex_action {
    regislex_uuid_t id;
    regislex_uuid_t workflow_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_action_type_t type;
    int sequence_order;

    /* Action parameters */
    int param_count;
    regislex_action_param_t* params;

    /* Branching */
    int condition_count;
    regislex_condition_t* conditions;
    regislex_uuid_t on_success_action_id;
    regislex_uuid_t on_failure_action_id;

    /* Timing */
    int delay_minutes;
    int timeout_minutes;
    int retry_count;
    int retry_delay_minutes;

    /* Status */
    bool is_active;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_action_t;

/**
 * @brief Workflow definition
 */
struct regislex_workflow {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char category[128];
    regislex_workflow_status_t status;
    int version;

    /* Triggers */
    int trigger_count;
    regislex_trigger_t** triggers;

    /* Actions */
    int action_count;
    regislex_action_t** actions;

    /* Settings */
    bool run_once;
    bool allow_parallel;
    int max_parallel_runs;
    int timeout_minutes;

    /* Scope */
    regislex_uuid_t case_type_filter;
    char applicable_jurisdictions[1024];

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
    regislex_uuid_t updated_by;
};

/**
 * @brief Workflow execution instance
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t workflow_id;
    regislex_uuid_t case_id;
    regislex_uuid_t triggered_by;
    char trigger_data[8192];     /* JSON trigger context */
    regislex_workflow_status_t status;
    regislex_uuid_t current_action_id;
    int current_step;
    regislex_datetime_t started_at;
    regislex_datetime_t completed_at;
    char error_message[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char execution_log[16384];
    regislex_datetime_t created_at;
} regislex_workflow_run_t;

/**
 * @brief Task in workflow or standalone
 */
struct regislex_task {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    regislex_uuid_t workflow_run_id;
    regislex_uuid_t parent_task_id;
    char title[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_task_status_t status;
    regislex_priority_t priority;

    /* Assignment */
    regislex_uuid_t assigned_to_id;
    regislex_uuid_t assigned_by;
    char assigned_role[64];

    /* Timing */
    regislex_datetime_t due_date;
    int estimated_minutes;
    int actual_minutes;

    /* Progress */
    int percent_complete;
    char completion_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];

    /* Approval */
    bool requires_approval;
    regislex_uuid_t approver_id;
    regislex_datetime_t approved_at;
    char approval_notes[REGISLEX_MAX_DESCRIPTION_LENGTH];

    /* Dependencies */
    char depends_on[1024];       /* Comma-separated task IDs */
    bool blocks_workflow;

    /* Audit */
    regislex_datetime_t started_at;
    regislex_datetime_t completed_at;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Workflow template for common processes
 */
typedef struct {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char category[128];
    char template_data[32768];   /* JSON workflow definition */
    bool is_system;
    bool is_active;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
} regislex_workflow_template_t;

/**
 * @brief Task filter criteria
 */
typedef struct {
    regislex_uuid_t* case_id;
    regislex_uuid_t* assigned_to_id;
    regislex_task_status_t* status;
    regislex_priority_t* priority;
    regislex_datetime_t* due_before;
    bool include_completed;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_task_filter_t;

/**
 * @brief Task list result
 */
typedef struct {
    regislex_task_t** tasks;
    int count;
    int total_count;
} regislex_task_list_t;

/* ============================================================================
 * Workflow Management Functions
 * ============================================================================ */

/**
 * @brief Create a new workflow
 * @param ctx Context
 * @param workflow Workflow data
 * @param out_workflow Output created workflow
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_create(
    regislex_context_t* ctx,
    const regislex_workflow_t* workflow,
    regislex_workflow_t** out_workflow
);

/**
 * @brief Get workflow by ID
 * @param ctx Context
 * @param id Workflow ID
 * @param out_workflow Output workflow
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_workflow_t** out_workflow
);

/**
 * @brief Update workflow
 * @param ctx Context
 * @param workflow Updated workflow data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_update(
    regislex_context_t* ctx,
    const regislex_workflow_t* workflow
);

/**
 * @brief Delete workflow
 * @param ctx Context
 * @param id Workflow ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief List all workflows
 * @param ctx Context
 * @param category Filter by category (NULL for all)
 * @param workflows Output workflow array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_list(
    regislex_context_t* ctx,
    const char* category,
    regislex_workflow_t*** workflows,
    int* count
);

/**
 * @brief Activate workflow
 * @param ctx Context
 * @param id Workflow ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_activate(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Pause workflow
 * @param ctx Context
 * @param id Workflow ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_pause(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Execute workflow manually
 * @param ctx Context
 * @param workflow_id Workflow ID
 * @param case_id Case ID (optional)
 * @param trigger_data JSON trigger context
 * @param out_run Output workflow run
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_execute(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_uuid_t* case_id,
    const char* trigger_data,
    regislex_workflow_run_t** out_run
);

/**
 * @brief Get workflow run status
 * @param ctx Context
 * @param run_id Run ID
 * @param out_run Output workflow run
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_run_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* run_id,
    regislex_workflow_run_t** out_run
);

/**
 * @brief Cancel workflow run
 * @param ctx Context
 * @param run_id Run ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_run_cancel(
    regislex_context_t* ctx,
    const regislex_uuid_t* run_id
);

/**
 * @brief Free workflow structure
 * @param workflow Workflow to free
 */
REGISLEX_API void regislex_workflow_free(regislex_workflow_t* workflow);

/**
 * @brief Free workflow run structure
 * @param run Run to free
 */
REGISLEX_API void regislex_workflow_run_free(regislex_workflow_run_t* run);

/* ============================================================================
 * Trigger Functions
 * ============================================================================ */

/**
 * @brief Add trigger to workflow
 * @param ctx Context
 * @param workflow_id Workflow ID
 * @param trigger Trigger data
 * @param out_trigger Output created trigger
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_trigger_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_trigger_t* trigger,
    regislex_trigger_t** out_trigger
);

/**
 * @brief Update trigger
 * @param ctx Context
 * @param trigger Updated trigger data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_trigger_update(
    regislex_context_t* ctx,
    const regislex_trigger_t* trigger
);

/**
 * @brief Remove trigger
 * @param ctx Context
 * @param id Trigger ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_trigger_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Free trigger structure
 * @param trigger Trigger to free
 */
REGISLEX_API void regislex_trigger_free(regislex_trigger_t* trigger);

/* ============================================================================
 * Action Functions
 * ============================================================================ */

/**
 * @brief Add action to workflow
 * @param ctx Context
 * @param workflow_id Workflow ID
 * @param action Action data
 * @param out_action Output created action
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_action_add(
    regislex_context_t* ctx,
    const regislex_uuid_t* workflow_id,
    const regislex_action_t* action,
    regislex_action_t** out_action
);

/**
 * @brief Update action
 * @param ctx Context
 * @param action Updated action data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_action_update(
    regislex_context_t* ctx,
    const regislex_action_t* action
);

/**
 * @brief Remove action
 * @param ctx Context
 * @param id Action ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_action_remove(
    regislex_context_t* ctx,
    const regislex_uuid_t* id
);

/**
 * @brief Free action structure
 * @param action Action to free
 */
REGISLEX_API void regislex_action_free(regislex_action_t* action);

/* ============================================================================
 * Task Functions
 * ============================================================================ */

/**
 * @brief Create task
 * @param ctx Context
 * @param task Task data
 * @param out_task Output created task
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_create(
    regislex_context_t* ctx,
    const regislex_task_t* task,
    regislex_task_t** out_task
);

/**
 * @brief Get task by ID
 * @param ctx Context
 * @param id Task ID
 * @param out_task Output task
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_task_t** out_task
);

/**
 * @brief Update task
 * @param ctx Context
 * @param task Updated task data
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_update(
    regislex_context_t* ctx,
    const regislex_task_t* task
);

/**
 * @brief List tasks with filtering
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output task list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_list(
    regislex_context_t* ctx,
    const regislex_task_filter_t* filter,
    regislex_task_list_t** out_list
);

/**
 * @brief Assign task to user
 * @param ctx Context
 * @param task_id Task ID
 * @param user_id User ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_assign(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const regislex_uuid_t* user_id
);

/**
 * @brief Start working on task
 * @param ctx Context
 * @param task_id Task ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_start(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id
);

/**
 * @brief Complete task
 * @param ctx Context
 * @param task_id Task ID
 * @param notes Completion notes
 * @param actual_minutes Actual time spent
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_complete(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const char* notes,
    int actual_minutes
);

/**
 * @brief Request task approval
 * @param ctx Context
 * @param task_id Task ID
 * @param approver_id Approver user ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_request_approval(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const regislex_uuid_t* approver_id
);

/**
 * @brief Approve task
 * @param ctx Context
 * @param task_id Task ID
 * @param notes Approval notes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_approve(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const char* notes
);

/**
 * @brief Reject task
 * @param ctx Context
 * @param task_id Task ID
 * @param reason Rejection reason
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_task_reject(
    regislex_context_t* ctx,
    const regislex_uuid_t* task_id,
    const char* reason
);

/**
 * @brief Free task structure
 * @param task Task to free
 */
REGISLEX_API void regislex_task_free(regislex_task_t* task);

/**
 * @brief Free task list
 * @param list List to free
 */
REGISLEX_API void regislex_task_list_free(regislex_task_list_t* list);

/* ============================================================================
 * Template Functions
 * ============================================================================ */

/**
 * @brief Create workflow from template
 * @param ctx Context
 * @param template_id Template ID
 * @param name New workflow name
 * @param out_workflow Output created workflow
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_from_template(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    const char* name,
    regislex_workflow_t** out_workflow
);

/**
 * @brief List workflow templates
 * @param ctx Context
 * @param category Filter by category (NULL for all)
 * @param templates Output template array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_workflow_template_list(
    regislex_context_t* ctx,
    const char* category,
    regislex_workflow_template_t*** templates,
    int* count
);

/**
 * @brief Free workflow template
 * @param template_ptr Template to free
 */
REGISLEX_API void regislex_workflow_template_free(regislex_workflow_template_t* template_ptr);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_WORKFLOW_H */
