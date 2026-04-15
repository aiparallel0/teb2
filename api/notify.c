#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "exec/exec.h"
#include "api/api.h"
#include "api/json.h"

HttpResp handle_notify_email(HttpReq req, Ctx *ctx)
{
    MailReq mq;
    MailResult mr;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&mq, 0, sizeof(mq));
    (void)extract_json_str(req.body, "\"to\"", mq.to, sizeof(mq.to));
    (void)extract_json_str(req.body, "\"subject\"", mq.subject, sizeof(mq.subject));
    (void)extract_json_str(req.body, "\"body\"", mq.body, sizeof(mq.body));
    if (!mq.to[0]) return json_error(400, "missing_to");
    mr = send_mail(mq, ctx->cfg);
    if (mr.err != ERR_OK) return json_error(502, "smtp_error");
    return json_ok("{\"sent\":true}");
}
