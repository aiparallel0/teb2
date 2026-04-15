#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "db/db.h"

ChatResult store_chat(Db *db, ChatQuery q)
{
    ChatResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO chat_messages(ws_id,sender_id,body)"
        " VALUES(?,?,?) RETURNING id,ws_id,sender_id,body,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.ws_id);
    sqlite3_bind_text(s, 2, q.sender_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.body, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.rows[0].id = sqlite3_column_int64(s, 0);
        r.rows[0].ws_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.rows[0].sender_id,
                        sizeof(r.rows[0].sender_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.rows[0].body, sizeof(r.rows[0].body), "%s", t);
        r.rows[0].created_at = sqlite3_column_int64(s, 4);
        r.count = 1; r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

ChatResult list_chats(Db *db, ChatQuery q)
{
    ChatResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,ws_id,sender_id,body,created_at"
        " FROM chat_messages WHERE ws_id=? AND id > ?"
        " ORDER BY id ASC LIMIT ?;";
    int lim;
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 16) ? q.limit : 16;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.ws_id);
    sqlite3_bind_int64(s, 2, q.cursor);
    sqlite3_bind_int(s, 3, lim);
    while (sqlite3_step(s) == SQLITE_ROW && r.count < 16) {
        const char *t;
        r.rows[r.count].id = sqlite3_column_int64(s, 0);
        r.rows[r.count].ws_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.rows[r.count].sender_id,
                        sizeof(r.rows[r.count].sender_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.rows[r.count].body,
                        sizeof(r.rows[r.count].body), "%s", t);
        r.rows[r.count].created_at = sqlite3_column_int64(s, 4);
        r.count++;
    }
    r.err = ERR_OK;
    sqlite3_finalize(s);
    return r;
}
