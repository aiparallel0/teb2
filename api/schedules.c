#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"

HttpResp handle_sched_create(HttpReq req, Ctx *ctx)
{
    SchedQuery q;
    SchedResult sr;
    char tid[32], rat[32], buf[128];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"task_id\"", tid, sizeof(tid)))
        q.task_id = strtoll(tid, NULL, 10);
    if (extract_json_str(req.body, "\"run_at\"", rat, sizeof(rat)))
        q.run_at = strtoll(rat, NULL, 10);
    if (q.task_id <= 0) return json_error(400, "bad_task_id");
    if (q.run_at <= 0)  return json_error(400, "bad_run_at");

    sr = store_sched(ctx->db, q);
    if (sr.err != ERR_OK) return json_error(500, "db_error");

    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"task_id\":%lld,\"run_at\":%lld}",
             (long long)sr.entry.id, (long long)sr.entry.task_id,
             (long long)sr.entry.run_at);
    return json_ok(buf);
}

HttpResp handle_sched_list(HttpReq req, Ctx *ctx)
{
    SchedQuery q;
    SchedResult sr;
    char buf[256];
    const char *tidstr;

    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    tidstr = strrchr(req.path, '/');
    q.task_id = tidstr ? strtoll(tidstr + 1, NULL, 10) : 0;
    if (q.task_id <= 0) return json_error(400, "bad_task_id");

    sr = fetch_sched(ctx->db, q);
    if (sr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (sr.err != ERR_OK)        return json_error(500, "db_error");

    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"task_id\":%lld,\"run_at\":%lld}",
             (long long)sr.entry.id, (long long)sr.entry.task_id,
             (long long)sr.entry.run_at);
    return json_ok(buf);
}
