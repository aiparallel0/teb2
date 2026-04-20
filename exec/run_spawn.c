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
#include "db/db.h"
#include "agents/channel.h"
#include "exec/exec.h"

/* Timeout flag is process-local: set by SIGALRM in the child. */
static volatile sig_atomic_t g_step_timed_out = 0;
static void step_alarm(int sig) { (void)sig; g_step_timed_out = 1; }

static MsgTag step_tag(const char *a)
{
    if (strcmp(a, "finance")  == 0) return MSG_FINANCE_REQ;
    if (strcmp(a, "outreach") == 0) return MSG_NOTIFY;
    if (strcmp(a, "research") == 0) return MSG_RESEARCH;
    return MSG_EXEC_RUN;
}

static int needs_hitl(Db *db, int64_t task_id)
{
    TaskPlanResult pr = fetch_task_plan(db, task_id);
    return (pr.err == ERR_OK && pr.plan.requires_hitl);
}

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

static void execute_steps(Db *db, Config *cfg, int64_t run_id, TaskResult tr)
{
    StepResult sr, usr;
    AgentMsg msg, res;
    int i;
    sr = list_steps(db, run_id);
    for (i = 0; i < sr.count; i++) {
        int64_t task_id = (i < tr.count) ? tr.rows[i].id : run_id;
        if (needs_hitl(db, task_id) &&
            !has_approval(db, (i < tr.count) ? tr.rows[i].user_id : "")) {
            usr = update_step_status(db, sr.steps[i].id,
                                     "awaiting_approval", "");
            (void)usr;
            teb_log_info("run_spawn", "step %lld awaiting approval",
                         (long long)sr.steps[i].id);
            continue;
        }
        usr = update_step_status(db, sr.steps[i].id, "running", "");
        (void)usr;
        memset(&msg, 0, sizeof(msg));
        msg.tag = step_tag(sr.steps[i].agent);
        msg.id  = task_id;
        msg.db  = db;
        msg.cfg = cfg;
        snprintf(msg.user_id, sizeof(msg.user_id), "%s",
                 (i < tr.count) ? tr.rows[i].user_id : "");
        snprintf(msg.payload, sizeof(msg.payload), "%s", sr.steps[i].payload);
        res = coord_handle(msg);
        usr = update_step_status(db, sr.steps[i].id,
                                 res.err == ERR_OK ? "done" : "failed",
                                 res.payload);
        (void)usr;
        teb_log_info("run_spawn", "step %lld status=%s",
                     (long long)sr.steps[i].id,
                     res.err == ERR_OK ? "done" : "failed");
    }
}

static void run_child(Config *cfg, RunSpawnReq req)
{
    Db cdb;
    RunResult ur;
    memset(&cdb, 0, sizeof(cdb));
    signal(SIGALRM, step_alarm);
    alarm((unsigned int)req.timeout_sec);
    if (db_open(cfg->db_path, &cdb) == ERR_OK) {
        execute_steps(&cdb, cfg, req.run_id, req.tasks);
        ur = update_run_status(&cdb, req.run_id,
            g_step_timed_out ? "timed_out" : "done", "");
        (void)ur;
        db_close(&cdb);
    } else {
        teb_log_error("run_spawn", "child db_open failed for run %lld",
                      (long long)req.run_id);
    }
}

RunSpawnResult exec_run_spawn(RunSpawnReq req, Ctx *ctx)
{
    RunSpawnResult out;
    pid_t pid;
    Err   pe;
    RunResult ur;

    memset(&out, 0, sizeof(out));
    if (!ctx || !ctx->cfg || !ctx->db) { out.err = ERR_UNKNOWN; return out; }

    pid = fork();
    if (pid == 0) {
        run_child(ctx->cfg, req);
        _exit(0);
    }
    if (pid < 0) { out.err = ERR_IO; return out; }

    pe = update_run_pid(ctx->db, req.run_id, (int64_t)pid);
    (void)pe;
    ur = update_run_status(ctx->db, req.run_id, "running", "");
    (void)ur;
    teb_log_info("run_spawn", "run %lld started pid=%d timeout=%ds",
                 (long long)req.run_id, (int)pid, req.timeout_sec);
    out.err = ERR_OK;
    out.pid = (int)pid;
    return out;
}
