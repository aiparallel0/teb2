#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

BudgetResult store_budget(Db *db, BudgetQuery q)
{
    BudgetResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO budgets(user_id,limit_cents) VALUES(?,?)"
        " ON CONFLICT(user_id) DO UPDATE SET limit_cents=excluded.limit_cents"
        " RETURNING id,user_id,limit_cents,spent_cents;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, q.amount_cents);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.budget.id          = sqlite3_column_int64(stmt, 0);
        snprintf(r.budget.user_id, sizeof(r.budget.user_id), "%s",
                 (const char *)sqlite3_column_text(stmt, 1));
        r.budget.limit_cents = sqlite3_column_int64(stmt, 2);
        r.budget.spent_cents = sqlite3_column_int64(stmt, 3);
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

BudgetResult fetch_budget(Db *db, BudgetQuery q)
{
    BudgetResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,user_id,limit_cents,spent_cents FROM budgets"
                      " WHERE user_id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.budget.id          = sqlite3_column_int64(stmt, 0);
        snprintf(r.budget.user_id, sizeof(r.budget.user_id), "%s",
                 (const char *)sqlite3_column_text(stmt, 1));
        r.budget.limit_cents = sqlite3_column_int64(stmt, 2);
        r.budget.spent_cents = sqlite3_column_int64(stmt, 3);
        r.err = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

BudgetResult record_spend(Db *db, BudgetQuery q)
{
    BudgetResult r;
    sqlite3_stmt *stmt = NULL;
    const char *ins = "INSERT INTO spending(user_id,amount_cents) VALUES(?,?);";
    const char *upd = "UPDATE budgets SET spent_cents=spent_cents+?"
                      " WHERE user_id=?"
                      " RETURNING id,user_id,limit_cents,spent_cents;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, ins, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, q.amount_cents);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        r.err = ERR_DB; return r;
    }
    sqlite3_finalize(stmt);
    stmt = NULL;
    if (sqlite3_prepare_v2(db->handle, upd, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(stmt, 1, q.amount_cents);
    sqlite3_bind_text(stmt, 2, q.user_id, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.budget.id          = sqlite3_column_int64(stmt, 0);
        snprintf(r.budget.user_id, sizeof(r.budget.user_id), "%s",
                 (const char *)sqlite3_column_text(stmt, 1));
        r.budget.limit_cents = sqlite3_column_int64(stmt, 2);
        r.budget.spent_cents = sqlite3_column_int64(stmt, 3);
        r.err = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}
