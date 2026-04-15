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

/* Bundled context for fork child step execution */
typedef struct { Db *db; Config *cfg; int64_t run_id; } RunJob;

/* Execute all pending workflow steps sequentially in a child process */
static int execute_steps(RunJob job, TaskResult tr)
{
    StepResult sr;
    StepResult usr;
    AgentMsg   msg, res;
    int        i;

    sr = list_steps(job.db, job.run_id);
    for (i = 0; i < sr.count; i++) {
        usr = update_step_status(job.db, sr.steps[i].id, "running", "");
        (void)usr;
        memset(&msg, 0, sizeof(msg));
        msg.tag = MSG_EXEC_REQ;
        msg.id  = (i < tr.count) ? tr.rows[i].id : job.run_id;
        msg.db  = job.db;
        msg.cfg = job.cfg;
        snprintf(msg.user_id, sizeof(msg.user_id), "%s",
                 (i < tr.count) ? tr.rows[i].user_id : "");
        snprintf(msg.payload, sizeof(msg.payload), "%s", sr.steps[i].payload);
        res = coord_handle(msg);
        usr = update_step_status(job.db, sr.steps[i].id,
                                 res.err == ERR_OK ? "done" : "failed",
                                 res.payload);
        (void)usr;
    }
    return (sr.count > 0) ? 1 : 0;
}

HttpResp handle_run_create(HttpReq req, Ctx *ctx)
{
    char       gid[32], buf[256];
    RunQuery   rq;
    RunResult  rr, ur;
    TaskQuery  tq;
    TaskResult tr;
    StepQuery  sq;
    StepResult sr;
    RunJob     job;
    pid_t      pid;
    int        i;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&rq, 0, sizeof(rq));
    if (!extract_json_str(req.body, "\"goal_id\"", gid, sizeof(gid)))
        return json_error(400, "missing_goal_id");
    rq.goal_id = strtoll(gid, NULL, 10);

    rr = store_run(ctx->db, rq);
    if (rr.err != ERR_OK) return json_error(500, "db_error");

    /* Fetch tasks for this goal to create workflow steps */
    memset(&tq, 0, sizeof(tq));
    tq.goal_id = rq.goal_id;
    tq.limit   = 16;
    tr = list_tasks(ctx->db, tq);

    for (i = 0; i < tr.count; i++) {
        memset(&sq, 0, sizeof(sq));
        sq.run_id = rr.run.id;
        sq.id     = i;
        snprintf(sq.agent,   sizeof(sq.agent),   "%s",
                 tr.rows[i].agent[0] ? tr.rows[i].agent : "exec");
        snprintf(sq.payload, sizeof(sq.payload), "%s",
                 tr.rows[i].description);
        sr = store_step(ctx->db, sq);
        (void)sr;
    }

    /* Fork child to execute steps asynchronously */
    pid = fork();
    if (pid == 0) {
        Db cdb;
        const char *final_status = "failed";
        memset(&cdb, 0, sizeof(cdb));
        if (db_open(ctx->cfg->db_path, &cdb) == ERR_OK) {
            job.db     = &cdb;
            job.cfg    = ctx->cfg;
            job.run_id = rr.run.id;
            (void)execute_steps(job, tr);
            final_status = "done";
            ur = update_run_status(&cdb, rr.run.id, final_status, "");
            (void)ur;
            db_close(&cdb);
        } else {
            /* Attempt status update with a fresh handle to avoid stuck 'running' */
            Db fdb;
            memset(&fdb, 0, sizeof(fdb));
            if (db_open(ctx->cfg->db_path, &fdb) == ERR_OK) {
                ur = update_run_status(&fdb, rr.run.id, final_status, "");
                (void)ur;
                db_close(&fdb);
            }
        }
        _exit(0);
    }
    /* Parent returns immediately with run_id */
    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lld,\"status\":\"running\"}",
             (long long)rr.run.id);
    return json_ok(buf);
}

HttpResp handle_run_get(HttpReq req, Ctx *ctx)
{
    RunQuery   rq;
    RunResult  rr;
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
