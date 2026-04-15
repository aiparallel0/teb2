#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static Task row_to_task(sqlite3_stmt *stmt)
{
    Task t;
    const char *s;
    memset(&t, 0, sizeof(t));
    t.id         = sqlite3_column_int64(stmt, 0);
    t.goal_id    = sqlite3_column_int64(stmt, 1);
    s            = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(t.user_id,     sizeof(t.user_id),     "%s", s);
    s            = (const char *)sqlite3_column_text(stmt, 3);
    if (s) snprintf(t.title,       sizeof(t.title),       "%s", s);
    s            = (const char *)sqlite3_column_text(stmt, 4);
    if (s) snprintf(t.description, sizeof(t.description), "%s", s);
    s            = (const char *)sqlite3_column_text(stmt, 5);
    if (s) snprintf(t.status,      sizeof(t.status),      "%s", s);
    s            = (const char *)sqlite3_column_text(stmt, 6);
    if (s) snprintf(t.agent,       sizeof(t.agent),       "%s", s);
    t.created_at = sqlite3_column_int64(stmt, 7);
    return t;
}

TaskResult fetch_task(Db *db, TaskQuery q)
{
    TaskResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,goal_id,user_id,title,description,status,agent,created_at"
                      " FROM tasks WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[0] = row_to_task(stmt);
        r.count   = 1;
        r.err     = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

TaskResult store_task(Db *db, TaskQuery q)
{
    TaskResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO tasks(goal_id,user_id,title,description,agent)"
        " VALUES(?,?,?,?,?)"
        " RETURNING id,goal_id,user_id,title,description,status,agent,created_at;";
    char buf[768];
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.goal_id);
    sqlite3_bind_text(stmt,  2, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt,  3, q.title[0] ? q.title : "untitled", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt,  4, q.description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt,  5, q.agent, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[0] = row_to_task(stmt);
        r.count   = 1;
        r.err     = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    if (r.err == ERR_OK) {
        snprintf(buf, sizeof(buf), "%s %s", r.rows[0].title, r.rows[0].description);
        { Err ie = index_entity(db, "task", r.rows[0].id, buf); (void)ie; }
    }
    return r;
}

TaskResult list_tasks(Db *db, TaskQuery q)
{
    TaskResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,goal_id,user_id,title,description,status,agent,created_at"
                      " FROM tasks WHERE goal_id=? AND id > ? ORDER BY id ASC LIMIT ?;";
    int lim;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 16) ? q.limit : 16;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.goal_id);
    sqlite3_bind_int64(stmt, 2, q.cursor);
    sqlite3_bind_int(stmt,   3, lim);
    while (sqlite3_step(stmt) == SQLITE_ROW && r.count < 16)
        r.rows[r.count++] = row_to_task(stmt);
    r.err = ERR_OK;
    sqlite3_finalize(stmt);
    return r;
}

TaskResult update_task(Db *db, TaskQuery q)
{
    TaskResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "UPDATE tasks SET status=?,"
        " description=CASE WHEN ?='' THEN description ELSE ? END,"
        " agent=CASE WHEN ?='' THEN agent ELSE ? END"
        " WHERE id=?"
        " RETURNING id,goal_id,user_id,title,description,status,agent,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (q.id <= 0) { r.err = ERR_NOT_FOUND; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.status[0] ? q.status : "pending", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, q.description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, q.agent, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, q.agent, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, q.id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[0] = row_to_task(stmt);
        r.count   = 1;
        r.err     = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
