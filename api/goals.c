#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"

static HttpResp json_error(int status, const char *msg)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = status;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                  "{\"error\":\"%s\"}", msg);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

static HttpResp json_ok(const char *body)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 200;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body), "%s", body);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

HttpResp handle_goal_create(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    q.limit = 1;
    (void)req;

    gr = store_goal(ctx->db, q);
    if (gr.err != ERR_OK) return json_error(500, "db_error");

    {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)gr.rows[0].id);
        return json_ok(buf);
    }
}

HttpResp handle_goal_get(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;
    char buf[512];
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_READ))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");

    gr = fetch_goal(ctx->db, q);
    if (gr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (gr.err != ERR_OK)        return json_error(500, "db_error");

    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"title\":\"%s\",\"status\":\"%s\"}",
             (long long)gr.rows[0].id, gr.rows[0].title, gr.rows[0].status);
    return json_ok(buf);
}

HttpResp handle_goal_list(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;
    char buf[1024];
    int i, pos;

    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_READ))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    q.limit = 16;

    gr = list_goals(ctx->db, q);
    if (gr.err != ERR_OK) return json_error(500, "db_error");

    pos = snprintf(buf, sizeof(buf), "[");
    for (i = 0; i < gr.count; i++) {
        int added;
        if (pos < 0 || (size_t)pos >= sizeof(buf) - 2) break;
        added = snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        "%s{\"id\":%lld,\"title\":\"%s\"}",
                        i ? "," : "", (long long)gr.rows[i].id,
                        gr.rows[i].title);
        if (added > 0) pos += added;
    }
    if (pos >= 0 && (size_t)pos < sizeof(buf) - 1)
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]");
    (void)pos;
    return json_ok(buf);
}

HttpResp handle_goal_decompose(HttpReq req, Ctx *ctx)
{
    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    return json_ok("{\"status\":\"decompose_queued\"}");
}
