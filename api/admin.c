#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/log.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"

/*
 * Admin account seeding + admin dashboard API.
 *
 * seed_admin_account: called once from main.c after db_open. If
 * ADMIN_EMAIL + ADMIN_PASSWORD are set in the env/config, the admin
 * user is created (or promoted) idempotently:
 *   INSERT ... ON CONFLICT DO UPDATE SET role=ROLE_ADMIN
 * The password is only written if the row is freshly inserted; existing
 * rows keep their current password so /auth/reset still works.
 *
 * Admin API endpoints (all require ROLE_ADMIN):
 *   GET  /admin/stats        — aggregate counts
 *   GET  /admin/users        — paginated user list
 *   POST /admin/users/:id/role — promote or demote a user
 */

static int require_admin(Ctx *ctx)
{
    return ctx && ctx->user && ctx->user->role == ROLE_ADMIN;
}

void seed_admin_account(Db *db, Config *cfg)
{
    HashConfig hcfg;
    HashResult hr;
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    if (!db || !cfg) return;
    if (!cfg->admin_email[0] || !cfg->admin_password[0]) return;
    if (strlen(cfg->admin_password) < 8) {
        teb_log_warn("admin", "ADMIN_PASSWORD too short — skip seed");
        return;
    }
    hcfg = default_hash_config();
    hr   = hash_password(cfg->admin_password, hcfg);
    if (hr.err != ERR_OK) { teb_log_warn("admin", "hash failed"); return; }
    /* Upsert: insert admin or promote existing user; never demote. */
    sql = "INSERT INTO users(email,password_hash,role) VALUES(?,?,1)"
          " ON CONFLICT(email) DO UPDATE SET role=1"
          " WHERE role<1;";
    if (sqlite3_prepare_v2(db->handle, sql, -1, &stmt, NULL) != SQLITE_OK)
        return;
    sqlite3_bind_text(stmt, 1, cfg->admin_email, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hr.hash,          -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        teb_log_info("admin", "seed ok email=%s", cfg->admin_email);
    else
        teb_log_warn("admin", "seed sql_err");
    sqlite3_finalize(stmt);
}

HttpResp handle_admin_stats(HttpReq req, Ctx *ctx)
{
    sqlite3_stmt *stmt = NULL;
    int64_t users, goals, tasks, pending;
    char buf[256];
    const char *sql =
        "SELECT"
        " (SELECT COUNT(*) FROM users),"
        " (SELECT COUNT(*) FROM goals),"
        " (SELECT COUNT(*) FROM tasks),"
        " (SELECT COUNT(*) FROM approvals WHERE status='pending');";
    (void)req;
    if (!require_admin(ctx)) return json_error(403, "admin_required");
    if (sqlite3_prepare_v2(ctx->db->handle, sql, -1, &stmt, NULL) != SQLITE_OK)
        return json_error(500, "db_error");
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return json_error(500, "db_error");
    }
    users   = sqlite3_column_int64(stmt, 0);
    goals   = sqlite3_column_int64(stmt, 1);
    tasks   = sqlite3_column_int64(stmt, 2);
    pending = sqlite3_column_int64(stmt, 3);
    sqlite3_finalize(stmt);
    snprintf(buf, sizeof(buf),
             "{\"users\":%lld,\"goals\":%lld,\"tasks\":%lld"
             ",\"pending_approvals\":%lld}",
             (long long)users, (long long)goals,
             (long long)tasks, (long long)pending);
    return json_ok(buf);
}

HttpResp handle_admin_users(HttpReq req, Ctx *ctx)
{
    UserListResult r;
    char buf[8192];
    size_t off = 0;
    int i;
    (void)req;
    if (!require_admin(ctx)) return json_error(403, "admin_required");
    r = list_users(ctx->db, 50);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    off += (size_t)snprintf(buf + off, sizeof(buf) - off, "{\"users\":[");
    for (i = 0; i < r.count; i++) {
        if (i) buf[off++] = ',';
        off += (size_t)snprintf(buf + off, sizeof(buf) - off,
            "{\"id\":%lld,\"email\":\"%s\",\"role\":%d}",
            (long long)r.rows[i].id, r.rows[i].email, (int)r.rows[i].role);
    }
    snprintf(buf + off, sizeof(buf) - off, "]}");
    return json_ok(buf);
}

HttpResp handle_admin_role(HttpReq req, Ctx *ctx)
{
    int64_t uid;
    char role_str[16];
    UserRole new_role;
    if (!require_admin(ctx)) return json_error(403, "admin_required");
    uid = strtoll(req.path + 13, NULL, 10); /* /admin/users/<id>/role */
    if (uid <= 0) return json_error(400, "bad_id");
    if (!extract_json_str(req.body, "\"role\"", role_str, sizeof(role_str)))
        return json_error(400, "missing_role");
    if (strcmp(role_str, "admin") == 0)   new_role = ROLE_ADMIN;
    else if (strcmp(role_str, "user") == 0) new_role = ROLE_USER;
    else return json_error(400, "invalid_role");
    if (update_user_role(ctx->db, uid, new_role) != ERR_OK)
        return json_error(500, "db_error");
    return json_ok("{\"ok\":true}");
}
