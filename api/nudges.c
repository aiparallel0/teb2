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

HttpResp handle_nudge_store(HttpReq req, Ctx *ctx)
{
    NudgeQuery q;
    NudgeResult nr;
    char buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_NUDGE_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    extract_json_str(req.body, "\"message\"", q.message, sizeof(q.message));
    nr = store_nudge(ctx->db, q);
    if (nr.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)nr.nudge.id);
    return json_ok(buf);
}

HttpResp handle_nudge_get(HttpReq req, Ctx *ctx)
{
    NudgeQuery q;
    NudgeResult nr;
    char buf[1024], eu[128], em[768];
    const char *idstr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_NUDGE_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");
    nr = fetch_nudge(ctx->db, q);
    if (nr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (nr.err != ERR_OK)        return json_error(500, "db_error");
    json_escape(nr.nudge.user_id, eu, sizeof(eu));
    json_escape(nr.nudge.message, em, sizeof(em));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"user_id\":\"%s\",\"message\":\"%s\"}",
             (long long)nr.nudge.id, eu, em);
    return json_ok(buf);
}
