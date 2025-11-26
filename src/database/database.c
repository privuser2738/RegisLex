/**
 * @file database.c
 * @brief Database Abstraction Layer Implementation
 */

#include "database/database.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SQLite includes - will use bundled or system SQLite */
#include "sqlite3.h"

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

struct regislex_db_context {
    char type[32];
    sqlite3* sqlite_db;
    char last_error[1024];
    bool connected;
    platform_mutex_t* mutex;
};

struct regislex_db_stmt {
    regislex_db_context_t* ctx;
    sqlite3_stmt* sqlite_stmt;
    int param_count;
    int column_count;
};

struct regislex_db_transaction {
    regislex_db_context_t* ctx;
    bool active;
};

struct regislex_query_builder {
    regislex_db_context_t* ctx;
    char type[16];      /* SELECT, INSERT, UPDATE, DELETE */
    char table[256];
    char columns[4096];
    char values[4096];
    char set_clause[4096];
    char where_clause[4096];
    char join_clause[4096];
    char order_by[512];
    int limit;
    int offset;
    char sql[16384];
};

/* ============================================================================
 * Error Handling
 * ============================================================================ */

static void set_db_error(regislex_db_context_t* ctx, const char* msg) {
    if (!ctx) return;
    strncpy(ctx->last_error, msg, sizeof(ctx->last_error) - 1);
}

static void set_sqlite_error(regislex_db_context_t* ctx) {
    if (!ctx || !ctx->sqlite_db) return;
    strncpy(ctx->last_error, sqlite3_errmsg(ctx->sqlite_db),
            sizeof(ctx->last_error) - 1);
}

const char* regislex_db_error(regislex_db_context_t* ctx) {
    if (!ctx) return "Invalid context";
    return ctx->last_error;
}

/* ============================================================================
 * Connection Functions
 * ============================================================================ */

