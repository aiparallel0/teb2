#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "api/api.h"
#include "api/json.h"

HttpResp handle_sse_subscribe(HttpReq req, Ctx *ctx)
{
    HttpResp resp;
    TokenResult ar;

    memset(&resp, 0, sizeof(resp));
    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) {
        return json_error(401, "unauthorized");
    }
    if (!rbac_allow(ar.claims.role, PERM_GOAL_READ)) {
        return json_error(403, "forbidden");
    }
    resp.status = 200;
    snprintf(resp.content_type, sizeof(resp.content_type),
             "text/event-stream");
    resp.body_len = 0;
    return resp;
}
