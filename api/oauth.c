#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "agents/channel.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

static const char *auth_base(const char *p)
{
    if (strcmp(p, "google") == 0)
        return "https://accounts.google.com/o/oauth2/v2/auth";
    if (strcmp(p, "github") == 0)
        return "https://github.com/login/oauth/authorize";
    if (strcmp(p, "slack") == 0)
        return "https://slack.com/oauth/v2/authorize";
    return "";
}

HttpResp handle_oauth_redirect(HttpReq req, Ctx *ctx)
{
    char prov[32], ruri[256], buf[1024];
    AgentMsg msg, res;
    TokenResult ar;
    const char *cid, *base, *seg, *qp;
    char env_id[64], up[32];
    int i;

    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) return json_error(401, "unauthorized");
    seg = req.path + 16;
    snprintf(prov, sizeof(prov), "%.*s",
             (int)strcspn(seg, "?/ "), seg);
    if (!prov[0]) return json_error(400, "missing_provider");
    memset(ruri, 0, sizeof(ruri));
    qp = strstr(req.query, "redirect_uri=");
    if (qp && (qp == req.query || qp[-1] == '&'))
        snprintf(ruri, sizeof(ruri), "%.*s",
                 (int)strcspn(qp + 13, "& "), qp + 13);
    memset(&msg, 0, sizeof(msg));
    msg.tag = MSG_OAUTH;
    msg.db  = ctx->db;
    msg.cfg = ctx->cfg;
    snprintf(msg.user_id, sizeof(msg.user_id), "%lld",
             (long long)ar.claims.user_id);
    snprintf(msg.payload, sizeof(msg.payload), "state:%.31s", prov);
    res = coord_handle(msg);
    if (res.err != ERR_OK) return json_error(500, "state_gen_failed");
    for (i = 0; i < (int)sizeof(up) - 1 && prov[i]; i++)
        up[i] = (prov[i]>='a' && prov[i]<='z') ? (char)(prov[i]-32) : prov[i];
    up[i] = '\0';
    snprintf(env_id, sizeof(env_id), "%s_CLIENT_ID", up);
    cid = getenv(env_id);
    if (!cid) cid = "";
    base = auth_base(prov);
    snprintf(buf, sizeof(buf),
        "{\"url\":\"%s?client_id=%s&redirect_uri=%.200s&state=%.64s\"}",
        base, cid, ruri, res.payload);
    return json_ok(buf);
}

HttpResp handle_oauth_callback(HttpReq req, Ctx *ctx)
{
    char prov[32], code[256], state[64], ruri[256], buf[256];
    AgentMsg msg, res;
    TokenResult ar;
    MemQuery mq;
    MemResult mr;

    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) return json_error(401, "unauthorized");
    (void)extract_json_str(req.body, "\"provider\"", prov, sizeof(prov));
    (void)extract_json_str(req.body, "\"code\"", code, sizeof(code));
    (void)extract_json_str(req.body, "\"state\"", state, sizeof(state));
    (void)extract_json_str(req.body, "\"redirect_uri\"", ruri, sizeof(ruri));
    if (!prov[0] || !code[0]) return json_error(400, "missing_fields");
    memset(&mq, 0, sizeof(mq));
    snprintf(mq.agent, sizeof(mq.agent), "oauth");
    snprintf(mq.key, sizeof(mq.key), "state_%.20s", prov);
    mr = fetch_mem(ctx->db, mq);
    if (mr.err != ERR_OK || strcmp(mr.entry.val, state) != 0)
        return json_error(400, "state_mismatch");
    memset(&msg, 0, sizeof(msg));
    msg.tag = MSG_OAUTH;
    msg.db  = ctx->db;
    msg.cfg = ctx->cfg;
    snprintf(msg.user_id, sizeof(msg.user_id), "%lld",
             (long long)ar.claims.user_id);
    snprintf(msg.payload, sizeof(msg.payload),
             "exchange:%.31s:%.200s:%.200s", prov, code, ruri);
    res = coord_handle(msg);
    if (res.err != ERR_OK) return json_error(500, "exchange_failed");
    snprintf(buf, sizeof(buf),
             "{\"provider\":\"%.31s\",\"access_token\":\"%.64s\"}",
             prov, res.payload);
    return json_ok(buf);
}