regislex_error_t regislex_db_init(const regislex_db_config_t* config,
                                  regislex_db_context_t** ctx) {
    if (!config || !ctx) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    *ctx = (regislex_db_context_t*)platform_calloc(1, sizeof(regislex_db_context_t));
    if (!*ctx) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    strncpy((*ctx)->type, config->type, sizeof((*ctx)->type) - 1);

    if (platform_mutex_create(&(*ctx)->mutex) != PLATFORM_OK) {
        platform_free(*ctx);
        *ctx = NULL;
        return REGISLEX_ERROR;
    }

    /* SQLite connection */
    if (strcmp(config->type, "sqlite") == 0) {
        int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
        int rc = sqlite3_open_v2(config->database, &(*ctx)->sqlite_db, flags, NULL);

        if (rc != SQLITE_OK) {
            set_sqlite_error(*ctx);
            platform_mutex_destroy((*ctx)->mutex);
            platform_free(*ctx);
            *ctx = NULL;
            return REGISLEX_ERROR_DATABASE;
        }

        /* Enable foreign keys */
        sqlite3_exec((*ctx)->sqlite_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

        /* Set journal mode to WAL for better concurrency */
        sqlite3_exec((*ctx)->sqlite_db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);

        /* Set busy timeout */
        sqlite3_busy_timeout((*ctx)->sqlite_db, config->timeout_seconds * 1000);

        (*ctx)->connected = true;
    } else {
        set_db_error(*ctx, "Unsupported database type");
        platform_mutex_destroy((*ctx)->mutex);
        platform_free(*ctx);
        *ctx = NULL;
        return REGISLEX_ERROR_NOT_SUPPORTED;
    }

    return REGISLEX_OK;
}

void regislex_db_shutdown(regislex_db_context_t* ctx) {
    if (!ctx) return;

    if (ctx->sqlite_db) {
        sqlite3_close(ctx->sqlite_db);
        ctx->sqlite_db = NULL;
    }

    if (ctx->mutex) {
        platform_mutex_destroy(ctx->mutex);
        ctx->mutex = NULL;
    }

    ctx->connected = false;
    platform_free(ctx);
}

bool regislex_db_is_connected(regislex_db_context_t* ctx) {
    return ctx && ctx->connected;
}

/* ============================================================================
 * Migration Functions
 * ============================================================================ */

static const char* MIGRATION_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS _migrations ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  version INTEGER NOT NULL,"
    "  name TEXT NOT NULL,"
    "  applied_at TEXT NOT NULL"
    ");";

/* Database schema migrations */
static const char* MIGRATIONS[] = {
    /* Migration 1: Core tables */
    "CREATE TABLE IF NOT EXISTS users ("
    "  id TEXT PRIMARY KEY,"
    "  username TEXT UNIQUE NOT NULL,"
    "  email TEXT UNIQUE NOT NULL,"
    "  password_hash TEXT NOT NULL,"
    "  full_name TEXT,"
    "  role TEXT DEFAULT 'user',"
    "  is_active INTEGER DEFAULT 1,"
    "  created_at TEXT NOT NULL,"
    "  last_login TEXT"
    ");"
    "CREATE INDEX idx_users_username ON users(username);"
    "CREATE INDEX idx_users_email ON users(email);",

    /* Migration 2: Cases table */
    "CREATE TABLE IF NOT EXISTS cases ("
    "  id TEXT PRIMARY KEY,"
    "  case_number TEXT UNIQUE NOT NULL,"
    "  title TEXT NOT NULL,"
    "  short_title TEXT,"
    "  description TEXT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER DEFAULT 0,"
    "  priority INTEGER DEFAULT 1,"
    "  outcome INTEGER DEFAULT 0,"
    "  court_name TEXT,"
    "  court_division TEXT,"
    "  docket_number TEXT,"
    "  internal_reference TEXT,"
    "  client_reference TEXT,"
    "  estimated_value INTEGER DEFAULT 0,"
    "  settlement_amount INTEGER DEFAULT 0,"
    "  filed_date TEXT,"
    "  trial_date TEXT,"
    "  closed_date TEXT,"
    "  statute_of_limitations TEXT,"
    "  lead_attorney_id TEXT REFERENCES users(id),"
    "  assigned_to_id TEXT REFERENCES users(id),"
    "  parent_case_id TEXT REFERENCES cases(id),"
    "  tags TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id),"
    "  updated_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_cases_case_number ON cases(case_number);"
    "CREATE INDEX idx_cases_status ON cases(status);"
    "CREATE INDEX idx_cases_assigned_to ON cases(assigned_to_id);",

    /* Migration 3: Parties table */
    "CREATE TABLE IF NOT EXISTS parties ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT NOT NULL REFERENCES cases(id) ON DELETE CASCADE,"
    "  name TEXT NOT NULL,"
    "  display_name TEXT,"
    "  type INTEGER NOT NULL,"
    "  role INTEGER NOT NULL,"
    "  address_line1 TEXT,"
    "  address_line2 TEXT,"
    "  city TEXT,"
    "  state TEXT,"
    "  postal_code TEXT,"
    "  country TEXT,"
    "  phone TEXT,"
    "  email TEXT,"
    "  attorney_name TEXT,"
    "  attorney_firm TEXT,"
    "  bar_number TEXT,"
    "  is_primary INTEGER DEFAULT 0,"
    "  notes TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL"
    ");"
    "CREATE INDEX idx_parties_case_id ON parties(case_id);",

    /* Migration 4: Deadlines table */
    "CREATE TABLE IF NOT EXISTS deadlines ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT REFERENCES cases(id) ON DELETE CASCADE,"
    "  matter_id TEXT,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER DEFAULT 0,"
    "  priority INTEGER DEFAULT 1,"
    "  due_date TEXT NOT NULL,"
    "  start_date TEXT,"
    "  is_all_day INTEGER DEFAULT 0,"
    "  duration_minutes INTEGER DEFAULT 0,"
    "  recurrence INTEGER DEFAULT 0,"
    "  assigned_to_id TEXT REFERENCES users(id),"
    "  rule_reference TEXT,"
    "  days_from_trigger INTEGER,"
    "  count_business_days INTEGER DEFAULT 0,"
    "  completed_at TEXT,"
    "  completed_by TEXT REFERENCES users(id),"
    "  completion_notes TEXT,"
    "  location TEXT,"
    "  tags TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_deadlines_case_id ON deadlines(case_id);"
    "CREATE INDEX idx_deadlines_due_date ON deadlines(due_date);"
    "CREATE INDEX idx_deadlines_status ON deadlines(status);",

    /* Migration 5: Reminders table */
    "CREATE TABLE IF NOT EXISTS reminders ("
    "  id TEXT PRIMARY KEY,"
    "  deadline_id TEXT NOT NULL REFERENCES deadlines(id) ON DELETE CASCADE,"
    "  user_id TEXT NOT NULL REFERENCES users(id),"
    "  type INTEGER NOT NULL,"
    "  minutes_before INTEGER NOT NULL,"
    "  is_sent INTEGER DEFAULT 0,"
    "  send_at TEXT NOT NULL,"
    "  sent_at TEXT,"
    "  message TEXT,"
    "  is_active INTEGER DEFAULT 1,"
    "  created_at TEXT NOT NULL"
    ");"
    "CREATE INDEX idx_reminders_deadline_id ON reminders(deadline_id);"
    "CREATE INDEX idx_reminders_send_at ON reminders(send_at);",

    /* Migration 6: Documents table */
    "CREATE TABLE IF NOT EXISTS documents ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT REFERENCES cases(id) ON DELETE SET NULL,"
    "  matter_id TEXT,"
    "  folder_id TEXT REFERENCES folders(id),"
    "  name TEXT NOT NULL,"
    "  display_name TEXT,"
    "  description TEXT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER DEFAULT 0,"
    "  access_level INTEGER DEFAULT 1,"
    "  current_version INTEGER DEFAULT 1,"
    "  file_name TEXT NOT NULL,"
    "  mime_type TEXT,"
    "  file_size INTEGER DEFAULT 0,"
    "  storage_path TEXT NOT NULL,"
    "  checksum TEXT,"
    "  tags TEXT,"
    "  bates_number TEXT,"
    "  exhibit_number TEXT,"
    "  filed_date TEXT,"
    "  is_locked INTEGER DEFAULT 0,"
    "  locked_by TEXT REFERENCES users(id),"
    "  locked_at TEXT,"
    "  is_encrypted INTEGER DEFAULT 0,"
    "  extracted_text TEXT,"
    "  ocr_processed INTEGER DEFAULT 0,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id),"
    "  updated_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_documents_case_id ON documents(case_id);"
    "CREATE INDEX idx_documents_folder_id ON documents(folder_id);"
    "CREATE INDEX idx_documents_name ON documents(name);",

    /* Migration 7: Document versions table */
    "CREATE TABLE IF NOT EXISTS document_versions ("
    "  id TEXT PRIMARY KEY,"
    "  document_id TEXT NOT NULL REFERENCES documents(id) ON DELETE CASCADE,"
    "  version_number INTEGER NOT NULL,"
    "  version_label TEXT,"
    "  change_summary TEXT,"
    "  storage_path TEXT NOT NULL,"
    "  checksum TEXT,"
    "  file_size INTEGER,"
    "  mime_type TEXT,"
    "  is_current INTEGER DEFAULT 0,"
    "  created_by TEXT REFERENCES users(id),"
    "  created_at TEXT NOT NULL"
    ");"
    "CREATE INDEX idx_doc_versions_doc_id ON document_versions(document_id);",

    /* Migration 8: Folders table */
    "CREATE TABLE IF NOT EXISTS folders ("
    "  id TEXT PRIMARY KEY,"
    "  parent_id TEXT REFERENCES folders(id),"
    "  case_id TEXT REFERENCES cases(id) ON DELETE CASCADE,"
    "  name TEXT NOT NULL,"
    "  path TEXT NOT NULL,"
    "  description TEXT,"
    "  access_level INTEGER DEFAULT 1,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_folders_parent_id ON folders(parent_id);"
    "CREATE INDEX idx_folders_case_id ON folders(case_id);",

    /* Migration 9: Workflows table */
    "CREATE TABLE IF NOT EXISTS workflows ("
    "  id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  description TEXT,"
    "  category TEXT,"
    "  status INTEGER DEFAULT 0,"
    "  version INTEGER DEFAULT 1,"
    "  run_once INTEGER DEFAULT 0,"
    "  allow_parallel INTEGER DEFAULT 0,"
    "  timeout_minutes INTEGER DEFAULT 60,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_workflows_status ON workflows(status);",

    /* Migration 10: Tasks table */
    "CREATE TABLE IF NOT EXISTS tasks ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT REFERENCES cases(id) ON DELETE SET NULL,"
    "  matter_id TEXT,"
    "  workflow_run_id TEXT,"
    "  parent_task_id TEXT REFERENCES tasks(id),"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  status INTEGER DEFAULT 0,"
    "  priority INTEGER DEFAULT 1,"
    "  assigned_to_id TEXT REFERENCES users(id),"
    "  assigned_by TEXT REFERENCES users(id),"
    "  due_date TEXT,"
    "  estimated_minutes INTEGER,"
    "  actual_minutes INTEGER,"
    "  percent_complete INTEGER DEFAULT 0,"
    "  completion_notes TEXT,"
    "  requires_approval INTEGER DEFAULT 0,"
    "  approver_id TEXT REFERENCES users(id),"
    "  approved_at TEXT,"
    "  started_at TEXT,"
    "  completed_at TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_tasks_case_id ON tasks(case_id);"
    "CREATE INDEX idx_tasks_assigned_to ON tasks(assigned_to_id);"
    "CREATE INDEX idx_tasks_status ON tasks(status);",

    /* Migration 11: Legislation table */
    "CREATE TABLE IF NOT EXISTS legislation ("
    "  id TEXT PRIMARY KEY,"
    "  bill_number TEXT NOT NULL,"
    "  title TEXT NOT NULL,"
    "  short_title TEXT,"
    "  summary TEXT,"
    "  full_text_url TEXT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER DEFAULT 0,"
    "  gov_level INTEGER NOT NULL,"
    "  jurisdiction TEXT NOT NULL,"
    "  chamber_of_origin TEXT,"
    "  session TEXT,"
    "  primary_sponsor_id TEXT,"
    "  subjects TEXT,"
    "  keywords TEXT,"
    "  introduced_date TEXT,"
    "  last_action_date TEXT,"
    "  effective_date TEXT,"
    "  is_tracked INTEGER DEFAULT 0,"
    "  position INTEGER DEFAULT 0,"
    "  position_notes TEXT,"
    "  priority INTEGER DEFAULT 1,"
    "  assigned_to_id TEXT REFERENCES users(id),"
    "  external_id TEXT,"
    "  source_url TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL"
    ");"
    "CREATE INDEX idx_legislation_bill_number ON legislation(bill_number);"
    "CREATE INDEX idx_legislation_jurisdiction ON legislation(jurisdiction);"
    "CREATE INDEX idx_legislation_is_tracked ON legislation(is_tracked);",

    /* Migration 12: Contracts table */
    "CREATE TABLE IF NOT EXISTS contracts ("
    "  id TEXT PRIMARY KEY,"
    "  contract_number TEXT UNIQUE,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER DEFAULT 0,"
    "  vendor_id TEXT REFERENCES vendors(id),"
    "  counterparty_name TEXT,"
    "  case_id TEXT REFERENCES cases(id),"
    "  effective_date TEXT,"
    "  expiration_date TEXT,"
    "  execution_date TEXT,"
    "  auto_renewal INTEGER DEFAULT 0,"
    "  renewal_term_months INTEGER,"
    "  total_value INTEGER DEFAULT 0,"
    "  annual_value INTEGER DEFAULT 0,"
    "  billing_type INTEGER DEFAULT 0,"
    "  document_id TEXT REFERENCES documents(id),"
    "  risk_level INTEGER DEFAULT 0,"
    "  owner_id TEXT REFERENCES users(id),"
    "  department TEXT,"
    "  tags TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_contracts_vendor_id ON contracts(vendor_id);"
    "CREATE INDEX idx_contracts_status ON contracts(status);"
    "CREATE INDEX idx_contracts_expiration ON contracts(expiration_date);",

    /* Migration 13: Vendors table */
    "CREATE TABLE IF NOT EXISTS vendors ("
    "  id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  legal_name TEXT,"
    "  tax_id TEXT,"
    "  status INTEGER DEFAULT 0,"
    "  vendor_type TEXT,"
    "  address_line1 TEXT,"
    "  city TEXT,"
    "  state TEXT,"
    "  postal_code TEXT,"
    "  country TEXT,"
    "  phone TEXT,"
    "  email TEXT,"
    "  website TEXT,"
    "  practice_areas TEXT,"
    "  quality_rating INTEGER,"
    "  payment_terms TEXT,"
    "  primary_contact TEXT,"
    "  total_spend_ytd INTEGER DEFAULT 0,"
    "  total_spend_lifetime INTEGER DEFAULT 0,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_vendors_name ON vendors(name);"
    "CREATE INDEX idx_vendors_status ON vendors(status);",

    /* Migration 14: Invoices table */
    "CREATE TABLE IF NOT EXISTS invoices ("
    "  id TEXT PRIMARY KEY,"
    "  vendor_id TEXT NOT NULL REFERENCES vendors(id),"
    "  case_id TEXT REFERENCES cases(id),"
    "  matter_id TEXT,"
    "  invoice_number TEXT NOT NULL,"
    "  vendor_invoice_number TEXT,"
    "  status INTEGER DEFAULT 0,"
    "  invoice_date TEXT NOT NULL,"
    "  received_date TEXT,"
    "  due_date TEXT,"
    "  paid_date TEXT,"
    "  subtotal_fees INTEGER DEFAULT 0,"
    "  subtotal_expenses INTEGER DEFAULT 0,"
    "  adjustments INTEGER DEFAULT 0,"
    "  taxes INTEGER DEFAULT 0,"
    "  total_amount INTEGER NOT NULL,"
    "  amount_paid INTEGER DEFAULT 0,"
    "  total_hours REAL DEFAULT 0,"
    "  reviewed_by TEXT REFERENCES users(id),"
    "  reviewed_at TEXT,"
    "  review_notes TEXT,"
    "  payment_reference TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_invoices_vendor_id ON invoices(vendor_id);"
    "CREATE INDEX idx_invoices_case_id ON invoices(case_id);"
    "CREATE INDEX idx_invoices_status ON invoices(status);",

    /* Migration 15: Risks table */
    "CREATE TABLE IF NOT EXISTS risks ("
    "  id TEXT PRIMARY KEY,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  category INTEGER NOT NULL,"
    "  level INTEGER DEFAULT 0,"
    "  status INTEGER DEFAULT 0,"
    "  likelihood_score INTEGER DEFAULT 0,"
    "  impact_score INTEGER DEFAULT 0,"
    "  potential_exposure INTEGER DEFAULT 0,"
    "  case_id TEXT REFERENCES cases(id),"
    "  contract_id TEXT REFERENCES contracts(id),"
    "  mitigation_strategy TEXT,"
    "  contingency_plan TEXT,"
    "  owner_id TEXT REFERENCES users(id),"
    "  department TEXT,"
    "  identified_date TEXT,"
    "  last_assessed TEXT,"
    "  next_review TEXT,"
    "  created_at TEXT NOT NULL,"
    "  updated_at TEXT NOT NULL,"
    "  created_by TEXT REFERENCES users(id)"
    ");"
    "CREATE INDEX idx_risks_level ON risks(level);"
    "CREATE INDEX idx_risks_status ON risks(status);",

    /* Migration 16: Audit log table */
    "CREATE TABLE IF NOT EXISTS audit_log ("
    "  id TEXT PRIMARY KEY,"
    "  user_id TEXT REFERENCES users(id),"
    "  action TEXT NOT NULL,"
    "  entity_type TEXT NOT NULL,"
    "  entity_id TEXT,"
    "  old_values TEXT,"
    "  new_values TEXT,"
    "  ip_address TEXT,"
    "  user_agent TEXT,"
    "  created_at TEXT NOT NULL"
    ");"
    "CREATE INDEX idx_audit_log_user_id ON audit_log(user_id);"
    "CREATE INDEX idx_audit_log_entity ON audit_log(entity_type, entity_id);"
    "CREATE INDEX idx_audit_log_created_at ON audit_log(created_at);",

    NULL
};

regislex_error_t regislex_db_migrate(regislex_db_context_t* ctx) {
    if (!ctx || !ctx->connected) {
        return REGISLEX_ERROR_NOT_INITIALIZED;
    }

    /* Create migrations table if not exists */
    char* err_msg = NULL;
    int rc = sqlite3_exec(ctx->sqlite_db, MIGRATION_TABLE_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        set_db_error(ctx, err_msg);
        sqlite3_free(err_msg);
        return REGISLEX_ERROR_DATABASE;
    }

    /* Get current version */
    int current_version = 0;
    regislex_db_migration_version(ctx, &current_version);

    /* Apply pending migrations */
    int migration_num = 0;
    while (MIGRATIONS[migration_num] != NULL) {
        migration_num++;
        if (migration_num <= current_version) {
            continue;
        }

        /* Begin transaction */
        rc = sqlite3_exec(ctx->sqlite_db, "BEGIN TRANSACTION;", NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            set_db_error(ctx, err_msg);
            sqlite3_free(err_msg);
            return REGISLEX_ERROR_DATABASE;
        }

        /* Execute migration */
        rc = sqlite3_exec(ctx->sqlite_db, MIGRATIONS[migration_num - 1], NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            set_db_error(ctx, err_msg);
            sqlite3_free(err_msg);
            sqlite3_exec(ctx->sqlite_db, "ROLLBACK;", NULL, NULL, NULL);
            return REGISLEX_ERROR_DATABASE;
        }

        /* Record migration */
        char record_sql[512];
        char timestamp[32];
        platform_format_time(platform_time_ms() / 1000, timestamp, sizeof(timestamp), true);

        snprintf(record_sql, sizeof(record_sql),
                "INSERT INTO _migrations (version, name, applied_at) VALUES (%d, 'migration_%d', '%s');",
                migration_num, migration_num, timestamp);

        rc = sqlite3_exec(ctx->sqlite_db, record_sql, NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            set_db_error(ctx, err_msg);
            sqlite3_free(err_msg);
            sqlite3_exec(ctx->sqlite_db, "ROLLBACK;", NULL, NULL, NULL);
            return REGISLEX_ERROR_DATABASE;
        }

        /* Commit transaction */
        rc = sqlite3_exec(ctx->sqlite_db, "COMMIT;", NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            set_db_error(ctx, err_msg);
            sqlite3_free(err_msg);
            return REGISLEX_ERROR_DATABASE;
        }
    }

    return REGISLEX_OK;
}

regislex_error_t regislex_db_migration_version(regislex_db_context_t* ctx, int* version) {
    if (!ctx || !version) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    *version = 0;

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(ctx->sqlite_db,
                                "SELECT MAX(version) FROM _migrations;",
                                -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return REGISLEX_OK; /* Table might not exist yet */
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *version = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return REGISLEX_OK;
}

/* ============================================================================
 * Transaction Functions
 * ============================================================================ */

regislex_error_t regislex_db_begin(regislex_db_context_t* ctx,
                                   regislex_db_transaction_t** tx) {
    if (!ctx || !tx) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    *tx = (regislex_db_transaction_t*)platform_calloc(1, sizeof(regislex_db_transaction_t));
    if (!*tx) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    (*tx)->ctx = ctx;

    int rc = sqlite3_exec(ctx->sqlite_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        set_sqlite_error(ctx);
        platform_free(*tx);
        *tx = NULL;
        return REGISLEX_ERROR_DATABASE;
    }

    (*tx)->active = true;
    return REGISLEX_OK;
}

regislex_error_t regislex_db_commit(regislex_db_transaction_t* tx) {
    if (!tx || !tx->ctx) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    if (!tx->active) {
        return REGISLEX_ERROR_INVALID_STATE;
    }

    int rc = sqlite3_exec(tx->ctx->sqlite_db, "COMMIT;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        set_sqlite_error(tx->ctx);
        return REGISLEX_ERROR_DATABASE;
    }

    tx->active = false;
    platform_free(tx);
    return REGISLEX_OK;
}

regislex_error_t regislex_db_rollback(regislex_db_transaction_t* tx) {
    if (!tx || !tx->ctx) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    if (!tx->active) {
        platform_free(tx);
        return REGISLEX_OK;
    }

    sqlite3_exec(tx->ctx->sqlite_db, "ROLLBACK;", NULL, NULL, NULL);
    tx->active = false;
    platform_free(tx);
    return REGISLEX_OK;
}

/* ============================================================================
 * Query Execution Functions
 * ============================================================================ */

regislex_error_t regislex_db_exec(regislex_db_context_t* ctx, const char* sql) {
    if (!ctx || !sql) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    char* err_msg = NULL;
    int rc = sqlite3_exec(ctx->sqlite_db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        if (err_msg) {
            set_db_error(ctx, err_msg);
            sqlite3_free(err_msg);
        } else {
            set_sqlite_error(ctx);
        }
        return REGISLEX_ERROR_DATABASE;
    }

    return REGISLEX_OK;
}

regislex_error_t regislex_db_exec_tx(regislex_db_transaction_t* tx, const char* sql) {
    if (!tx || !tx->ctx || !sql) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }
    return regislex_db_exec(tx->ctx, sql);
}

regislex_error_t regislex_db_prepare(regislex_db_context_t* ctx,
                                     const char* sql,
                                     regislex_db_stmt_t** stmt) {
    if (!ctx || !sql || !stmt) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    *stmt = (regislex_db_stmt_t*)platform_calloc(1, sizeof(regislex_db_stmt_t));
    if (!*stmt) {
        return REGISLEX_ERROR_OUT_OF_MEMORY;
    }

    (*stmt)->ctx = ctx;

    int rc = sqlite3_prepare_v2(ctx->sqlite_db, sql, -1, &(*stmt)->sqlite_stmt, NULL);
    if (rc != SQLITE_OK) {
        set_sqlite_error(ctx);
        platform_free(*stmt);
        *stmt = NULL;
        return REGISLEX_ERROR_DATABASE;
    }

    (*stmt)->param_count = sqlite3_bind_parameter_count((*stmt)->sqlite_stmt);
    (*stmt)->column_count = sqlite3_column_count((*stmt)->sqlite_stmt);

    return REGISLEX_OK;
}

void regislex_db_finalize(regislex_db_stmt_t* stmt) {
    if (!stmt) return;

    if (stmt->sqlite_stmt) {
        sqlite3_finalize(stmt->sqlite_stmt);
    }

    platform_free(stmt);
}

regislex_error_t regislex_db_reset(regislex_db_stmt_t* stmt) {
    if (!stmt || !stmt->sqlite_stmt) {
        return REGISLEX_ERROR_INVALID_ARGUMENT;
    }

    sqlite3_reset(stmt->sqlite_stmt);
    sqlite3_clear_bindings(stmt->sqlite_stmt);
    return REGISLEX_OK;
}

/* ============================================================================
 * Parameter Binding Functions
 * ============================================================================ */

regislex_error_t regislex_db_bind_null(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int rc = sqlite3_bind_null(stmt->sqlite_stmt, index);
    return (rc == SQLITE_OK) ? REGISLEX_OK : REGISLEX_ERROR_DATABASE;
}

regislex_error_t regislex_db_bind_int(regislex_db_stmt_t* stmt, int index, int64_t value) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int rc = sqlite3_bind_int64(stmt->sqlite_stmt, index, value);
    return (rc == SQLITE_OK) ? REGISLEX_OK : REGISLEX_ERROR_DATABASE;
}

