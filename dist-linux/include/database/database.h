/**
 * @file database.h
 * @brief Database Abstraction Layer
 *
 * Provides a unified interface for database operations with
 * support for SQLite, PostgreSQL, and other backends.
 */

#ifndef REGISLEX_DATABASE_H
#define REGISLEX_DATABASE_H

#include "regislex/regislex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Types
 * ============================================================================ */

typedef struct regislex_db_context regislex_db_context_t;
typedef struct regislex_db_stmt regislex_db_stmt_t;
typedef struct regislex_db_result regislex_db_result_t;
typedef struct regislex_db_transaction regislex_db_transaction_t;

/**
 * @brief Database column type
 */
typedef enum {
    REGISLEX_DB_TYPE_NULL = 0,
    REGISLEX_DB_TYPE_INTEGER,
    REGISLEX_DB_TYPE_REAL,
    REGISLEX_DB_TYPE_TEXT,
    REGISLEX_DB_TYPE_BLOB,
    REGISLEX_DB_TYPE_DATETIME
} regislex_db_type_t;

/**
 * @brief Database value
 */
typedef struct {
    regislex_db_type_t type;
    union {
        int64_t integer;
        double real;
        struct {
            char* data;
            size_t length;
        } text;
        struct {
            void* data;
            size_t length;
        } blob;
    } value;
} regislex_db_value_t;

/**
 * @brief Query result row
 */
typedef struct {
    int column_count;
    char** column_names;
    regislex_db_value_t* values;
} regislex_db_row_t;

/* ============================================================================
 * Connection Functions
 * ============================================================================ */

/**
 * @brief Initialize database connection
 * @param config Database configuration
 * @param ctx Output context
 * @return Error code
 */
regislex_error_t regislex_db_init(const regislex_db_config_t* config,
                                  regislex_db_context_t** ctx);

/**
 * @brief Shutdown database connection
 * @param ctx Database context
 */
void regislex_db_shutdown(regislex_db_context_t* ctx);

/**
 * @brief Check if database is connected
 * @param ctx Database context
 * @return true if connected
 */
bool regislex_db_is_connected(regislex_db_context_t* ctx);

/**
 * @brief Get last database error message
 * @param ctx Database context
 * @return Error message
 */
const char* regislex_db_error(regislex_db_context_t* ctx);

/* ============================================================================
 * Migration Functions
 * ============================================================================ */

/**
 * @brief Run database migrations
 * @param ctx Database context
 * @return Error code
 */
regislex_error_t regislex_db_migrate(regislex_db_context_t* ctx);

/**
 * @brief Get current migration version
 * @param ctx Database context
 * @param version Output version number
 * @return Error code
 */
regislex_error_t regislex_db_migration_version(regislex_db_context_t* ctx,
                                               int* version);

/* ============================================================================
 * Transaction Functions
 * ============================================================================ */

/**
 * @brief Begin a transaction
 * @param ctx Database context
 * @param tx Output transaction handle
 * @return Error code
 */
regislex_error_t regislex_db_begin(regislex_db_context_t* ctx,
                                   regislex_db_transaction_t** tx);

/**
 * @brief Commit a transaction
 * @param tx Transaction handle
 * @return Error code
 */
regislex_error_t regislex_db_commit(regislex_db_transaction_t* tx);

/**
 * @brief Rollback a transaction
 * @param tx Transaction handle
 * @return Error code
 */
regislex_error_t regislex_db_rollback(regislex_db_transaction_t* tx);

/* ============================================================================
 * Query Execution Functions
 * ============================================================================ */

/**
 * @brief Execute a SQL statement (no results)
 * @param ctx Database context
 * @param sql SQL statement
 * @return Error code
 */
regislex_error_t regislex_db_exec(regislex_db_context_t* ctx, const char* sql);

/**
 * @brief Execute a SQL statement in transaction (no results)
 * @param tx Transaction handle
 * @param sql SQL statement
 * @return Error code
 */
regislex_error_t regislex_db_exec_tx(regislex_db_transaction_t* tx, const char* sql);

/**
 * @brief Prepare a SQL statement
 * @param ctx Database context
 * @param sql SQL statement
 * @param stmt Output statement handle
 * @return Error code
 */
