#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"

HttpResp handle_snap_store(HttpReq req, Ctx *ctx)
{
    SnapQuery q;
    SnapResult r;
    char buf[128], gid[32], pct[8];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        q.goal_id = strtoll(gid, NULL, 10);
    if (extract_json_str(req.body, "\"pct\"", pct, sizeof(pct)))
        q.pct = atoi(pct);
    r = store_snap(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"goal_id\":%lld,\"pct\":%d}",
             (long long)r.snap.id, (long long)r.snap.goal_id, r.snap.pct);
    return json_ok(buf);
}

HttpResp handle_roi_get(HttpReq req, Ctx *ctx)
{
    RoiQuery q;
    RoiResult r;
    char buf[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.goal_id = strtoll(req.path + 5, NULL, 10);
    r = fetch_roi(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"value_cents\":%lld,\"cost_cents\":%lld}",
             (long long)r.roi.value_cents, (long long)r.roi.cost_cents);
    return json_ok(buf);
}

HttpResp handle_time_store(HttpReq req, Ctx *ctx)
{
    TimeQuery q;
    TimeResult r;
    char buf[128], tid[32], mins[16];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"task_id\"", tid, sizeof(tid)))
        q.task_id = strtoll(tid, NULL, 10);
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    if (extract_json_str(req.body, "\"minutes\"", mins, sizeof(mins)))
        q.minutes = atoi(mins);
    r = store_time(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"minutes\":%d}",
             (long long)r.entry.id, r.entry.minutes);
    return json_ok(buf);
}
