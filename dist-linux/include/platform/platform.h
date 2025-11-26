/**
 * @file platform.h
 * @brief Platform Abstraction Layer
 *
 * Provides cross-platform abstractions for OS-specific functionality.
 */

#ifndef REGISLEX_PLATFORM_H
#define REGISLEX_PLATFORM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Platform detection macros */
#if defined(_WIN32) || defined(_WIN64)
    #ifndef REGISLEX_PLATFORM_WINDOWS
        #define REGISLEX_PLATFORM_WINDOWS
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define REGISLEX_PLATFORM_IOS
    #else
        #ifndef REGISLEX_PLATFORM_MACOS
            #define REGISLEX_PLATFORM_MACOS
        #endif
    #endif
#elif defined(__ANDROID__)
    #ifndef REGISLEX_PLATFORM_ANDROID
        #define REGISLEX_PLATFORM_ANDROID
    #endif
#elif defined(__linux__)
    #ifndef REGISLEX_PLATFORM_LINUX
        #define REGISLEX_PLATFORM_LINUX
    #endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #define REGISLEX_PLATFORM_BSD
#else
    #error "Unsupported platform"
#endif

/* POSIX-like systems */
#if defined(REGISLEX_PLATFORM_LINUX) || defined(REGISLEX_PLATFORM_MACOS) || \
    defined(REGISLEX_PLATFORM_BSD) || defined(REGISLEX_PLATFORM_ANDROID)
    #define REGISLEX_PLATFORM_POSIX
#endif

/* Path separator */
#ifdef REGISLEX_PLATFORM_WINDOWS
    #define REGISLEX_PATH_SEPARATOR '\\'
    #define REGISLEX_PATH_SEPARATOR_STR "\\"
    #define REGISLEX_PATH_LIST_SEPARATOR ';'
#else
    #define REGISLEX_PATH_SEPARATOR '/'
    #define REGISLEX_PATH_SEPARATOR_STR "/"
    #define REGISLEX_PATH_LIST_SEPARATOR ':'
#endif

/* Maximum path length */
#ifdef REGISLEX_PLATFORM_WINDOWS
    #define REGISLEX_MAX_PATH 32767
#else
    #define REGISLEX_MAX_PATH 4096
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================ */

typedef enum {
    PLATFORM_OK = 0,
    PLATFORM_ERROR = -1,
    PLATFORM_ERROR_INVALID_ARGUMENT = -2,
    PLATFORM_ERROR_OUT_OF_MEMORY = -3,
    PLATFORM_ERROR_NOT_FOUND = -4,
    PLATFORM_ERROR_ALREADY_EXISTS = -5,
    PLATFORM_ERROR_PERMISSION_DENIED = -6,
    PLATFORM_ERROR_IO = -7,
    PLATFORM_ERROR_TIMEOUT = -8,
    PLATFORM_ERROR_WOULD_BLOCK = -9,
    PLATFORM_ERROR_NOT_SUPPORTED = -10
} platform_error_t;

/* ============================================================================
 * Memory Functions
 * ============================================================================ */

/**
 * @brief Allocate memory
 * @param size Size in bytes
 * @return Allocated memory or NULL on failure
 */
void* platform_malloc(size_t size);

/**
 * @brief Allocate zeroed memory
 * @param count Number of elements
 * @param size Size of each element
 * @return Allocated memory or NULL on failure
 */
void* platform_calloc(size_t count, size_t size);

/**
 * @brief Reallocate memory
 * @param ptr Existing pointer
 * @param size New size
 * @return Reallocated memory or NULL on failure
 */
void* platform_realloc(void* ptr, size_t size);

/**
 * @brief Free memory
 * @param ptr Pointer to free
 */
void platform_free(void* ptr);

/**
 * @brief Duplicate string
 * @param str String to duplicate
 * @return Duplicated string or NULL on failure
 */
char* platform_strdup(const char* str);

