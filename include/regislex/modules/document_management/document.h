/**
 * @file document.h
 * @brief Document Management Module
 *
 * Provides functionality for securely storing, organizing, and retrieving
 * legal documents, with version control and template automation.
 */

#ifndef REGISLEX_DOCUMENT_H
#define REGISLEX_DOCUMENT_H

#include "../../regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Document Types and Enums
 * ============================================================================ */

/**
 * @brief Document type classification
 */
typedef enum {
    REGISLEX_DOC_PLEADING = 0,
    REGISLEX_DOC_MOTION,
    REGISLEX_DOC_BRIEF,
    REGISLEX_DOC_ORDER,
    REGISLEX_DOC_JUDGMENT,
    REGISLEX_DOC_CONTRACT,
    REGISLEX_DOC_AGREEMENT,
    REGISLEX_DOC_CORRESPONDENCE,
    REGISLEX_DOC_MEMO,
    REGISLEX_DOC_EVIDENCE,
    REGISLEX_DOC_EXHIBIT,
    REGISLEX_DOC_TRANSCRIPT,
    REGISLEX_DOC_DISCOVERY,
    REGISLEX_DOC_SUBPOENA,
    REGISLEX_DOC_AFFIDAVIT,
    REGISLEX_DOC_DECLARATION,
    REGISLEX_DOC_NOTICE,
    REGISLEX_DOC_REPORT,
    REGISLEX_DOC_FORM,
    REGISLEX_DOC_TEMPLATE,
    REGISLEX_DOC_OTHER
} regislex_doc_type_t;

/**
 * @brief Document status
 */
typedef enum {
    REGISLEX_DOC_STATUS_DRAFT = 0,
    REGISLEX_DOC_STATUS_REVIEW,
    REGISLEX_DOC_STATUS_APPROVED,
    REGISLEX_DOC_STATUS_FINAL,
    REGISLEX_DOC_STATUS_FILED,
    REGISLEX_DOC_STATUS_SERVED,
    REGISLEX_DOC_STATUS_EXECUTED,
    REGISLEX_DOC_STATUS_ARCHIVED,
    REGISLEX_DOC_STATUS_SUPERSEDED
} regislex_doc_status_t;

/**
 * @brief Document access level
 */
typedef enum {
    REGISLEX_ACCESS_PUBLIC = 0,
    REGISLEX_ACCESS_INTERNAL,
    REGISLEX_ACCESS_CONFIDENTIAL,
    REGISLEX_ACCESS_PRIVILEGED,
    REGISLEX_ACCESS_RESTRICTED
} regislex_access_level_t;

/**
 * @brief Storage backend type
 */
typedef enum {
    REGISLEX_STORAGE_FILESYSTEM = 0,
    REGISLEX_STORAGE_S3,
    REGISLEX_STORAGE_AZURE,
    REGISLEX_STORAGE_GCS,
    REGISLEX_STORAGE_DATABASE
} regislex_storage_type_t;

/**
 * @brief Template field type
 */
typedef enum {
    REGISLEX_FIELD_TEXT = 0,
    REGISLEX_FIELD_NUMBER,
    REGISLEX_FIELD_DATE,
    REGISLEX_FIELD_CURRENCY,
    REGISLEX_FIELD_CHECKBOX,
    REGISLEX_FIELD_DROPDOWN,
    REGISLEX_FIELD_SIGNATURE,
    REGISLEX_FIELD_IMAGE,
    REGISLEX_FIELD_TABLE,
    REGISLEX_FIELD_CALCULATED
} regislex_field_type_t;

/**
 * @brief Signature status
 */
typedef enum {
    REGISLEX_SIG_PENDING = 0,
    REGISLEX_SIG_SIGNED,
    REGISLEX_SIG_DECLINED,
    REGISLEX_SIG_EXPIRED
} regislex_signature_status_t;

/* ============================================================================
 * Structures
 * ============================================================================ */

/**
 * @brief Document version
 */
struct regislex_doc_version {
    regislex_uuid_t id;
    regislex_uuid_t document_id;
    int version_number;
    char version_label[64];
    char change_summary[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char storage_path[REGISLEX_MAX_PATH_LENGTH];
    char checksum[128];
    size_t file_size;
    char mime_type[128];
    regislex_uuid_t created_by;
    regislex_datetime_t created_at;
    bool is_current;
};

/**
 * @brief Document metadata
 */
typedef struct {
    char title[REGISLEX_MAX_NAME_LENGTH];
    char author[256];
    char subject[512];
    char keywords[1024];
    char comments[REGISLEX_MAX_DESCRIPTION_LENGTH];
    int page_count;
    int word_count;
    char language[16];
    regislex_datetime_t doc_created;
    regislex_datetime_t doc_modified;
} regislex_doc_metadata_t;

/**
 * @brief Document
 */
struct regislex_document {
    regislex_uuid_t id;
    regislex_uuid_t case_id;
    regislex_uuid_t matter_id;
    regislex_uuid_t folder_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char display_name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_doc_type_t type;
    regislex_doc_status_t status;
    regislex_access_level_t access_level;

