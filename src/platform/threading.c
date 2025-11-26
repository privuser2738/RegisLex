/**
 * @file threading.c
 * @brief Platform Threading Implementation
 */

#include "platform/platform.h"
#include <stdlib.h>

#ifdef REGISLEX_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/* Mutex implementation */
struct platform_mutex {
#ifdef REGISLEX_PLATFORM_WINDOWS
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mutex;
#endif
};

platform_error_t platform_mutex_create(platform_mutex_t** mutex) {
    if (!mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
    *mutex = (platform_mutex_t*)platform_calloc(1, sizeof(platform_mutex_t));
    if (!*mutex) return PLATFORM_ERROR_OUT_OF_MEMORY;
#ifdef REGISLEX_PLATFORM_WINDOWS
    InitializeCriticalSection(&(*mutex)->cs);
#else
    pthread_mutex_init(&(*mutex)->mutex, NULL);
#endif
    return PLATFORM_OK;
}

void platform_mutex_destroy(platform_mutex_t* mutex) {
    if (!mutex) return;
#ifdef REGISLEX_PLATFORM_WINDOWS
    DeleteCriticalSection(&mutex->cs);
#else
    pthread_mutex_destroy(&mutex->mutex);
#endif
    platform_free(mutex);
}

platform_error_t platform_mutex_lock(platform_mutex_t* mutex) {
    if (!mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    EnterCriticalSection(&mutex->cs);
#else
    pthread_mutex_lock(&mutex->mutex);
#endif
    return PLATFORM_OK;
}

platform_error_t platform_mutex_unlock(platform_mutex_t* mutex) {
    if (!mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    LeaveCriticalSection(&mutex->cs);
#else
    pthread_mutex_unlock(&mutex->mutex);
#endif
    return PLATFORM_OK;
}

platform_error_t platform_mutex_trylock(platform_mutex_t* mutex) {
    if (!mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    return TryEnterCriticalSection(&mutex->cs) ? PLATFORM_OK : PLATFORM_ERROR_WOULD_BLOCK;
#else
    return pthread_mutex_trylock(&mutex->mutex) == 0 ? PLATFORM_OK : PLATFORM_ERROR_WOULD_BLOCK;
#endif
}

/* Thread implementation */
struct platform_thread {
#ifdef REGISLEX_PLATFORM_WINDOWS
    HANDLE handle;
    DWORD id;
#else
    pthread_t thread;
#endif
    platform_thread_func_t func;
    void* arg;
};

#ifdef REGISLEX_PLATFORM_WINDOWS
static DWORD WINAPI thread_wrapper(LPVOID arg) {
    platform_thread_t* t = (platform_thread_t*)arg;
    t->func(t->arg);
    return 0;
}
#else
static void* thread_wrapper(void* arg) {
    platform_thread_t* t = (platform_thread_t*)arg;
    return t->func(t->arg);
}
#endif

platform_error_t platform_thread_create(platform_thread_t** thread, platform_thread_func_t func, void* arg) {
    if (!thread || !func) return PLATFORM_ERROR_INVALID_ARGUMENT;
    *thread = (platform_thread_t*)platform_calloc(1, sizeof(platform_thread_t));
    if (!*thread) return PLATFORM_ERROR_OUT_OF_MEMORY;
    (*thread)->func = func;
    (*thread)->arg = arg;
#ifdef REGISLEX_PLATFORM_WINDOWS
    (*thread)->handle = CreateThread(NULL, 0, thread_wrapper, *thread, 0, &(*thread)->id);
    if (!(*thread)->handle) { platform_free(*thread); return PLATFORM_ERROR; }
#else
    if (pthread_create(&(*thread)->thread, NULL, thread_wrapper, *thread) != 0) {
        platform_free(*thread);
        return PLATFORM_ERROR;
    }
#endif
    return PLATFORM_OK;
}

platform_error_t platform_thread_join(platform_thread_t* thread, void** result) {
    if (!thread) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    if (result) *result = NULL;
#else
    pthread_join(thread->thread, result);
#endif
    platform_free(thread);
    return PLATFORM_OK;
}

platform_error_t platform_thread_detach(platform_thread_t* thread) {
    if (!thread) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    CloseHandle(thread->handle);
#else
    pthread_detach(thread->thread);
#endif
    platform_free(thread);
    return PLATFORM_OK;
}

void platform_sleep_ms(int ms) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
}

/* Condition variable */
struct platform_cond {
#ifdef REGISLEX_PLATFORM_WINDOWS
    CONDITION_VARIABLE cv;
#else
    pthread_cond_t cond;
#endif
};

platform_error_t platform_cond_create(platform_cond_t** cond) {
    if (!cond) return PLATFORM_ERROR_INVALID_ARGUMENT;
    *cond = (platform_cond_t*)platform_calloc(1, sizeof(platform_cond_t));
    if (!*cond) return PLATFORM_ERROR_OUT_OF_MEMORY;
#ifdef REGISLEX_PLATFORM_WINDOWS
    InitializeConditionVariable(&(*cond)->cv);
#else
    pthread_cond_init(&(*cond)->cond, NULL);
#endif
    return PLATFORM_OK;
}

void platform_cond_destroy(platform_cond_t* cond) {
    if (!cond) return;
#ifndef REGISLEX_PLATFORM_WINDOWS
    pthread_cond_destroy(&cond->cond);
#endif
    platform_free(cond);
}

platform_error_t platform_cond_wait(platform_cond_t* cond, platform_mutex_t* mutex) {
    if (!cond || !mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    SleepConditionVariableCS(&cond->cv, &mutex->cs, INFINITE);
#else
    pthread_cond_wait(&cond->cond, &mutex->mutex);
#endif
    return PLATFORM_OK;
}

platform_error_t platform_cond_timedwait(platform_cond_t* cond, platform_mutex_t* mutex, int timeout_ms) {
    if (!cond || !mutex) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    if (!SleepConditionVariableCS(&cond->cv, &mutex->cs, (DWORD)timeout_ms)) {
        return PLATFORM_ERROR_TIMEOUT;
    }
#else
    (void)timeout_ms;
    pthread_cond_wait(&cond->cond, &mutex->mutex);
#endif
    return PLATFORM_OK;
}

platform_error_t platform_cond_signal(platform_cond_t* cond) {
    if (!cond) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WakeConditionVariable(&cond->cv);
#else
    pthread_cond_signal(&cond->cond);
#endif
    return PLATFORM_OK;
}

platform_error_t platform_cond_broadcast(platform_cond_t* cond) {
    if (!cond) return PLATFORM_ERROR_INVALID_ARGUMENT;
#ifdef REGISLEX_PLATFORM_WINDOWS
    WakeAllConditionVariable(&cond->cv);
#else
    pthread_cond_broadcast(&cond->cond);
#endif
    return PLATFORM_OK;
}

/* Read-write lock stubs */
struct platform_rwlock {
    platform_mutex_t* mutex;
};

platform_error_t platform_rwlock_create(platform_rwlock_t** rwlock) {
    if (!rwlock) return PLATFORM_ERROR_INVALID_ARGUMENT;
    *rwlock = (platform_rwlock_t*)platform_calloc(1, sizeof(platform_rwlock_t));
    if (!*rwlock) return PLATFORM_ERROR_OUT_OF_MEMORY;
    return platform_mutex_create(&(*rwlock)->mutex);
}

void platform_rwlock_destroy(platform_rwlock_t* rwlock) {
    if (rwlock) {
        platform_mutex_destroy(rwlock->mutex);
        platform_free(rwlock);
    }
}

platform_error_t platform_rwlock_rdlock(platform_rwlock_t* rwlock) {
    return rwlock ? platform_mutex_lock(rwlock->mutex) : PLATFORM_ERROR_INVALID_ARGUMENT;
}

platform_error_t platform_rwlock_wrlock(platform_rwlock_t* rwlock) {
    return rwlock ? platform_mutex_lock(rwlock->mutex) : PLATFORM_ERROR_INVALID_ARGUMENT;
}

platform_error_t platform_rwlock_unlock(platform_rwlock_t* rwlock) {
    return rwlock ? platform_mutex_unlock(rwlock->mutex) : PLATFORM_ERROR_INVALID_ARGUMENT;
}

/* Thread ID */
uint64_t platform_thread_id(void) {
#ifdef REGISLEX_PLATFORM_WINDOWS
    return (uint64_t)GetCurrentThreadId();
#else
    return (uint64_t)pthread_self();
#endif
}
