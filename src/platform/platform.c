/**
 * @file platform.c
 * @brief Platform Abstraction Layer Implementation
 *
 * This file contains only functions NOT implemented in filesystem.c, threading.c, or network.c.
 * File system functions are in filesystem.c
 * Threading functions are in threading.c
 * Network functions are in network.c
 */

#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#ifdef REGISLEX_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shlwapi.h>
    #include <shlobj.h>
    #include <bcrypt.h>
    #pragma comment(lib, "shlwapi.lib")
    #pragma comment(lib, "bcrypt.lib")
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <dirent.h>
    #include <dlfcn.h>
    #include <fcntl.h>
    #include <pwd.h>

    #ifdef REGISLEX_PLATFORM_LINUX
        #include <sys/sysinfo.h>
        #include <sys/random.h>
    #endif

    #ifdef REGISLEX_PLATFORM_MACOS
        #include <mach/mach_time.h>
        #include <sys/sysctl.h>
        #include <Security/Security.h>
    #endif
#endif

/* ============================================================================
 * Memory Functions
 * ============================================================================ */

void* platform_malloc(size_t size) {
    return malloc(size);
}

void* platform_calloc(size_t count, size_t size) {
    return calloc(count, size);
}

void* platform_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void platform_free(void* ptr) {
    free(ptr);
}

char* platform_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = (char*)platform_malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

/* ============================================================================
 * Time Functions
 * ============================================================================ */

int64_t platform_time_ms(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.HighPart = ft.dwHighDateTime;
    uli.LowPart = ft.dwLowDateTime;
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) / 10000ULL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

int64_t platform_time_us(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.HighPart = ft.dwHighDateTime;
    uli.LowPart = ft.dwLowDateTime;
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) / 10ULL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
#endif
}

uint64_t platform_monotonic_ns(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000000ULL / freq.QuadPart);
#elif defined(REGISLEX_PLATFORM_MACOS)
    static mach_timebase_info_data_t info = {0};
    if (info.denom == 0) {
        mach_timebase_info(&info);
    }
    return mach_absolute_time() * info.numer / info.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

platform_error_t platform_format_time(int64_t timestamp, char* buffer,
                                      size_t size, bool utc) {
    if (!buffer || size < 25) return PLATFORM_ERROR_INVALID_ARGUMENT;

    time_t t = (time_t)timestamp;
    struct tm* tm_info;

    if (utc) {
#ifdef REGISLEX_PLATFORM_WINDOWS
        struct tm tm_buf;
        gmtime_s(&tm_buf, &t);
        tm_info = &tm_buf;
#else
        tm_info = gmtime(&t);
#endif
    } else {
#ifdef REGISLEX_PLATFORM_WINDOWS
        struct tm tm_buf;
        localtime_s(&tm_buf, &t);
        tm_info = &tm_buf;
#else
        tm_info = localtime(&t);
#endif
    }

    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return PLATFORM_OK;
}

/* ============================================================================
 * Process Functions
 * ============================================================================ */

int platform_getpid(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    return (int)GetCurrentProcessId();
#else
    return (int)getpid();
#endif
}

platform_error_t platform_getenv(const char* name, char* buffer, size_t size) {
    if (!name || !buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD len = GetEnvironmentVariableA(name, buffer, (DWORD)size);
    if (len == 0) return PLATFORM_ERROR_NOT_FOUND;
    if (len >= size) return PLATFORM_ERROR_IO;
#else
    const char* val = getenv(name);
    if (!val) return PLATFORM_ERROR_NOT_FOUND;
    if (strlen(val) >= size) return PLATFORM_ERROR_IO;
    strcpy(buffer, val);
#endif

    return PLATFORM_OK;
}

platform_error_t platform_setenv(const char* name, const char* value) {
    if (!name) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (!SetEnvironmentVariableA(name, value)) {
        return PLATFORM_ERROR_IO;
    }
#else
    if (value) {
        if (setenv(name, value, 1) != 0) {
            return PLATFORM_ERROR_IO;
        }
    } else {
        unsetenv(name);
    }
#endif

    return PLATFORM_OK;
}

platform_error_t platform_get_hostname(char* buffer, size_t size) {
    if (!buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD len = (DWORD)size;
    if (!GetComputerNameA(buffer, &len)) {
        return PLATFORM_ERROR_IO;
    }
#else
    if (gethostname(buffer, size) != 0) {
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}

int platform_get_cpu_count(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#elif defined(REGISLEX_PLATFORM_MACOS)
    int count;
    size_t siz = sizeof(count);
    if (sysctlbyname("hw.ncpu", &count, &siz, NULL, 0) == 0) {
        return count;
    }
    return 1;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

uint64_t platform_get_total_memory(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        return ms.ullTotalPhys;
    }
    return 0;
#elif defined(REGISLEX_PLATFORM_MACOS)
    int64_t mem;
    size_t siz = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &siz, NULL, 0) == 0) {
        return (uint64_t)mem;
    }
    return 0;
#else
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return (uint64_t)si.totalram * si.mem_unit;
    }
    return 0;
#endif
}

