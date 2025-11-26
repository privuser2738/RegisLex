/**
 * @file migration.c
 * @brief Database Migration Management
 */

#include "database/database.h"
#include "regislex/regislex.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

/* Migration scripts */
static const char* migrations[] = {
    /* Migration 001: Core tables */
    "CREATE TABLE IF NOT EXISTS schema_migrations ("
    "  version INTEGER PRIMARY KEY,"
    "  applied_at TEXT NOT NULL DEFAULT (datetime('now'))"
    ");"
    "CREATE TABLE IF NOT EXISTS users ("
    "  id TEXT PRIMARY KEY,"
    "  username TEXT UNIQUE NOT NULL,"
    "  email TEXT UNIQUE NOT NULL,"
    "  password_hash TEXT NOT NULL,"
    "  first_name TEXT,"
    "  last_name TEXT,"
    "  role TEXT DEFAULT 'user',"
    "  is_active INTEGER DEFAULT 1,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  updated_at TEXT DEFAULT (datetime('now'))"
    ");"
    "CREATE TABLE IF NOT EXISTS sessions ("
    "  id TEXT PRIMARY KEY,"
    "  user_id TEXT NOT NULL,"
    "  token TEXT UNIQUE NOT NULL,"
    "  expires_at TEXT NOT NULL,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(token);"
    "CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);",

    /* Migration 002: Cases */
    "CREATE TABLE IF NOT EXISTS cases ("
    "  id TEXT PRIMARY KEY,"
    "  case_number TEXT UNIQUE,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  case_type TEXT,"
    "  status TEXT DEFAULT 'open',"
    "  priority TEXT DEFAULT 'normal',"
    "  client_id TEXT,"
    "  assigned_attorney_id TEXT,"
    "  court_name TEXT,"
    "  court_case_number TEXT,"
    "  filed_date TEXT,"
    "  closed_date TEXT,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  updated_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (assigned_attorney_id) REFERENCES users(id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_cases_status ON cases(status);"
    "CREATE INDEX IF NOT EXISTS idx_cases_attorney ON cases(assigned_attorney_id);",

    /* Migration 003: Deadlines */
    "CREATE TABLE IF NOT EXISTS deadlines ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  due_date TEXT NOT NULL,"
    "  deadline_type TEXT,"
    "  priority TEXT DEFAULT 'normal',"
    "  status TEXT DEFAULT 'pending',"
    "  assigned_to TEXT,"
    "  reminder_days INTEGER DEFAULT 3,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  updated_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (case_id) REFERENCES cases(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (assigned_to) REFERENCES users(id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_deadlines_due ON deadlines(due_date);"
    "CREATE INDEX IF NOT EXISTS idx_deadlines_case ON deadlines(case_id);",

    /* Migration 004: Documents */
    "CREATE TABLE IF NOT EXISTS documents ("
    "  id TEXT PRIMARY KEY,"
    "  case_id TEXT,"
    "  title TEXT NOT NULL,"
    "  description TEXT,"
    "  file_path TEXT NOT NULL,"
    "  file_name TEXT NOT NULL,"
    "  file_size INTEGER,"
    "  mime_type TEXT,"
    "  checksum TEXT,"
    "  version INTEGER DEFAULT 1,"
    "  is_template INTEGER DEFAULT 0,"
    "  uploaded_by TEXT,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  updated_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (case_id) REFERENCES cases(id) ON DELETE SET NULL,"
    "  FOREIGN KEY (uploaded_by) REFERENCES users(id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_documents_case ON documents(case_id);",

    /* Migration 005: Workflow */
    "CREATE TABLE IF NOT EXISTS workflows ("
    "  id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  description TEXT,"
    "  trigger_type TEXT,"
    "  trigger_config TEXT,"
    "  is_active INTEGER DEFAULT 1,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  updated_at TEXT DEFAULT (datetime('now'))"
    ");"
    "CREATE TABLE IF NOT EXISTS workflow_tasks ("
    "  id TEXT PRIMARY KEY,"
    "  workflow_id TEXT NOT NULL,"
    "  case_id TEXT,"
    "  name TEXT NOT NULL,"
    "  description TEXT,"
    "  status TEXT DEFAULT 'pending',"
    "  assigned_to TEXT,"
    "  due_date TEXT,"
    "  completed_at TEXT,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (workflow_id) REFERENCES workflows(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (case_id) REFERENCES cases(id) ON DELETE CASCADE,"
    "  FOREIGN KEY (assigned_to) REFERENCES users(id)"
    ");",

    /* Migration 006: Audit Log */
    "CREATE TABLE IF NOT EXISTS audit_log ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  user_id TEXT,"
    "  action TEXT NOT NULL,"
    "  entity_type TEXT,"
    "  entity_id TEXT,"
    "  old_values TEXT,"
    "  new_values TEXT,"
    "  ip_address TEXT,"
    "  user_agent TEXT,"
    "  created_at TEXT DEFAULT (datetime('now')),"
    "  FOREIGN KEY (user_id) REFERENCES users(id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(user_id);"
    "CREATE INDEX IF NOT EXISTS idx_audit_entity ON audit_log(entity_type, entity_id);",

    NULL  /* Sentinel */
};

/* regislex_db_migrate is implemented in database.c */

int regislex_db_get_schema_version(regislex_db_context_t* ctx) {
    (void)ctx;
    /* Would query schema_migrations table */
    return 0;
}
