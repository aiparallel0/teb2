#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

static HttpResp integ_json(IntegResult ir)
{
    char buf[512], en[128];
    json_escape(ir.integ.name, en, sizeof(en));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"name\":\"%s\",\"kind\":\"%s\",\"enabled\":%d}",
             (long long)ir.integ.id, en, ir.integ.kind, ir.integ.enabled);
    return json_ok(buf);
}

HttpResp handle_integ_toggle(HttpReq req, Ctx *ctx)
{
    IntegQuery q;
    IntegResult r;
    char en[8];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_ADMIN))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    (void)extract_json_str(req.body, "\"name\"", q.name, sizeof(q.name));
    (void)extract_json_str(req.body, "\"kind\"", q.kind, sizeof(q.kind));
    q.enabled = 0;
    if (extract_json_str(req.body, "\"enabled\"", en, sizeof(en)))
        q.enabled = (en[0] == '1' || en[0] == 't') ? 1 : 0;
    r = store_integ(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    return integ_json(r);
}

HttpResp handle_integ_get(HttpReq req, Ctx *ctx)
{
    IntegQuery q;
    IntegResult r;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.id = strtoll(req.path + 14, NULL, 10);
    r = fetch_integ(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    return integ_json(r);
}

HttpResp handle_webhook_route(HttpReq req, Ctx *ctx)
{
    IntegQuery q;
    IntegResult r;
    char name[128];
    if (!ctx) return json_error(500, "no_context");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"integration\"", name, sizeof(name)))
        snprintf(q.name, sizeof(q.name), "%s", name);
    q.enabled = 1;
    r = fetch_integ(ctx->db, q);
    if (r.err != ERR_OK) return json_error(404, "integration_not_found");
    if (!r.integ.enabled) return json_error(403, "integration_disabled");
    return json_ok("{\"status\":\"dispatched\"}");
}
