/**
 * @file storage.c
 * @brief Document Storage Backend
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

static char storage_base_path[512] = "";

regislex_error_t regislex_storage_init(const char* base_path) {
    if (!base_path) return REGISLEX_ERROR_INVALID_ARGUMENT;
    strncpy(storage_base_path, base_path, sizeof(storage_base_path) - 1);
    if (!platform_file_exists(base_path)) {
        return platform_mkdir(base_path, true) == PLATFORM_OK ? REGISLEX_OK : REGISLEX_ERROR_IO;
    }
    return REGISLEX_OK;
}

regislex_error_t regislex_storage_store(const char* doc_id, const void* data, size_t size, char* path_out, size_t path_size) {
    if (!doc_id || !data || !path_out) return REGISLEX_ERROR_INVALID_ARGUMENT;
    char full_path[512];
    platform_path_join(full_path, sizeof(full_path), storage_base_path, doc_id);
    FILE* fp = fopen(full_path, "wb");
    if (!fp) return REGISLEX_ERROR_IO;
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    if (written != size) return REGISLEX_ERROR_IO;
    strncpy(path_out, full_path, path_size - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_storage_retrieve(const char* path, void** data, size_t* size) {
    if (!path || !data || !size) return REGISLEX_ERROR_INVALID_ARGUMENT;
    int64_t file_size;
    if (platform_file_size(path, &file_size) != PLATFORM_OK) return REGISLEX_ERROR_NOT_FOUND;
    *data = platform_malloc((size_t)file_size);
    if (!*data) return REGISLEX_ERROR_OUT_OF_MEMORY;
    FILE* fp = fopen(path, "rb");
    if (!fp) { platform_free(*data); return REGISLEX_ERROR_IO; }
    *size = fread(*data, 1, (size_t)file_size, fp);
    fclose(fp);
    return REGISLEX_OK;
}

regislex_error_t regislex_storage_delete(const char* path) {
    if (!path) return REGISLEX_ERROR_INVALID_ARGUMENT;
    return platform_remove_file(path) == PLATFORM_OK ? REGISLEX_OK : REGISLEX_ERROR_IO;
}
