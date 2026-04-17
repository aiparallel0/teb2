#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * Mutation audit trail.
 *
 * Every HTTP request that reaches a mutating handler (POST/PUT/DELETE)
 * SHOULD produce exactly one row here. We keep the columns small and
 * indexed so "who did what from where, with what outcome" is one SQL
 * query away in production. This table is append-only by convention;
 * no API route deletes from it.
 *
 * The caller is expected to pass:
 *   user_id -- "" if unauthenticated
 *   action  -- HTTP method (e.g. "POST")
 *   path    -- request path, already stripped of query string
 *   status  -- integer HTTP status returned to the client
 *   src_ip  -- X-Forwarded-For value or "" if unknown
 *
 * On DB failure we return the error code but never abort the caller --
 * a broken audit table must not deny service; the caller has already
 * taken the action. It does, however, surface ERR_DB so an operator
 * monitoring logs can see the audit gap.
 */
Err audit_log(Db *db, const char *user_id, const char *action,
              const char *path, int status, const char *src_ip)
{
    sqlite3_stmt *st = NULL;
    const char *sql =
        "INSERT INTO audit_log(user_id,action,path,status,src_ip) "
        "VALUES(?,?,?,?,?)";
    int rc;
    if (!db || !db->handle || !action || !path) return ERR_DB;
    rc = sqlite3_prepare_v2(db->handle, sql, -1, &st, NULL);
    if (rc != SQLITE_OK) return ERR_DB;
    sqlite3_bind_text(st, 1, user_id ? user_id : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, action,                 -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, path,                   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (st, 4, status);
    sqlite3_bind_text(st, 5, src_ip ? src_ip : "",   -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return (rc == SQLITE_DONE) ? ERR_OK : ERR_DB;
}