/* ============================================================================
 * File System Functions
 * ============================================================================ */

/**
 * @brief Check if file exists
 * @param path File path
 * @return true if exists
 */
bool platform_file_exists(const char* path);

/**
 * @brief Check if path is a directory
 * @param path Path
 * @return true if directory
 */
bool platform_is_directory(const char* path);

/**
 * @brief Get file size
 * @param path File path
 * @param size Output size
 * @return Error code
 */
platform_error_t platform_file_size(const char* path, int64_t* size);

/**
 * @brief Create directory
 * @param path Directory path
 * @param recursive Create parent directories if needed
 * @return Error code
 */
platform_error_t platform_mkdir(const char* path, bool recursive);

/**
 * @brief Remove file
 * @param path File path
 * @return Error code
 */
platform_error_t platform_remove(const char* path);

/**
 * @brief Remove directory
 * @param path Directory path
 * @param recursive Remove contents recursively
 * @return Error code
 */
platform_error_t platform_rmdir(const char* path, bool recursive);

/**
 * @brief Rename/move file
 * @param old_path Source path
 * @param new_path Destination path
 * @return Error code
 */
platform_error_t platform_rename(const char* old_path, const char* new_path);

/**
 * @brief Copy file
 * @param src_path Source path
 * @param dst_path Destination path
 * @return Error code
 */
platform_error_t platform_copy_file(const char* src_path, const char* dst_path);

/**
 * @brief Get current working directory
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_getcwd(char* buffer, size_t size);

/**
 * @brief Get temporary directory
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_get_temp_dir(char* buffer, size_t size);

/**
 * @brief Get user home directory
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_get_home_dir(char* buffer, size_t size);

/**
 * @brief Get application data directory
 * @param app_name Application name
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_get_app_data_dir(const char* app_name, char* buffer, size_t size);

/**
 * @brief Normalize path separators
 * @param path Path to normalize (modified in place)
 */
void platform_normalize_path(char* path);

/**
 * @brief Join path components
 * @param buffer Output buffer
 * @param size Buffer size
 * @param path1 First path component
 * @param path2 Second path component
 * @return Error code
 */
platform_error_t platform_path_join(char* buffer, size_t size,
                                    const char* path1, const char* path2);

/**
 * @brief Get file extension
 * @param path File path
 * @return Extension (including dot) or empty string
 */
const char* platform_get_extension(const char* path);

/**
 * @brief Get file name from path
 * @param path File path
 * @return File name portion
 */
const char* platform_get_filename(const char* path);

/**
 * @brief Get directory from path
 * @param path File path
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_get_dirname(const char* path, char* buffer, size_t size);

/* ============================================================================
 * Directory Iterator
 * ============================================================================ */

typedef struct platform_dir_iterator platform_dir_iterator_t;

typedef struct {
    char name[REGISLEX_MAX_PATH];
    bool is_directory;
    int64_t size;
    uint64_t modified_time;
} platform_dir_entry_t;

/**
 * @brief Open directory for iteration
 * @param path Directory path
 * @param iter Output iterator
 * @return Error code
 */
platform_error_t platform_dir_open(const char* path, platform_dir_iterator_t** iter);

/**
 * @brief Get next directory entry
 * @param iter Iterator
 * @param entry Output entry
 * @return Error code (PLATFORM_ERROR_NOT_FOUND when done)
 */
platform_error_t platform_dir_next(platform_dir_iterator_t* iter, platform_dir_entry_t* entry);

/**
 * @brief Close directory iterator
 * @param iter Iterator
 */
void platform_dir_close(platform_dir_iterator_t* iter);

/* ============================================================================
 * Time Functions
 * ============================================================================ */

/**
 * @brief Get current time in milliseconds since epoch
 * @return Milliseconds since Unix epoch
 */
int64_t platform_time_ms(void);

/**
 * @brief Get current time in microseconds since epoch
 * @return Microseconds since Unix epoch
 */
int64_t platform_time_us(void);

