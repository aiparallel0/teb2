#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * Central schema initialization — all tables created at db_open.
 * No lazy ensure_*_schema() needed in per-entity files.
 */
static const char *SCHEMA =
    "CREATE TABLE IF NOT EXISTS users("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "password_hash TEXT NOT NULL,"
    "role INTEGER NOT NULL DEFAULT 0,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS goals("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,"
    "title TEXT NOT NULL,"
    "description TEXT NOT NULL DEFAULT '',"
    "status TEXT NOT NULL DEFAULT 'pending',"
    "parent_id INTEGER NOT NULL DEFAULT 0,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS tasks("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "goal_id INTEGER NOT NULL,"
    "user_id TEXT NOT NULL,"
    "title TEXT NOT NULL,"
    "description TEXT NOT NULL DEFAULT '',"
    "status TEXT NOT NULL DEFAULT 'pending',"
    "agent TEXT NOT NULL DEFAULT '',"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "FOREIGN KEY (goal_id) REFERENCES goals(id));"

    "CREATE TABLE IF NOT EXISTS tickets("
    "user_id INTEGER NOT NULL,"
    "role INTEGER NOT NULL,"
    "expiry INTEGER NOT NULL,"
    "mac BLOB NOT NULL);"

    "CREATE TABLE IF NOT EXISTS nudges("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,"
    "message TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS outcomes("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "task_id INTEGER NOT NULL,"
    "result TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS learnings("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "goal_id INTEGER NOT NULL,"
    "insight TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "FOREIGN KEY (goal_id) REFERENCES goals(id));"

    "CREATE TABLE IF NOT EXISTS schedules("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "task_id INTEGER NOT NULL,"
    "run_at INTEGER NOT NULL,"
    "FOREIGN KEY (task_id) REFERENCES tasks(id));"

    "CREATE TABLE IF NOT EXISTS budgets("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL UNIQUE,"
    "limit_cents INTEGER NOT NULL DEFAULT 0,"
    "spent_cents INTEGER NOT NULL DEFAULT 0);"

    "CREATE TABLE IF NOT EXISTS spending("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,"
    "amount_cents INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS agent_memory("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "agent TEXT NOT NULL,"
    "key TEXT NOT NULL,"
    "val TEXT NOT NULL DEFAULT '',"
    "ts INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "UNIQUE(agent,key));"

    "CREATE TABLE IF NOT EXISTS audit_log("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL DEFAULT '',"
    "action TEXT NOT NULL,"
    "path TEXT NOT NULL,"
    "status INTEGER NOT NULL,"
    "src_ip TEXT NOT NULL DEFAULT '',"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"
    "CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(user_id);"
    "CREATE INDEX IF NOT EXISTS idx_audit_ts   ON audit_log(created_at);";

Err db_open(const char *path, Db *out)
{
    char *errmsg = NULL;
    int rc;

    if (!path || !out) return ERR_DB;
    out->handle = NULL;
    rc = sqlite3_open(path, &out->handle);
    if (rc != SQLITE_OK) {
        sqlite3_close(out->handle);
        out->handle = NULL;
        return ERR_DB;
    }
    /* Enable WAL mode for concurrent multi-process access */
    sqlite3_exec(out->handle, "PRAGMA journal_mode=WAL;",  NULL, NULL, NULL);
    sqlite3_exec(out->handle, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    rc = sqlite3_exec(out->handle, SCHEMA, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        sqlite3_close(out->handle);
        out->handle = NULL;
        return ERR_DB;
    }
    if (db_init_ext(out) != ERR_OK) {
        sqlite3_close(out->handle);
        out->handle = NULL;
        return ERR_DB;
    }
    return ERR_OK;
}

void db_close(Db *db)
{
    if (db && db->handle) {
        sqlite3_close(db->handle);
        db->handle = NULL;
    }
}
