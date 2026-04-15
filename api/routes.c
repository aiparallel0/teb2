#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "core/types.h"
#include "api/api.h"
#include "api/json.h"

static HttpResp ext_not_found(void)
{
    return json_error(404, "not_found");
}

HttpResp dispatch_ext(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;

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
    if (strncmp(p, "/integrations/", 14) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_integ_get(req, ctx);
    if (strcmp(p, "/webhook") == 0 && strcmp(req.method, "POST") == 0)
        return handle_webhook_route(req, ctx);

    if (strcmp(p, "/orgs") == 0 && strcmp(req.method, "POST") == 0)
        return handle_org_create(req, ctx);
    if (strncmp(p, "/orgs/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_org_get(req, ctx);
    if (strcmp(p, "/sso/validate") == 0 && strcmp(req.method, "POST") == 0)
        return handle_sso_validate(req, ctx);
    if (strcmp(p, "/ip/check") == 0 && strcmp(req.method, "POST") == 0)
        return handle_ip_check(req, ctx);

    if (strcmp(p, "/analytics/snap") == 0 && strcmp(req.method, "POST") == 0)
        return handle_snap_store(req, ctx);
    if (strncmp(p, "/roi/", 5) == 0 && strcmp(req.method, "GET") == 0)
        return handle_roi_get(req, ctx);
    if (strcmp(p, "/analytics/time") == 0 && strcmp(req.method, "POST") == 0)
        return handle_time_store(req, ctx);

    if (strcmp(p, "/xp") == 0 && strcmp(req.method, "POST") == 0)
        return handle_xp_credit(req, ctx);
    if (strcmp(p, "/streak") == 0 && strcmp(req.method, "GET") == 0)
        return handle_streak_get(req, ctx);
    if (strcmp(p, "/leaderboard") == 0 && strcmp(req.method, "GET") == 0)
        return handle_leaderboard(req, ctx);

    if (strcmp(p, "/blog") == 0 && strcmp(req.method, "POST") == 0)
        return handle_blog_store(req, ctx);
    if (strncmp(p, "/blog/", 6) == 0 && strcmp(req.method, "GET") == 0)
        return handle_blog_get(req, ctx);
    if (strcmp(p, "/votes") == 0 && strcmp(req.method, "POST") == 0)
        return handle_vote_upsert(req, ctx);

    return ext_not_found();
}
