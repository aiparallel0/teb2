#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static Learning row_to_learning(sqlite3_stmt *stmt)
{
    Learning l;
    const char *s;
    memset(&l, 0, sizeof(l));
    l.id         = sqlite3_column_int64(stmt, 0);
    l.goal_id    = sqlite3_column_int64(stmt, 1);
    s            = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(l.insight, sizeof(l.insight), "%s", s);
    l.created_at = sqlite3_column_int64(stmt, 3);
    return l;
}

LearnResult store_learning(Db *db, LearnQuery q)
{
    LearnResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO learnings(goal_id,insight) VALUES(?,?)"
                      " RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.goal_id);
    sqlite3_bind_text(stmt, 2, q.insight[0] ? q.insight : "", -1,
                      SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.learning.id      = sqlite3_column_int64(stmt, 0);
        r.learning.goal_id = q.goal_id;
        snprintf(r.learning.insight, sizeof(r.learning.insight),
                 "%s", q.insight);
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

LearnResult fetch_learning(Db *db, LearnQuery q)
{
    LearnResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (q.id > 0) {
        sql = "SELECT id,goal_id,insight,created_at FROM learnings"
              " WHERE id=? LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK)
        { r.err = ERR_DB; return r; }
        sqlite3_bind_int64(stmt, 1, q.id);
    } else {
        sql = "SELECT id,goal_id,insight,created_at FROM learnings"
              " WHERE goal_id=? ORDER BY created_at DESC LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK)
        { r.err = ERR_DB; return r; }
        sqlite3_bind_int64(stmt, 1, q.goal_id);
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.learning = row_to_learning(stmt);
        r.err      = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

/* Return up to `limit` recent learnings for a user (via goals.user_id). */
LearnListResult list_learnings(Db *db, const char *user_id, int limit)
{
    LearnListResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT l.id,l.goal_id,l.insight,l.created_at FROM learnings l"
        " JOIN goals g ON g.id=l.goal_id"
        " WHERE g.user_id=? ORDER BY l.created_at DESC LIMIT ?;";
    int cap, n = 0;

    memset(&r, 0, sizeof(r));
    cap = (int)(sizeof(r.rows) / sizeof(r.rows[0]));
    if (limit <= 0 || limit > cap) limit = cap;
    if (!db || !db->handle || !user_id) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, limit);
    while (n < cap && sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[n++] = row_to_learning(stmt);
    }
    r.count = n;
    r.err   = ERR_OK;
    sqlite3_finalize(stmt);
    return r;
}
