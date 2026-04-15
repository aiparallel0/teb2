#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"

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

static int extract_json_str(const char *body, const char *key,
                            char *out, size_t outsz)
{
    const char *k = strstr(body, key), *v;
    size_t i;
    if (!k) return 0;
    k += strlen(key);
    while (*k == ' ' || *k == ':' || *k == '"') k++;
    for (i = 0, v = k; i < outsz - 1 && v[i] && v[i] != '"'; i++)
        out[i] = v[i];
    out[i] = '\0';
    return i > 0 ? 1 : 0;
}

static HttpResp budget_json(BudgetResult br)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"id\":%lld,\"user_id\":\"%s\","
             "\"limit_cents\":%lld,\"spent_cents\":%lld}",
             (long long)br.budget.id, br.budget.user_id,
             (long long)br.budget.limit_cents,
             (long long)br.budget.spent_cents);
    return json_ok(buf);
}

HttpResp handle_budget_create(HttpReq req, Ctx *ctx)
{
    BudgetQuery q;
    BudgetResult br;
    char amt[32];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    if (extract_json_str(req.body, "\"limit_cents\"", amt, sizeof(amt)))
        q.amount_cents = strtoll(amt, NULL, 10);
    if (q.amount_cents <= 0) return json_error(400, "bad_limit");

    br = store_budget(ctx->db, q);
    if (br.err != ERR_OK) return json_error(500, "db_error");
    return budget_json(br);
}

HttpResp handle_budget_get(HttpReq req, Ctx *ctx)
{
    BudgetQuery q;
    BudgetResult br;

    (void)req;
    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_READ))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);

    br = fetch_budget(ctx->db, q);
    if (br.err == ERR_NOT_FOUND) return json_error(404, "not_found");
    if (br.err != ERR_OK)        return json_error(500, "db_error");
    return budget_json(br);
}

HttpResp handle_spend_record(HttpReq req, Ctx *ctx)
{
    BudgetQuery q;
    BudgetResult br;
    char amt[32];

    if (!ctx || !ctx->user) return json_error(401, "unauthorized");
    if (!rbac_allow(ctx->user->role, PERM_TASK_WRITE))
        return json_error(403, "forbidden");

    memset(&q, 0, sizeof(q));
    snprintf(q.user_id, sizeof(q.user_id), "%lld",
             (long long)ctx->user->user_id);
    if (extract_json_str(req.body, "\"amount_cents\"", amt, sizeof(amt)))
        q.amount_cents = strtoll(amt, NULL, 10);
    if (q.amount_cents <= 0) return json_error(400, "bad_amount");

    br = record_spend(ctx->db, q);
    if (br.err == ERR_NOT_FOUND) return json_error(404, "no_budget");
    if (br.err != ERR_OK)        return json_error(500, "db_error");
    return budget_json(br);
}
