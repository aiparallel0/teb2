#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

BlogResult store_blog(Db *db, BlogQuery q)
{
    BlogResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO blog_posts(user_id,title,body)"
        " VALUES(?,?,?) RETURNING id,user_id,title,body,created_at;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.title, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 3, q.body, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.post.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.post.user_id, sizeof(r.post.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.post.title, sizeof(r.post.title), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.post.body, sizeof(r.post.body), "%s", t);
        r.post.created_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}

BlogResult fetch_blog(Db *db, BlogQuery q)
{
    BlogResult r;
    sqlite3_stmt *s = NULL;
    const char *sql = "SELECT id,user_id,title,body,created_at"
        " FROM blog_posts WHERE id=? LIMIT 1;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_int64(s, 1, q.id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        const char *t;
        r.post.id = sqlite3_column_int64(s, 0);
        t = (const char *)sqlite3_column_text(s, 1);
        if (t) snprintf(r.post.user_id, sizeof(r.post.user_id), "%s", t);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.post.title, sizeof(r.post.title), "%s", t);
        t = (const char *)sqlite3_column_text(s, 3);
        if (t) snprintf(r.post.body, sizeof(r.post.body), "%s", t);
        r.post.created_at = sqlite3_column_int64(s, 4);
        r.err = ERR_OK;
    } else { r.err = ERR_NOT_FOUND; }
    sqlite3_finalize(s);
    return r;
}

VoteResult store_vote(Db *db, VoteQuery q)
{
    VoteResult r;
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO feature_votes(user_id,feature) VALUES(?,?)"
        " ON CONFLICT(user_id,feature) DO NOTHING;";
    const char *cnt = "SELECT COUNT(*) FROM feature_votes WHERE feature=?;";
    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(s, 2, q.feature, -1, SQLITE_STATIC);
    if (sqlite3_step(s) != SQLITE_DONE) {
        sqlite3_finalize(s); r.err = ERR_DB; return r;
    }
    sqlite3_finalize(s); s = NULL;
    if (sqlite3_prepare_v2(db->handle, cnt, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.feature, -1, SQLITE_STATIC);
    if (sqlite3_step(s) == SQLITE_ROW) {
        snprintf(r.vote.user_id, sizeof(r.vote.user_id), "%s", q.user_id);
        snprintf(r.vote.feature, sizeof(r.vote.feature), "%s", q.feature);
        r.count = sqlite3_column_int(s, 0);
        r.err = ERR_OK;
    } else { r.err = ERR_DB; }
    sqlite3_finalize(s);
    return r;
}
