#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

SnapResult store_snap(Db *db, SnapQuery q)
{
    SnapResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO progress_snapshots(goal_id,pct)"
        " VALUES(?,?) RETURNING id,goal_id,pct,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.goal_id);
    sqlite3_bind_int(s, 2, q.pct);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.snap.id = sqlite3_column_int64(s, 0);
        r.snap.goal_id = sqlite3_column_int64(s, 1);
        r.snap.pct = sqlite3_column_int(s, 2);
        r.snap.created_at = sqlite3_column_int64(s, 3);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

TimeResult store_time(Db *db, TimeQuery q)
{
    TimeResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO time_entries(task_id,user_id,minutes)"
        " VALUES(?,?,?) RETURNING id,task_id,user_id,minutes,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.task_id);
    sqlite3_bind_text(s, 2, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(s, 3, q.minutes);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.entry.id = sqlite3_column_int64(s, 0);
        r.entry.task_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.entry.user_id, sizeof(r.entry.user_id), "%s", t);
        r.entry.minutes = sqlite3_column_int(s, 3);
        r.entry.created_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

RoiResult fetch_roi(Db *db, RoiQuery q)
{
    RoiResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT COALESCE(SUM(value_cents),0),COALESCE(SUM(cost_cents),0)"
        " FROM roi_metrics WHERE goal_id=?;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.goal_id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.roi.goal_id = q.goal_id;
        r.roi.value_cents = sqlite3_column_int64(s, 0);
        r.roi.cost_cents = sqlite3_column_int64(s, 1);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
