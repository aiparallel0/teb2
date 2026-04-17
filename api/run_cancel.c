#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "core/log.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

/*
 * POST /runs/<id>/cancel — send SIGTERM to the running child.
 * Only the owning user or an admin can cancel a run.
 */
HttpResp handle_run_cancel(HttpReq req, Ctx *ctx)
{
    RunQuery rq; RunResult rr;
    GoalQuery gq; GoalResult gr;
    char uid[32], buf[128];
    const char *idstr;
    int64_t pid;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&rq, 0, sizeof(rq));
    idstr = strrchr(req.path, '/');
    rq.id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (rq.id <= 0) return json_error(400, "bad_id");
    rr = fetch_run(ctx->db, rq);
    if (rr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (rr.err != ERR_OK) return json_error(500, "db_error");

    /* ownership check */
    memset(&gq, 0, sizeof(gq)); gq.id = rr.run.goal_id;
    gr = fetch_goal(ctx->db, gq);
    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    if (ctx->user->role != ROLE_ADMIN &&
        (gr.err != ERR_OK || strcmp(gr.rows[0].user_id, uid) != 0))
        return json_error(403, "forbidden");

    if (strcmp(rr.run.status, "running") != 0)
        return json_error(409, "not_running");

    /* Phase 3: read PID from workflow_runs and send SIGTERM to the
     * child process group. The child has alarm()+signal handler
     * installed so SIGTERM unwinds cleanly; the SIGCHLD reaper in
     * main.c::install_signals() prevents zombies. */
    pid = fetch_run_pid(ctx->db, rq.id);
    if (pid > 0) {
        if (kill((pid_t)pid, SIGTERM) != 0) {
            teb_log_warn("workflow",
                         "run %lld SIGTERM failed pid=%lld errno=%d(%s)",
                         (long long)rq.id, (long long)pid,
                         errno, strerror(errno));
        }
    }

    rr = update_run_status(ctx->db, rq.id, "cancelled", "cancelled by user");
    teb_log_info("workflow", "run %lld cancelled by user %s pid=%lld",
                 (long long)rq.id, uid, (long long)pid);

    snprintf(buf, sizeof(buf),
             "{\"run_id\":%lld,\"status\":\"cancelled\"}",
             (long long)rq.id);
    return json_ok(buf);
}

/*
 * GET /runs/<id> — retrieve run status with steps.
 * Moved from workflow.c to stay under 166-line cap.
 */
HttpResp handle_run_get(HttpReq req, Ctx *ctx)
{
    RunQuery rq; RunResult rr; StepResult sr;
    GoalQuery gq; GoalResult gr;
    char buf[2048], es[32], uid[64];
    int i, off;
    const char *idstr;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_READ))
        return json_error(403, "forbidden");
    memset(&rq, 0, sizeof(rq));
    idstr = strrchr(req.path, '/');
    rq.id = idstr ? strtoll(idstr + 1, NULL, 10) : 0;
    if (rq.id <= 0) return json_error(400, "bad_id");
    rr = fetch_run(ctx->db, rq);
    if (rr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (rr.err != ERR_OK) return json_error(500, "db_error");
    memset(&gq, 0, sizeof(gq)); gq.id = rr.run.goal_id;
    gr = fetch_goal(ctx->db, gq);
    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    if (ctx->user->role != ROLE_ADMIN &&
        (gr.err != ERR_OK || strcmp(gr.rows[0].user_id, uid) != 0))
        return json_error(403, "forbidden");
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
