#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_xp_credit(HttpReq req, Ctx *ctx)
{
    XpQuery q;
    XpResult r;
    char buf[256], amt[16];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    if (extract_json_str(req.body, "\"amount\"", amt, sizeof(amt)))
        q.amount = atoi(amt);
    (void)extract_json_str(req.body, "\"reason\"", q.reason, sizeof(q.reason));
    if (q.amount <= 0) return json_error(400, "bad_amount");
    r = credit_xp(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"amount\":%d}",
             (long long)r.xp.id, r.xp.amount);
    return json_ok(buf);
}

HttpResp handle_streak_get(HttpReq req, Ctx *ctx)
{
    StreakQuery q;
    StreakResult r;
    char buf[128];
    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    r = check_streak(ctx->db, q);
    if (r.err == ERR_NOT_FOUND)
        return json_ok("{\"current\":0,\"longest\":0}");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"current\":%d,\"longest\":%d}",
             r.streak.current, r.streak.longest);
    return json_ok(buf);
}

HttpResp handle_leaderboard(HttpReq req, Ctx *ctx)
{
    XpQuery q;
    XpResult r;
    char buf[256];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.limit = 10;
    q.cursor = extract_cursor(req.query);
    r = list_xp(ctx->db, q);
    if (r.err == ERR_NOT_FOUND)
        return json_ok("{\"next_cursor\":0,\"top\":[]}");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"next_cursor\":%lld,\"top\":[{\"user_id\":\"%s\",\"total\":%d}]}",
             (long long)(q.cursor + q.limit), r.xp.user_id, r.xp.amount);
    return json_ok(buf);
}
