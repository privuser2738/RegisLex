/**
 * RegisLex - Enterprise Legal Software Suite
 * Document Management Implementation
 *
 * Provides document storage, version control, templates, check-in/check-out,
 * full-text search, and digital signature support.
 */

#include "regislex/modules/document_management/document.h"
#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Helper Functions
 * ========================================================================== */

static const char* get_mime_type(const char* filename) {
    if (!filename) return "application/octet-stream";

    const char* ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";

    ext++; /* Skip the dot */

    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "doc") == 0) return "application/msword";
    if (strcasecmp(ext, "docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcasecmp(ext, "xls") == 0) return "application/vnd.ms-excel";
    if (strcasecmp(ext, "xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcasecmp(ext, "ppt") == 0) return "application/vnd.ms-powerpoint";
    if (strcasecmp(ext, "pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "rtf") == 0) return "application/rtf";
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "tiff") == 0 || strcasecmp(ext, "tif") == 0) return "image/tiff";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";

    return "application/octet-stream";
}

static char* compute_file_hash(const char* file_path) {
    /* Simplified hash computation - in production, use SHA-256 */
    FILE* f = fopen(file_path, "rb");
    if (!f) return NULL;

    unsigned long hash = 5381;
    int c;
    while ((c = fgetc(f)) != EOF) {
        hash = ((hash << 5) + hash) + c;
    }
    fclose(f);

    char* result = (char*)malloc(32);
    if (result) {
        snprintf(result, 32, "%016lx", hash);
    }
    return result;
}

static regislex_error_t ensure_storage_path(regislex_context_t* ctx, char* path, size_t path_size) {
    /* Get base storage path from config or use default */
    const char* base_path = regislex_config_get(ctx, "storage.base_path");
    if (!base_path || base_path[0] == '\0') {
        base_path = "./documents";
    }

    /* Create directory if it doesn't exist */
    platform_mkdir(base_path, 0755);

    strncpy(path, base_path, path_size - 1);
    path[path_size - 1] = '\0';

    return REGISLEX_SUCCESS;
}

static regislex_error_t get_document_storage_path(
    regislex_context_t* ctx,
    const regislex_uuid_t* doc_id,
    int version,
    char* path,
    size_t path_size)
{
    char base_path[512];
    ensure_storage_path(ctx, base_path, sizeof(base_path));

    /* Use first 2 chars of UUID for sharding */
    char shard[3] = {doc_id->value[0], doc_id->value[1], '\0'};

    snprintf(path, path_size, "%s/%s/%s/v%d",
             base_path, shard, doc_id->value, version);

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Folder Management
 * ========================================================================== */

regislex_error_t regislex_folder_create(
    regislex_context_t* ctx,
    regislex_folder_t* folder_data,
    regislex_folder_t** out_folder)
{
    if (!ctx || !folder_data || !out_folder) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_folder = NULL;

    regislex_uuid_t id;
    if (folder_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = folder_data->id;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    /* Build full path */
    char full_path[1024] = {0};
    if (folder_data->parent_id.value[0] != '\0') {
        /* Get parent path */
        const char* parent_sql = "SELECT full_path FROM folders WHERE id = ?";
        regislex_db_stmt_t* pstmt = NULL;
        if (regislex_db_prepare(ctx, parent_sql, &pstmt) == REGISLEX_SUCCESS) {
            regislex_db_bind_text(pstmt, 1, folder_data->parent_id.value);
            if (regislex_db_step(pstmt) == REGISLEX_ROW) {
                const char* parent_path = regislex_db_column_text(pstmt, 0);
                if (parent_path) {
                    snprintf(full_path, sizeof(full_path), "%s/%s", parent_path, folder_data->name);
                }
            }
            regislex_db_finalize(pstmt);
        }
    }
    if (full_path[0] == '\0') {
        snprintf(full_path, sizeof(full_path), "/%s", folder_data->name);
    }

    const char* sql =
        "INSERT INTO folders (id, name, parent_id, full_path, case_id, "
        "access_level, owner_id, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, folder_data->name);

    if (folder_data->parent_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 3, folder_data->parent_id.value);
    } else {
        regislex_db_bind_null(stmt, 3);
    }

    regislex_db_bind_text(stmt, 4, full_path);

    if (folder_data->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 5, folder_data->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 5);
    }

    regislex_db_bind_int(stmt, 6, folder_data->access_level);
    regislex_db_bind_text(stmt, 7, folder_data->owner_id.value);
    regislex_db_bind_text(stmt, 8, now_str);
    regislex_db_bind_text(stmt, 9, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_folder_get(ctx, &id, out_folder);
}

regislex_error_t regislex_folder_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_folder_t** out_folder)
{
    if (!ctx || !id || !out_folder) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_folder = NULL;

    const char* sql =
        "SELECT id, name, parent_id, full_path, case_id, access_level, owner_id, "
        "created_at, updated_at FROM folders WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_folder_t* folder = (regislex_folder_t*)calloc(1, sizeof(regislex_folder_t));
    if (!folder) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(folder->id.value, str, sizeof(folder->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(folder->name, str, sizeof(folder->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(folder->parent_id.value, str, sizeof(folder->parent_id.value) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(folder->full_path, str, sizeof(folder->full_path) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(folder->case_id.value, str, sizeof(folder->case_id.value) - 1);

    folder->access_level = (regislex_access_level_t)regislex_db_column_int(stmt, 5);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(folder->owner_id.value, str, sizeof(folder->owner_id.value) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &folder->created_at);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &folder->updated_at);

    regislex_db_finalize(stmt);

    *out_folder = folder;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_folder_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* parent_id,
    regislex_folder_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    if (parent_id && parent_id->value[0] != '\0') {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, parent_id, full_path, case_id, access_level "
                 "FROM folders WHERE parent_id = '%s' ORDER BY name", parent_id->value);
    } else {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, parent_id, full_path, case_id, access_level "
                 "FROM folders WHERE parent_id IS NULL ORDER BY name");
    }

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_folder_list_t* list = (regislex_folder_list_t*)calloc(1,
        sizeof(regislex_folder_list_t) + count * sizeof(regislex_folder_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->folders = (regislex_folder_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->folders[i].id.value, str, sizeof(list->folders[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->folders[i].name, str, sizeof(list->folders[i].name) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->folders[i].parent_id.value, str, sizeof(list->folders[i].parent_id.value) - 1);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->folders[i].full_path, str, sizeof(list->folders[i].full_path) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->folders[i].case_id.value, str, sizeof(list->folders[i].case_id.value) - 1);

        list->folders[i].access_level = (regislex_access_level_t)regislex_db_result_get_int(result, 5);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_folder_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    bool recursive)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    if (recursive) {
        /* Delete all documents in this folder */
        const char* doc_sql = "DELETE FROM documents WHERE folder_id = ?";
        regislex_db_stmt_t* doc_stmt = NULL;
        if (regislex_db_prepare(ctx, doc_sql, &doc_stmt) == REGISLEX_SUCCESS) {
            regislex_db_bind_text(doc_stmt, 1, id->value);
            regislex_db_step(doc_stmt);
            regislex_db_finalize(doc_stmt);
        }

        /* Recursively delete subfolders */
        regislex_folder_list_t* subfolders = NULL;
        if (regislex_folder_list(ctx, id, &subfolders) == REGISLEX_SUCCESS && subfolders) {
            for (size_t i = 0; i < subfolders->count; i++) {
                regislex_folder_delete(ctx, &subfolders->folders[i].id, true);
            }
            regislex_folder_list_free(subfolders);
        }
    }

    const char* sql = "DELETE FROM folders WHERE id = ?";
    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);
    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

void regislex_folder_free(regislex_folder_t* folder) {
    free(folder);
}

void regislex_folder_list_free(regislex_folder_list_t* list) {
    free(list);
}

/* ============================================================================
 * Document Management
 * ========================================================================== */

regislex_error_t regislex_document_create(
    regislex_context_t* ctx,
    regislex_document_t* doc_data,
    const char* source_file_path,
    regislex_document_t** out_document)
{
    if (!ctx || !doc_data || !source_file_path || !out_document) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_document = NULL;

    /* Generate document ID */
    regislex_uuid_t id;
    if (doc_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = doc_data->id;
    }

    /* Get file info */
    platform_file_info_t file_info;
    if (platform_file_stat(source_file_path, &file_info) != 0) {
        return REGISLEX_ERROR_IO;
    }

    /* Determine MIME type */
    const char* mime_type = get_mime_type(doc_data->filename);

    /* Compute hash */
    char* file_hash = compute_file_hash(source_file_path);

    /* Get timestamps */
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    /* Determine storage path */
    char storage_path[1024];
    get_document_storage_path(ctx, &id, 1, storage_path, sizeof(storage_path));

    /* Create directory structure */
    char dir_path[1024];
    snprintf(dir_path, sizeof(dir_path), "%s", storage_path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        platform_mkdir_recursive(dir_path, 0755);
    }

    /* Copy file to storage */
    if (platform_file_copy(source_file_path, storage_path) != 0) {
        free(file_hash);
        return REGISLEX_ERROR_IO;
    }

    /* Begin transaction */
    regislex_db_transaction_begin(ctx);

    /* Insert document record */
    const char* doc_sql =
        "INSERT INTO documents (id, filename, title, description, document_type, "
        "status, folder_id, case_id, mime_type, file_size, storage_path, "
        "current_version, access_level, owner_id, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, doc_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        free(file_hash);
        return err;
    }

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, doc_data->filename);
    regislex_db_bind_text(stmt, 3, doc_data->title[0] ? doc_data->title : doc_data->filename);
    regislex_db_bind_text(stmt, 4, doc_data->description);
    regislex_db_bind_int(stmt, 5, doc_data->document_type);
    regislex_db_bind_int(stmt, 6, REGISLEX_DOC_STATUS_ACTIVE);

    if (doc_data->folder_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 7, doc_data->folder_id.value);
    } else {
        regislex_db_bind_null(stmt, 7);
    }

    if (doc_data->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 8, doc_data->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 8);
    }

    regislex_db_bind_text(stmt, 9, mime_type);
    regislex_db_bind_int64(stmt, 10, file_info.size);
    regislex_db_bind_text(stmt, 11, storage_path);
    regislex_db_bind_int(stmt, 12, 1);
    regislex_db_bind_int(stmt, 13, doc_data->access_level);
    regislex_db_bind_text(stmt, 14, doc_data->owner_id.value);
    regislex_db_bind_text(stmt, 15, now_str);
    regislex_db_bind_text(stmt, 16, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        free(file_hash);
        return err;
    }

    /* Insert version record */
    regislex_uuid_t version_id;
    regislex_uuid_generate(&version_id);

    const char* ver_sql =
        "INSERT INTO document_versions (id, document_id, version_number, "
        "file_hash, file_size, storage_path, change_description, "
        "created_by_id, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    err = regislex_db_prepare(ctx, ver_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        free(file_hash);
        return err;
    }

    regislex_db_bind_text(stmt, 1, version_id.value);
    regislex_db_bind_text(stmt, 2, id.value);
    regislex_db_bind_int(stmt, 3, 1);
    regislex_db_bind_text(stmt, 4, file_hash ? file_hash : "");
    regislex_db_bind_int64(stmt, 5, file_info.size);
    regislex_db_bind_text(stmt, 6, storage_path);
    regislex_db_bind_text(stmt, 7, "Initial upload");
    regislex_db_bind_text(stmt, 8, doc_data->owner_id.value);
    regislex_db_bind_text(stmt, 9, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    free(file_hash);

    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        return err;
    }

    regislex_db_transaction_commit(ctx);

    return regislex_document_get(ctx, &id, out_document);
}

regislex_error_t regislex_document_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_document_t** out_document)
{
    if (!ctx || !id || !out_document) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_document = NULL;

    const char* sql =
        "SELECT id, filename, title, description, document_type, status, "
        "folder_id, case_id, mime_type, file_size, storage_path, current_version, "
        "access_level, owner_id, locked_by_id, locked_at, created_at, updated_at "
        "FROM documents WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_document_t* doc = (regislex_document_t*)calloc(1, sizeof(regislex_document_t));
    if (!doc) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(doc->id.value, str, sizeof(doc->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(doc->filename, str, sizeof(doc->filename) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(doc->title, str, sizeof(doc->title) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(doc->description, str, sizeof(doc->description) - 1);

    doc->document_type = (regislex_doc_type_t)regislex_db_column_int(stmt, 4);
    doc->status = (regislex_doc_status_t)regislex_db_column_int(stmt, 5);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(doc->folder_id.value, str, sizeof(doc->folder_id.value) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) strncpy(doc->case_id.value, str, sizeof(doc->case_id.value) - 1);

    str = regislex_db_column_text(stmt, 8);
    if (str) strncpy(doc->mime_type, str, sizeof(doc->mime_type) - 1);

    doc->file_size = regislex_db_column_int64(stmt, 9);

    str = regislex_db_column_text(stmt, 10);
    if (str) strncpy(doc->storage_path, str, sizeof(doc->storage_path) - 1);

    doc->current_version = regislex_db_column_int(stmt, 11);
    doc->access_level = (regislex_access_level_t)regislex_db_column_int(stmt, 12);

    str = regislex_db_column_text(stmt, 13);
    if (str) strncpy(doc->owner_id.value, str, sizeof(doc->owner_id.value) - 1);

    str = regislex_db_column_text(stmt, 14);
    if (str) {
        strncpy(doc->locked_by_id.value, str, sizeof(doc->locked_by_id.value) - 1);
        doc->is_locked = true;
    }

    str = regislex_db_column_text(stmt, 15);
    if (str) regislex_datetime_parse(str, &doc->locked_at);

    str = regislex_db_column_text(stmt, 16);
    if (str) regislex_datetime_parse(str, &doc->created_at);

    str = regislex_db_column_text(stmt, 17);
    if (str) regislex_datetime_parse(str, &doc->updated_at);

    regislex_db_finalize(stmt);

    *out_document = doc;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_document_update(
    regislex_context_t* ctx,
    regislex_document_t* document)
{
    if (!ctx || !document || document->id.value[0] == '\0') {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "UPDATE documents SET filename = ?, title = ?, description = ?, "
        "document_type = ?, status = ?, folder_id = ?, case_id = ?, "
        "access_level = ?, updated_at = ? WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, document->filename);
    regislex_db_bind_text(stmt, 2, document->title);
    regislex_db_bind_text(stmt, 3, document->description);
    regislex_db_bind_int(stmt, 4, document->document_type);
    regislex_db_bind_int(stmt, 5, document->status);

    if (document->folder_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 6, document->folder_id.value);
    } else {
        regislex_db_bind_null(stmt, 6);
    }

    if (document->case_id.value[0] != '\0') {
        regislex_db_bind_text(stmt, 7, document->case_id.value);
    } else {
        regislex_db_bind_null(stmt, 7);
    }

    regislex_db_bind_int(stmt, 8, document->access_level);
    regislex_db_bind_text(stmt, 9, now_str);
    regislex_db_bind_text(stmt, 10, document->id.value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_document_delete(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    bool permanent)
{
    if (!ctx || !id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    if (permanent) {
        /* Delete file from storage */
        regislex_document_t* doc = NULL;
        if (regislex_document_get(ctx, id, &doc) == REGISLEX_SUCCESS && doc) {
            /* Delete all versions */
            regislex_doc_version_list_t* versions = NULL;
            if (regislex_document_version_list(ctx, id, &versions) == REGISLEX_SUCCESS && versions) {
                for (size_t i = 0; i < versions->count; i++) {
                    if (versions->versions[i].storage_path[0]) {
                        platform_file_delete(versions->versions[i].storage_path);
                    }
                }
                regislex_document_version_list_free(versions);
            }
            regislex_document_free(doc);
        }

        /* Delete version records */
        const char* ver_sql = "DELETE FROM document_versions WHERE document_id = ?";
        regislex_db_stmt_t* ver_stmt = NULL;
        if (regislex_db_prepare(ctx, ver_sql, &ver_stmt) == REGISLEX_SUCCESS) {
            regislex_db_bind_text(ver_stmt, 1, id->value);
            regislex_db_step(ver_stmt);
            regislex_db_finalize(ver_stmt);
        }

        /* Delete document record */
        const char* sql = "DELETE FROM documents WHERE id = ?";
        regislex_db_stmt_t* stmt = NULL;
        regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
        if (err != REGISLEX_SUCCESS) return err;

        regislex_db_bind_text(stmt, 1, id->value);
        err = regislex_db_step(stmt);
        regislex_db_finalize(stmt);

        return err;
    } else {
        /* Soft delete - just mark as deleted */
        const char* sql = "UPDATE documents SET status = ? WHERE id = ?";
        regislex_db_stmt_t* stmt = NULL;
        regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
        if (err != REGISLEX_SUCCESS) return err;

        regislex_db_bind_int(stmt, 1, REGISLEX_DOC_STATUS_DELETED);
        regislex_db_bind_text(stmt, 2, id->value);
        err = regislex_db_step(stmt);
        regislex_db_finalize(stmt);

        return err;
    }
}

regislex_error_t regislex_document_list(
    regislex_context_t* ctx,
    regislex_document_filter_t* filter,
    regislex_document_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    /* Build query with filters */
    char sql[2048];
    char where_clause[1024] = "WHERE 1=1";

    if (filter) {
        if (filter->folder_id.value[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND folder_id = '%s'", filter->folder_id.value);
            strcat(where_clause, buf);
        }

        if (filter->case_id.value[0] != '\0') {
            char buf[128];
            snprintf(buf, sizeof(buf), " AND case_id = '%s'", filter->case_id.value);
            strcat(where_clause, buf);
        }

        if (filter->document_type >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND document_type = %d", filter->document_type);
            strcat(where_clause, buf);
        }

        if (filter->status >= 0) {
            char buf[64];
            snprintf(buf, sizeof(buf), " AND status = %d", filter->status);
            strcat(where_clause, buf);
        } else {
            /* By default, exclude deleted documents */
            strcat(where_clause, " AND status != 4");
        }

        if (filter->search_text[0] != '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf), " AND (title LIKE '%%%s%%' OR filename LIKE '%%%s%%' OR description LIKE '%%%s%%')",
                     filter->search_text, filter->search_text, filter->search_text);
            strcat(where_clause, buf);
        }
    }

    snprintf(sql, sizeof(sql),
             "SELECT id, filename, title, document_type, status, folder_id, "
             "case_id, mime_type, file_size, current_version, created_at "
             "FROM documents %s ORDER BY updated_at DESC", where_clause);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_document_list_t* list = (regislex_document_list_t*)calloc(1,
        sizeof(regislex_document_list_t) + count * sizeof(regislex_document_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->documents = (regislex_document_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->documents[i].id.value, str, sizeof(list->documents[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->documents[i].filename, str, sizeof(list->documents[i].filename) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->documents[i].title, str, sizeof(list->documents[i].title) - 1);

        list->documents[i].document_type = (regislex_doc_type_t)regislex_db_result_get_int(result, 3);
        list->documents[i].status = (regislex_doc_status_t)regislex_db_result_get_int(result, 4);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->documents[i].folder_id.value, str, sizeof(list->documents[i].folder_id.value) - 1);

        str = regislex_db_result_get_text(result, 6);
        if (str) strncpy(list->documents[i].case_id.value, str, sizeof(list->documents[i].case_id.value) - 1);

        str = regislex_db_result_get_text(result, 7);
        if (str) strncpy(list->documents[i].mime_type, str, sizeof(list->documents[i].mime_type) - 1);

        list->documents[i].file_size = regislex_db_result_get_int64(result, 8);
        list->documents[i].current_version = regislex_db_result_get_int(result, 9);

        str = regislex_db_result_get_text(result, 10);
        if (str) regislex_datetime_parse(str, &list->documents[i].created_at);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_document_free(regislex_document_t* document) {
    free(document);
}

void regislex_document_list_free(regislex_document_list_t* list) {
    free(list);
}

/* ============================================================================
 * Version Control
 * ========================================================================== */

regislex_error_t regislex_document_upload_version(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const char* source_file_path,
    const char* change_description,
    const regislex_uuid_t* created_by_id,
    regislex_doc_version_t** out_version)
{
    if (!ctx || !document_id || !source_file_path || !created_by_id || !out_version) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_version = NULL;

    /* Get current document */
    regislex_document_t* doc = NULL;
    regislex_error_t err = regislex_document_get(ctx, document_id, &doc);
    if (err != REGISLEX_SUCCESS) return err;

    /* Check if document is locked by someone else */
    if (doc->is_locked && strcmp(doc->locked_by_id.value, created_by_id->value) != 0) {
        regislex_document_free(doc);
        return REGISLEX_ERROR_LOCKED;
    }

    int new_version = doc->current_version + 1;

    /* Get file info */
    platform_file_info_t file_info;
    if (platform_file_stat(source_file_path, &file_info) != 0) {
        regislex_document_free(doc);
        return REGISLEX_ERROR_IO;
    }

    /* Compute hash */
    char* file_hash = compute_file_hash(source_file_path);

    /* Get storage path for new version */
    char storage_path[1024];
    get_document_storage_path(ctx, document_id, new_version, storage_path, sizeof(storage_path));

    /* Create directory structure */
    char dir_path[1024];
    snprintf(dir_path, sizeof(dir_path), "%s", storage_path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        platform_mkdir_recursive(dir_path, 0755);
    }

    /* Copy file to storage */
    if (platform_file_copy(source_file_path, storage_path) != 0) {
        free(file_hash);
        regislex_document_free(doc);
        return REGISLEX_ERROR_IO;
    }

    /* Get timestamp */
    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    /* Begin transaction */
    regislex_db_transaction_begin(ctx);

    /* Insert version record */
    regislex_uuid_t version_id;
    regislex_uuid_generate(&version_id);

    const char* ver_sql =
        "INSERT INTO document_versions (id, document_id, version_number, "
        "file_hash, file_size, storage_path, change_description, "
        "created_by_id, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, ver_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        free(file_hash);
        regislex_document_free(doc);
        return err;
    }

    regislex_db_bind_text(stmt, 1, version_id.value);
    regislex_db_bind_text(stmt, 2, document_id->value);
    regislex_db_bind_int(stmt, 3, new_version);
    regislex_db_bind_text(stmt, 4, file_hash ? file_hash : "");
    regislex_db_bind_int64(stmt, 5, file_info.size);
    regislex_db_bind_text(stmt, 6, storage_path);
    regislex_db_bind_text(stmt, 7, change_description ? change_description : "");
    regislex_db_bind_text(stmt, 8, created_by_id->value);
    regislex_db_bind_text(stmt, 9, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    free(file_hash);

    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        regislex_document_free(doc);
        return err;
    }

    /* Update document */
    const char* doc_sql =
        "UPDATE documents SET current_version = ?, file_size = ?, "
        "storage_path = ?, updated_at = ? WHERE id = ?";

    err = regislex_db_prepare(ctx, doc_sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        regislex_document_free(doc);
        return err;
    }

    regislex_db_bind_int(stmt, 1, new_version);
    regislex_db_bind_int64(stmt, 2, file_info.size);
    regislex_db_bind_text(stmt, 3, storage_path);
    regislex_db_bind_text(stmt, 4, now_str);
    regislex_db_bind_text(stmt, 5, document_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    if (err != REGISLEX_SUCCESS) {
        regislex_db_transaction_rollback(ctx);
        regislex_document_free(doc);
        return err;
    }

    regislex_db_transaction_commit(ctx);
    regislex_document_free(doc);

    return regislex_document_version_get(ctx, &version_id, out_version);
}

regislex_error_t regislex_document_version_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* version_id,
    regislex_doc_version_t** out_version)
{
    if (!ctx || !version_id || !out_version) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_version = NULL;

    const char* sql =
        "SELECT id, document_id, version_number, file_hash, file_size, "
        "storage_path, change_description, created_by_id, created_at "
        "FROM document_versions WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, version_id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_doc_version_t* version = (regislex_doc_version_t*)calloc(1, sizeof(regislex_doc_version_t));
    if (!version) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(version->id.value, str, sizeof(version->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(version->document_id.value, str, sizeof(version->document_id.value) - 1);

    version->version_number = regislex_db_column_int(stmt, 2);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(version->file_hash, str, sizeof(version->file_hash) - 1);

    version->file_size = regislex_db_column_int64(stmt, 4);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(version->storage_path, str, sizeof(version->storage_path) - 1);

    str = regislex_db_column_text(stmt, 6);
    if (str) strncpy(version->change_description, str, sizeof(version->change_description) - 1);

    str = regislex_db_column_text(stmt, 7);
    if (str) strncpy(version->created_by_id.value, str, sizeof(version->created_by_id.value) - 1);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &version->created_at);

    regislex_db_finalize(stmt);

    *out_version = version;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_document_version_list(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    regislex_doc_version_list_t** out_list)
{
    if (!ctx || !document_id || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, document_id, version_number, file_hash, file_size, "
             "storage_path, change_description, created_by_id, created_at "
             "FROM document_versions WHERE document_id = '%s' "
             "ORDER BY version_number DESC", document_id->value);

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_doc_version_list_t* list = (regislex_doc_version_list_t*)calloc(1,
        sizeof(regislex_doc_version_list_t) + count * sizeof(regislex_doc_version_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->versions = (regislex_doc_version_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->versions[i].id.value, str, sizeof(list->versions[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->versions[i].document_id.value, str, sizeof(list->versions[i].document_id.value) - 1);

        list->versions[i].version_number = regislex_db_result_get_int(result, 2);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->versions[i].file_hash, str, sizeof(list->versions[i].file_hash) - 1);

        list->versions[i].file_size = regislex_db_result_get_int64(result, 4);

        str = regislex_db_result_get_text(result, 5);
        if (str) strncpy(list->versions[i].storage_path, str, sizeof(list->versions[i].storage_path) - 1);

        str = regislex_db_result_get_text(result, 6);
        if (str) strncpy(list->versions[i].change_description, str, sizeof(list->versions[i].change_description) - 1);

        str = regislex_db_result_get_text(result, 7);
        if (str) strncpy(list->versions[i].created_by_id.value, str, sizeof(list->versions[i].created_by_id.value) - 1);

        str = regislex_db_result_get_text(result, 8);
        if (str) regislex_datetime_parse(str, &list->versions[i].created_at);

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_document_version_free(regislex_doc_version_t* version) {
    free(version);
}

void regislex_document_version_list_free(regislex_doc_version_list_t* list) {
    free(list);
}

/* ============================================================================
 * Check-In/Check-Out (Locking)
 * ========================================================================== */

regislex_error_t regislex_document_checkout(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const regislex_uuid_t* user_id)
{
    if (!ctx || !document_id || !user_id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Check if already locked */
    regislex_document_t* doc = NULL;
    regislex_error_t err = regislex_document_get(ctx, document_id, &doc);
    if (err != REGISLEX_SUCCESS) return err;

    if (doc->is_locked) {
        regislex_document_free(doc);
        return REGISLEX_ERROR_LOCKED;
    }
    regislex_document_free(doc);

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql = "UPDATE documents SET locked_by_id = ?, locked_at = ? WHERE id = ?";
    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, user_id->value);
    regislex_db_bind_text(stmt, 2, now_str);
    regislex_db_bind_text(stmt, 3, document_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_document_checkin(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const regislex_uuid_t* user_id,
    const char* new_file_path,
    const char* change_description)
{
    if (!ctx || !document_id || !user_id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* Verify lock ownership */
    regislex_document_t* doc = NULL;
    regislex_error_t err = regislex_document_get(ctx, document_id, &doc);
    if (err != REGISLEX_SUCCESS) return err;

    if (!doc->is_locked || strcmp(doc->locked_by_id.value, user_id->value) != 0) {
        regislex_document_free(doc);
        return REGISLEX_ERROR_PERMISSION_DENIED;
    }
    regislex_document_free(doc);

    /* Upload new version if file provided */
    if (new_file_path) {
        regislex_doc_version_t* version = NULL;
        err = regislex_document_upload_version(ctx, document_id, new_file_path,
                                                change_description, user_id, &version);
        if (err != REGISLEX_SUCCESS) return err;
        regislex_document_version_free(version);
    }

    /* Release lock */
    const char* sql = "UPDATE documents SET locked_by_id = NULL, locked_at = NULL WHERE id = ?";
    regislex_db_stmt_t* stmt = NULL;
    err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, document_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

regislex_error_t regislex_document_unlock(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    const regislex_uuid_t* admin_user_id)
{
    if (!ctx || !document_id) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* In production, verify admin permissions here */

    const char* sql = "UPDATE documents SET locked_by_id = NULL, locked_at = NULL WHERE id = ?";
    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, document_id->value);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);

    return err;
}

/* ============================================================================
 * Document Download
 * ========================================================================== */

regislex_error_t regislex_document_download(
    regislex_context_t* ctx,
    const regislex_uuid_t* document_id,
    int version,
    const char* destination_path)
{
    if (!ctx || !document_id || !destination_path) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    char storage_path[1024] = {0};

    if (version <= 0) {
        /* Get current version */
        regislex_document_t* doc = NULL;
        regislex_error_t err = regislex_document_get(ctx, document_id, &doc);
        if (err != REGISLEX_SUCCESS) return err;

        strncpy(storage_path, doc->storage_path, sizeof(storage_path) - 1);
        regislex_document_free(doc);
    } else {
        /* Get specific version */
        char sql[512];
        snprintf(sql, sizeof(sql),
                 "SELECT storage_path FROM document_versions "
                 "WHERE document_id = '%s' AND version_number = %d",
                 document_id->value, version);

        regislex_db_result_t* result = NULL;
        regislex_error_t err = regislex_db_query(ctx, sql, &result);
        if (err != REGISLEX_SUCCESS) return err;

        if (regislex_db_result_next(result)) {
            const char* path = regislex_db_result_get_text(result, 0);
            if (path) strncpy(storage_path, path, sizeof(storage_path) - 1);
        }
        regislex_db_result_free(result);
    }

    if (storage_path[0] == '\0') {
        return REGISLEX_ERROR_NOT_FOUND;
    }

    /* Copy file to destination */
    if (platform_file_copy(storage_path, destination_path) != 0) {
        return REGISLEX_ERROR_IO;
    }

    return REGISLEX_SUCCESS;
}

/* ============================================================================
 * Document Templates
 * ========================================================================== */

regislex_error_t regislex_document_template_create(
    regislex_context_t* ctx,
    regislex_doc_template_t* template_data,
    const char* template_file_path,
    regislex_doc_template_t** out_template)
{
    if (!ctx || !template_data || !template_file_path || !out_template) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_template = NULL;

    regislex_uuid_t id;
    if (template_data->id.value[0] == '\0') {
        regislex_uuid_generate(&id);
    } else {
        id = template_data->id;
    }

    /* Read template content */
    FILE* f = fopen(template_file_path, "rb");
    if (!f) {
        return REGISLEX_ERROR_IO;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(f);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    regislex_datetime_t now;
    regislex_datetime_now(&now);
    char now_str[32];
    regislex_datetime_format(&now, now_str, sizeof(now_str));

    const char* sql =
        "INSERT INTO document_templates (id, name, description, category, "
        "template_content, output_format, field_definitions, is_active, "
        "created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) {
        free(content);
        return err;
    }

    regislex_db_bind_text(stmt, 1, id.value);
    regislex_db_bind_text(stmt, 2, template_data->name);
    regislex_db_bind_text(stmt, 3, template_data->description);
    regislex_db_bind_text(stmt, 4, template_data->category);
    regislex_db_bind_text(stmt, 5, content);
    regislex_db_bind_text(stmt, 6, template_data->output_format);
    regislex_db_bind_text(stmt, 7, template_data->field_definitions);
    regislex_db_bind_int(stmt, 8, template_data->is_active ? 1 : 0);
    regislex_db_bind_text(stmt, 9, now_str);
    regislex_db_bind_text(stmt, 10, now_str);

    err = regislex_db_step(stmt);
    regislex_db_finalize(stmt);
    free(content);

    if (err != REGISLEX_SUCCESS) return err;

    return regislex_document_template_get(ctx, &id, out_template);
}

regislex_error_t regislex_document_template_get(
    regislex_context_t* ctx,
    const regislex_uuid_t* id,
    regislex_doc_template_t** out_template)
{
    if (!ctx || !id || !out_template) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_template = NULL;

    const char* sql =
        "SELECT id, name, description, category, output_format, "
        "field_definitions, is_active, created_at, updated_at "
        "FROM document_templates WHERE id = ?";

    regislex_db_stmt_t* stmt = NULL;
    regislex_error_t err = regislex_db_prepare(ctx, sql, &stmt);
    if (err != REGISLEX_SUCCESS) return err;

    regislex_db_bind_text(stmt, 1, id->value);

    err = regislex_db_step(stmt);
    if (err != REGISLEX_ROW) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NOT_FOUND;
    }

    regislex_doc_template_t* tmpl = (regislex_doc_template_t*)calloc(1, sizeof(regislex_doc_template_t));
    if (!tmpl) {
        regislex_db_finalize(stmt);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    const char* str;
    str = regislex_db_column_text(stmt, 0);
    if (str) strncpy(tmpl->id.value, str, sizeof(tmpl->id.value) - 1);

    str = regislex_db_column_text(stmt, 1);
    if (str) strncpy(tmpl->name, str, sizeof(tmpl->name) - 1);

    str = regislex_db_column_text(stmt, 2);
    if (str) strncpy(tmpl->description, str, sizeof(tmpl->description) - 1);

    str = regislex_db_column_text(stmt, 3);
    if (str) strncpy(tmpl->category, str, sizeof(tmpl->category) - 1);

    str = regislex_db_column_text(stmt, 4);
    if (str) strncpy(tmpl->output_format, str, sizeof(tmpl->output_format) - 1);

    str = regislex_db_column_text(stmt, 5);
    if (str) strncpy(tmpl->field_definitions, str, sizeof(tmpl->field_definitions) - 1);

    tmpl->is_active = regislex_db_column_int(stmt, 6) != 0;

    str = regislex_db_column_text(stmt, 7);
    if (str) regislex_datetime_parse(str, &tmpl->created_at);

    str = regislex_db_column_text(stmt, 8);
    if (str) regislex_datetime_parse(str, &tmpl->updated_at);

    regislex_db_finalize(stmt);

    *out_template = tmpl;
    return REGISLEX_SUCCESS;
}

regislex_error_t regislex_document_template_list(
    regislex_context_t* ctx,
    const char* category_filter,
    regislex_doc_template_list_t** out_list)
{
    if (!ctx || !out_list) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    *out_list = NULL;

    char sql[512];
    if (category_filter && category_filter[0] != '\0') {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, description, category, output_format, is_active "
                 "FROM document_templates WHERE category = '%s' AND is_active = 1 "
                 "ORDER BY name", category_filter);
    } else {
        snprintf(sql, sizeof(sql),
                 "SELECT id, name, description, category, output_format, is_active "
                 "FROM document_templates WHERE is_active = 1 "
                 "ORDER BY category, name");
    }

    regislex_db_result_t* result = NULL;
    regislex_error_t err = regislex_db_query(ctx, sql, &result);
    if (err != REGISLEX_SUCCESS) return err;

    /* Count results */
    size_t count = 0;
    while (regislex_db_result_next(result)) count++;
    regislex_db_result_reset(result);

    /* Allocate list */
    regislex_doc_template_list_t* list = (regislex_doc_template_list_t*)calloc(1,
        sizeof(regislex_doc_template_list_t) + count * sizeof(regislex_doc_template_t));
    if (!list) {
        regislex_db_result_free(result);
        return REGISLEX_ERROR_NO_MEMORY;
    }

    list->count = count;
    list->templates = (regislex_doc_template_t*)(list + 1);

    size_t i = 0;
    while (regislex_db_result_next(result) && i < count) {
        const char* str;

        str = regislex_db_result_get_text(result, 0);
        if (str) strncpy(list->templates[i].id.value, str, sizeof(list->templates[i].id.value) - 1);

        str = regislex_db_result_get_text(result, 1);
        if (str) strncpy(list->templates[i].name, str, sizeof(list->templates[i].name) - 1);

        str = regislex_db_result_get_text(result, 2);
        if (str) strncpy(list->templates[i].description, str, sizeof(list->templates[i].description) - 1);

        str = regislex_db_result_get_text(result, 3);
        if (str) strncpy(list->templates[i].category, str, sizeof(list->templates[i].category) - 1);

        str = regislex_db_result_get_text(result, 4);
        if (str) strncpy(list->templates[i].output_format, str, sizeof(list->templates[i].output_format) - 1);

        list->templates[i].is_active = regislex_db_result_get_int(result, 5) != 0;

        i++;
    }

    regislex_db_result_free(result);

    *out_list = list;
    return REGISLEX_SUCCESS;
}

void regislex_document_template_free(regislex_doc_template_t* tmpl) {
    free(tmpl);
}

void regislex_document_template_list_free(regislex_doc_template_list_t* list) {
    free(list);
}

/* ============================================================================
 * Document Search
 * ========================================================================== */

regislex_error_t regislex_document_search(
    regislex_context_t* ctx,
    const char* search_query,
    regislex_document_filter_t* filter,
    regislex_document_list_t** out_results)
{
    if (!ctx || !search_query || !out_results) {
        return REGISLEX_ERROR_INVALID_PARAM;
    }

    /* For full-text search, we'd use FTS5 in SQLite */
    /* This is a basic LIKE-based search */

    regislex_document_filter_t search_filter = {0};
    if (filter) {
        search_filter = *filter;
    }
    strncpy(search_filter.search_text, search_query, sizeof(search_filter.search_text) - 1);

    return regislex_document_list(ctx, &search_filter, out_results);
}
