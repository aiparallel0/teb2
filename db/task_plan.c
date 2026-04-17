#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * task_plan — per-task DAG/HITL metadata. Previously every field
 * the decompose/measure prompts produced (depends_on, effort_minutes,
 * est_cost_cents, requires_hitl, success_criteria, next_action) was
 * parsed out of the LLM reply then silently dropped. This table is
 * where those fields land so downstream scheduling, HITL, retries
 * and budgeting can actually use them.
 *
 * The table is keyed by task_id (1:1 with tasks). store_task_plan
 * upserts to keep "decompose writes, measure updates" idempotent.
 */

TaskPlanResult store_task_plan(Db *db, TaskPlanQuery q)
    __attribute__((warn_unused_result));
TaskPlanResult fetch_task_plan(Db *db, int64_t task_id)
    __attribute__((warn_unused_result));

TaskPlanResult store_task_plan(Db *db, TaskPlanQuery q)
{
    TaskPlanResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO task_plan(task_id,depends_on,effort_minutes,"
        "est_cost_cents,requires_hitl,success_criteria,next_action,"
        "score_0_100,attempts) VALUES(?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(task_id) DO UPDATE SET"
        "  depends_on=excluded.depends_on,"
        "  effort_minutes=excluded.effort_minutes,"
        "  est_cost_cents=excluded.est_cost_cents,"
        "  requires_hitl=excluded.requires_hitl,"
        "  success_criteria=excluded.success_criteria,"
        "  next_action=CASE WHEN excluded.next_action='' "
        "    THEN task_plan.next_action ELSE excluded.next_action END,"
        "  score_0_100=CASE WHEN excluded.score_0_100=0 "
        "    THEN task_plan.score_0_100 ELSE excluded.score_0_100 END,"
        "  attempts=task_plan.attempts+excluded.attempts;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle || q.task_id <= 0) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.task_id);
    sqlite3_bind_text (stmt, 2, q.plan.depends_on, -1, SQLITE_STATIC);
    sqlite3_bind_int  (stmt, 3, q.plan.effort_minutes);
    sqlite3_bind_int  (stmt, 4, q.plan.est_cost_cents);
    sqlite3_bind_int  (stmt, 5, q.plan.requires_hitl ? 1 : 0);
    sqlite3_bind_text (stmt, 6, q.plan.success_criteria, -1, SQLITE_STATIC);
    sqlite3_bind_text (stmt, 7, q.plan.next_action, -1, SQLITE_STATIC);
    sqlite3_bind_int  (stmt, 8, q.plan.score_0_100);
    sqlite3_bind_int  (stmt, 9, q.plan.attempts);
    r.err = (sqlite3_step(stmt) == SQLITE_DONE) ? ERR_OK : ERR_DB;
    sqlite3_finalize(stmt);
    r.plan         = q.plan;
    r.plan.task_id = q.task_id;
    return r;
}

TaskPlanResult fetch_task_plan(Db *db, int64_t task_id)
{
    TaskPlanResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT depends_on,effort_minutes,est_cost_cents,requires_hitl,"
        "success_criteria,next_action,score_0_100,attempts"
        " FROM task_plan WHERE task_id=? LIMIT 1;";
    const char *s;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle || task_id <= 0) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, task_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.plan.task_id        = task_id;
        s = (const char *)sqlite3_column_text(stmt, 0);
        if (s) snprintf(r.plan.depends_on, sizeof(r.plan.depends_on), "%s", s);
        r.plan.effort_minutes = sqlite3_column_int(stmt, 1);
        r.plan.est_cost_cents = sqlite3_column_int(stmt, 2);
        r.plan.requires_hitl  = sqlite3_column_int(stmt, 3);
        s = (const char *)sqlite3_column_text(stmt, 4);
        if (s) snprintf(r.plan.success_criteria,
                        sizeof(r.plan.success_criteria), "%s", s);
        s = (const char *)sqlite3_column_text(stmt, 5);
        if (s) snprintf(r.plan.next_action,
                        sizeof(r.plan.next_action), "%s", s);
        r.plan.score_0_100    = sqlite3_column_int(stmt, 6);
        r.plan.attempts       = sqlite3_column_int(stmt, 7);
        r.err = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