regislex_error_t regislex_db_bind_real(regislex_db_stmt_t* stmt, int index, double value) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int rc = sqlite3_bind_double(stmt->sqlite_stmt, index, value);
    return (rc == SQLITE_OK) ? REGISLEX_OK : REGISLEX_ERROR_DATABASE;
}

regislex_error_t regislex_db_bind_text(regislex_db_stmt_t* stmt, int index, const char* value) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    if (!value) {
        return regislex_db_bind_null(stmt, index);
    }

    int rc = sqlite3_bind_text(stmt->sqlite_stmt, index, value, -1, SQLITE_TRANSIENT);
    return (rc == SQLITE_OK) ? REGISLEX_OK : REGISLEX_ERROR_DATABASE;
}

regislex_error_t regislex_db_bind_blob(regislex_db_stmt_t* stmt, int index,
                                       const void* value, size_t size) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    if (!value || size == 0) {
        return regislex_db_bind_null(stmt, index);
    }

    int rc = sqlite3_bind_blob(stmt->sqlite_stmt, index, value, (int)size, SQLITE_TRANSIENT);
    return (rc == SQLITE_OK) ? REGISLEX_OK : REGISLEX_ERROR_DATABASE;
}

regislex_error_t regislex_db_bind_uuid(regislex_db_stmt_t* stmt, int index,
                                       const regislex_uuid_t* uuid) {
    if (!uuid) return regislex_db_bind_null(stmt, index);
    return regislex_db_bind_text(stmt, index, uuid->value);
}

