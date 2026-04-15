#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"

static void ticket_to_hex(const Ticket *t, char *out, size_t outsz)
{
    const unsigned char *p = (const unsigned char *)t;
    size_t i, sz = sizeof(Ticket);
    if (outsz < sz * 2 + 1) { out[0] = '\0'; return; }
    for (i = 0; i < sz; i++)
        snprintf(out + i * 2, 3, "%02x", p[i]);
}

static int hex_to_ticket(const char *hex, Ticket *out)
{
    unsigned char *p = (unsigned char *)out;
    size_t i, sz = sizeof(Ticket);
    unsigned int b;
    if (strlen(hex) < sz * 2) return -1;
    for (i = 0; i < sz; i++) {
        if (sscanf(hex + i * 2, "%2x", &b) != 1) return -1;
        p[i] = (unsigned char)b;
    }
    return 0;
}

TokenResult authenticate_request(HttpReq req, const char *secret)
{
    TokenResult r;
    Ticket t;
    memset(&r, 0, sizeof(r));
    if (req.auth_header[0] == '\0' || !secret || secret[0] == '\0') {
        r.err = ERR_AUTH;
        return r;
    }
    if (hex_to_ticket(req.auth_header, &t) != 0) {
        r.err = ERR_AUTH;
        return r;
    }
    return check_ticket(t, secret);
}

HttpResp handle_register(HttpReq req, Ctx *ctx)
{
    HashConfig hcfg;
    HashResult hr;
    UserQuery  uq;
    UserResult ur;
    char email[128], password[128];
    if (!ctx || !ctx->db) return json_error(500, "no_db");
    if (!extract_json_str(req.body, "\"email\"", email, sizeof(email)))
        return json_error(400, "missing_email");
    if (!extract_json_str(req.body, "\"password\"", password,
                          sizeof(password)))
        return json_error(400, "missing_password");
    hcfg = default_hash_config();
    hr   = hash_password(password, hcfg);
    if (hr.err != ERR_OK) return json_error(500, "hash_error");
    memset(&uq, 0, sizeof(uq));
    snprintf(uq.email, sizeof(uq.email), "%s", email);
    snprintf(uq.password_hash, sizeof(uq.password_hash), "%s", hr.hash);
    uq.role = ROLE_USER;
    ur = store_user(ctx->db, uq);
    if (ur.err != ERR_OK) return json_error(409, "user_exists");
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"id\":%lld}",
                 (long long)ur.user.id);
        return json_ok(buf);
    }
}

HttpResp handle_login(HttpReq req, Ctx *ctx)
{
    UserQuery  uq;
    UserResult ur;
    HashResult stored, check;
    TokenResult tr;
    UserClaims claims;
    char email[128], password[128];
    char hex[sizeof(Ticket) * 2 + 1], body[256];
    if (!ctx || !ctx->db || !ctx->cfg) return json_error(500, "no_ctx");
    if (!extract_json_str(req.body, "\"email\"", email, sizeof(email)))
        return json_error(400, "missing_email");
    if (!extract_json_str(req.body, "\"password\"", password,
                          sizeof(password)))
        return json_error(400, "missing_password");
    memset(&uq, 0, sizeof(uq));
    snprintf(uq.email, sizeof(uq.email), "%s", email);
    ur = fetch_user(ctx->db, uq);
    if (ur.err == ERR_NOT_FOUND)
        return json_error(401, "invalid_credentials");
    if (ur.err != ERR_OK) return json_error(500, "db_error");
    memset(&stored, 0, sizeof(stored));
    snprintf(stored.hash, sizeof(stored.hash), "%s",
             ur.user.password_hash);
    check = verify_password(password, stored);
    if (check.err != ERR_OK || !check.match)
        return json_error(401, "invalid_credentials");
    memset(&claims, 0, sizeof(claims));
    claims.user_id = ur.user.id;
    snprintf(claims.email, sizeof(claims.email), "%s", ur.user.email);
    claims.role = ur.user.role;
    tr = make_ticket(claims, ctx->cfg->secret);
    if (tr.err != ERR_OK) return json_error(500, "token_error");
    ticket_to_hex(&tr.ticket, hex, sizeof(hex));
    snprintf(body, sizeof(body), "{\"token\":\"%s\"}", hex);
    return json_ok(body);
}

HttpResp handle_refresh(HttpReq req, Ctx *ctx)
{
    TokenResult tr;
    char hex[sizeof(Ticket) * 2 + 1], body[256];
    (void)req;
    if (!ctx || !ctx->user || !ctx->cfg)
        return json_error(401, "unauthorized");
    tr = make_ticket(*ctx->user, ctx->cfg->secret);
    if (tr.err != ERR_OK) return json_error(500, "token_error");
    ticket_to_hex(&tr.ticket, hex, sizeof(hex));
    snprintf(body, sizeof(body), "{\"token\":\"%s\"}", hex);
    return json_ok(body);
}
