#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"
#include "agents/channel.h"

HttpResp handle_oauth_redirect(HttpReq req, Ctx *ctx)
{
    char prov[32], ruri[256], buf[1024];
    AgentMsg msg, res;
    TokenResult ar;

    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) return json_error(401, "unauthorized");
    (void)extract_json_str(req.body, "\"provider\"", prov, sizeof(prov));
    (void)extract_json_str(req.body, "\"redirect_uri\"", ruri, sizeof(ruri));
    if (!prov[0]) return json_error(400, "missing_provider");
    memset(&msg, 0, sizeof(msg));
    msg.tag = MSG_OAUTH;
    msg.db  = ctx->db;
    snprintf(msg.user_id, sizeof(msg.user_id), "%lld",
             (long long)ar.claims.user_id);
    snprintf(msg.payload, sizeof(msg.payload), "state:%.31s", prov);
    res = coord_handle(msg);
    if (res.err != ERR_OK) return json_error(500, "state_gen_failed");
    snprintf(buf, sizeof(buf),
             "{\"state\":\"%s\",\"redirect_uri\":\"%s\"}", res.payload, ruri);
    return json_ok(buf);
}

HttpResp handle_oauth_callback(HttpReq req, Ctx *ctx)
{
    char prov[32], code[256], ruri[256], buf[256];
    AgentMsg msg, res;
    TokenResult ar;

    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) return json_error(401, "unauthorized");
    (void)extract_json_str(req.body, "\"provider\"", prov, sizeof(prov));
    (void)extract_json_str(req.body, "\"code\"", code, sizeof(code));
    (void)extract_json_str(req.body, "\"redirect_uri\"", ruri, sizeof(ruri));
    if (!prov[0] || !code[0]) return json_error(400, "missing_fields");
    memset(&msg, 0, sizeof(msg));
    msg.tag = MSG_OAUTH;
    msg.db  = ctx->db;
    snprintf(msg.user_id, sizeof(msg.user_id), "%lld",
             (long long)ar.claims.user_id);
    snprintf(msg.payload, sizeof(msg.payload), "exchange:%.31s:%.200s:%.200s",
             prov, code, ruri);
    res = coord_handle(msg);
    if (res.err != ERR_OK) return json_error(500, "exchange_failed");
    snprintf(buf, sizeof(buf), "{\"ok\":true}");
    return json_ok(buf);
}
