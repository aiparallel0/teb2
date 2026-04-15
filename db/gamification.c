#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

XpResult credit_xp(Db *db, XpQuery q)
{
    XpResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO xp_events(user_id,amount,reason)"
        " VALUES(?,?,?) RETURNING id,user_id,amount,reason,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(s, 2, q.amount);
    sqlite3_bind_text(s, 3, q.reason, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.xp.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.xp.user_id, sizeof(r.xp.user_id), "%s", t);
        r.xp.amount = sqlite3_column_int(s, 2);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.xp.reason, sizeof(r.xp.reason), "%s", t);
        r.xp.created_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

StreakResult check_streak(Db *db, StreakQuery q)
{
    StreakResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,user_id,current,longest,updated_at"
        " FROM streaks WHERE user_id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.streak.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.streak.user_id,
                        sizeof(r.streak.user_id), "%s", t);
        r.streak.current = sqlite3_column_int(s, 2);
        r.streak.longest = sqlite3_column_int(s, 3);
        r.streak.updated_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

XpResult list_xp(Db *db, XpQuery q)
{
    XpResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT user_id,SUM(amount) as total"
        " FROM xp_events GROUP BY user_id ORDER BY total DESC LIMIT ?;";
    int lim;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 16) ? q.limit : 10;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int(s, 1, lim);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        t = (const char *)sqlite3_column_text(s, 0);
        if (t) snprintf(r.xp.user_id, sizeof(r.xp.user_id), "%s", t);
        r.xp.amount = sqlite3_column_int(s, 1);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