regislex_error_t regislex_db_bind_datetime(regislex_db_stmt_t* stmt, int index,
                                           const regislex_datetime_t* dt) {
    if (!dt) return regislex_db_bind_null(stmt, index);

    char buffer[32];
    regislex_datetime_format(dt, buffer, sizeof(buffer));
    return regislex_db_bind_text(stmt, index, buffer);
}

regislex_error_t regislex_db_bind_money(regislex_db_stmt_t* stmt, int index,
                                        const regislex_money_t* money) {
    if (!money) return regislex_db_bind_null(stmt, index);
    return regislex_db_bind_int(stmt, index, money->amount);
}

/* ============================================================================
 * Result Functions
 * ============================================================================ */

regislex_error_t regislex_db_step(regislex_db_stmt_t* stmt) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    int rc = sqlite3_step(stmt->sqlite_stmt);

    if (rc == SQLITE_ROW) {
        return REGISLEX_OK;
    } else if (rc == SQLITE_DONE) {
        return REGISLEX_ERROR_NOT_FOUND;
    } else {
        set_sqlite_error(stmt->ctx);
        return REGISLEX_ERROR_DATABASE;
    }
}

int regislex_db_column_count(regislex_db_stmt_t* stmt) {
    if (!stmt || !stmt->sqlite_stmt) return 0;
    return sqlite3_column_count(stmt->sqlite_stmt);
}

