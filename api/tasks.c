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

HttpResp handle_task_update(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    TaskResult tr;
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");

    tr = fetch_task(ctx->db, q);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

    (void)req;
    return json_ok("{\"status\":\"updated\"}");
}

HttpResp handle_task_execute(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    TaskResult tr;
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");

    tr = fetch_task(ctx->db, q);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

    (void)req;
    return json_ok("{\"status\":\"executing\"}");
}

HttpResp handle_task_status(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    TaskResult tr;
    char buf[256];
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");

    tr = fetch_task(ctx->db, q);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

    (void)req;
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"status\":\"%s\"}",
             (long long)tr.rows[0].id, tr.rows[0].status);
    return json_ok(buf);
}
