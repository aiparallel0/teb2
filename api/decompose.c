#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"

static TaskResult create_subtask(Db *db, GoalResult gr,
                                 const char *prefix)
{
    TaskQuery tq;
    memset(&tq, 0, sizeof(tq));
    tq.goal_id = gr.rows[0].id;
    snprintf(tq.user_id, sizeof(tq.user_id), "%s", gr.rows[0].user_id);
    snprintf(tq.title, sizeof(tq.title), "%.10s: %.240s", prefix,
             gr.rows[0].title);
    return store_task(db, tq);
}

/*
 * POST /decompose/{goal_id}
 * Fetches goal, creates Research/Execute/Measure subtasks,
 * sets goal status to "decomposed".
 */
HttpResp handle_decompose_run(HttpReq req, Ctx *ctx)
{
    GoalQuery gq;
    GoalResult gr;
    TaskResult tr;
    const char *idstr;
    int created;
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

    created = 0;
    tr = create_subtask(ctx->db, gr, "Research");
    if (tr.err == ERR_OK) created++;
    tr = create_subtask(ctx->db, gr, "Execute");
    if (tr.err == ERR_OK) created++;
    tr = create_subtask(ctx->db, gr, "Measure");
    if (tr.err == ERR_OK) created++;

    memset(&gq, 0, sizeof(gq));
    gq.id = gr.rows[0].id;
    snprintf(gq.status, sizeof(gq.status), "%s", "decomposed");
    gr = update_goal(ctx->db, gq);
    if (gr.err != ERR_OK) return json_error(500, "status_error");

    snprintf(buf, sizeof(buf),
             "{\"goal_id\":%lld,\"status\":\"decomposed\","
             "\"tasks\":%d}",
             (long long)gq.id, created);
    return json_ok(buf);
}
