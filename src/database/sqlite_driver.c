/**
 * @file sqlite_driver.c
 * @brief SQLite Driver Implementation
 *
 * The main database.c handles SQLite operations.
 * This file provides any additional SQLite-specific utilities.
 */

#include "database/database.h"
#include "platform/platform.h"
#include <string.h>

/* SQLite-specific utilities - most functionality is in database.c */

/**
 * @brief Get SQLite version string
 */
const char* regislex_sqlite_version(void) {
    return "3.45.0";
}
