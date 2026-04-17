#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "core/prompts.h"
#include "core/log.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

/*
 * PUT /prompts/<name>  — per-user prompt override.
 * Body: {"body":"<new prompt text>"}
 * Replaces the compiled prompt with user's version for all their
 * future LLM calls. Only admin or the owning user can override.
 */
HttpResp handle_prompt_edit(HttpReq req, Ctx *ctx)
{
    const char *name;
    char body_text[4096], ename[64], uid[32], buf[256];
    OverrideQuery oq; OverrideResult or_;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");

    name = req.path + 9; /* skip "/prompts/" */
    if (!name[0]) return json_error(400, "missing_name");

    /* Validate name exists in compiled registry */
    if (!prompt_get(name)) return json_error(404, "prompt_not_found");

    if (!extract_json_str(req.body, "\"body\"", body_text, sizeof(body_text)))
        return json_error(400, "missing_body");
    if (strlen(body_text) < 10)
        return json_error(400, "body_too_short");

    memset(&oq, 0, sizeof(oq));
    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    snprintf(oq.user_id, sizeof(oq.user_id), "%s", uid);
    snprintf(oq.name, sizeof(oq.name), "%.63s", name);
    snprintf(oq.body, sizeof(oq.body), "%s", body_text);

    or_ = store_override(ctx->db, oq);
    if (or_.err != ERR_OK) return json_error(500, "db_error");

    teb_log_info("prompts", "user %s overrode prompt '%s'", uid, name);
    json_escape(name, ename, sizeof(ename));
    snprintf(buf, sizeof(buf),
             "{\"name\":\"%s\",\"status\":\"saved\"}", ename);
    return json_ok(buf);
}

/*
 * DELETE /prompts/<name> — revert to compiled default.
 */
HttpResp handle_prompt_delete(HttpReq req, Ctx *ctx)
{
    const char *name;
    char ename[64], uid[32], buf[256];
    Err de;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");

    name = req.path + 9;
    if (!name[0]) return json_error(400, "missing_name");

    snprintf(uid, sizeof(uid), "%lld", (long long)ctx->user->user_id);
    de = delete_override(ctx->db, uid, name);
    (void)de;

    teb_log_info("prompts", "user %s reverted prompt '%s'", uid, name);
    json_escape(name, ename, sizeof(ename));
    snprintf(buf, sizeof(buf),
             "{\"name\":\"%s\",\"status\":\"reverted\"}", ename);
    return json_ok(buf);
}
