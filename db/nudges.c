#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static const char *SCHEMA_NUDGES =
    "CREATE TABLE IF NOT EXISTS nudges("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,"
    "message TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));";

static void ensure_nudge_schema(struct sqlite3 *h)
{
    char *err = NULL;
    (void)sqlite3_exec(h, SCHEMA_NUDGES, NULL, NULL, &err);
    sqlite3_free(err);
}

NudgeResult store_nudge(Db *db, NudgeQuery q)
{
    NudgeResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO nudges(user_id,message) VALUES(?,?)"
                      " RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_nudge_schema(db->handle);
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.message[0] ? q.message : "", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.nudge.id = sqlite3_column_int64(stmt, 0);
        snprintf(r.nudge.user_id, sizeof(r.nudge.user_id), "%s", q.user_id);
        snprintf(r.nudge.message, sizeof(r.nudge.message), "%s", q.message);
        r.count = 1;
        r.err   = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

NudgeResult list_nudges(Db *db, NudgeQuery q)
{
    NudgeResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,user_id,message,created_at FROM nudges"
                      " WHERE user_id=? ORDER BY created_at DESC LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_nudge_schema(db->handle);
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *s;
        r.nudge.id = sqlite3_column_int64(stmt, 0);
        s = (const char *)sqlite3_column_text(stmt, 1);
        if (s) snprintf(r.nudge.user_id, sizeof(r.nudge.user_id), "%s", s);
        s = (const char *)sqlite3_column_text(stmt, 2);
        if (s) snprintf(r.nudge.message, sizeof(r.nudge.message), "%s", s);
        r.nudge.created_at = sqlite3_column_int64(stmt, 3);
        r.count = 1;
        r.err   = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
