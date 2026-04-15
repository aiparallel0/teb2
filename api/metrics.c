#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "core/types.h"
#include "api/api.h"
#include "api/json.h"

#define METRIC_SLOTS 16

typedef struct { char method[8]; char path[32]; int status; long count; } MetricBucket;
static MetricBucket mb[METRIC_SLOTS];
static int mb_count;

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
    }
}

HttpResp handle_healthz(HttpReq req, Ctx *ctx)
{
    char buf[128];
    (void)req; (void)ctx;
    snprintf(buf, sizeof(buf), "{\"status\":\"ok\",\"ts\":%lld}",
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
    resp.body_len = (size_t)off;
    return resp;
}
