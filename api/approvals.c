#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_ext.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"
#include "api/json.h"
#include "api/escape.h"

/*
 * Finance HITL approvals endpoint.
 *
 *   GET  /approvals                 — list current user's pending approvals
 *   GET  /approvals?status=approved — filter by status
 *   POST /approvals/<id>/approve    — mark an approval approved
 *   POST /approvals/<id>/deny       — mark an approval denied
 *
 * Output is JSON; amounts are integer cents. Authorization requires
 * PERM_GOAL_WRITE (the same tier that can queue finance work).
 */

static void append_row(char *buf, size_t cap, size_t *pos,
                       const Approval *a, int first)
{
    char esc[640], risk[64];
    json_escape(a->payload, esc,  sizeof(esc));
    json_escape(a->risk,    risk, sizeof(risk));
    *pos += snprintf(buf + *pos, (*pos < cap) ? cap - *pos : 0,
        "%s{\"id\":%lld,\"amount_cents\":%lld,\"status\":\"%s\","
        "\"risk\":\"%s\",\"kind\":\"%s\",\"payload\":\"%s\","
        "\"created_at\":%lld}",
        first ? "" : ",",
        (long long)a->id, (long long)a->amount_cents,
        a->status, risk, a->kind, esc,
        (long long)a->created_at);
}

HttpResp handle_approval_list(HttpReq req, Ctx *ctx)
{
    ApprovalQuery q; ApprovalResult r;
    char body[4096];
    size_t pos = 0;
    const char *s;
    int i;

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    s = strstr(req.path, "status=");
    if (s) snprintf(q.status, sizeof(q.status), "%.15s", s + 7);
    else   snprintf(q.status, sizeof(q.status), "pending");
    q.limit = 16;

    r = list_approvals(ctx->db, q);
    if (r.err != ERR_OK) return json_error(500, "db_error");

    pos += snprintf(body + pos, sizeof(body) - pos, "{\"rows\":[");
    for (i = 0; i < r.count; i++)
        append_row(body, sizeof(body), &pos, &r.rows[i], i == 0);
    snprintf(body + pos, (pos < sizeof(body)) ? sizeof(body) - pos : 0,
             "],\"count\":%d}", r.count);
    return json_ok(body);
}

HttpResp handle_approval_update(HttpReq req, Ctx *ctx)
{
    ApprovalQuery q; ApprovalResult r;
    const char *p;
    char buf[256];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_GOAL_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    /* Path: /approvals/<id>/approve or /approvals/<id>/deny */
    p = req.path + 11; /* skip "/approvals/" */
    q.id = strtoll(p, NULL, 10);
    if (q.id <= 0) return json_error(400, "bad_id");
    if (strstr(req.path, "/approve"))   snprintf(q.status, sizeof(q.status), "approved");
    else if (strstr(req.path, "/deny")) snprintf(q.status, sizeof(q.status), "denied");
    else return json_error(400, "bad_action");

    r = update_approval(ctx->db, q);
    if (r.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (r.err != ERR_OK)        return json_error(500, "db_error");
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"status\":\"%s\",\"amount_cents\":%lld}",
             (long long)r.ap.id, r.ap.status,
             (long long)r.ap.amount_cents);
    return json_ok(buf);
}
