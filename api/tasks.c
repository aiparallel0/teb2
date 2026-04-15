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

    tr = update_task(ctx->db, q);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

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
    snprintf(q.status, sizeof(q.status), "%s", "executing");

    tr = update_task(ctx->db, q);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

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

HttpResp handle_task_create(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    TaskResult tr;
    char gid[32], buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    extract_json_str(req.body, "\"title\"", q.title, sizeof(q.title));
    if (extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        q.goal_id = strtoll(gid, NULL, 10);
    if (q.goal_id <= 0) return json_error(400, "bad_goal_id");
    tr = store_task(ctx->db, q);
    if (tr.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)tr.rows[0].id);
    return json_ok(buf);
}

HttpResp handle_task_list(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    TaskResult tr;
    char buf[1024];
    int i, pos, added;
    const char *gidstr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    gidstr = strrchr(req.path, '/');
    q.goal_id = gidstr ? strtoll(gidstr + 1, NULL, 10) : 0;
    if (q.goal_id <= 0) return json_error(400, "bad_goal_id");
    q.limit = 16;
    tr = list_tasks(ctx->db, q);
    if (tr.err != ERR_OK) return json_error(500, "db_error");
    pos = snprintf(buf, sizeof(buf), "[");
    for (i = 0; i < tr.count && pos > 0 && (size_t)pos < sizeof(buf) - 2; i++) {
        added = snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        "%s{\"id\":%lld,\"title\":\"%s\"}",
                        i ? "," : "", (long long)tr.rows[i].id, tr.rows[i].title);
        if (added > 0) pos += added;
    }
    if (pos > 0 && (size_t)pos < sizeof(buf) - 1)
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]");
    (void)pos;
    return json_ok(buf);
}
