#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "agents/channel.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_run_create(HttpReq req, Ctx *ctx)
{
    char gid[32], buf[512];
    RunQuery rq;
    RunResult rr, ur;
    AgentMsg msg, res;
    StepQuery sq;
    StepResult sr;
    int ok;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&rq, 0, sizeof(rq));
    if (!extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        return json_error(400, "missing_goal_id");
    rq.goal_id = strtoll(gid, NULL, 10);
    rr = store_run(ctx->db, rq);
    if (rr.err != ERR_OK) return json_error(500, "db_error");
    memset(&msg, 0, sizeof(msg));
    msg.tag = MSG_GOAL_NEW;
    msg.id  = rq.goal_id;
    msg.db  = ctx->db;
    snprintf(msg.user_id, sizeof(msg.user_id), "%lld",
             (long long)ctx->user->user_id);
    snprintf(msg.payload, sizeof(msg.payload), "%lld", (long long)rq.goal_id);
    res = coord_handle(msg);
    memset(&sq, 0, sizeof(sq));
    sq.run_id = rr.run.id;
    sq.id = 0;
    snprintf(sq.agent, sizeof(sq.agent), "coord");
    snprintf(sq.payload, sizeof(sq.payload), "%s", res.payload);
    sr = store_step(ctx->db, sq);
    ok = (res.err == ERR_OK);
    if (!ok) {
        StepResult uss;
        sleep(1);
        res = coord_handle(msg);
        ok = (res.err == ERR_OK);
        if (sr.err == ERR_OK) {
            uss = update_step_status(ctx->db, sr.step.id,
                ok ? "done" : "error", res.payload);
            (void)uss;
        }
    }
    ur = update_run_status(ctx->db, rr.run.id,
             ok ? "done" : "error", ok ? "" : res.payload);
    (void)ur;
    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lld,\"status\":\"%s\"}",
             (long long)rr.run.id, ok ? "done" : "error");
    return json_ok(buf);
}

HttpResp handle_run_get(HttpReq req, Ctx *ctx)
{
    RunQuery rq;
    RunResult rr;
    StepResult sr;
    char buf[2048], es[32];
    int i, off;
    const char *idstr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&rq, 0, sizeof(rq));
    idstr = strrchr(req.path, '/');
    rq.id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (rq.id <= 0) return json_error(400, "bad_id");
    rr = fetch_run(ctx->db, rq);
    if (rr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (rr.err != ERR_OK) return json_error(500, "db_error");
    sr = list_steps(ctx->db, rr.run.id);
    json_escape(rr.run.status, es, sizeof(es));
    off = snprintf(buf, sizeof(buf),
        "{\"id\":%lld,\"goal_id\":%lld,\"status\":\"%s\",\"steps\":[",
        (long long)rr.run.id, (long long)rr.run.goal_id, es);
    for (i = 0; i < sr.count && off > 0 && (size_t)off < sizeof(buf) - 128; i++) {
        char ea[64], est[32];
        json_escape(sr.steps[i].agent, ea, sizeof(ea));
        json_escape(sr.steps[i].status, est, sizeof(est));
        if (i > 0 && off < (int)sizeof(buf) - 1) buf[off++] = ',';
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            "{\"id\":%lld,\"agent\":\"%s\",\"status\":\"%s\"}",
            (long long)sr.steps[i].id, ea, est);
    }
    if (off > 0 && (size_t)off < sizeof(buf) - 3)
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "]}");
    (void)off;
    return json_ok(buf);
}
