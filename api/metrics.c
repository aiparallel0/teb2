#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sqlite3.h>
#include "core/types.h"
#include "api/api.h"
#include "api/json.h"

#define METRIC_SLOTS 16

typedef struct { char method[8]; char path[32]; int status; long count; } MetricBucket;
static MetricBucket mb[METRIC_SLOTS];
static int mb_count;
static long mb_overflow;
/* Phase 9: LLM call aggregates, updated by core/llm.c via
 * metrics_observe_llm(tokens, latency_ms). Exposed on /metrics. */
static long llm_calls_total;
static long llm_tokens_total;
static long long llm_latency_sum_ms;

void metrics_observe_llm(long tokens, long latency_ms)
{
    llm_calls_total++;
    if (tokens > 0) llm_tokens_total += tokens;
    if (latency_ms > 0) llm_latency_sum_ms += latency_ms;
}

void metrics_inc(const char *method, const char *path, int status)
{
    int i;
    for (i = 0; i < mb_count; i++) {
        if (strcmp(mb[i].method, method) == 0 &&
            strcmp(mb[i].path, path) == 0 &&
            mb[i].status == status) {
            mb[i].count++;
            return;
        }
    }
    if (mb_count < METRIC_SLOTS) {
        snprintf(mb[mb_count].method, sizeof(mb[mb_count].method), "%s", method);
        snprintf(mb[mb_count].path, sizeof(mb[mb_count].path), "%.31s", path);
        mb[mb_count].status = status;
        mb[mb_count].count = 1;
        mb_count++;
    } else {
        mb_overflow++;
    }
}

/* Phase 9: /healthz now probes the DB with a trivial SELECT 1 so a
 * wedged SQLite file makes the endpoint fail. Returns 503 on probe
 * failure; Kubernetes / nginx upstream checks treat that as unhealthy. */
HttpResp handle_healthz(HttpReq req, Ctx *ctx)
{
    char buf[192];
    int db_ok = 0;
    (void)req;
    if (ctx && ctx->db && ctx->db->handle) {
        sqlite3_stmt *s = NULL;
        if (sqlite3_prepare_v2(ctx->db->handle, "SELECT 1;",
                               -1, &s, NULL) == SQLITE_OK) {
            db_ok = (sqlite3_step(s) == SQLITE_ROW);
            sqlite3_finalize(s);
        }
    }
    if (!db_ok) return json_error(503, "db_unreachable");
    snprintf(buf, sizeof(buf),
             "{\"status\":\"ok\",\"db\":\"ok\",\"ts\":%lld}",
             (long long)time(NULL));
    return json_ok(buf);
}

HttpResp handle_metrics(HttpReq req, Ctx *ctx)
{
    HttpResp resp;
    int i, off;
    (void)req; (void)ctx;
    memset(&resp, 0, sizeof(resp));
    resp.status = 200;
    snprintf(resp.content_type, sizeof(resp.content_type), "text/plain");
    off = 0;
    for (i = 0; i < mb_count; i++) {
        int w = snprintf(resp.body + off, sizeof(resp.body) - (size_t)off,
            "teb_requests_total{method=\"%s\",path=\"%s\",status=\"%d\"} %ld\n",
            mb[i].method, mb[i].path, mb[i].status, mb[i].count);
        if (w > 0 && (size_t)(off + w) < sizeof(resp.body))
            off += w;
    }
    if (mb_overflow > 0) {
        int w = snprintf(resp.body + off, sizeof(resp.body) - (size_t)off,
            "teb_routes_dropped_total %ld\n", mb_overflow);
        if (w > 0 && (size_t)(off + w) < sizeof(resp.body))
            off += w;
    }
    {
        int w = snprintf(resp.body + off, sizeof(resp.body) - (size_t)off,
            "teb2_llm_calls_total %ld\n"
            "teb2_llm_tokens_total %ld\n"
            "teb2_llm_latency_ms_sum %lld\n",
            llm_calls_total, llm_tokens_total, llm_latency_sum_ms);
        if (w > 0 && (size_t)(off + w) < sizeof(resp.body))
            off += w;
    }
    resp.body_len = (size_t)off;
    return resp;
}
