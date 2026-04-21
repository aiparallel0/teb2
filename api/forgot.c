#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/log.h"
#include "auth/auth.h"
#include "exec/exec.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"

/*
 * Password reset flow.
 *
 *   POST /auth/forgot { "email": ... }
 *       Always returns 200 {"ok":true} so callers cannot enumerate which
 *       emails are registered. If the email matches a user and SMTP is
 *       configured, a 32-hex single-use reset token (1h TTL) is emailed
 *       to the user. If SMTP is not configured, returns 503 so operators
 *       learn the feature is disabled rather than silently failing.
 *
 *   POST /auth/reset  { "token": ..., "password": ... }
 *       Atomically consumes the token and replaces the user's password
 *       hash. Same 8-char minimum as /auth/register.
 *
 * Both routes are rate-limited via the strict /auth bucket in
 * api/dispatch.c to blunt token-guessing / enumeration attempts.
 */

#define RESET_TTL_SEC 3600L
#define RESET_TOKEN_BYTES 16 /* 32 hex chars */

static int gen_token_hex(char out[RESET_TOKEN_BYTES * 2 + 1])
{
    unsigned char raw[RESET_TOKEN_BYTES];
    ssize_t n;
    size_t i;
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    n = read(fd, raw, sizeof(raw));
    close(fd);
    if (n != (ssize_t)sizeof(raw)) return -1;
    for (i = 0; i < sizeof(raw); i++)
        snprintf(out + i * 2, 3, "%02x", raw[i]);
    out[RESET_TOKEN_BYTES * 2] = '\0';
    return 0;
}

static Err email_reset_token(Ctx *ctx, const char *email, const char *token)
{
    MailReq mq;
    MailResult mr;
    memset(&mq, 0, sizeof(mq));
    snprintf(mq.to,      sizeof(mq.to),      "%s", email);
    snprintf(mq.subject, sizeof(mq.subject), "%s",
             "teb2 password reset");
    snprintf(mq.body,    sizeof(mq.body),
             "A password reset was requested for this account.\r\n"
             "Reset token (valid 1 hour, single use): %s\r\n"
             "If you did not request this, ignore this email.\r\n",
             token);
    mr = send_mail(mq, ctx->cfg);
    return mr.err;
}

HttpResp handle_forgot(HttpReq req, Ctx *ctx)
{
    UserQuery  uq;
    UserResult ur;
    char email[128];
    char token[RESET_TOKEN_BYTES * 2 + 1];
    int64_t expiry;

    if (!ctx || !ctx->db || !ctx->cfg) return json_error(500, "no_ctx");
    if (!ctx->cfg->smtp_host[0])
        return json_error(503, "email_not_configured");
    if (!extract_json_str(req.body, "\"email\"", email, sizeof(email)))
        return json_error(400, "missing_email");
    auth_email_normalize(email);
    if (!strchr(email, '@')) return json_error(400, "invalid_email");

    memset(&uq, 0, sizeof(uq));
    snprintf(uq.email, sizeof(uq.email), "%s", email);
    ur = fetch_user(ctx->db, uq);
    /* Always respond ok: do not leak whether the email is registered. */
    if (ur.err != ERR_OK) return json_ok("{\"ok\":true}");

    if (gen_token_hex(token) != 0) return json_error(500, "rng_error");
    expiry = (int64_t)time(NULL) + RESET_TTL_SEC;
    if (create_password_reset(ctx->db, ur.user.id, token, expiry) != ERR_OK)
        return json_error(500, "db_error");
    if (email_reset_token(ctx, ur.user.email, token) != ERR_OK) {
        teb_log_warn("reset", "smtp_send_failed user_id=%lld",
                     (long long)ur.user.id);
        /* Still return ok to avoid enumeration; operator sees log. */
    }
    return json_ok("{\"ok\":true}");
}

HttpResp handle_reset(HttpReq req, Ctx *ctx)
{
    HashConfig hcfg;
    HashResult hr;
    char token[128], password[128];
    int64_t user_id = 0;

    if (!ctx || !ctx->db) return json_error(500, "no_db");
    if (!extract_json_str(req.body, "\"token\"", token, sizeof(token)))
        return json_error(400, "missing_token");
    if (!extract_json_str(req.body, "\"password\"", password,
                          sizeof(password)))
        return json_error(400, "missing_password");
    if (strlen(password) < 8)
        return json_error(400, "password_too_short");
    if (consume_password_reset(ctx->db, token, &user_id) != ERR_OK)
        return json_error(400, "invalid_token");
    hcfg = default_hash_config();
    hr   = hash_password(password, hcfg);
    if (hr.err != ERR_OK) return json_error(500, "hash_error");
    if (update_user_password(ctx->db, user_id, hr.hash) != ERR_OK)
        return json_error(500, "db_error");
    return json_ok("{\"ok\":true}");
}
