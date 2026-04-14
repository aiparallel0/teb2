#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static const char *SCHEMA_USERS =
    "CREATE TABLE IF NOT EXISTS users("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "email TEXT NOT NULL UNIQUE,"
    "password_hash TEXT NOT NULL,"
    "role INTEGER NOT NULL DEFAULT 0,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));";

static void ensure_users_schema(struct sqlite3 *h)
{
    char *err = NULL;
    (void)sqlite3_exec(h, SCHEMA_USERS, NULL, NULL, &err);
    sqlite3_free(err);
}

static User row_to_user(sqlite3_stmt *stmt)
{
    User u;
    const char *s;
    memset(&u, 0, sizeof(u));
    u.id = sqlite3_column_int64(stmt, 0);
    s    = (const char *)sqlite3_column_text(stmt, 1);
    if (s) snprintf(u.email, sizeof(u.email), "%s", s);
    s    = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(u.password_hash, sizeof(u.password_hash), "%s", s);
    u.role = (UserRole)sqlite3_column_int(stmt, 3);
    snprintf(u.user_id, sizeof(u.user_id), "%lld", (long long)u.id);
    return u;
}

UserResult fetch_user(Db *db, UserQuery q)
{
    UserResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_users_schema(db->handle);
    if (q.id > 0) {
        sql = "SELECT id,email,password_hash,role FROM users WHERE id=? LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
            r.err = ERR_DB; return r;
        }
        sqlite3_bind_int64(stmt, 1, q.id);
    } else {
        sql = "SELECT id,email,password_hash,role FROM users WHERE email=? LIMIT 1;";
        if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
            r.err = ERR_DB; return r;
        }
        sqlite3_bind_text(stmt, 1, q.email, -1, SQLITE_STATIC);
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.user = row_to_user(stmt);
        r.err  = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

UserResult store_user(Db *db, UserQuery q)
{
    UserResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO users(email,password_hash,role) VALUES(?,?,?)"
                      " RETURNING id;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    ensure_users_schema(db->handle);
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt,  3, (int)q.role);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.user.id = sqlite3_column_int64(stmt, 0);
        snprintf(r.user.email, sizeof(r.user.email), "%s", q.email);
        snprintf(r.user.password_hash, sizeof(r.user.password_hash),
                 "%s", q.password_hash);
        snprintf(r.user.user_id, sizeof(r.user.user_id), "%lld",
                 (long long)r.user.id);
        r.user.role = q.role;
        r.err = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}
