#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "api/api.h"
#include "api/json.h"

HttpResp handle_run_create(HttpReq req, Ctx *ctx)
{
    char gid[32], buf[256];
    RunQuery rq;
    RunResult rr;
    TaskQuery tq; TaskResult tr;
    StepQuery sq; StepResult sr;
    RunSpawnReq spawn;
    RunSpawnResult sp;
    int i, timeout;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&rq, 0, sizeof(rq));
    if (!extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        return json_error(400, "missing_goal_id");
    rq.goal_id = strtoll(gid, NULL, 10);
    rr = store_run(ctx->db, rq);
    if (rr.err != ERR_OK) return json_error(500, "db_error");

    memset(&tq, 0, sizeof(tq));
    tq.goal_id = rq.goal_id; tq.limit = 16;
    tr = list_tasks(ctx->db, tq);
    for (i = 0; i < tr.count; i++) {
        memset(&sq, 0, sizeof(sq));
        sq.run_id = rr.run.id; sq.id = i;
        snprintf(sq.agent, sizeof(sq.agent), "%s",
                 tr.rows[i].agent[0] ? tr.rows[i].agent : "exec");
        snprintf(sq.payload, sizeof(sq.payload), "%s", tr.rows[i].description);
        sr = store_step(ctx->db, sq); (void)sr;
    }
    timeout = ctx->cfg->run_timeout_sec > 0 ? ctx->cfg->run_timeout_sec : 300;

    memset(&spawn, 0, sizeof(spawn));
    spawn.run_id      = rr.run.id;
    spawn.timeout_sec = timeout;
    spawn.tasks       = tr;
    sp = exec_run_spawn(spawn, ctx);
    if (sp.err != ERR_OK) return json_error(500, "spawn_failed");

    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lld,\"status\":\"running\"}",
             (long long)rr.run.id);
    return json_ok(buf);
}
