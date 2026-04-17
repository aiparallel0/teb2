#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static MemEntry row_to_mem(sqlite3_stmt *stmt)
{
    MemEntry e;
    const char *s;
    memset(&e, 0, sizeof(e));
    e.id = sqlite3_column_int64(stmt, 0);
    s = (const char *)sqlite3_column_text(stmt, 1);
    if (s) snprintf(e.agent, sizeof(e.agent), "%s", s);
    s = (const char *)sqlite3_column_text(stmt, 2);
    if (s) snprintf(e.key, sizeof(e.key), "%s", s);
    s = (const char *)sqlite3_column_text(stmt, 3);
    if (s) snprintf(e.val, sizeof(e.val), "%s", s);
    e.ts = sqlite3_column_int64(stmt, 4);
    return e;
}

MemResult store_mem(Db *db, MemQuery q)
{
    MemResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO agent_memory(agent,key,val) VALUES(?,?,?)"
        " ON CONFLICT(agent,key) DO UPDATE SET val=excluded.val,"
        "ts=(strftime('%s','now'))"
        " RETURNING id,agent,key,val,ts;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.agent, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.key,   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, q.val,   -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.entry = row_to_mem(stmt);
        r.err   = ERR_OK;
    } else {
        r.err = ERR_DB;
    }
    sqlite3_finalize(stmt);
    return r;
}

MemResult fetch_mem(Db *db, MemQuery q)
{
    MemResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,agent,key,val,ts FROM agent_memory"
                      " WHERE agent=? AND key=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, q.agent, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, q.key,   -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        r.entry = row_to_mem(stmt);
        r.err   = ERR_OK;
    } else {
        r.err = ERR_NOT_FOUND;
    }
    sqlite3_finalize(stmt);
    return r;
}

/* List up to `limit` recent memory rows matching agent and key prefix. */
MemListResult list_mem(Db *db, const char *agent_name,
                       const char *key_prefix, int limit)
{
    MemListResult r;
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id,agent,key,val,ts FROM agent_memory"
                      " WHERE agent=? AND key LIKE ?"
                      " ORDER BY ts DESC LIMIT ?;";
    char pat[144];
    int cap, n = 0;

    memset(&r, 0, sizeof(r));
    cap = (int)(sizeof(r.rows) / sizeof(r.rows[0]));
    if (limit <= 0 || limit > cap) limit = cap;
    if (!db || !db->handle || !agent_name) { r.err = ERR_DB; return r; }
    snprintf(pat, sizeof(pat), "%s%%", key_prefix ? key_prefix : "");
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(stmt, 1, agent_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pat,        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, limit);
    while (n < cap && sqlite3_step(stmt) == SQLITE_ROW) {
        r.rows[n++] = row_to_mem(stmt);
    }
    r.count = n;
    r.err   = ERR_OK;
    sqlite3_finalize(stmt);
    return r;
}
