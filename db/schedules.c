#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

SchedResult store_sched(Db *db, SchedQuery q)
{
    SchedResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO schedules(task_id,run_at) VALUES(?,?)"
                      " RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.task_id);
    sqlite3_bind_int64(stmt, 2, q.run_at);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.entry.id      = sqlite3_column_int64(stmt, 0);
        r.entry.task_id = q.task_id;
        r.entry.run_at  = q.run_at;
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

SchedResult fetch_sched(Db *db, SchedQuery q)
{
    SchedResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,task_id,run_at FROM schedules"
                      " WHERE task_id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.task_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.entry.id      = sqlite3_column_int64(stmt, 0);
        r.entry.task_id = sqlite3_column_int64(stmt, 1);
        r.entry.run_at  = sqlite3_column_int64(stmt, 2);
        r.err = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
