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

static int extract_json_str(const char *body, const char *key,
                            char *out, size_t outsz)
{
    const char *k = strstr(body, key), *v;
    size_t i;
    if (!k) return 0;
    k += strlen(key);
    while (*k == ' ' || *k == ':' || *k == '"') k++;
    for (i = 0, v = k; i < outsz - 1 && v[i] && v[i] != '"'; i++)
        out[i] = v[i];
    out[i] = '\0';
    return i > 0 ? 1 : 0;
}

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
    char buf[768];
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

    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"user_id\":\"%s\",\"message\":\"%s\"}",
             (long long)nr.nudge.id, nr.nudge.user_id, nr.nudge.message);
    return json_ok(buf);
}
