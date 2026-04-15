#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

static const char *SSE_HEADERS =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/event-stream\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: keep-alive\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "\r\n";

HttpResp handle_sse_subscribe(HttpReq req, Ctx *ctx)
{
    HttpResp resp;
    TokenResult ar;
    int64_t ws_id, last_id;
    SseConn c;
    ChatQuery q;
    ChatResult cr;
    int i;
    ssize_t nw;

    memset(&resp, 0, sizeof(resp));
    ar = authenticate_request(req, ctx->cfg->secret);
    if (ar.err != ERR_OK) return json_error(401, "unauthorized");
    if (!rbac_allow(ar.claims.role, PERM_GOAL_READ))
        return json_error(403, "forbidden");
    ws_id = strtoll(req.path + 10, NULL, 10);
    nw = write(req.fd, SSE_HEADERS, strlen(SSE_HEADERS));
    if (nw < 0) return resp;
    c.fd = req.fd;
    c.open = 1;
    last_id = 0;
    while (c.open) {
        char buf[512];
        sleep(2);
        memset(&q, 0, sizeof(q));
        q.ws_id = ws_id;
        q.cursor = last_id;
        q.limit = 16;
        cr = list_chats(ctx->db, q);
        if (cr.err != ERR_OK) continue;
        for (i = 0; i < cr.count && c.open; i++) {
            char eb[480];
            json_escape(cr.rows[i].body, eb, sizeof(eb));
            snprintf(buf, sizeof(buf),
                     "{\"id\":%lld,\"body\":\"%s\"}",
                     (long long)cr.rows[i].id, eb);
            sse_write(&c, "message", buf);
            last_id = cr.rows[i].id;
        }
    }
    return resp;
}
