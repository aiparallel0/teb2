#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

static Asset row_to_asset(sqlite3_stmt *s)
{
    Asset a;
    const char *t;
    memset(&a, 0, sizeof(a));
    a.id = sqlite3_column_int64(s, 0);
    t = (const char *)sqlite3_column_text(s, 1);
    if (t) snprintf(a.user_id, sizeof(a.user_id), "%s", t);
    t = (const char *)sqlite3_column_text(s, 2);
    if (t) snprintf(a.filename, sizeof(a.filename), "%s", t);
    t = (const char *)sqlite3_column_text(s, 3);
    if (t) snprintf(a.mime_type, sizeof(a.mime_type), "%s", t);
    a.size_bytes = sqlite3_column_int64(s, 4);
    t = (const char *)sqlite3_column_text(s, 5);
    if (t) snprintf(a.path, sizeof(a.path), "%s", t);
    a.created_at = sqlite3_column_int64(s, 6);
    return a;
}

AssetResult store_asset(Db *db, AssetQuery q)
{
    AssetResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO assets(user_id,filename,mime_type,size_bytes,path)"
        " VALUES(?,?,?,?,?)"
        " RETURNING id,user_id,filename,mime_type,size_bytes,path,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.filename, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.mime_type, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 4, q.size_bytes);
    sqlite3_bind_text(s, 5, q.path, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.asset = row_to_asset(s);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

AssetResult fetch_asset(Db *db, AssetQuery q)
{
    AssetResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT id,user_id,filename,mime_type,size_bytes,path,created_at"
        " FROM assets WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.asset = row_to_asset(s);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