const char* regislex_db_column_name(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return NULL;
    return sqlite3_column_name(stmt->sqlite_stmt, index);
}

regislex_db_type_t regislex_db_column_type(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return REGISLEX_DB_TYPE_NULL;

    int type = sqlite3_column_type(stmt->sqlite_stmt, index);

    switch (type) {
        case SQLITE_INTEGER: return REGISLEX_DB_TYPE_INTEGER;
        case SQLITE_FLOAT:   return REGISLEX_DB_TYPE_REAL;
        case SQLITE_TEXT:    return REGISLEX_DB_TYPE_TEXT;
        case SQLITE_BLOB:    return REGISLEX_DB_TYPE_BLOB;
        default:             return REGISLEX_DB_TYPE_NULL;
    }
}

bool regislex_db_column_is_null(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return true;
    return sqlite3_column_type(stmt->sqlite_stmt, index) == SQLITE_NULL;
}

int64_t regislex_db_column_int(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return 0;
    return sqlite3_column_int64(stmt->sqlite_stmt, index);
}

double regislex_db_column_real(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return 0.0;
    return sqlite3_column_double(stmt->sqlite_stmt, index);
}

const char* regislex_db_column_text(regislex_db_stmt_t* stmt, int index) {
    if (!stmt || !stmt->sqlite_stmt) return NULL;
    return (const char*)sqlite3_column_text(stmt->sqlite_stmt, index);
}

