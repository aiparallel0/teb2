#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_goal_create(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;
    char buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    extract_json_str(req.body, "\"title\"", q.title, sizeof(q.title));
    extract_json_str(req.body, "\"description\"", q.description,
                     sizeof(q.description));
    gr = store_goal(ctx->db, q);
    if (gr.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)gr.rows[0].id);
    return json_ok(buf);
}

HttpResp handle_goal_get(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;
    char buf[1024], et[512], es[64];
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
    json_escape(gr.rows[0].title, et, sizeof(et));
    json_escape(gr.rows[0].status, es, sizeof(es));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"title\":\"%s\",\"status\":\"%s\"}",
             (long long)gr.rows[0].id, et, es);
    return json_ok(buf);
}

HttpResp handle_goal_list(HttpReq req, Ctx *ctx)
{
    GoalQuery q;
    GoalResult gr;
    char buf[2048], et[512];
    int i, pos, added;
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
        if (pos < 0 || (size_t)pos >= sizeof(buf) - 2) break;
        json_escape(gr.rows[i].title, et, sizeof(et));
        added = snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        "%s{\"id\":%lld,\"title\":\"%s\"}",
                        i ? "," : "", (long long)gr.rows[i].id, et);
        if (added > 0) pos += added;
    }
    if (pos >= 0 && (size_t)pos < sizeof(buf) - 1)
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]");
    (void)pos;
    return json_ok(buf);
}

HttpResp handle_goal_decompose(HttpReq req, Ctx *ctx)
{
    GoalQuery gq;
    GoalResult gr;
    TaskQuery tq;
    TaskResult tr;
    const char *idstr;
    char buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&gq, 0, sizeof(gq));
    idstr = strrchr(req.path, '/');
    gq.id = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (gq.id <= 0) return json_error(400, "bad_id");
    gr = fetch_goal(ctx->db, gq);
    if (gr.err != ERR_OK) return json_error(404, "goal_not_found");
    memset(&tq, 0, sizeof(tq));
    tq.goal_id = gq.id;
    snprintf(tq.user_id, sizeof(tq.user_id), "%s", gr.rows[0].user_id);
    snprintf(tq.title, sizeof(tq.title), "%s", gr.rows[0].title);
    snprintf(tq.description, sizeof(tq.description), "%s",
             gr.rows[0].description);
    tr = store_task(ctx->db, tq);
    if (tr.err != ERR_OK) return json_error(500, "task_create_failed");
    snprintf(gq.status, sizeof(gq.status), "%s", "decomposed");
    gr = update_goal(ctx->db, gq);
    if (gr.err != ERR_OK) return json_error(500, "status_update_failed");
    snprintf(buf, sizeof(buf), "{\"task_id\":%lld}",
             (long long)tr.rows[0].id);
    return json_ok(buf);
}
