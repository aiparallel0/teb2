#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "agents/channel.h"
#include "api/json.h"
#include "api/escape.h"

/*
 * POST /exec/{task_id}
 * Fetches task, dispatches to agent system via coord_handle,
 * stores outcome, updates task status.
 */

/* Browser singleton — spawned on first exec call */
static BrowserProc g_browser;
static int         g_browser_ready;

static void ensure_browser(void)
{
    if (!g_browser_ready) {
        if (browser_spawn(&g_browser) == 0)
            g_browser_ready = 1;
    }
}

HttpResp handle_exec_run(HttpReq req, Ctx *ctx)
{
    TaskQuery   tq;
    TaskResult  tr;
    AgentMsg    in, out;
    OutcomeQuery oq;
    OutcomeResult otr;
    const char *idstr;
    char buf[1024], esnip[512];
    char snippet[200];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&tq, 0, sizeof(tq));
    idstr = strrchr(req.path, '/');
    tq.id = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (tq.id <= 0) return json_error(400, "bad_id");

    tr = fetch_task(ctx->db, tq);
    if (tr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (tr.err != ERR_OK)        return json_error(500, "db_error");

    /* Ensure browser subprocess is available for browser automation tasks */
    ensure_browser();

    memset(&in, 0, sizeof(in));
    /* Route by the agent the decompose phase chose. Previously every
     * task ran as MSG_EXEC_REQ and coord.c collapsed that onto
     * research_handle, so "exec" / "finance" / "outreach" / "browser"
     * tasks all silently ran the research prompt. Browser is routed
     * to MSG_EXEC_RUN as a deliberate interim: exec_handle falls
     * through to exec.plan when no keyword matches, which yields a
     * step-by-step plan the user (or the future Playwright worker —
     * see FOLLOWUPS §Z) can execute. Running the research prompt on
     * a browser task, the prior behaviour, produces a citation
     * summary, which is strictly less useful. */
    {
        const char *a = tr.rows[0].agent;
        if      (strcmp(a, "finance")  == 0) in.tag = MSG_FINANCE_REQ;
        else if (strcmp(a, "outreach") == 0) in.tag = MSG_NOTIFY;
        else if (strcmp(a, "exec")     == 0) in.tag = MSG_EXEC_RUN;
        else if (strcmp(a, "browser")  == 0) in.tag = MSG_EXEC_RUN;
        else                                 in.tag = MSG_EXEC_REQ; /* research */
    }
    in.id  = tr.rows[0].id;
    in.db  = ctx->db;
    in.cfg = ctx->cfg;
    snprintf(in.user_id, sizeof(in.user_id), "%s", tr.rows[0].user_id);
    snprintf(in.payload, sizeof(in.payload), "%s", tr.rows[0].description);

    out = coord_handle(in);

    memset(&oq, 0, sizeof(oq));
    oq.task_id = tq.id;
    snprintf(oq.result, sizeof(oq.result), "%s", out.payload);
    otr = store_outcome(ctx->db, oq);
    if (otr.err != ERR_OK) return json_error(500, "outcome_error");

    memset(&tq, 0, sizeof(tq));
    tq.id = tr.rows[0].id;
    snprintf(tq.status, sizeof(tq.status), "%s",
             out.err == ERR_OK ? "done" : "failed");
    tr = update_task(ctx->db, tq);
    if (tr.err != ERR_OK) return json_error(500, "status_error");

    snprintf(snippet, sizeof(snippet), "%.*s", 180, out.payload);
    json_escape(snippet, esnip, sizeof(esnip));
    snprintf(buf, sizeof(buf),
             "{\"task_id\":%lld,\"status\":\"%s\",\"result\":\"%s\"}",
             (long long)otr.outcome.task_id,
             out.err == ERR_OK ? "done" : "failed",
             esnip);
    return json_ok(buf);
}