/**
 * @brief Get monotonic clock for elapsed time measurement
 * @return Monotonic time in nanoseconds
 */
uint64_t platform_monotonic_ns(void);

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep
 */
void platform_sleep_ms(int ms);

/**
 * @brief Format time as ISO 8601 string
 * @param timestamp Unix timestamp in seconds
 * @param buffer Output buffer
 * @param size Buffer size
 * @param utc Use UTC (true) or local time (false)
 * @return Error code
 */
platform_error_t platform_format_time(int64_t timestamp, char* buffer,
                                      size_t size, bool utc);

/**
 * @brief Parse ISO 8601 time string
 * @param str Time string
 * @param timestamp Output timestamp
 * @return Error code
 */
platform_error_t platform_parse_time(const char* str, int64_t* timestamp);

/* ============================================================================
 * Threading
 * ============================================================================ */

typedef struct platform_thread platform_thread_t;
typedef struct platform_mutex platform_mutex_t;
typedef struct platform_cond platform_cond_t;
typedef struct platform_rwlock platform_rwlock_t;

typedef void* (*platform_thread_func_t)(void* arg);

/**
 * @brief Create and start a thread
 * @param thread Output thread handle
 * @param func Thread function
 * @param arg Argument to pass to function
 * @return Error code
 */
platform_error_t platform_thread_create(platform_thread_t** thread,
                                        platform_thread_func_t func,
                                        void* arg);

/**
 * @brief Wait for thread to complete
 * @param thread Thread handle
 * @param result Output result from thread function
 * @return Error code
 */
platform_error_t platform_thread_join(platform_thread_t* thread, void** result);

/**
 * @brief Detach thread
 * @param thread Thread handle
 * @return Error code
 */
platform_error_t platform_thread_detach(platform_thread_t* thread);

/**
 * @brief Get current thread ID
 * @return Thread ID
 */
uint64_t platform_thread_id(void);

/**
 * @brief Create mutex
 * @param mutex Output mutex handle
 * @return Error code
 */
platform_error_t platform_mutex_create(platform_mutex_t** mutex);

/**
 * @brief Destroy mutex
 * @param mutex Mutex handle
 */
void platform_mutex_destroy(platform_mutex_t* mutex);

/**
 * @brief Lock mutex
 * @param mutex Mutex handle
 * @return Error code
 */
platform_error_t platform_mutex_lock(platform_mutex_t* mutex);

/**
 * @brief Try to lock mutex
 * @param mutex Mutex handle
 * @return Error code (PLATFORM_ERROR_WOULD_BLOCK if not acquired)
 */
platform_error_t platform_mutex_trylock(platform_mutex_t* mutex);

/**
 * @brief Unlock mutex
 * @param mutex Mutex handle
 * @return Error code
 */
platform_error_t platform_mutex_unlock(platform_mutex_t* mutex);

/**
 * @brief Create condition variable
 * @param cond Output condition handle
 * @return Error code
 */
platform_error_t platform_cond_create(platform_cond_t** cond);

/**
 * @brief Destroy condition variable
 * @param cond Condition handle
 */
void platform_cond_destroy(platform_cond_t* cond);

/**
 * @brief Wait on condition variable
 * @param cond Condition handle
 * @param mutex Associated mutex (must be locked)
 * @return Error code
 */
platform_error_t platform_cond_wait(platform_cond_t* cond, platform_mutex_t* mutex);

/**
 * @brief Wait on condition variable with timeout
 * @param cond Condition handle
 * @param mutex Associated mutex (must be locked)
 * @param timeout_ms Timeout in milliseconds
 * @return Error code (PLATFORM_ERROR_TIMEOUT if timed out)
 */
platform_error_t platform_cond_timedwait(platform_cond_t* cond,
                                         platform_mutex_t* mutex,
                                         int timeout_ms);

/**
 * @brief Signal one waiter
 * @param cond Condition handle
 * @return Error code
 */