regislex_error_t regislex_db_prepare(regislex_db_context_t* ctx,
                                     const char* sql,
                                     regislex_db_stmt_t** stmt);

/**
 * @brief Finalize (free) a prepared statement
 * @param stmt Statement handle
 */
void regislex_db_finalize(regislex_db_stmt_t* stmt);

/**
 * @brief Reset a prepared statement for re-execution
 * @param stmt Statement handle
 * @return Error code
 */
regislex_error_t regislex_db_reset(regislex_db_stmt_t* stmt);

/* ============================================================================
 * Parameter Binding Functions
 * ============================================================================ */

/**
 * @brief Bind NULL parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @return Error code
 */
regislex_error_t regislex_db_bind_null(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Bind integer parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Integer value
 * @return Error code
 */
regislex_error_t regislex_db_bind_int(regislex_db_stmt_t* stmt, int index, int64_t value);

/**
 * @brief Bind real/double parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Double value
 * @return Error code
 */
regislex_error_t regislex_db_bind_real(regislex_db_stmt_t* stmt, int index, double value);

/**
 * @brief Bind text parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Text value
 * @return Error code
 */
regislex_error_t regislex_db_bind_text(regislex_db_stmt_t* stmt, int index, const char* value);

/**
 * @brief Bind blob parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param value Blob data
 * @param size Blob size
 * @return Error code
 */
regislex_error_t regislex_db_bind_blob(regislex_db_stmt_t* stmt, int index,
                                       const void* value, size_t size);

/**
 * @brief Bind UUID parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param uuid UUID value
 * @return Error code
 */
regislex_error_t regislex_db_bind_uuid(regislex_db_stmt_t* stmt, int index,
                                       const regislex_uuid_t* uuid);

/**
 * @brief Bind datetime parameter
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param dt Datetime value
 * @return Error code
 */
regislex_error_t regislex_db_bind_datetime(regislex_db_stmt_t* stmt, int index,
                                           const regislex_datetime_t* dt);

/**
 * @brief Bind money parameter (stores as integer cents)
 * @param stmt Statement handle
 * @param index Parameter index (1-based)
 * @param money Money value
 * @return Error code
 */
regislex_error_t regislex_db_bind_money(regislex_db_stmt_t* stmt, int index,
                                        const regislex_money_t* money);

/* ============================================================================
 * Result Functions
 * ============================================================================ */

/**
 * @brief Step through query results
 * @param stmt Statement handle
 * @return REGISLEX_OK for next row, REGISLEX_ERROR_NOT_FOUND when done, error otherwise
 */
regislex_error_t regislex_db_step(regislex_db_stmt_t* stmt);

/**
 * @brief Get column count
 * @param stmt Statement handle
 * @return Column count
 */
int regislex_db_column_count(regislex_db_stmt_t* stmt);

/**
 * @brief Get column name
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Column name
 */
const char* regislex_db_column_name(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Get column type
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Column type
 */
regislex_db_type_t regislex_db_column_type(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Check if column is NULL
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return true if NULL
 */
bool regislex_db_column_is_null(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Get integer column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Integer value
 */
int64_t regislex_db_column_int(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Get real/double column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Double value
 */
double regislex_db_column_real(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Get text column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @return Text value (valid until next step/finalize)
 */
const char* regislex_db_column_text(regislex_db_stmt_t* stmt, int index);

/**
 * @brief Get blob column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @param size Output size
 * @return Blob data (valid until next step/finalize)
 */
const void* regislex_db_column_blob(regislex_db_stmt_t* stmt, int index, size_t* size);

/**
 * @brief Get UUID column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @param uuid Output UUID
 * @return Error code
 */
regislex_error_t regislex_db_column_uuid(regislex_db_stmt_t* stmt, int index,
                                         regislex_uuid_t* uuid);

/**
 * @brief Get datetime column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @param dt Output datetime
 * @return Error code
 */
regislex_error_t regislex_db_column_datetime(regislex_db_stmt_t* stmt, int index,
                                             regislex_datetime_t* dt);

/**
 * @brief Get money column value
 * @param stmt Statement handle
 * @param index Column index (0-based)
 * @param money Output money
 * @return Error code
 */
regislex_error_t regislex_db_column_money(regislex_db_stmt_t* stmt, int index,
                                          regislex_money_t* money);

/**
 * @brief Get last inserted row ID
 * @param ctx Database context
 * @return Row ID
 */
int64_t regislex_db_last_insert_id(regislex_db_context_t* ctx);

/**
 * @brief Get number of rows affected by last statement
 * @param ctx Database context
 * @return Row count
 */
int regislex_db_changes(regislex_db_context_t* ctx);

/* ============================================================================
 * Query Builder Functions
 * ============================================================================ */

typedef struct regislex_query_builder regislex_query_builder_t;

/**
 * @brief Create a SELECT query builder
 * @param ctx Database context
 * @param table Table name
 * @return Query builder
 */
regislex_query_builder_t* regislex_db_select(regislex_db_context_t* ctx,
                                              const char* table);

/**
 * @brief Create an INSERT query builder
 * @param ctx Database context
 * @param table Table name
 * @return Query builder
 */
regislex_query_builder_t* regislex_db_insert(regislex_db_context_t* ctx,
                                              const char* table);

/**
 * @brief Create an UPDATE query builder
 * @param ctx Database context
 * @param table Table name
 * @return Query builder
 */
regislex_query_builder_t* regislex_db_update(regislex_db_context_t* ctx,
                                              const char* table);

/**
 * @brief Create a DELETE query builder
 * @param ctx Database context
 * @param table Table name
 * @return Query builder
 */
regislex_query_builder_t* regislex_db_delete(regislex_db_context_t* ctx,
                                              const char* table);

/**
 * @brief Add columns to query
 * @param qb Query builder
 * @param columns Comma-separated column names
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_columns(regislex_query_builder_t* qb,
                                               const char* columns);

/**
 * @brief Add SET clause for UPDATE
 * @param qb Query builder
 * @param column Column name
 * @param value Value (use ? for parameter)
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_set(regislex_query_builder_t* qb,
                                           const char* column,
                                           const char* value);

/**
 * @brief Add VALUES for INSERT
 * @param qb Query builder
 * @param values Comma-separated values (use ? for parameters)
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_values(regislex_query_builder_t* qb,
                                              const char* values);

/**
 * @brief Add WHERE clause
 * @param qb Query builder
 * @param condition Condition string
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_where(regislex_query_builder_t* qb,
                                             const char* condition);

/**
 * @brief Add AND condition
 * @param qb Query builder
 * @param condition Condition string
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_and(regislex_query_builder_t* qb,
                                           const char* condition);

/**
 * @brief Add OR condition
 * @param qb Query builder
 * @param condition Condition string
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_or(regislex_query_builder_t* qb,
                                          const char* condition);

/**
 * @brief Add ORDER BY clause
 * @param qb Query builder
 * @param column Column name
 * @param desc true for descending
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_order_by(regislex_query_builder_t* qb,
                                                const char* column,
                                                bool desc);

/**
 * @brief Add LIMIT clause
 * @param qb Query builder
 * @param limit Maximum rows
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_limit(regislex_query_builder_t* qb, int limit);

/**
 * @brief Add OFFSET clause
 * @param qb Query builder
 * @param offset Row offset
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_offset(regislex_query_builder_t* qb, int offset);

/**
 * @brief Add JOIN clause
 * @param qb Query builder
 * @param table Table to join
 * @param condition Join condition
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_join(regislex_query_builder_t* qb,
                                            const char* table,
                                            const char* condition);

/**
 * @brief Add LEFT JOIN clause
 * @param qb Query builder
 * @param table Table to join
 * @param condition Join condition
 * @return Query builder
 */
regislex_query_builder_t* regislex_qb_left_join(regislex_query_builder_t* qb,
                                                 const char* table,
                                                 const char* condition);

/**
 * @brief Build the query string
 * @param qb Query builder
 * @return SQL string (owned by builder)
 */
const char* regislex_qb_build(regislex_query_builder_t* qb);

/**
 * @brief Execute the query and return prepared statement
 * @param qb Query builder
 * @param stmt Output statement
 * @return Error code
 */
regislex_error_t regislex_qb_execute(regislex_query_builder_t* qb,
                                      regislex_db_stmt_t** stmt);

/**
 * @brief Free query builder
 * @param qb Query builder
 */
void regislex_qb_free(regislex_query_builder_t* qb);

#ifdef __cplusplus
}
#endif

#endif /* REGISLEX_DATABASE_H */
