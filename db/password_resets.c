#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

/*
 * Password-reset tokens.
 *
 * One row per issued token. A token is single-use: consume_password_reset
 * atomically matches an unused, non-expired token and marks it used in a
 * single UPDATE ... WHERE ... RETURNING, so a concurrent request cannot
 * redeem the same token twice. Expired/used rows are left in place for
 * audit; prune later if needed.
 */

Err create_password_reset(Db *db, int64_t user_id,
                          const char *token, int64_t expiry)
{
    sqlite3_stmt *s = NULL;
    const char *sql = "INSERT INTO password_resets(token,user_id,expiry)"
                      " VALUES(?,?,?);";
    int rc;
    if (!db || !db->handle || !token || !token[0]) return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_text (s, 1, token, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 2, user_id);
    sqlite3_bind_int64(s, 3, expiry);
    rc = sqlite3_step(s);
    sqlite3_finalize(s);
    return (rc == SQLITE_DONE) ? ERR_OK : ERR_DB;
}

Err consume_password_reset(Db *db, const char *token, int64_t *user_id_out)
{
    sqlite3_stmt *s = NULL;
    const char *sql =
        "UPDATE password_resets SET used=1"
        " WHERE token=? AND used=0 AND expiry >= ?"
        " RETURNING user_id;";
    int rc;
    if (!db || !db->handle || !token || !token[0] || !user_id_out)
        return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_text (s, 1, token, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 2, (int64_t)time(NULL));
    rc = sqlite3_step(s);
    if (rc == SQLITE_ROW) {
        *user_id_out = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);
        return ERR_OK;
    }
    sqlite3_finalize(s);
    return ERR_NOT_FOUND;
}

Err update_user_password(Db *db, int64_t user_id, const char *password_hash)
{
    sqlite3_stmt *s = NULL;
    const char *sql = "UPDATE users SET password_hash=? WHERE id=?;";
    int rc;
    if (!db || !db->handle || !password_hash || !password_hash[0])
        return ERR_DB;
    if (sqlite3_prepare_v2(db->handle, sql, -1, &s, NULL) != SQLITE_OK)
        return ERR_DB;
    sqlite3_bind_text (s, 1, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int64(s, 2, user_id);
    rc = sqlite3_step(s);
    sqlite3_finalize(s);
    if (rc != SQLITE_DONE) return ERR_DB;
    return (sqlite3_changes(db->handle) == 1) ? ERR_OK : ERR_NOT_FOUND;
}
