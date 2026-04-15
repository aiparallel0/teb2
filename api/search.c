#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include <stdlib.h>
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_search(HttpReq req, Ctx *ctx)
{
    SearchQuery q;
    SearchResult sr;
    char buf[4096], raw_q[256], lim[16];
    int i, off;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_READ))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    (void)extract_json_str(req.body, "\"q\"", raw_q, sizeof(raw_q));
    (void)extract_json_str(req.body, "\"entity\"", q.entity, sizeof(q.entity));
    if (extract_json_str(req.body, "\"limit\"", lim, sizeof(lim)))
        q.limit = (int)strtol(lim, NULL, 10);
    if (!raw_q[0]) return json_error(400, "missing_query");
    snprintf(q.query, sizeof(q.query), "%s", raw_q);
    sr = full_text_search(ctx->db, q);
    if (sr.err != ERR_OK) return json_error(500, "search_error");
    off = snprintf(buf, sizeof(buf), "{\"hits\":[");
    for (i = 0; i < sr.count && off > 0 && (size_t)off < sizeof(buf) - 128; i++) {
        char ee[64], es[256];
        json_escape(sr.hits[i].entity, ee, sizeof(ee));
        json_escape(sr.hits[i].snippet, es, sizeof(es));
        if (i > 0 && off < (int)sizeof(buf) - 1) buf[off++] = ',';
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            "{\"entity\":\"%s\",\"entity_id\":%lld,\"snippet\":\"%s\"}",
            ee, (long long)sr.hits[i].entity_id, es);
    }
    if (off > 0 && (size_t)off < sizeof(buf) - 3)
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "]}");
    (void)off;
    return json_ok(buf);
}
