#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "agents/channel.h"
#include "db/db.h"
#include "api/json.h"

/*
 * POST /decompose/{goal_id}
 *
 * Fetches the goal, calls decompose_handle (which invokes the LLM
 * with prompts/decompose.md), and reports how many tasks the
 * LLM-driven decomposition persisted. Replaces the earlier hardcoded
 * Research/Execute/Measure stub — that was gap L.1 in the audit.
 */
HttpResp handle_decompose_run(HttpReq req, Ctx *ctx)
{
    GoalQuery  gq;
    GoalResult gr;
    AgentMsg   am, res;
    const char *idstr;
    int created = 0;
    char buf[256];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&gq, 0, sizeof(gq));
    idstr = strrchr(req.path, '/');
    gq.id = idstr ? (int64_t)strtoll(idstr + 1, NULL, 10) : 0;
    if (gq.id <= 0) return json_error(400, "bad_id");

    gr = fetch_goal(ctx->db, gq);
    if (gr.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (gr.err != ERR_OK)        return json_error(500, "db_error");

    memset(&am, 0, sizeof(am));
    am.tag = MSG_DECOMPOSE;
    am.id  = gr.rows[0].id;
    snprintf(am.user_id, sizeof(am.user_id), "%s", gr.rows[0].user_id);
    snprintf(am.payload, sizeof(am.payload), "%.255s %.255s",
             gr.rows[0].title, gr.rows[0].description);
    am.cfg = ctx->cfg;
    am.db  = ctx->db;

    res = decompose_handle(am);
    if (res.err == ERR_OK) {
        const char *c = strchr(res.payload, ':');
        if (c) created = atoi(c + 1);
    }

    memset(&gq, 0, sizeof(gq));
    gq.id = gr.rows[0].id;
    snprintf(gq.status, sizeof(gq.status), "%s", "decomposed");
    gr = update_goal(ctx->db, gq);
    if (gr.err != ERR_OK) return json_error(500, "status_error");

    snprintf(buf, sizeof(buf),
             "{\"goal_id\":%lld,\"status\":\"decomposed\",\"tasks\":%d}",
             (long long)gq.id, created);
    return json_ok(buf);
}

