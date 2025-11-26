/**
 * @file filesystem.c
 * @brief Platform Filesystem Operations
 */

#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef REGISLEX_PLATFORM_WINDOWS
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <io.h>
#define PATH_SEP '\\'
#else
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#define PATH_SEP '/'
#endif

/* Ensure max path is defined */
#ifndef REGISLEX_MAX_PATH
#define REGISLEX_MAX_PATH 4096
#endif

bool platform_file_exists(const char* path) {
    if (!path) return false;
#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES;
#else
    return access(path, F_OK) == 0;
#endif
}

bool platform_is_directory(const char* path) {
    if (!path) return false;
#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

platform_error_t platform_mkdir(const char* path, bool recursive) {
    if (!path || !*path) return PLATFORM_ERROR_INVALID_ARGUMENT;

    if (platform_is_directory(path)) {
        return PLATFORM_ERROR_ALREADY_EXISTS;
    }

    if (recursive) {
        char* tmp = platform_strdup(path);
        if (!tmp) return PLATFORM_ERROR_OUT_OF_MEMORY;

        for (char* p = tmp + 1; *p; p++) {
            if (*p == '/' || *p == '\\') {
                *p = '\0';
                if (!platform_is_directory(tmp)) {
#ifdef REGISLEX_PLATFORM_WINDOWS
                    _mkdir(tmp);
#else
                    mkdir(tmp, 0755);
#endif
                }
                *p = PATH_SEP;
            }
        }
        platform_free(tmp);
    }

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (_mkdir(path) != 0) {
        return PLATFORM_ERROR_IO;
    }
#else
    if (mkdir(path, 0755) != 0) {
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}

platform_error_t platform_rmdir(const char* path, bool recursive) {
    if (!path) return PLATFORM_ERROR_INVALID_ARGUMENT;

    if (!recursive) {
#ifdef REGISLEX_PLATFORM_WINDOWS
        if (_rmdir(path) != 0) return PLATFORM_ERROR_IO;
#else
        if (rmdir(path) != 0) return PLATFORM_ERROR_IO;
#endif
        return PLATFORM_OK;
    }

    /* Recursive removal - simplified */
#ifdef REGISLEX_PLATFORM_WINDOWS
    WIN32_FIND_DATAA fd;
    char pattern[REGISLEX_MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                char fullpath[REGISLEX_MAX_PATH];
                snprintf(fullpath, sizeof(fullpath), "%s\\%s", path, fd.cFileName);
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    platform_rmdir(fullpath, true);
                } else {
                    DeleteFileA(fullpath);
                }
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    _rmdir(path);
#else
    DIR* dir = opendir(path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char fullpath[REGISLEX_MAX_PATH];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
                if (platform_is_directory(fullpath)) {
                    platform_rmdir(fullpath, true);
                } else {
                    unlink(fullpath);
                }
            }
        }
        closedir(dir);
    }
    rmdir(path);
#endif

    return PLATFORM_OK;
}

platform_error_t platform_remove_file(const char* path) {
    if (!path) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    if (!DeleteFileA(path)) return PLATFORM_ERROR_IO;
#else
    if (unlink(path) != 0) return PLATFORM_ERROR_IO;
#endif
    return PLATFORM_OK;
}

platform_error_t platform_copy_file(const char* src, const char* dst) {
    if (!src || !dst) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (!CopyFileA(src, dst, FALSE)) return PLATFORM_ERROR_IO;
    return PLATFORM_OK;
#else
    FILE* fin = fopen(src, "rb");
    if (!fin) return PLATFORM_ERROR_IO;

    FILE* fout = fopen(dst, "wb");
    if (!fout) {
        fclose(fin);
        return PLATFORM_ERROR_IO;
    }

    char buffer[8192];
    size_t n;
    while ((n = fread(buffer, 1, sizeof(buffer), fin)) > 0) {
        if (fwrite(buffer, 1, n, fout) != n) {
            fclose(fin);
            fclose(fout);
            return PLATFORM_ERROR_IO;
        }
    }

    fclose(fin);
    fclose(fout);
    return PLATFORM_OK;
#endif
}

platform_error_t platform_rename(const char* old_path, const char* new_path) {
    if (!old_path || !new_path) return PLATFORM_ERROR_INVALID_ARGUMENT;
    if (rename(old_path, new_path) != 0) return PLATFORM_ERROR_IO;
    return PLATFORM_OK;
}

platform_error_t platform_file_size(const char* path, int64_t* size) {
    if (!path || !size) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) return PLATFORM_ERROR_NOT_FOUND;
    *size = ((int64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
#else
    struct stat st;
    if (stat(path, &st) != 0) return PLATFORM_ERROR_NOT_FOUND;
    *size = st.st_size;
#endif
    return PLATFORM_OK;
}

platform_error_t platform_path_join(char* result, size_t size, const char* base, const char* path) {
    if (!result || !base) return PLATFORM_ERROR_INVALID_ARGUMENT;

    if (!path || !*path) {
        strncpy(result, base, size - 1);
        result[size - 1] = '\0';
        return PLATFORM_OK;
    }

    size_t base_len = strlen(base);
    bool base_has_sep = base_len > 0 && (base[base_len - 1] == '/' || base[base_len - 1] == '\\');

    if (base_has_sep) {
        snprintf(result, size, "%s%s", base, path);
    } else {
        snprintf(result, size, "%s%c%s", base, PATH_SEP, path);
    }

    return PLATFORM_OK;
}

platform_error_t platform_get_app_data_dir(const char* app_name, char* buffer, size_t size) {
    if (!app_name || !buffer) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    char* appdata = getenv("APPDATA");
    if (!appdata) appdata = getenv("LOCALAPPDATA");
    if (!appdata) return PLATFORM_ERROR;
    snprintf(buffer, size, "%s\\%s", appdata, app_name);
#elif defined(REGISLEX_PLATFORM_MACOS)
    char* home = getenv("HOME");
    if (!home) return PLATFORM_ERROR;
    snprintf(buffer, size, "%s/Library/Application Support/%s", home, app_name);
#else
    char* home = getenv("HOME");
    if (!home) return PLATFORM_ERROR;
    snprintf(buffer, size, "%s/.local/share/%s", home, app_name);
#endif

    return PLATFORM_OK;
}
