#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/ratelimit.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"

/*
 * Request dispatcher.
 *
 * Routes every request by (method, path) to a handler. The order of
 * checks matters: GET static/UI routes first (no auth needed, no rate
 * limit), then IP-scoped rate limit, then per-user rate limit for
 * mutating methods, then the route table itself. Every mutating
 * request that produces a non-2xx is still audited so operators see
 * attempted-but-rejected activity in audit_log.
 *
 * Kept separate from parse_request / write_response in api/server.c to
 * stay under the 166-line cap and to let the route table grow as new
 * handlers are added.
 */

static HttpResp options_resp(void)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 204;
    r.body_len = 0;
    snprintf(r.content_type, sizeof(r.content_type), "%s",
             "application/json");
    return r;
}

static int is_mutating(const char *method)
{
    return (strcmp(method, "POST") == 0 ||
            strcmp(method, "PUT")  == 0 ||
            strcmp(method, "DELETE") == 0);
}

static HttpResp dispatch_ui(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;
    if (strcmp(p, "/") == 0) return handle_ui_index(req, ctx);
    if (strcmp(p, "/app.js") == 0) return handle_ui_appjs(req, ctx);
    if (strcmp(p, "/style.css") == 0) return handle_ui_style(req, ctx);
    if (strcmp(p, "/goals.js") == 0) return handle_ui_goalsjs(req, ctx);
    if (strcmp(p, "/tasks.js") == 0) return handle_ui_tasksjs(req, ctx);
    if (strcmp(p, "/finance.js") == 0) return handle_ui_financejs(req, ctx);
    if (strcmp(p, "/collab.js") == 0) return handle_ui_collabjs(req, ctx);
    if (strcmp(p, "/dash.js") == 0) return handle_ui_dashjs(req, ctx);
    if (strcmp(p, "/enterprise.js") == 0) return handle_ui_enterprisejs(req, ctx);
    if (strcmp(p, "/analytics.js") == 0) return handle_ui_analyticsjs(req, ctx);
    if (strcmp(p, "/approvals.js") == 0) return handle_ui_approvalsjs(req, ctx);
    if (strcmp(p, "/prompts.js") == 0)   return handle_ui_promptsjs(req, ctx);
    if (strcmp(p, "/workflows.js") == 0) return handle_ui_workflowsjs(req, ctx);
    if (strcmp(p, "/healthz") == 0) return handle_healthz(req, ctx);
    if (strcmp(p, "/metrics") == 0) return handle_metrics(req, ctx);
    { HttpResp nr; memset(&nr, 0, sizeof(nr)); return nr; }
}

static HttpResp dispatch_core(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;
    if (strncmp(p, "/goals", 6) == 0) {
        if (strcmp(req.method, "POST") == 0) return handle_goal_create(req, ctx);
        if (strcmp(req.method, "GET")  == 0) return handle_goal_list(req, ctx);
    }
    if (strncmp(p, "/goal/", 6) == 0) {
        if (strcmp(req.method, "GET")  == 0) return handle_goal_get(req, ctx);
        if (strcmp(req.method, "POST") == 0) return handle_goal_decompose(req, ctx);
    }
    if (strncmp(p, "/tasks/goal/", 12) == 0 && strcmp(req.method, "GET") == 0)
        return handle_task_list(req, ctx);
    if (strcmp(p, "/tasks") == 0 && strcmp(req.method, "POST") == 0)
        return handle_task_create(req, ctx);
    if (strncmp(p, "/tasks/", 7) == 0) {
        if (strcmp(req.method, "PUT")  == 0) return handle_task_update(req, ctx);
        if (strcmp(req.method, "POST") == 0) return handle_task_execute(req, ctx);
        if (strcmp(req.method, "GET")  == 0) return handle_task_status(req, ctx);
    }
    if (strcmp(p, "/auth/register") == 0) return handle_register(req, ctx);
    if (strcmp(p, "/auth/login") == 0)    return handle_login(req, ctx);
    if (strcmp(p, "/auth/refresh") == 0)  return handle_refresh(req, ctx);
    if (strncmp(p, "/sse/chat/", 10) == 0 && strcmp(req.method, "GET") == 0)
        return handle_sse_subscribe(req, ctx);
    return dispatch_ext(req, ctx);
}

static HttpResp dispatch_internal(HttpReq req, Ctx *ctx)
{
    const char *rlkey;
    /* 413: body saturated the buffer — reject mutating requests */
    if (is_mutating(req.method) && req.body_len >= sizeof(req.body) - 1)
        return json_error(413, "payload_too_large");
    if (strcmp(req.method, "OPTIONS") == 0) return options_resp();
    if (strcmp(req.method, "GET") == 0) {
        HttpResp ui = dispatch_ui(req, ctx);
        if (ui.status != 0) return ui;
    }
    rlkey = req.fwd_for[0] ? req.fwd_for : "unknown";
    if (strcmp(req.path, "/auth/register") == 0 ||
        strcmp(req.path, "/auth/login") == 0) {
        if (!rl_check(rlkey, 10))  return json_error(429, "rate_limited");
    } else {
        if (!rl_check(rlkey, 120)) return json_error(429, "rate_limited");
    }
    if (ctx && ctx->user && is_mutating(req.method)) {
        char ukey[80];
        snprintf(ukey, sizeof(ukey), "u:%lld",
                 (long long)ctx->user->user_id);
        if (!rl_check(ukey, 300))  return json_error(429, "rate_limited");
    }
    return dispatch_core(req, ctx);
}

HttpResp dispatch(HttpReq req, Ctx *ctx)
{
    HttpResp resp = dispatch_internal(req, ctx);
    metrics_inc(req.method, req.path, resp.status);
    if (ctx && ctx->db && is_mutating(req.method)) {
        char uid[32] = "";
        Err ae;
        if (ctx->user)
            snprintf(uid, sizeof(uid), "%lld",
                     (long long)ctx->user->user_id);
        ae = audit_log(ctx->db, uid, req.method, req.path,
                       resp.status, req.fwd_for);
        (void)ae; /* audit failure never denies a completed request */
    }
    return resp;
}
