#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_org_create(HttpReq req, Ctx *ctx)
{
    OrgQuery q;
    OrgResult r;
    char buf[512], en[128], ed[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_ADMIN))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    (void)extract_json_str(req.body, "\"name\"", q.name, sizeof(q.name));
    (void)extract_json_str(req.body, "\"domain\"", q.domain, sizeof(q.domain));
    if (!q.name[0] || !q.domain[0])
        return json_error(400, "missing_fields");
    r = store_org(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.org.name, en, sizeof(en));
    json_escape(r.org.domain, ed, sizeof(ed));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"name\":\"%s\",\"domain\":\"%s\"}",
             (long long)r.org.id, en, ed);
    return json_ok(buf);
}

HttpResp handle_org_get(HttpReq req, Ctx *ctx)
{
    OrgQuery q;
    OrgResult r;
    char buf[512], en[128], ed[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.id = strtoll(req.path + 6, NULL, 10);
    r = fetch_org(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.org.name, en, sizeof(en));
    json_escape(r.org.domain, ed, sizeof(ed));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"name\":\"%s\",\"domain\":\"%s\"}",
             (long long)r.org.id, en, ed);
    return json_ok(buf);
}

HttpResp handle_sso_validate(HttpReq req, Ctx *ctx)
{
    SsoQuery q;
    SsoResult r;
    char buf[512], orgid[32];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"org_id\"", orgid, sizeof(orgid)))
        q.org_id = strtoll(orgid, NULL, 10);
    (void)extract_json_str(req.body, "\"provider\"", q.provider,
                           sizeof(q.provider));
    r = store_sso(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"provider\":\"%s\"}",
             (long long)r.sso.id, r.sso.provider);
    return json_ok(buf);
}

HttpResp handle_ip_check(HttpReq req, Ctx *ctx)
{
    IpQuery q;
    IpResult r;
    char orgid[32];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"org_id\"", orgid, sizeof(orgid)))
        q.org_id = strtoll(orgid, NULL, 10);
    (void)extract_json_str(req.body, "\"cidr\"", q.cidr, sizeof(q.cidr));
    r = check_ip(ctx->db, q);
    if (r.err == ERR_NOT_FOUND)
        return json_ok("{\"allowed\":false}");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    return json_ok("{\"allowed\":true}");
}
