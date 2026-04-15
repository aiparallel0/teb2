#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "db/db.h"

IntegResult store_integ(Db *db, IntegQuery q)
{
    IntegResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO integrations(name,kind,enabled) VALUES(?,?,?)"
        " ON CONFLICT(name) DO UPDATE SET enabled=excluded.enabled"
        " RETURNING id,name,kind,webhook_url,enabled,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.kind, -1, SQLITE_STATIC);
    sqlite3_bind_int(s, 3, q.enabled);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.integ.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.integ.name, sizeof(r.integ.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.integ.kind, sizeof(r.integ.kind), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.integ.webhook_url,
                        sizeof(r.integ.webhook_url), "%s", t);
        r.integ.enabled = sqlite3_column_int(s, 4);
        r.integ.created_at = sqlite3_column_int64(s, 5);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

IntegResult fetch_integ(Db *db, IntegQuery q)
{
    IntegResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,name,kind,webhook_url,enabled,created_at"
        " FROM integrations WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.integ.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.integ.name, sizeof(r.integ.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.integ.kind, sizeof(r.integ.kind), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.integ.webhook_url,
                        sizeof(r.integ.webhook_url), "%s", t);
        r.integ.enabled = sqlite3_column_int(s, 4);
        r.integ.created_at = sqlite3_column_int64(s, 5);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

OAuthResult store_oauth(Db *db, OAuthQuery q)
{
    OAuthResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO oauth_tokens(user_id,provider,access_tok,refresh_tok,expiry)"
        " VALUES(?,?,?,?,?) ON CONFLICT(user_id,provider)"
        " DO UPDATE SET access_tok=excluded.access_tok,"
        "refresh_tok=excluded.refresh_tok,expiry=excluded.expiry"
        " RETURNING id,user_id,provider,access_tok,refresh_tok,expiry;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.provider, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.access_tok, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 4, q.refresh_tok, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 5, q.expiry);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.tok.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.tok.user_id, sizeof(r.tok.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.tok.provider, sizeof(r.tok.provider), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.tok.access_tok, sizeof(r.tok.access_tok), "%s", t);
        t = (const char *)sqlite3_column_text(s, 4);
        if (t) snprintf(r.tok.refresh_tok, sizeof(r.tok.refresh_tok), "%s", t);
        r.tok.expiry = sqlite3_column_int64(s, 5);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

OAuthResult fetch_oauth(Db *db, OAuthQuery q)
{
    OAuthResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,user_id,provider,access_tok,refresh_tok,expiry"
        " FROM oauth_tokens WHERE user_id=? AND provider=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.provider, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.tok.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.tok.user_id, sizeof(r.tok.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.tok.provider, sizeof(r.tok.provider), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.tok.access_tok, sizeof(r.tok.access_tok), "%s", t);
        t = (const char *)sqlite3_column_text(s, 4);
        if (t) snprintf(r.tok.refresh_tok, sizeof(r.tok.refresh_tok), "%s", t);
        r.tok.expiry = sqlite3_column_int64(s, 5);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