platform_error_t platform_getcwd(char* buffer, size_t size) {
    if (!buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (GetCurrentDirectoryA((DWORD)size, buffer) == 0) {
        return PLATFORM_ERROR_IO;
    }
#else
    if (getcwd(buffer, size) == NULL) {
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}

platform_error_t platform_get_temp_dir(char* buffer, size_t size) {
    if (!buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    DWORD len = GetTempPathA((DWORD)size, buffer);
    if (len == 0 || len >= size) {
        return PLATFORM_ERROR_IO;
    }
#else
    const char* tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";

    if (strlen(tmp) >= size) {
        return PLATFORM_ERROR_IO;
    }
    strcpy(buffer, tmp);
#endif

    return PLATFORM_OK;
}

platform_error_t platform_get_home_dir(char* buffer, size_t size) {
    if (!buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buffer))) {
        return PLATFORM_OK;
    }
    return PLATFORM_ERROR_IO;
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) return PLATFORM_ERROR_NOT_FOUND;

    if (strlen(home) >= size) {
        return PLATFORM_ERROR_IO;
    }
    strcpy(buffer, home);
    return PLATFORM_OK;
#endif
}

/* ============================================================================
 * Path utility functions (helpers for filesystem.c)
 * ============================================================================ */

void platform_normalize_path(char* path) {
    if (!path) return;

    for (char* p = path; *p; p++) {
#ifdef REGISLEX_PLATFORM_WINDOWS
        if (*p == '/') *p = '\\';
#else
        if (*p == '\\') *p = '/';
#endif
    }
}

const char* platform_get_extension(const char* path) {
    if (!path) return "";

    const char* dot = strrchr(path, '.');
    const char* sep = strrchr(path, REGISLEX_PATH_SEPARATOR);

    if (!dot || (sep && dot < sep)) {
        return "";
    }

    return dot;
}

const char* platform_get_filename(const char* path) {
    if (!path) return "";

    const char* sep = strrchr(path, REGISLEX_PATH_SEPARATOR);
#ifdef REGISLEX_PLATFORM_WINDOWS
    const char* sep2 = strrchr(path, '/');
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;
#endif

    return sep ? sep + 1 : path;
}

