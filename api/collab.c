#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_ws_create(HttpReq req, Ctx *ctx)
{
    WsQuery q;
    WsResult r;
    char buf[256], en[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.owner_id, sizeof(q.owner_id), "%lld",
             (long long)ctx->user->user_id);
    (void)extract_json_str(req.body, "\"name\"", q.name, sizeof(q.name));
    r = store_ws(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.ws.name, en, sizeof(en));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"name\":\"%s\"}", (long long)r.ws.id, en);
    return json_ok(buf);
}

HttpResp handle_ws_get(HttpReq req, Ctx *ctx)
{
    WsQuery q;
    WsResult r;
    char buf[256], en[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.id = strtoll(req.path + 12, NULL, 10);
    r = fetch_ws(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.ws.name, en, sizeof(en));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"name\":\"%s\"}", (long long)r.ws.id, en);
    return json_ok(buf);
}

HttpResp handle_collab_add(HttpReq req, Ctx *ctx)
{
    CollabQuery q;
    CollabResult r;
    char buf[256], wsid[32];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"ws_id\"", wsid, sizeof(wsid)))
        q.ws_id = strtoll(wsid, NULL, 10);
    (void)extract_json_str(req.body, "\"user_id\"", q.user_id, sizeof(q.user_id));
    (void)extract_json_str(req.body, "\"role\"", q.role_name, sizeof(q.role_name));
    r = store_collab(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"ws_id\":%lld}",
             (long long)r.rows[0].id, (long long)r.rows[0].ws_id);
    return json_ok(buf);
}

HttpResp handle_collab_list(HttpReq req, Ctx *ctx)
{
    CollabQuery q;
    CollabResult r;
    char buf[2048];
    int i, off;
    int64_t next_cursor;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.ws_id = strtoll(req.path + 9, NULL, 10);
    q.limit = 16;
    q.cursor = extract_cursor(req.query);
    r = list_collabs(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    next_cursor = (r.count > 0) ? r.rows[r.count - 1].id : 0;
    off = snprintf(buf, sizeof(buf), "{\"next_cursor\":%lld,\"items\":[",
                   (long long)next_cursor);
    for (i = 0; i < r.count && off < (int)sizeof(buf) - 64; i++) {
        if (i > 0 && off < (int)sizeof(buf) - 1) buf[off++] = ',';
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            "{\"id\":%lld,\"user_id\":\"%s\"}",
            (long long)r.rows[i].id, r.rows[i].user_id);
    }
    if (off > 0 && (size_t)off < sizeof(buf) - 3)
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "]}");
    (void)off;
    return json_ok(buf);
}

HttpResp handle_chat_send(HttpReq req, Ctx *ctx)
{
    ChatQuery q;
    ChatResult r;
    char buf[256], wsid[32];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    if (extract_json_str(req.body, "\"ws_id\"", wsid, sizeof(wsid)))
        q.ws_id = strtoll(wsid, NULL, 10);
    snprintf(q.sender_id, sizeof(q.sender_id), "%lld",
             (long long)ctx->user->user_id);
    (void)extract_json_str(req.body, "\"body\"", q.body, sizeof(q.body));
    r = store_chat(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    snprintf(buf, sizeof(buf), "{\"id\":%lld}", (long long)r.rows[0].id);
    return json_ok(buf);
}

HttpResp handle_chat_list(HttpReq req, Ctx *ctx)
{
    ChatQuery q;
    ChatResult r;
    char buf[4096];
    int i, off;
    int64_t next_cursor;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.ws_id = strtoll(req.path + 6, NULL, 10);
    q.limit = 16;
    q.cursor = extract_cursor(req.query);
    r = list_chats(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    next_cursor = (r.count > 0) ? r.rows[r.count - 1].id : 0;
    off = snprintf(buf, sizeof(buf), "{\"next_cursor\":%lld,\"items\":[",
                   (long long)next_cursor);
    for (i = 0; i < r.count && off < (int)sizeof(buf) - 128; i++) {
        char eb[512];
        json_escape(r.rows[i].body, eb, sizeof(eb));
        if (i > 0 && off < (int)sizeof(buf) - 1) buf[off++] = ',';
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            "{\"id\":%lld,\"body\":\"%s\"}",
            (long long)r.rows[i].id, eb);
    }
    if (off > 0 && (size_t)off < sizeof(buf) - 3)
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "]}");
    (void)off;
    return json_ok(buf);
}
