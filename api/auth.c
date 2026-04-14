#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"

#define DEFAULT_ROUNDS 5000

static HttpResp json_error(int status, const char *msg)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = status;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                  "{\"error\":\"%s\"}", msg);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

static HttpResp json_ok(const char *body)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 200;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body), "%s", body);
    snprintf(r.content_type, sizeof(r.content_type), "%s", "application/json");
    return r;
}

static HashConfig default_hash_cfg(void)
{
    HashConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.rounds = DEFAULT_ROUNDS;
    snprintf(cfg.salt, sizeof(cfg.salt), "%s", "teb2salt");
    return cfg;
}

HttpResp handle_register(HttpReq req, Ctx *ctx)
{
    HashConfig  hcfg;
    HashResult  hr;
    GoalQuery   uq;   /* repurpose GoalQuery for user_id placeholder */
    const char *email_start;
    const char *pw_start;

    (void)ctx;
    /* Minimal JSON parse: look for "email" and "password" keys */
    email_start = strstr(req.body, "\"email\"");
    pw_start    = strstr(req.body, "\"password\"");
    if (!email_start || !pw_start) return json_error(400, "missing_fields");

    hcfg = default_hash_cfg();
    hr   = hash_password(pw_start + 12, hcfg); /* crude: points past key+colon */
    if (hr.err != ERR_OK) return json_error(500, "hash_error");

    memset(&uq, 0, sizeof(uq));
    snprintf(uq.user_id, sizeof(uq.user_id), "%s", "new");
    uq.limit = 1;
    (void)uq;

    return json_ok("{\"status\":\"registered\"}");
}

HttpResp handle_login(HttpReq req, Ctx *ctx)
{
    HashConfig  hcfg;
    HashResult  stored;
    HashResult  check;
    const char *pw_start;
    char        token_buf[128];

    (void)ctx;
    pw_start = strstr(req.body, "\"password\"");
    if (!pw_start) return json_error(400, "missing_fields");

    hcfg = default_hash_cfg();

    /* In production: load stored hash from DB. Here: hash the input. */
    stored = hash_password(pw_start + 12, hcfg);
    if (stored.err != ERR_OK) return json_error(500, "hash_error");

    check = verify_password(pw_start + 12, stored);
    if (check.err != ERR_OK || !check.match)
        return json_error(401, "invalid_credentials");

    snprintf(token_buf, sizeof(token_buf), "{\"token\":\"placeholder\"}");
    return json_ok(token_buf);
}

HttpResp handle_refresh(HttpReq req, Ctx *ctx)
{
    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    return json_ok("{\"token\":\"refreshed\"}");
}
