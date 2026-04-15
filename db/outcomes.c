#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static const char *SCHEMA_OUTCOMES =
    "CREATE TABLE IF NOT EXISTS outcomes("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "task_id INTEGER NOT NULL,"
    "result TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));";

static void ensure_outcomes_schema(struct sqlite3 *h)
{
    char *err = NULL;
    (void)sqlite3_exec(h, SCHEMA_OUTCOMES, NULL, NULL, &err);
    sqlite3_free(err);
}

static Outcome row_to_outcome(sqlite3_stmt *stmt)
{
    Outcome o;
    const char *s;
    memset(&o, 0, sizeof(o));
    o.id         = sqlite3_column_int64(stmt, 0);
    o.task_id    = sqlite3_column_int64(stmt, 1);
    s            = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(o.result, sizeof(o.result), "%s", s);
    o.created_at = sqlite3_column_int64(stmt, 3);
    return o;
}

OutcomeResult store_outcome(Db *db, OutcomeQuery q)
{
    OutcomeResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO outcomes(task_id,result) VALUES(?,?)"
                      " RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_outcomes_schema(db->handle);
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.task_id);
    sqlite3_bind_text(stmt, 2, q.result[0] ? q.result : "", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.outcome.id      = sqlite3_column_int64(stmt, 0);
        r.outcome.task_id = q.task_id;
        snprintf(r.outcome.result, sizeof(r.outcome.result), "%s", q.result);
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

OutcomeResult fetch_outcome(Db *db, OutcomeQuery q)
{
    OutcomeResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_outcomes_schema(db->handle);
    if (q.id > 0) {
        sql = "SELECT id,task_id,result,created_at FROM outcomes"
              " WHERE id=? LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
            r.err = ERR_DB; return r;
        }
        sqlite3_bind_int64(stmt, 1, q.id);
    } else {
        sql = "SELECT id,task_id,result,created_at FROM outcomes"
              " WHERE task_id=? ORDER BY created_at DESC LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
            r.err = ERR_DB; return r;
        }
        sqlite3_bind_int64(stmt, 1, q.task_id);
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.outcome = row_to_outcome(stmt);
        r.err     = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
