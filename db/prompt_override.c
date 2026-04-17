#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * Per-user prompt overrides — runtime customization.
 * Users can replace any built-in prompt with their own version.
 * prompt_get_with_override() checks this table before falling back
 * to the compiled prompt. Overrides are scoped per user so one
 * tenant's edits never affect another.
 */

OverrideResult store_override(Db *db, OverrideQuery q)
{
    OverrideResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO prompt_overrides(user_id,name,body,"
        "updated_at) VALUES(?,?,?,strftime('%s','now'))"
        " ON CONFLICT(user_id,name) DO UPDATE SET body=excluded.body,"
        "updated_at=excluded.updated_at"
        " RETURNING id,user_id,name,body,updated_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
    { r.err = ERR_DB; return r; }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.body, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.ov.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.ov.user_id, sizeof(r.ov.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.ov.name, sizeof(r.ov.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.ov.body, sizeof(r.ov.body), "%s", t);
        r.ov.updated_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

OverrideResult fetch_override(Db *db, const char *user_id, const char *name)
{
    OverrideResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT id,user_id,name,body,updated_at FROM prompt_overrides"
        " WHERE user_id=? AND name=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
    { r.err = ERR_DB; return r; }
    sqlite3_bind_text(s, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, name, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.ov.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.ov.user_id, sizeof(r.ov.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.ov.name, sizeof(r.ov.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.ov.body, sizeof(r.ov.body), "%s", t);
        r.ov.updated_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

Err delete_override(Db *db, const char *user_id, const char *name)
{
    sqlite3_stmt *s = NULL;
    const char *sql =
        "DELETE FROM prompt_overrides WHERE user_id=? AND name=?;";
    if (!db || !db->handle) return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_text(s, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, name, -1, SQLITE_STATIC);
    sqlite3_step(s);
    sqlite3_finalize(s);
    return ERR_OK;
}
