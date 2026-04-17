#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

static WorkflowRun row_to_run(sqlite3_stmt *s)
{
    WorkflowRun w;
    const char *t;
    memset(&w, 0, sizeof(w));
    w.id = sqlite3_column_int64(s, 0);
    w.goal_id = sqlite3_column_int64(s, 1);
    t = (const char *)sqlite3_column_text(s, 2);
    if (t) snprintf(w.status, sizeof(w.status), "%s", t);
    w.started_at = sqlite3_column_int64(s, 3);
    w.finished_at = sqlite3_column_int64(s, 4);
    t = (const char *)sqlite3_column_text(s, 5);
    if (t) snprintf(w.error_msg, sizeof(w.error_msg), "%s", t);
    return w;
}

RunResult store_run(Db *db, RunQuery q)
{
    RunResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO workflow_runs(goal_id) VALUES(?)"
        " RETURNING id,goal_id,status,started_at,finished_at,error_msg;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.goal_id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.run = row_to_run(s); r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

RunResult fetch_run(Db *db, RunQuery q)
{
    RunResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,goal_id,status,started_at,finished_at,error_msg"
        " FROM workflow_runs WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.run = row_to_run(s); r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

RunResult update_run_status(Db *db, int64_t run_id,
                            const char *status, const char *error_msg)
{
    RunResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "UPDATE workflow_runs SET status=?,error_msg=?,"
        "finished_at=(strftime('%s','now')) WHERE id=?"
        " RETURNING id,goal_id,status,started_at,finished_at,error_msg;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, error_msg ? error_msg : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 3, run_id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.run = row_to_run(s); r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

Err update_run_pid(Db *db, int64_t run_id, int64_t pid)
{
    sqlite3_stmt *s = NULL;
    const char *sql = "UPDATE workflow_runs SET pid=? WHERE id=?;";
    if (!db || !db->handle) return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_int64(s, 1, pid);
    sqlite3_bind_int64(s, 2, run_id);
    sqlite3_step(s); sqlite3_finalize(s);
    return ERR_OK;
}

Err update_run_tokens(Db *db, int64_t run_id, int64_t tokens)
{
    sqlite3_stmt *s = NULL;
    const char *sql =
        "UPDATE workflow_runs SET token_spend=token_spend+? WHERE id=?;";
    if (!db || !db->handle) return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_int64(s, 1, tokens);
    sqlite3_bind_int64(s, 2, run_id);
    sqlite3_step(s); sqlite3_finalize(s);
    return ERR_OK;
}
