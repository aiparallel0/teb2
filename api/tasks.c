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

HttpResp handle_task_update(HttpReq req, Ctx *ctx)
{
    TaskQuery q;
    const char *idstr;
    TaskResult tr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    idstr = strrchr(req.path, '/');
    q.id  = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (q.id <= 0) return json_error(400, "bad_id");
    extract_json_str(req.body, "\"status\"", q.status, sizeof(q.status));
    extract_json_str(req.body, "\"description\"", q.description,
                     sizeof(q.description));
    extract_json_str(req.body, "\"agent\"", q.agent, sizeof(q.agent));
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
    char buf[2048], es[64];
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
    {
        char et[512], ed[1024], ea[128];
        json_escape(tr.rows[0].status, es, sizeof(es));
        json_escape(tr.rows[0].title, et, sizeof(et));
        json_escape(tr.rows[0].description, ed, sizeof(ed));
        json_escape(tr.rows[0].agent, ea, sizeof(ea));
        snprintf(buf, sizeof(buf),
            "{\"id\":%lld,\"goal_id\":%lld,\"title\":\"%s\","
            "\"status\":\"%s\",\"agent\":\"%s\","
            "\"description\":\"%s\"}",
            (long long)tr.rows[0].id, (long long)tr.rows[0].goal_id,
            et, es, ea, ed);
    }
    return json_ok(buf);
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
    extract_json_str(req.body, "\"description\"", q.description,
                     sizeof(q.description));
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
    char buf[2048], et[512];
    int i, pos, added;
    const char *gidstr;
    int64_t next_cursor;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    gidstr = strrchr(req.path, '/');
    q.goal_id = gidstr ? strtoll(gidstr + 1, NULL, 10) : 0;
    if (q.goal_id <= 0) return json_error(400, "bad_goal_id");
    q.limit = 16;
    q.cursor = extract_cursor(req.query);
    tr = list_tasks(ctx->db, q);
    if (tr.err != ERR_OK) return json_error(500, "db_error");
    next_cursor = (tr.count > 0) ? tr.rows[tr.count - 1].id : 0;
    pos = snprintf(buf, sizeof(buf), "{\"next_cursor\":%lld,\"items\":[",
                   (long long)next_cursor);
    for (i = 0; i < tr.count && pos > 0 && (size_t)pos < sizeof(buf) - 2;
         i++) {
        {
            char es2[64], ea[128];
            json_escape(tr.rows[i].title, et, sizeof(et));
            json_escape(tr.rows[i].status, es2, sizeof(es2));
            json_escape(tr.rows[i].agent, ea, sizeof(ea));
            added = snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                "%s{\"id\":%lld,\"goal_id\":%lld,\"title\":\"%s\","
                "\"status\":\"%s\",\"agent\":\"%s\"}",
                i ? "," : "", (long long)tr.rows[i].id,
                (long long)tr.rows[i].goal_id, et, es2, ea);
        }
        if (added > 0) pos += added;
    }
    if (pos > 0 && (size_t)pos < sizeof(buf) - 3)
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "]}");
    (void)pos;
    return json_ok(buf);
}