    /* Current version info */
    int current_version;
    char file_name[REGISLEX_MAX_NAME_LENGTH];
    char mime_type[128];
    size_t file_size;
    char storage_path[REGISLEX_MAX_PATH_LENGTH];
    char checksum[128];

    /* Metadata */
    regislex_doc_metadata_t metadata;

    /* Classification */
    char tags[1024];
    char categories[512];
    char bates_number[64];
    char exhibit_number[64];

    /* Legal specific */
    regislex_datetime_t filed_date;
    regislex_datetime_t served_date;
    regislex_datetime_t execution_date;
    char court_file_stamp[256];

    /* Retention */
    regislex_datetime_t retention_date;
    bool hold_for_litigation;
    char hold_reason[512];

    /* Version history */
    int version_count;
    regislex_doc_version_t** versions;

    /* Lock */
    bool is_locked;
    regislex_uuid_t locked_by;
    regislex_datetime_t locked_at;
    regislex_datetime_t lock_expires;

    /* Encryption */
    bool is_encrypted;
    char encryption_key_id[128];

    /* Full-text search */
    char extracted_text[32768];
    bool ocr_processed;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
    regislex_uuid_t updated_by;
};

/**
 * @brief Document folder
 */
typedef struct regislex_folder {
    regislex_uuid_t id;
    regislex_uuid_t parent_id;
    regislex_uuid_t case_id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char path[REGISLEX_MAX_PATH_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    regislex_access_level_t access_level;
    int document_count;
    int subfolder_count;
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
} regislex_folder_t;

/**
 * @brief Template field definition
 */
typedef struct {
    char name[128];
    char label[256];
    regislex_field_type_t type;
    char default_value[1024];
    char data_source[256];      /* For auto-population (e.g., "case.title") */
    char validation_regex[512];
    char format_string[128];
    char options[2048];         /* JSON array for dropdowns */
    bool required;
    bool readonly;
    int sequence;
} regislex_template_field_t;

/**
 * @brief Document template
 */
struct regislex_doc_template {
    regislex_uuid_t id;
    char name[REGISLEX_MAX_NAME_LENGTH];
    char description[REGISLEX_MAX_DESCRIPTION_LENGTH];
    char category[128];
    regislex_doc_type_t document_type;
    char jurisdiction[128];

    /* Template file */
    char template_path[REGISLEX_MAX_PATH_LENGTH];
    char template_format[32];   /* "docx", "pdf", "html" */
    char mime_type[128];

    /* Fields */
    int field_count;
    regislex_template_field_t* fields;

    /* Output settings */
    char output_name_pattern[256];
    char output_format[32];

    /* Permissions */
    char allowed_roles[512];
    char allowed_case_types[512];

    /* Status */
    bool is_active;
    bool is_system;
    int version;

    /* Audit */
    regislex_datetime_t created_at;
    regislex_datetime_t updated_at;
    regislex_uuid_t created_by;
};

/**
 * @brief Signature request
 */
typedef struct {
    regislex_uuid_t id;
    regislex_uuid_t document_id;
    char signer_name[REGISLEX_MAX_NAME_LENGTH];
    char signer_email[256];
    char signer_role[64];
    int signing_order;
    regislex_signature_status_t status;
    regislex_datetime_t requested_at;
    regislex_datetime_t signed_at;
    regislex_datetime_t expires_at;
    char signature_data[4096];  /* Base64 encoded signature */
    char ip_address[64];
    char user_agent[512];
    char decline_reason[512];
    regislex_uuid_t requested_by;
} regislex_signature_request_t;

/**
 * @brief Document filter criteria
 */
typedef struct {
    regislex_uuid_t* case_id;
    regislex_uuid_t* matter_id;
    regislex_uuid_t* folder_id;
    regislex_doc_type_t* type;
    regislex_doc_status_t* status;
    regislex_access_level_t* access_level;
    const char* name_contains;
    const char* full_text_search;
    const char* tags_contain;
    const char* mime_type;
    regislex_datetime_t* created_after;
    regislex_datetime_t* created_before;
    regislex_uuid_t* created_by;
    int offset;
    int limit;
    const char* order_by;
    bool order_desc;
} regislex_doc_filter_t;

/**
 * @brief Document list result
 */
typedef struct {
    regislex_document_t** documents;
    int count;
    int total_count;
} regislex_doc_list_t;

/* ============================================================================
 * Document Functions
 * ============================================================================ */

/**
 * @brief Upload/create a new document
 * @param ctx Context
 * @param document Document metadata
 * @param file_path Path to file to upload
 * @param out_document Output created document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_create(
    regislex_context_t* ctx,
    const regislex_document_t* document,
    const char* file_path,
    regislex_document_t** out_document
);

/**
 * @brief Upload document from buffer
 * @param ctx Context
 * @param document Document metadata
 * @param data File data buffer
 * @param size Data size
 * @param out_document Output created document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_create_from_buffer(
    regislex_context_t* ctx,
    const regislex_document_t* document,
    const void* data,
    size_t size,
    regislex_document_t** out_document
);

/**
 * @brief Get document by ID
 * @param ctx Context
 * @param id Document ID
 * @param out_document Output document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_document_t** out_document
);

/**
 * @brief Update document metadata
 * @param ctx Context
 * @param document Updated document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_update(
    regislex_context_t* ctx,
    const regislex_document_t* document
);

/**
 * @brief Delete document
 * @param ctx Context
 * @param id Document ID
 * @param permanent Permanently delete (skip trash)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    bool permanent
);

/**
 * @brief List documents with filtering
 * @param ctx Context
 * @param filter Filter criteria
 * @param out_list Output document list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_list(
    regislex_context_t* ctx,
    const regislex_doc_filter_t* filter,
    regislex_doc_list_t** out_list
);

/**
 * @brief Download document to file
 * @param ctx Context
 * @param id Document ID
 * @param version_number Version number (0 for current)
 * @param destination_path Path to save file
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_download(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    int version_number,
    const char* destination_path
);

/**
 * @brief Get document content as buffer
 * @param ctx Context
 * @param id Document ID
 * @param version_number Version number (0 for current)
 * @param buffer Output buffer (caller must free)
 * @param size Output size
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_get_content(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    int version_number,
    void** buffer,
    size_t* size
);

/**
 * @brief Move document to folder
 * @param ctx Context
 * @param document_id Document ID
 * @param folder_id Destination folder ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_move(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const regislex_uuid_t* folder_id
);

/**
 * @brief Copy document
 * @param ctx Context
 * @param id Document ID
 * @param destination_case_id Destination case ID
 * @param out_document Output copied document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_copy(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    const regislex_uuid_t* destination_case_id,
    regislex_document_t** out_document
);

/**
 * @brief Free document structure
 * @param document Document to free
 */
REGISLEX_API void regislex_document_free(regislex_document_t* document);

/**
 * @brief Free document list
 * @param list List to free
 */
REGISLEX_API void regislex_document_list_free(regislex_doc_list_t* list);

/* ============================================================================
 * Version Control Functions
 * ============================================================================ */

/**
 * @brief Upload new version of document
 * @param ctx Context
 * @param document_id Document ID
 * @param file_path Path to new version file
 * @param change_summary Description of changes
 * @param out_version Output created version
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_new_version(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const char* file_path,
    const char* change_summary,
    regislex_doc_version_t** out_version
);

/**
 * @brief Get version history
 * @param ctx Context
 * @param document_id Document ID
 * @param versions Output version array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_versions(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    regislex_doc_version_t*** versions,
    int* count
);

/**
 * @brief Restore previous version
 * @param ctx Context
 * @param document_id Document ID
 * @param version_number Version to restore
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_restore_version(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    int version_number
);

/**
 * @brief Compare two versions
 * @param ctx Context
 * @param document_id Document ID
 * @param version1 First version number
 * @param version2 Second version number
 * @param diff_output Output diff (caller must free)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_compare_versions(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    int version1,
    int version2,
    char** diff_output
);

/**
 * @brief Free version structure
 * @param version Version to free
 */
REGISLEX_API void regislex_doc_version_free(regislex_doc_version_t* version);

/* ============================================================================
 * Lock/Checkout Functions
 * ============================================================================ */

/**
 * @brief Lock document for editing
 * @param ctx Context
 * @param document_id Document ID
 * @param duration_minutes Lock duration
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_lock(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    int duration_minutes
);

/**
 * @brief Unlock document
 * @param ctx Context
 * @param document_id Document ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_unlock(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id
);

/**
 * @brief Check out document (lock + download)
 * @param ctx Context
 * @param document_id Document ID
 * @param destination_path Path to save file
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_checkout(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const char* destination_path
);

/**
 * @brief Check in document (upload + unlock)
 * @param ctx Context
 * @param document_id Document ID
 * @param file_path Path to updated file
 * @param change_summary Description of changes
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_checkin(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const char* file_path,
    const char* change_summary
);

/* ============================================================================
 * Folder Functions
 * ============================================================================ */

/**
 * @brief Create folder
 * @param ctx Context
 * @param folder Folder data
 * @param out_folder Output created folder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_folder_create(
    regislex_context_t* ctx,
    const regislex_folder_t* folder,
    regislex_folder_t** out_folder
);

/**
 * @brief Get folder by ID
 * @param ctx Context
 * @param id Folder ID
 * @param out_folder Output folder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_folder_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_folder_t** out_folder
);

/**
 * @brief List subfolders
 * @param ctx Context
 * @param parent_id Parent folder ID (NULL for root)
 * @param case_id Case ID
 * @param folders Output folder array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_folder_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* parent_id,
    const regislex_uuid_t* case_id,
    regislex_folder_t*** folders,
    int* count
);

/**
 * @brief Update folder
 * @param ctx Context
 * @param folder Updated folder
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_folder_update(
    regislex_context_t* ctx,
    const regislex_folder_t* folder
);

/**
 * @brief Delete folder
 * @param ctx Context
 * @param id Folder ID
 * @param recursive Delete contents recursively
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_folder_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    bool recursive
);

/**
 * @brief Free folder structure
 * @param folder Folder to free
 */
REGISLEX_API void regislex_folder_free(regislex_folder_t* folder);

/* ============================================================================
 * Template Functions
 * ============================================================================ */

/**
 * @brief Create document template
 * @param ctx Context
 * @param template_data Template data
 * @param template_file_path Path to template file
 * @param out_template Output created template
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_doc_template_create(
    regislex_context_t* ctx,
    const regislex_doc_template_t* template_data,
    const char* template_file_path,
    regislex_doc_template_t** out_template
);

/**
 * @brief Get document template
 * @param ctx Context
 * @param id Template ID
 * @param out_template Output template
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_doc_template_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_doc_template_t** out_template
);

/**
 * @brief List document templates
 * @param ctx Context
 * @param category Filter by category (NULL for all)
 * @param doc_type Filter by document type (NULL for all)
 * @param templates Output template array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_doc_template_list(
    regislex_context_t* ctx,
    const char* category,
    const regislex_doc_type_t* doc_type,
    regislex_doc_template_t*** templates,
    int* count
);

/**
 * @brief Generate document from template
 * @param ctx Context
 * @param template_id Template ID
 * @param case_id Case ID for auto-population
 * @param field_values JSON object of field values
 * @param out_document Output generated document
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_doc_template_generate(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    const regislex_uuid_t* case_id,
    const char* field_values,
    regislex_document_t** out_document
);

/**
 * @brief Preview document from template
 * @param ctx Context
 * @param template_id Template ID
 * @param case_id Case ID for auto-population
 * @param field_values JSON object of field values
 * @param buffer Output preview buffer (caller must free)
 * @param size Output size
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_doc_template_preview(
    regislex_context_t* ctx,
    const regislex_uuid_t* template_id,
    const regislex_uuid_t* case_id,
    const char* field_values,
    void** buffer,
    size_t* size
);

/**
 * @brief Free document template
 * @param template_ptr Template to free
 */
REGISLEX_API void regislex_doc_template_free(regislex_doc_template_t* template_ptr);

/* ============================================================================
 * Search Functions
 * ============================================================================ */

/**
 * @brief Full-text search documents
 * @param ctx Context
 * @param query Search query
 * @param case_id Limit to case (NULL for all)
 * @param out_list Output document list
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_search(
    regislex_context_t* ctx,
    const char* query,
    const regislex_uuid_t* case_id,
    regislex_doc_list_t** out_list
);

/**
 * @brief Index document for search
 * @param ctx Context
 * @param document_id Document ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_index(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id
);

/**
 * @brief Extract text from document (OCR if needed)
 * @param ctx Context
 * @param document_id Document ID
 * @param out_text Output extracted text (caller must free)
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_document_extract_text(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    char** out_text
);

/* ============================================================================
 * Signature Functions
 * ============================================================================ */

/**
 * @brief Request signature on document
 * @param ctx Context
 * @param document_id Document ID
 * @param request Signature request data
 * @param out_request Output created request
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_signature_request(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const regislex_signature_request_t* request,
    regislex_signature_request_t** out_request
);

/**
 * @brief Get signature requests for document
 * @param ctx Context
 * @param document_id Document ID
 * @param requests Output request array
 * @param count Output count
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_signature_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    regislex_signature_request_t*** requests,
    int* count
);

/**
 * @brief Apply signature to document
 * @param ctx Context
 * @param request_id Signature request ID
 * @param signature_data Base64 encoded signature
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_signature_apply(
    regislex_context_t* ctx,
    const regislex_uuid_t* request_id,
    const char* signature_data
);

/**
 * @brief Cancel signature request
 * @param ctx Context
 * @param request_id Signature request ID
 * @return Error code
 */
REGISLEX_API regislex_error_t regislex_signature_cancel(
    regislex_context_t* ctx,
    const regislex_uuid_t* request_id
);

/**
 * @brief Free signature request
 * @param request Request to free
 */
REGISLEX_API void regislex_signature_request_free(regislex_signature_request_t* request);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_DOCUMENT_H */
