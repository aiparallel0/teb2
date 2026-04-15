#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "api/api.h"
#include "api/json.h"

#define METRIC_SLOTS 128

typedef struct { char key[64]; int count; } MetricEntry;
static MetricEntry mtable[METRIC_SLOTS];

static unsigned int hash_key(const char *s)
{
    unsigned int h = 5381;
    while (*s) h = ((h << 5) + h) + (unsigned char)*s++;
    return h;
}

void metrics_inc(const char *method, const char *path, int status)
{
    char key[64];
    unsigned int idx;
    snprintf(key, sizeof(key), "%s %s %d", method, path, status);
    idx = hash_key(key) % METRIC_SLOTS;
    if (mtable[idx].count == 0 || strcmp(mtable[idx].key, key) == 0) {
        snprintf(mtable[idx].key, sizeof(mtable[idx].key), "%s", key);
        mtable[idx].count++;
    }
}

HttpResp handle_healthz(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return json_ok("{\"status\":\"ok\"}");
}

HttpResp handle_metrics(HttpReq req, Ctx *ctx)
{
    char buf[4096];
    int i, off;
    (void)req; (void)ctx;
    off = snprintf(buf, sizeof(buf), "{\"counters\":[");
    for (i = 0; i < METRIC_SLOTS; i++) {
        if (mtable[i].count == 0) continue;
        if (off > 14 && off < (int)sizeof(buf) - 1) buf[off++] = ',';
        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
            "{\"key\":\"%s\",\"count\":%d}",
            mtable[i].key, mtable[i].count);
        if (off < 0 || (size_t)off >= sizeof(buf) - 2) break;
    }
    if (off > 0 && (size_t)off < sizeof(buf) - 3)
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "]}");
    (void)off;
    return json_ok(buf);
}
