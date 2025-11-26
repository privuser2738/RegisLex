/**
 * @file error.c
 * @brief RegisLex Error Handling Implementation
 */

#include "regislex/regislex.h"
#include <stdio.h>
#include <string.h>

/* Error message table */
static const char* error_messages[] = {
    "Success",                          /* REGISLEX_OK */
    "General error",                    /* REGISLEX_ERROR */
    "Invalid argument",                 /* REGISLEX_ERROR_INVALID_ARGUMENT */
    "Out of memory",                    /* REGISLEX_ERROR_OUT_OF_MEMORY */
    "Not found",                        /* REGISLEX_ERROR_NOT_FOUND */
    "Already exists",                   /* REGISLEX_ERROR_ALREADY_EXISTS */
    "Permission denied",                /* REGISLEX_ERROR_PERMISSION_DENIED */
    "Database error",                   /* REGISLEX_ERROR_DATABASE */
    "Network error",                    /* REGISLEX_ERROR_NETWORK */
    "I/O error",                        /* REGISLEX_ERROR_IO */
    "Timeout",                          /* REGISLEX_ERROR_TIMEOUT */
    "Authentication failed",            /* REGISLEX_ERROR_AUTH */
    "Validation failed",                /* REGISLEX_ERROR_VALIDATION */
    "Not implemented",                  /* REGISLEX_ERROR_NOT_IMPLEMENTED */
    "Busy",                             /* REGISLEX_ERROR_BUSY */
    "Cancelled",                        /* REGISLEX_ERROR_CANCELLED */
    "Workflow error",                   /* REGISLEX_ERROR_WORKFLOW */
    "Document error",                   /* REGISLEX_ERROR_DOCUMENT */
    "Deadline error",                   /* REGISLEX_ERROR_DEADLINE */
};

REGISLEX_API const char* regislex_error_string(regislex_error_t error) {
    if (error < 0 || error >= (regislex_error_t)(sizeof(error_messages) / sizeof(error_messages[0]))) {
        return "Unknown error";
    }
    return error_messages[error];
}

REGISLEX_API void regislex_error_log(regislex_error_t error, const char* context) {
    fprintf(stderr, "[RegisLex Error] %s: %s\n",
            context ? context : "Unknown",
            regislex_error_string(error));
}
