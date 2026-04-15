#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_ext.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/json.h"
#include "api/escape.h"

HttpResp handle_blog_store(HttpReq req, Ctx *ctx)
{
    BlogQuery q;
    BlogResult r;
    char buf[512], et[256];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    (void)extract_json_str(req.body, "\"title\"", q.title, sizeof(q.title));
    (void)extract_json_str(req.body, "\"body\"", q.body, sizeof(q.body));
    if (!q.title[0]) return json_error(400, "missing_title");
    r = store_blog(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.post.title, et, sizeof(et));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"title\":\"%s\"}",
             (long long)r.post.id, et);
    return json_ok(buf);
}

HttpResp handle_blog_get(HttpReq req, Ctx *ctx)
{
    BlogQuery q;
    BlogResult r;
    char buf[1024], et[256], eb[512];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    q.id = strtoll(req.path + 6, NULL, 10);
    r = fetch_blog(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.post.title, et, sizeof(et));
    json_escape(r.post.body, eb, sizeof(eb));
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"title\":\"%s\",\"body\":\"%s\"}",
             (long long)r.post.id, et, eb);
    return json_ok(buf);
}

HttpResp handle_vote_upsert(HttpReq req, Ctx *ctx)
{
    VoteQuery q;
    VoteResult r;
    char buf[256], ef[128];
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    (void)extract_json_str(req.body, "\"feature\"", q.feature,
                           sizeof(q.feature));
    if (!q.feature[0]) return json_error(400, "missing_feature");
    r = store_vote(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");
    json_escape(r.vote.feature, ef, sizeof(ef));
    snprintf(buf, sizeof(buf),
             "{\"feature\":\"%s\",\"count\":%d}",
             ef, r.count);
    return json_ok(buf);
}