platform_error_t platform_get_dirname(const char* path, char* buffer, size_t size) {
    if (!path || !buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

    const char* filename = platform_get_filename(path);
    size_t len = filename - path;

    if (len == 0) {
        buffer[0] = '.';
        buffer[1] = '\0';
        return PLATFORM_OK;
    }

    if (len >= size) {
        return PLATFORM_ERROR_IO;
    }

    memcpy(buffer, path, len);
    /* Remove trailing separator unless it's the root */
    if (len > 1 && buffer[len - 1] == REGISLEX_PATH_SEPARATOR) {
        buffer[len - 1] = '\0';
    } else {
        buffer[len] = '\0';
    }

    return PLATFORM_OK;
}

/* ============================================================================
 * Directory Iterator
 * ============================================================================ */

#ifdef REGISLEX_PLATFORM_WINDOWS
struct platform_dir_iterator {
    HANDLE handle;
    WIN32_FIND_DATAA find_data;
    bool first;
    char path[REGISLEX_MAX_PATH];
};
#else
struct platform_dir_iterator {
    DIR* dir;
    char path[REGISLEX_MAX_PATH];
};
#endif

platform_error_t platform_dir_open(const char* path, platform_dir_iterator_t** iter) {
    if (!path || !iter) return PLATFORM_ERROR_INVALID_ARGUMENT;

    *iter = (platform_dir_iterator_t*)platform_calloc(1, sizeof(platform_dir_iterator_t));
    if (!*iter) return PLATFORM_ERROR_OUT_OF_MEMORY;

#ifdef REGISLEX_PLATFORM_WINDOWS
    char search_path[REGISLEX_MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    (*iter)->handle = FindFirstFileA(search_path, &(*iter)->find_data);
    if ((*iter)->handle == INVALID_HANDLE_VALUE) {
        platform_free(*iter);
        *iter = NULL;
        return PLATFORM_ERROR_NOT_FOUND;
    }
    (*iter)->first = true;
    strncpy((*iter)->path, path, REGISLEX_MAX_PATH - 1);
#else
    (*iter)->dir = opendir(path);
    if (!(*iter)->dir) {
        platform_free(*iter);
        *iter = NULL;
        return PLATFORM_ERROR_NOT_FOUND;
    }
    strncpy((*iter)->path, path, REGISLEX_MAX_PATH - 1);
#endif

    return PLATFORM_OK;
}

platform_error_t platform_dir_next(platform_dir_iterator_t* iter, platform_dir_entry_t* entry) {
    if (!iter || !entry) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (!iter->first) {
        if (!FindNextFileA(iter->handle, &iter->find_data)) {
            return PLATFORM_ERROR_NOT_FOUND;
        }
    }
    iter->first = false;

    strncpy(entry->name, iter->find_data.cFileName, REGISLEX_MAX_PATH - 1);
    entry->is_directory = (iter->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    LARGE_INTEGER li;
    li.HighPart = iter->find_data.nFileSizeHigh;
    li.LowPart = iter->find_data.nFileSizeLow;
    entry->size = li.QuadPart;

    ULARGE_INTEGER uli;
    uli.HighPart = iter->find_data.ftLastWriteTime.dwHighDateTime;
    uli.LowPart = iter->find_data.ftLastWriteTime.dwLowDateTime;
    entry->modified_time = (uli.QuadPart - 116444736000000000ULL) / 10000000ULL;
#else
    struct dirent* de = readdir(iter->dir);
    if (!de) return PLATFORM_ERROR_NOT_FOUND;

    strncpy(entry->name, de->d_name, REGISLEX_MAX_PATH - 1);

    char full_path[REGISLEX_MAX_PATH];
    platform_path_join(full_path, sizeof(full_path), iter->path, de->d_name);

    struct stat st;
    if (stat(full_path, &st) == 0) {
        entry->is_directory = S_ISDIR(st.st_mode);
        entry->size = st.st_size;
        entry->modified_time = st.st_mtime;
    } else {
        entry->is_directory = (de->d_type == DT_DIR);
        entry->size = 0;
        entry->modified_time = 0;
    }
#endif

    return PLATFORM_OK;
}

void platform_dir_close(platform_dir_iterator_t* iter) {
    if (!iter) return;

#ifdef REGISLEX_PLATFORM_WINDOWS
    if (iter->handle != INVALID_HANDLE_VALUE) {
        FindClose(iter->handle);
    }
#else
    if (iter->dir) {
        closedir(iter->dir);
    }
#endif

    platform_free(iter);
}

/* ============================================================================
 * Random Number Generation
 * ============================================================================ */

platform_error_t platform_random_bytes(void* buffer, size_t size) {
    if (!buffer || size == 0) return PLATFORM_ERROR_INVALID_ARGUMENT;

#ifdef REGISLEX_PLATFORM_WINDOWS
    NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)buffer, (ULONG)size,
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        return PLATFORM_ERROR_IO;
    }
#elif defined(REGISLEX_PLATFORM_MACOS)
    if (SecRandomCopyBytes(kSecRandomDefault, size, buffer) != errSecSuccess) {
        return PLATFORM_ERROR_IO;
    }
#elif defined(REGISLEX_PLATFORM_LINUX)
    ssize_t ret = getrandom(buffer, size, 0);
    if (ret < 0 || (size_t)ret != size) {
        return PLATFORM_ERROR_IO;
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return PLATFORM_ERROR_IO;

    ssize_t ret = read(fd, buffer, size);
    close(fd);

    if (ret < 0 || (size_t)ret != size) {
        return PLATFORM_ERROR_IO;
    }
#endif

    return PLATFORM_OK;
}

uint32_t platform_random_u32(void) {
    uint32_t val;
    platform_random_bytes(&val, sizeof(val));
    return val;
}

uint64_t platform_random_u64(void) {
    uint64_t val;
    platform_random_bytes(&val, sizeof(val));
    return val;
}

/* platform_remove_file is defined in filesystem.c */
