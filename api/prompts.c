#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/prompts.h"
#include "auth/auth.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

/*
 * Read-only prompt registry endpoint.
 *
 *   GET /prompts           — list every prompt name
 *   GET /prompts/<name>    — return the prompt body as plain text
 *
 * Admin-only. Non-admins get 403 to avoid leaking the system prompt
 * in case it ever contains tenant-specific overrides. The body is
 * served as text/plain to keep the output usable in the UI diff view
 * without double-escaping.
 */

HttpResp handle_prompt_list(HttpReq req, Ctx *ctx)
{
    const char *const *names;
    char body[4096];
    size_t pos = 0, i;
    (void)req;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_ADMIN))
        return json_error(403, "forbidden");

    names = prompt_list();
    pos += snprintf(body + pos, sizeof(body) - pos, "{\"names\":[");
    for (i = 0; i < prompt_count() && names[i]; i++) {
        char esc[128];
        json_escape(names[i], esc, sizeof(esc));
        pos += snprintf(body + pos, (pos < sizeof(body)) ? sizeof(body) - pos : 0,
                        "%s\"%s\"", i ? "," : "", esc);
    }
    snprintf(body + pos, (pos < sizeof(body)) ? sizeof(body) - pos : 0, "]}");
    return json_ok(body);
}

HttpResp handle_prompt_get(HttpReq req, Ctx *ctx)
{
    const char *name, *body;
    HttpResp r;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_ADMIN))
        return json_error(403, "forbidden");

    name = req.path + 9; /* skip "/prompts/" */
    if (!name[0]) return json_error(400, "bad_name");
    body = prompt_get(name);
    if (!body) return json_error(404, "not_found");

    memset(&r, 0, sizeof(r));
    r.status = 200;
    snprintf(r.content_type, sizeof(r.content_type), "text/plain; charset=utf-8");
    snprintf(r.body, sizeof(r.body), "%.*s",
             (int)sizeof(r.body) - 1, body);
    r.body_len = strlen(r.body);
    return r;
}
