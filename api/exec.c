#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "agents/channel.h"

static HttpResp json_error(int status, const char *msg)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = status;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                  "{\"error\":\"%s\"}", msg);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

static HttpResp json_ok(const char *body)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 200;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body), "%s", body);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

/*
 * POST /exec/{task_id}
 * Fetches task, dispatches to agent system via coord_handle,
 * stores outcome, updates task status.
 */
HttpResp handle_exec_run(HttpReq req, Ctx *ctx)
{
    TaskQuery tq;
    TaskResult tr;
    AgentMsg in, out;
    OutcomeQuery oq;
    OutcomeResult otr;
    const char *idstr;
    char buf[512];
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

    memset(&in, 0, sizeof(in));
    in.tag = MSG_EXEC_REQ;
    in.id  = tr.rows[0].id;
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
    snprintf(buf, sizeof(buf),
             "{\"task_id\":%lld,\"status\":\"%s\",\"result\":\"%s\"}",
             (long long)otr.outcome.task_id,
             out.err == ERR_OK ? "done" : "failed",
             snippet);
    return json_ok(buf);
}
