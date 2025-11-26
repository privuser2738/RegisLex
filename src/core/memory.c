/**
 * @file memory.c
 * @brief RegisLex Memory Management Implementation
 */

#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>

/* Memory allocation tracking (debug builds) */
#ifdef REGISLEX_DEBUG
static size_t total_allocated = 0;
static size_t allocation_count = 0;
#endif

void* regislex_malloc(size_t size) {
    void* ptr = platform_malloc(size);
#ifdef REGISLEX_DEBUG
    if (ptr) {
        total_allocated += size;
        allocation_count++;
    }
#endif
    return ptr;
}

void* regislex_calloc(size_t count, size_t size) {
    void* ptr = platform_calloc(count, size);
#ifdef REGISLEX_DEBUG
    if (ptr) {
        total_allocated += count * size;
        allocation_count++;
    }
#endif
    return ptr;
}

void* regislex_realloc(void* ptr, size_t size) {
    return platform_realloc(ptr, size);
}

void regislex_free(void* ptr) {
    if (ptr) {
#ifdef REGISLEX_DEBUG
        allocation_count--;
#endif
        platform_free(ptr);
    }
}

char* regislex_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = (char*)regislex_malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

#ifdef REGISLEX_DEBUG
size_t regislex_memory_allocated(void) {
    return total_allocated;
}

size_t regislex_memory_allocation_count(void) {
    return allocation_count;
}
#endif
