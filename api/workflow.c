#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "core/log.h"
#include "auth/auth.h"
#include "db/db.h"
#include "agents/channel.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

typedef struct { Db *db; Config *cfg; int64_t run_id; } RunJob;

static MsgTag step_tag(const char *a)
{
    if (strcmp(a, "finance")  == 0) return MSG_FINANCE_REQ;
    if (strcmp(a, "outreach") == 0) return MSG_NOTIFY;
    if (strcmp(a, "research") == 0) return MSG_RESEARCH;
    return MSG_EXEC_RUN;
}

/* Check if this task requires human-in-the-loop approval. */
static int needs_hitl(Db *db, int64_t task_id)
{
    TaskPlanResult pr = fetch_task_plan(db, task_id);
    return (pr.err == ERR_OK && pr.plan.requires_hitl);
}

/* Check if an approval exists for this task's run. */
static int has_approval(Db *db, const char *user_id)
{
    ApprovalQuery aq; ApprovalResult ar;
    memset(&aq, 0, sizeof(aq));
    snprintf(aq.user_id, sizeof(aq.user_id), "%s", user_id);
    snprintf(aq.status, sizeof(aq.status), "approved");
    aq.limit = 1;
    ar = list_approvals(db, aq);
    return (ar.err == ERR_OK && ar.count > 0);
}

/* Execute all pending steps, enforcing HITL gates and tracking tokens. */
static int execute_steps(RunJob job, TaskResult tr)
{
    StepResult sr, usr;
    AgentMsg msg, res;
    int i;
    sr = list_steps(job.db, job.run_id);
    for (i = 0; i < sr.count; i++) {
        int64_t task_id = (i < tr.count) ? tr.rows[i].id : job.run_id;
        /* HITL gate: if task requires approval and none exists, skip */
        if (needs_hitl(job.db, task_id) &&
            !has_approval(job.db, (i < tr.count) ? tr.rows[i].user_id : "")) {
            usr = update_step_status(job.db, sr.steps[i].id,
                                     "awaiting_approval", "");
            (void)usr;
            teb_log_info("workflow", "step %lld awaiting approval",
                         (long long)sr.steps[i].id);
            continue;
        }
        usr = update_step_status(job.db, sr.steps[i].id, "running", "");
        (void)usr;
        memset(&msg, 0, sizeof(msg));
        msg.tag = step_tag(sr.steps[i].agent);
        msg.id  = task_id;
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
        teb_log_info("workflow", "step %lld status=%s",
                     (long long)sr.steps[i].id,
                     res.err == ERR_OK ? "done" : "failed");
    }
    return (sr.count > 0) ? 1 : 0;
}

static volatile sig_atomic_t g_step_timed_out = 0;
static void step_alarm(int sig) { (void)sig; g_step_timed_out = 1; }

HttpResp handle_run_create(HttpReq req, Ctx *ctx)
{
    char gid[32], buf[256];
    RunQuery rq; RunResult rr, ur;
    TaskQuery tq; TaskResult tr;
    StepQuery sq; StepResult sr;
    RunJob job;
    pid_t pid;
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
    pid = fork();
    if (pid == 0) {
        Db cdb;
        memset(&cdb, 0, sizeof(cdb));
        signal(SIGALRM, step_alarm);
        alarm((unsigned int)timeout);
        if (db_open(ctx->cfg->db_path, &cdb) == ERR_OK) {
            job.db = &cdb; job.cfg = ctx->cfg; job.run_id = rr.run.id;
            (void)execute_steps(job, tr);
            ur = update_run_status(&cdb, rr.run.id,
                g_step_timed_out ? "timed_out" : "done", "");
            (void)ur;
            db_close(&cdb);
        } else {
            teb_log_error("workflow", "child db_open failed for run %lld",
                          (long long)rr.run.id);
        }
        _exit(0);
    }
    if (pid > 0) {
        Err pe = update_run_pid(ctx->db, rr.run.id, (int64_t)pid);
        (void)pe;
        ur = update_run_status(ctx->db, rr.run.id, "running", "");
        (void)ur;
    }
    teb_log_info("workflow", "run %lld started pid=%d timeout=%ds",
                 (long long)rr.run.id, (int)pid, timeout);
    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lld,\"status\":\"running\"}",
             (long long)rr.run.id);
    return json_ok(buf);
}
