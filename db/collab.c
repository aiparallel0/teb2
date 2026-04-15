#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "db/db.h"

WsResult store_ws(Db *db, WsQuery q)
{
    WsResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO workspaces(owner_id,name) VALUES(?,?)"
        " RETURNING id,owner_id,name,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.owner_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.name[0] ? q.name : "workspace",
                      -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.ws.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.ws.owner_id, sizeof(r.ws.owner_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.ws.name, sizeof(r.ws.name), "%s", t);
        r.ws.created_at = sqlite3_column_int64(s, 3);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

WsResult fetch_ws(Db *db, WsQuery q)
{
    WsResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,owner_id,name,created_at FROM workspaces"
        " WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.ws.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.ws.owner_id, sizeof(r.ws.owner_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.ws.name, sizeof(r.ws.name), "%s", t);
        r.ws.created_at = sqlite3_column_int64(s, 3);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

CollabResult store_collab(Db *db, CollabQuery q)
{
    CollabResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO collaborators(ws_id,user_id,role_name)"
        " VALUES(?,?,?) RETURNING id,ws_id,user_id,role_name,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.ws_id);
    sqlite3_bind_text(s, 2, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.role_name[0] ? q.role_name : "member",
                      -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.rows[0].id = sqlite3_column_int64(s, 0);
        r.rows[0].ws_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.rows[0].user_id,
                        sizeof(r.rows[0].user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.rows[0].role_name,
                        sizeof(r.rows[0].role_name), "%s", t);
        r.rows[0].created_at = sqlite3_column_int64(s, 4);
        r.count = 1; r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

CollabResult list_collabs(Db *db, CollabQuery q)
{
    CollabResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,ws_id,user_id,role_name,created_at"
        " FROM collaborators WHERE ws_id=? LIMIT ?;";
    int lim;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 16) ? q.limit : 16;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.ws_id);
    sqlite3_bind_int(s, 2, lim);
    while (sqlite3_step(s) == SQLITE_ROW && r.count < 16) {
        const char *t;
        r.rows[r.count].id = sqlite3_column_int64(s, 0);
        r.rows[r.count].ws_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.rows[r.count].user_id,
                        sizeof(r.rows[r.count].user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.rows[r.count].role_name,
                        sizeof(r.rows[r.count].role_name), "%s", t);
        r.rows[r.count].created_at = sqlite3_column_int64(s, 4);
        r.count++;
    }
    r.err = ERR_OK;
    sqlite3_finalize(s);
    return r;
}