platform_error_t platform_cond_signal(platform_cond_t* cond);

/**
 * @brief Signal all waiters
 * @param cond Condition handle
 * @return Error code
 */
platform_error_t platform_cond_broadcast(platform_cond_t* cond);

/**
 * @brief Create read-write lock
 * @param rwlock Output rwlock handle
 * @return Error code
 */
platform_error_t platform_rwlock_create(platform_rwlock_t** rwlock);

/**
 * @brief Destroy read-write lock
 * @param rwlock RWLock handle
 */
void platform_rwlock_destroy(platform_rwlock_t* rwlock);

/**
 * @brief Acquire read lock
 * @param rwlock RWLock handle
 * @return Error code
 */
platform_error_t platform_rwlock_rdlock(platform_rwlock_t* rwlock);

/**
 * @brief Acquire write lock
 * @param rwlock RWLock handle
 * @return Error code
 */
platform_error_t platform_rwlock_wrlock(platform_rwlock_t* rwlock);

/**
 * @brief Release lock
 * @param rwlock RWLock handle
 * @return Error code
 */
platform_error_t platform_rwlock_unlock(platform_rwlock_t* rwlock);

/* ============================================================================
 * Atomic Operations
 * ============================================================================ */

typedef struct { volatile int32_t value; } platform_atomic_int32_t;
typedef struct { volatile int64_t value; } platform_atomic_int64_t;
typedef struct { void* volatile value; } platform_atomic_ptr_t;

int32_t platform_atomic_load_32(platform_atomic_int32_t* atomic);
void platform_atomic_store_32(platform_atomic_int32_t* atomic, int32_t value);
int32_t platform_atomic_add_32(platform_atomic_int32_t* atomic, int32_t value);
int32_t platform_atomic_sub_32(platform_atomic_int32_t* atomic, int32_t value);
bool platform_atomic_cas_32(platform_atomic_int32_t* atomic, int32_t expected, int32_t desired);

int64_t platform_atomic_load_64(platform_atomic_int64_t* atomic);
void platform_atomic_store_64(platform_atomic_int64_t* atomic, int64_t value);
int64_t platform_atomic_add_64(platform_atomic_int64_t* atomic, int64_t value);

void* platform_atomic_load_ptr(platform_atomic_ptr_t* atomic);
void platform_atomic_store_ptr(platform_atomic_ptr_t* atomic, void* value);
bool platform_atomic_cas_ptr(platform_atomic_ptr_t* atomic, void* expected, void* desired);

/* ============================================================================
 * Network Functions
 * ============================================================================ */

typedef struct platform_socket platform_socket_t;

typedef enum {
    PLATFORM_SOCKET_TCP = 0,
    PLATFORM_SOCKET_UDP
} platform_socket_type_t;

/**
 * @brief Initialize networking (call once at startup)
 * @return Error code
 */
platform_error_t platform_net_init(void);

/**
 * @brief Cleanup networking (call at shutdown)
 */
void platform_net_cleanup(void);

/**
 * @brief Create socket
 * @param type Socket type
 * @param socket Output socket handle
 * @return Error code
 */
platform_error_t platform_socket_create(platform_socket_type_t type,
                                        platform_socket_t** socket);

/**
 * @brief Close socket
 * @param socket Socket handle
 */
void platform_socket_close(platform_socket_t* socket);

/**
 * @brief Connect to server
 * @param socket Socket handle
 * @param host Host name or IP
 * @param port Port number
 * @return Error code
 */
platform_error_t platform_socket_connect(platform_socket_t* socket,
                                         const char* host,
                                         int port);

/**
 * @brief Bind socket to address
 * @param socket Socket handle
 * @param host Host (NULL for any)
 * @param port Port number
 * @return Error code
 */
platform_error_t platform_socket_bind(platform_socket_t* socket,
                                      const char* host,
                                      int port);

