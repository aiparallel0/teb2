#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * Finance HITL approval store.
 *
 * store_approval   — insert a new pending row.
 * list_approvals   — list by user_id; status filter optional (if q.status[0]).
 * update_approval  — set status to "approved"/"denied" by id; returns the row.
 */

static Approval row_to_approval(sqlite3_stmt *s)
{
    Approval a;
    const char *t;
    memset(&a, 0, sizeof(a));
    a.id           = sqlite3_column_int64(s, 0);
    t = (const char *)sqlite3_column_text(s, 1);
    if (t) snprintf(a.user_id, sizeof(a.user_id), "%s", t);
    t = (const char *)sqlite3_column_text(s, 2);
    if (t) snprintf(a.kind,    sizeof(a.kind),    "%s", t);
    a.amount_cents = sqlite3_column_int64(s, 3);
    t = (const char *)sqlite3_column_text(s, 4);
    if (t) snprintf(a.payload, sizeof(a.payload), "%s", t);
    t = (const char *)sqlite3_column_text(s, 5);
    if (t) snprintf(a.status,  sizeof(a.status),  "%s", t);
    t = (const char *)sqlite3_column_text(s, 6);
    if (t) snprintf(a.risk,    sizeof(a.risk),    "%s", t);
    a.created_at   = sqlite3_column_int64(s, 7);
    return a;
}

ApprovalResult store_approval(Db *db, ApprovalQuery q)
{
    ApprovalResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO approvals(user_id,kind,amount_cents,payload,risk)"
        " VALUES(?,?,?,?,?)"
        " RETURNING id,user_id,kind,amount_cents,payload,status,risk,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle || !q.user_id[0]) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text (s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text (s, 2, q.kind[0] ? q.kind : "finance", -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 3, q.amount_cents);
    sqlite3_bind_text (s, 4, q.payload, -1, SQLITE_STATIC);
    sqlite3_bind_text (s, 5, q.risk,    -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.ap  = row_to_approval(s);
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(s);
    return r;
}

ApprovalResult list_approvals(Db *db, ApprovalQuery q)
{
    ApprovalResult r;
    sqlite3_stmt *s = NULL;
    const char *sql;
    int cap, n = 0, limit;
    memset(&r, 0, sizeof(r));
    cap = (int)(sizeof(r.rows) / sizeof(r.rows[0]));
    limit = (q.limit > 0 && q.limit <= cap) ? q.limit : cap;
    if (!db || !db->handle) { r.err = ERR_DB; return r; }

    if (q.status[0]) {
        sql = "SELECT id,user_id,kind,amount_cents,payload,status,risk,created_at"
              " FROM approvals WHERE user_id=? AND status=?"
              " ORDER BY created_at DESC LIMIT ?;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        { r.err = ERR_DB; return r; }
        sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(s, 2, q.status,  -1, SQLITE_STATIC);
        sqlite3_bind_int (s, 3, limit);
    } else {
        sql = "SELECT id,user_id,kind,amount_cents,payload,status,risk,created_at"
              " FROM approvals WHERE user_id=?"
              " ORDER BY created_at DESC LIMIT ?;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        { r.err = ERR_DB; return r; }
        sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
        sqlite3_bind_int (s, 2, limit);
    }
    while (n < cap && sqlite3_step(s) == SQLITE_ROW) {
        r.rows[n++] = row_to_approval(s);
    }
    r.count = n;
    r.err   = ERR_OK;
    sqlite3_finalize(s);
    return r;
}

ApprovalResult update_approval(Db *db, ApprovalQuery q)
{
    ApprovalResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "UPDATE approvals SET status=? WHERE id=? AND user_id=?"
        " RETURNING id,user_id,kind,amount_cents,payload,status,risk,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle || !q.status[0] || q.id <= 0) {
        r.err = ERR_DB; return r;
    }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text (s, 1, q.status,  -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 2, q.id);
    sqlite3_bind_text (s, 3, q.user_id, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.ap  = row_to_approval(s);
        r.err = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(s);
    return r;
}
