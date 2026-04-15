#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

StepResult store_step(Db *db, StepQuery q)
{
    StepResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO workflow_steps(run_id,step_index,agent,payload)"
        " VALUES(?,?,?,?) RETURNING id,run_id,step_index,agent,payload,"
        "status,result,created_at;";
    const char *t;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.run_id);
    sqlite3_bind_int(s, 2, (int)q.id);
    sqlite3_bind_text(s, 3, q.agent, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 4, q.payload, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.step.id = sqlite3_column_int64(s, 0);
        r.step.run_id = sqlite3_column_int64(s, 1);
        r.step.step_index = sqlite3_column_int(s, 2);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.step.agent, sizeof(r.step.agent), "%s", t);
        t = (const char *)sqlite3_column_text(s, 4);
        if (t) snprintf(r.step.payload, sizeof(r.step.payload), "%s", t);
        t = (const char *)sqlite3_column_text(s, 5);
        if (t) snprintf(r.step.status, sizeof(r.step.status), "%s", t);
        t = (const char *)sqlite3_column_text(s, 6);
        if (t) snprintf(r.step.result, sizeof(r.step.result), "%s", t);
        r.step.created_at = sqlite3_column_int64(s, 7);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

StepResult list_steps(Db *db, int64_t run_id)
{
    StepResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,run_id,step_index,agent,payload,"
        "status,result,created_at FROM workflow_steps"
        " WHERE run_id=? ORDER BY step_index;";
    const char *t;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, run_id);
    while (sqlite3_step(s) == SQLITE_ROW && r.count < 32) {
        r.steps[r.count].id = sqlite3_column_int64(s, 0);
        r.steps[r.count].run_id = sqlite3_column_int64(s, 1);
        r.steps[r.count].step_index = sqlite3_column_int(s, 2);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.steps[r.count].agent,
                        sizeof(r.steps[r.count].agent), "%s", t);
        t = (const char *)sqlite3_column_text(s, 5);
        if (t) snprintf(r.steps[r.count].status,
                        sizeof(r.steps[r.count].status), "%s", t);
        t = (const char *)sqlite3_column_text(s, 6);
        if (t) snprintf(r.steps[r.count].result,
                        sizeof(r.steps[r.count].result), "%s", t);
        r.steps[r.count].created_at = sqlite3_column_int64(s, 7);
        r.count++;
    }
    r.err = ERR_OK;
    sqlite3_finalize(s);
    return r;
}

StepResult update_step_status(Db *db, int64_t step_id,
                              const char *status, const char *result)
{
    StepResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "UPDATE workflow_steps SET status=?,result=? WHERE id=?"
        " RETURNING id,run_id,step_index,agent,payload,status,result,created_at;";
    const char *t;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, result ? result : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 3, step_id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.step.id = sqlite3_column_int64(s, 0);
        r.step.run_id = sqlite3_column_int64(s, 1);
        r.step.step_index = sqlite3_column_int(s, 2);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.step.agent, sizeof(r.step.agent), "%s", t);
        t = (const char *)sqlite3_column_text(s, 5);
        if (t) snprintf(r.step.status, sizeof(r.step.status), "%s", t);
        t = (const char *)sqlite3_column_text(s, 6);
        if (t) snprintf(r.step.result, sizeof(r.step.result), "%s", t);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
