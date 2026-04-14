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

static int extract_json_str(const char *body, const char *key,
                            char *out, size_t outsz)
{
    const char *k = strstr(body, key);
    const char *v;
    size_t i;

    if (!k) return 0;
    k += strlen(key);
    while (*k == ' ' || *k == ':' || *k == '"') k++;
    v = k;
    for (i = 0; i < outsz - 1 && v[i] != '\0' && v[i] != '"'; i++)
        out[i] = v[i];
    out[i] = '\0';
    return i > 0 ? 1 : 0;
}

HttpResp handle_register(HttpReq req, Ctx *ctx)
{
    HashConfig  hcfg;
    HashResult  hr;
    char        password[128];

    (void)ctx;
    if (!extract_json_str(req.body, "\"password\"", password, sizeof(password)))
        return json_error(400, "missing_fields");

    hcfg = default_hash_cfg();
    hr   = hash_password(password, hcfg);
    if (hr.err != ERR_OK) return json_error(500, "hash_error");

    return json_ok("{\"status\":\"registered\"}");
}

HttpResp handle_login(HttpReq req, Ctx *ctx)
{
    HashConfig  hcfg;
    HashResult  stored;
    HashResult  check;
    char        password[128];

    (void)ctx;
    if (!extract_json_str(req.body, "\"password\"", password, sizeof(password)))
        return json_error(400, "missing_fields");

    hcfg = default_hash_cfg();

    /* In production: load stored hash from DB. Verify against stored hash. */
    stored = hash_password(password, hcfg);
    if (stored.err != ERR_OK) return json_error(500, "hash_error");

    check = verify_password(password, stored);
    if (check.err != ERR_OK || !check.match)
        return json_error(401, "invalid_credentials");

    return json_ok("{\"token\":\"placeholder\"}");
}

HttpResp handle_refresh(HttpReq req, Ctx *ctx)
{
    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    return json_ok("{\"token\":\"refreshed\"}");
}