/**
 * @brief Listen for connections
 * @param socket Socket handle
 * @param backlog Connection queue size
 * @return Error code
 */
platform_error_t platform_socket_listen(platform_socket_t* socket, int backlog);

/**
 * @brief Accept connection
 * @param socket Listening socket
 * @param client Output client socket
 * @return Error code
 */
platform_error_t platform_socket_accept(platform_socket_t* socket,
                                        platform_socket_t** client);

/**
 * @brief Send data
 * @param socket Socket handle
 * @param data Data buffer
 * @param size Data size
 * @param sent Output bytes sent
 * @return Error code
 */
platform_error_t platform_socket_send(platform_socket_t* socket,
                                      const void* data,
                                      size_t size,
                                      size_t* sent);

/**
 * @brief Receive data
 * @param socket Socket handle
 * @param buffer Receive buffer
 * @param size Buffer size
 * @param received Output bytes received
 * @return Error code
 */
platform_error_t platform_socket_recv(platform_socket_t* socket,
                                      void* buffer,
                                      size_t size,
                                      size_t* received);

/**
 * @brief Set socket timeout
 * @param socket Socket handle
 * @param send_timeout_ms Send timeout (0 for infinite)
 * @param recv_timeout_ms Receive timeout (0 for infinite)
 * @return Error code
 */
platform_error_t platform_socket_set_timeout(platform_socket_t* socket,
                                             int send_timeout_ms,
                                             int recv_timeout_ms);

/**
 * @brief Set socket to non-blocking mode
 * @param socket Socket handle
 * @param non_blocking true for non-blocking
 * @return Error code
 */
platform_error_t platform_socket_set_nonblocking(platform_socket_t* socket,
                                                 bool non_blocking);

/* ============================================================================
 * Process Functions
 * ============================================================================ */

/**
 * @brief Get process ID
 * @return Process ID
 */
int platform_getpid(void);

/**
 * @brief Get environment variable
 * @param name Variable name
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_getenv(const char* name, char* buffer, size_t size);

/**
 * @brief Set environment variable
 * @param name Variable name
 * @param value Value (NULL to unset)
 * @return Error code
 */
platform_error_t platform_setenv(const char* name, const char* value);

/**
 * @brief Get system hostname
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Error code
 */
platform_error_t platform_get_hostname(char* buffer, size_t size);

/**
 * @brief Get number of CPU cores
 * @return Number of cores
 */
int platform_get_cpu_count(void);

/**
 * @brief Get total system memory
 * @return Total memory in bytes
 */
uint64_t platform_get_total_memory(void);

/**
 * @brief Get available system memory
 * @return Available memory in bytes
 */
uint64_t platform_get_available_memory(void);

/* ============================================================================
 * Random Number Generation
 * ============================================================================ */

/**
 * @brief Generate cryptographically secure random bytes
 * @param buffer Output buffer
 * @param size Number of bytes to generate
 * @return Error code
 */
platform_error_t platform_random_bytes(void* buffer, size_t size);

/**
 * @brief Generate random 32-bit integer
 * @return Random value
 */
uint32_t platform_random_u32(void);

/**
 * @brief Generate random 64-bit integer
 * @return Random value
 */
uint64_t platform_random_u64(void);

/* ============================================================================
 * Dynamic Library Loading
 * ============================================================================ */

typedef struct platform_library platform_library_t;

/**
 * @brief Load dynamic library
 * @param path Library path
 * @param lib Output library handle
 * @return Error code
 */
platform_error_t platform_library_load(const char* path, platform_library_t** lib);

/**
 * @brief Get symbol from library
 * @param lib Library handle
 * @param name Symbol name
 * @param symbol Output symbol pointer
 * @return Error code
 */
platform_error_t platform_library_symbol(platform_library_t* lib,
                                         const char* name,
                                         void** symbol);

/**
 * @brief Unload library
 * @param lib Library handle
 */
void platform_library_unload(platform_library_t* lib);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_PLATFORM_H */