const void* regislex_db_column_blob(regislex_db_stmt_t* stmt, int index, size_t* size) {
    if (!stmt || !stmt->sqlite_stmt) {
        if (size) *size = 0;
        return NULL;
    }

    const void* blob = sqlite3_column_blob(stmt->sqlite_stmt, index);
    if (size) {
        *size = (size_t)sqlite3_column_bytes(stmt->sqlite_stmt, index);
    }
    return blob;
}

regislex_error_t regislex_db_column_uuid(regislex_db_stmt_t* stmt, int index,
                                         regislex_uuid_t* uuid) {
    if (!uuid) return REGISLEX_ERROR_INVALID_ARGUMENT;

    const char* text = regislex_db_column_text(stmt, index);
    if (!text) {
        memset(uuid->value, 0, sizeof(uuid->value));
        return REGISLEX_OK;
    }

    strncpy(uuid->value, text, sizeof(uuid->value) - 1);
    return REGISLEX_OK;
}

regislex_error_t regislex_db_column_datetime(regislex_db_stmt_t* stmt, int index,
                                             regislex_datetime_t* dt) {
    if (!dt) return REGISLEX_ERROR_INVALID_ARGUMENT;

    const char* text = regislex_db_column_text(stmt, index);
    if (!text) {
        memset(dt, 0, sizeof(regislex_datetime_t));
        return REGISLEX_OK;
    }

    return regislex_datetime_parse(text, dt);
}

regislex_error_t regislex_db_column_money(regislex_db_stmt_t* stmt, int index,
                                          regislex_money_t* money) {
    if (!money) return REGISLEX_ERROR_INVALID_ARGUMENT;

    money->amount = regislex_db_column_int(stmt, index);
    strcpy(money->currency, "USD"); /* Default currency */
    return REGISLEX_OK;
}

int64_t regislex_db_last_insert_id(regislex_db_context_t* ctx) {
    if (!ctx || !ctx->sqlite_db) return 0;
    return sqlite3_last_insert_rowid(ctx->sqlite_db);
}

int regislex_db_changes(regislex_db_context_t* ctx) {
    if (!ctx || !ctx->sqlite_db) return 0;
    return sqlite3_changes(ctx->sqlite_db);
}
