#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "db/db.h"

SearchResult full_text_search(Db *db, SearchQuery q)
{
    SearchResult r;
    sqlite3_stmt *s = NULL;
    int lim;
    const char *sql_all =
        "SELECT entity,entity_id,snippet(search_idx,2,'','','...',10)"
        " FROM search_idx WHERE search_idx MATCH ? LIMIT ?;";
    const char *sql_ent =
        "SELECT entity,entity_id,snippet(search_idx,2,'','','...',10)"
        " FROM search_idx WHERE search_idx MATCH ? AND entity=? LIMIT ?;";
    const char *use_sql;
    const char *t;

    memset(&r, 0, sizeof(r));
    if (!db || !db->handle) { r.err = ERR_DB; return r; }
    lim = (q.limit > 0 && q.limit <= 32) ? q.limit : 20;
    use_sql = q.entity[0] ? sql_ent : sql_all;
    if (sqlite3_prepare_v2(db->handle, use_sql, -1, &s, NULL) != SQLITE_OK) {
        r.err = ERR_DB; return r;
    }
    sqlite3_bind_text(s, 1, q.query, -1, SQLITE_STATIC);
    if (q.entity[0]) {
        sqlite3_bind_text(s, 2, q.entity, -1, SQLITE_STATIC);
        sqlite3_bind_int(s, 3, lim);
    } else {
        sqlite3_bind_int(s, 2, lim);
    }
    while (sqlite3_step(s) == SQLITE_ROW && r.count < 32) {
        t = (const char *)sqlite3_column_text(s, 0);
        if (t) snprintf(r.hits[r.count].entity,
                        sizeof(r.hits[r.count].entity), "%s", t);
        r.hits[r.count].entity_id = sqlite3_column_int64(s, 1);
        t = (const char *)sqlite3_column_text(s, 2);
        if (t) snprintf(r.hits[r.count].snippet,
                        sizeof(r.hits[r.count].snippet), "%s", t);
        r.count++;
    }
    r.err = ERR_OK;
    sqlite3_finalize(s);
    return r;
}

Err index_entity(Db *db, const char *entity,
                 int64_t entity_id, const char *content)
{
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO search_idx(entity,entity_id,content)"
        " VALUES(?,?,?);";
    int rc;

    if (!db || !db->handle) return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_text(s, 1, entity, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 2, entity_id);
    sqlite3_bind_text(s, 3, content, -1, SQLITE_STATIC);
    rc = sqlite3_step(s);
    sqlite3_finalize(s);
    return (rc == SQLITE_DONE) ? ERR_OK : ERR_DB;
}
