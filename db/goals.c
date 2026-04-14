#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static const char *SCHEMA_GOALS =
    "CREATE TABLE IF NOT EXISTS goals("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,"
    "title TEXT NOT NULL,"
    "description TEXT NOT NULL DEFAULT '',"
    "status TEXT NOT NULL DEFAULT 'pending',"
    "parent_id INTEGER NOT NULL DEFAULT 0,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));";

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
    rc = sqlite3_exec(out->handle, SCHEMA_GOALS, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
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

static Goal row_to_goal(sqlite3_stmt *stmt)
{
    Goal g;
    const char *s;
    memset(&g, 0, sizeof(g));
    g.id        = sqlite3_column_int64(stmt, 0);
    s           = (const char *)sqlite3_column_text(stmt, 1);
    if (s) snprintf(g.user_id,     sizeof(g.user_id),     "%s", s);
    s           = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(g.title,       sizeof(g.title),       "%s", s);
    s           = (const char *)sqlite3_column_text(stmt, 3);
    if (s) snprintf(g.description, sizeof(g.description), "%s", s);
    s           = (const char *)sqlite3_column_text(stmt, 4);
    if (s) snprintf(g.status,      sizeof(g.status),      "%s", s);
    g.parent_id = sqlite3_column_int64(stmt, 5);
    g.created_at = sqlite3_column_int64(stmt, 6);
    return g;
}

GoalResult fetch_goal(Db *db, GoalQuery q)
{
    GoalResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,user_id,title,description,status,parent_id,created_at"
                      " FROM goals WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[0] = row_to_goal(stmt);
        r.count   = 1;
        r.err     = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

GoalResult store_goal(Db *db, GoalQuery q)
{
    GoalResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO goals(user_id,title) VALUES(?,?) RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.title[0] ? q.title : "untitled", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[0].id = sqlite3_column_int64(stmt, 0);
        snprintf(r.rows[0].user_id, sizeof(r.rows[0].user_id), "%s", q.user_id);
        r.count = 1;
        r.err   = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

GoalResult list_goals(Db *db, GoalQuery q)
{
    GoalResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,user_id,title,description,status,parent_id,created_at"
                      " FROM goals WHERE user_id=? LIMIT ? OFFSET ?;";
    int lim;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 16) ? q.limit : 16;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt,  2, lim);
    sqlite3_bind_int(stmt,  3, q.offset);
    while (sqlite3_step(stmt) == SQLITE_ROW && r.count < 16)
        r.rows[r.count++] = row_to_goal(stmt);
    r.err = ERR_OK;
    sqlite3_finalize(stmt);
    return r;
}
