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

HttpResp handle_learn_store(HttpReq req, Ctx *ctx)
{
    LearnQuery q;
    LearnResult lr;
    char gid[32], buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_LEARN_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        q.goal_id = strtoll(gid, NULL, 10);
    if (q.goal_id <= 0) return json_error(400, "bad_goal_id");
    extract_json_str(req.body, "\"insight\"", q.insight,
                     sizeof(q.insight));
    lr = store_learning(ctx->db, q);
    if (lr.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}",
             (long long)lr.learning.id);
    return json_ok(buf);
}

HttpResp handle_learn_get(HttpReq req, Ctx *ctx)
{
    LearnQuery q;
    LearnResult lr;
    char buf[1024], ei[768];
    const char *idstr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_LEARN_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.goal_id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (q.goal_id <= 0) return json_error(400, "bad_goal_id");
    lr = fetch_learning(ctx->db, q);
    if (lr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (lr.err != ERR_OK)        return json_error(500, "db_error");
    json_escape(lr.learning.insight, ei, sizeof(ei));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"goal_id\":%lld,\"insight\":\"%s\"}",
             (long long)lr.learning.id,
             (long long)lr.learning.goal_id, ei);
    return json_ok(buf);
}
