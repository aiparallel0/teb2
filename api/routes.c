#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "core/types.h"
#include "api/api.h"
#include "api/json.h"

static HttpResp ext_not_found(void)
{
    return json_error(404, "not_found");
}

/*
 * dispatch_ext: extended route table. Auth guard applied at entry.
 * All endpoints here require an authenticated user.
 */
HttpResp dispatch_ext(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;

    /* Security: all extended routes require authentication */
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");

    if (strcmp(p, "/outcomes") == 0 && strcmp(req.method, "POST") == 0)
        return handle_outcome_store(req, ctx);
    if (strncmp(p, "/outcome/", 9) == 0 && strcmp(req.method, "GET") == 0)
        return handle_outcome_get(req, ctx);
    if (strcmp(p, "/nudges") == 0 && strcmp(req.method, "POST") == 0)
        return handle_nudge_store(req, ctx);
    if (strncmp(p, "/nudge/", 7) == 0 && strcmp(req.method, "GET") == 0)
        return handle_nudge_get(req, ctx);
    if (strcmp(p, "/learnings") == 0 && strcmp(req.method, "POST") == 0)
        return handle_learn_store(req, ctx);
    if (strncmp(p, "/learning/", 10) == 0 && strcmp(req.method, "GET") == 0)
        return handle_learn_get(req, ctx);
    if (strncmp(p, "/exec/", 6) == 0 && strcmp(req.method, "POST") == 0)
        return handle_exec_run(req, ctx);
    if (strncmp(p, "/decompose/", 11) == 0 && strcmp(req.method, "POST") == 0)
        return handle_decompose_run(req, ctx);
    if (strcmp(p, "/schedules") == 0 && strcmp(req.method, "POST") == 0)
        return handle_sched_create(req, ctx);
    if (strncmp(p, "/schedules/", 11) == 0 && strcmp(req.method, "GET") == 0)
        return handle_sched_list(req, ctx);
    if (strcmp(p, "/budgets") == 0 && strcmp(req.method, "POST") == 0)
        return handle_budget_create(req, ctx);
    if (strcmp(p, "/budgets") == 0 && strcmp(req.method, "GET") == 0)
        return handle_budget_get(req, ctx);
    if (strcmp(p, "/spending") == 0 && strcmp(req.method, "POST") == 0)
        return handle_spend_record(req, ctx);

    /* collab/chat/integrations */
    if (strcmp(p, "/workspaces") == 0 && strcmp(req.method, "POST") == 0)
        return handle_ws_create(req, ctx);
    if (strncmp(p, "/workspaces/", 12) == 0 && strcmp(req.method, "GET") == 0)
        return handle_ws_get(req, ctx);
    if (strcmp(p, "/collabs") == 0 && strcmp(req.method, "POST") == 0)
        return handle_collab_add(req, ctx);
    if (strncmp(p, "/collabs/", 9) == 0 && strcmp(req.method, "GET") == 0)
        return handle_collab_list(req, ctx);
    if (strcmp(p, "/chat") == 0 && strcmp(req.method, "POST") == 0)
        return handle_chat_send(req, ctx);
    if (strncmp(p, "/chat/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_chat_list(req, ctx);
    if (strcmp(p, "/integrations") == 0 && strcmp(req.method, "PUT") == 0)
        return handle_integ_toggle(req, ctx);
    if (strncmp(p, "/integrations/", 14) == 0 && strcmp(req.method, "GET") == 0)
        return handle_integ_get(req, ctx);
    if (strcmp(p, "/webhook") == 0 && strcmp(req.method, "POST") == 0)
        return handle_webhook_route(req, ctx);

    /* enterprise */
    if (strcmp(p, "/orgs") == 0 && strcmp(req.method, "POST") == 0)
        return handle_org_create(req, ctx);
    if (strncmp(p, "/orgs/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_org_get(req, ctx);
    if (strcmp(p, "/sso/validate") == 0 && strcmp(req.method, "POST") == 0)
        return handle_sso_validate(req, ctx);
    if (strcmp(p, "/ip/check") == 0 && strcmp(req.method, "POST") == 0)
        return handle_ip_check(req, ctx);

    /* analytics */
    if (strcmp(p, "/analytics/snap") == 0 && strcmp(req.method, "POST") == 0)
        return handle_snap_store(req, ctx);
    if (strncmp(p, "/roi/", 5) == 0 && strcmp(req.method, "GET") == 0)
        return handle_roi_get(req, ctx);
    if (strcmp(p, "/analytics/time") == 0 && strcmp(req.method, "POST") == 0)
        return handle_time_store(req, ctx);

    /* gamification */
    if (strcmp(p, "/xp") == 0 && strcmp(req.method, "POST") == 0)
        return handle_xp_credit(req, ctx);
    if (strcmp(p, "/streak") == 0 && strcmp(req.method, "GET") == 0)
        return handle_streak_get(req, ctx);
    if (strcmp(p, "/leaderboard") == 0 && strcmp(req.method, "GET") == 0)
        return handle_leaderboard(req, ctx);

    /* community */
    if (strcmp(p, "/blog") == 0 && strcmp(req.method, "POST") == 0)
        return handle_blog_store(req, ctx);
    if (strncmp(p, "/blog/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_blog_get(req, ctx);
    if (strcmp(p, "/votes") == 0 && strcmp(req.method, "POST") == 0)
        return handle_vote_upsert(req, ctx);

    /* assets, notify, workflow, search, oauth */
    if (strcmp(p, "/assets") == 0 && strcmp(req.method, "POST") == 0)
        return handle_asset_upload(req, ctx);
    if (strncmp(p, "/assets/", 8) == 0 && strcmp(req.method, "GET") == 0)
        return handle_asset_get(req, ctx);
    if (strcmp(p, "/notify/email") == 0 && strcmp(req.method, "POST") == 0)
        return handle_notify_email(req, ctx);
    if (strcmp(p, "/runs") == 0 && strcmp(req.method, "POST") == 0)
        return handle_run_create(req, ctx);
    if (strncmp(p, "/runs/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_run_get(req, ctx);
    if (strncmp(p, "/search", 7) == 0 && strcmp(req.method, "GET") == 0)
        return handle_search(req, ctx);
    if (strncmp(p, "/oauth/redirect", 15) == 0 && strcmp(req.method, "GET") == 0)
        return handle_oauth_redirect(req, ctx);
    if (strcmp(p, "/oauth/callback") == 0 && strcmp(req.method, "POST") == 0)
        return handle_oauth_callback(req, ctx);

    return ext_not_found();
}
