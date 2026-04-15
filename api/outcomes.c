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

HttpResp handle_outcome_store(HttpReq req, Ctx *ctx)
{
    OutcomeQuery q;
    OutcomeResult otr;
    char tid[32], buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"task_id\"", tid, sizeof(tid)))
        q.task_id = strtoll(tid, NULL, 10);
    if (q.task_id <= 0) return json_error(400, "bad_task_id");
    extract_json_str(req.body, "\"result\"", q.result, sizeof(q.result));
    otr = store_outcome(ctx->db, q);
    if (otr.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)otr.outcome.id);
    return json_ok(buf);
}

HttpResp handle_outcome_get(HttpReq req, Ctx *ctx)
{
    OutcomeQuery q;
    OutcomeResult otr;
    char buf[768];
    const char *idstr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.task_id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (q.task_id <= 0) return json_error(400, "bad_task_id");
    otr = fetch_outcome(ctx->db, q);
    if (otr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (otr.err != ERR_OK)        return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"task_id\":%lld,\"result\":\"%s\"}",
             (long long)otr.outcome.id, (long long)otr.outcome.task_id,
             otr.outcome.result);
    return json_ok(buf);
}
