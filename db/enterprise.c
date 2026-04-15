#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

OrgResult store_org(Db *db, OrgQuery q)
{
    OrgResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO orgs(name,domain) VALUES(?,?)"
        " RETURNING id,name,domain,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.domain, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.org.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.org.name, sizeof(r.org.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.org.domain, sizeof(r.org.domain), "%s", t);
        r.org.created_at = sqlite3_column_int64(s, 3);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

OrgResult fetch_org(Db *db, OrgQuery q)
{
    OrgResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,name,domain,created_at FROM orgs"
        " WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.org.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.org.name, sizeof(r.org.name), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.org.domain, sizeof(r.org.domain), "%s", t);
        r.org.created_at = sqlite3_column_int64(s, 3);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

SsoResult store_sso(Db *db, SsoQuery q)
{
    SsoResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO sso_configs(org_id,provider,endpoint)"
        " VALUES(?,?,?) RETURNING id,org_id,provider,endpoint,cert;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.org_id);
    sqlite3_bind_text(s, 2, q.provider, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.provider, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.sso.id = sqlite3_column_int64(s, 0);
        r.sso.org_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.sso.provider, sizeof(r.sso.provider), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.sso.endpoint, sizeof(r.sso.endpoint), "%s", t);
        t = (const char *)sqlite3_column_text(s, 4);
        if (t) snprintf(r.sso.cert, sizeof(r.sso.cert), "%s", t);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

IpResult check_ip(Db *db, IpQuery q)
{
    IpResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,org_id,cidr FROM ip_allowlist"
        " WHERE org_id=? AND cidr=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.org_id);
    sqlite3_bind_text(s, 2, q.cidr, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.ip.id = sqlite3_column_int64(s, 0);
        r.ip.org_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.ip.cidr, sizeof(r.ip.cidr), "%s", t);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}
