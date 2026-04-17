#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/util.h"

/* Locate the byte after a "key" token in obj. Returns NULL if absent. */
static const char *find_key(const char *obj, const char *key)
{
    char pat[80];
    int n;
    if (!obj || !key) return NULL;
    n = snprintf(pat, sizeof(pat), "\"%s\"", key);
    if (n <= 0 || (size_t)n >= sizeof(pat)) return NULL;
    return strstr(obj, pat);
}

int json_str(const char *obj, const char *key, char *out, size_t osz)
{
    const char *p, *e;
    size_t len;
    if (!out || osz == 0) return 0;
    out[0] = '\0';
    p = find_key(obj, key);
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    while (*p && *p != '"' && *p != ',' && *p != '}') p++;
    if (*p != '"') return 0;
    p++;
    e = p;
    while (*e && !(*e == '"' && (e == p || *(e - 1) != '\\'))) e++;
    if (!*e) return 0;
    len = (size_t)(e - p);
    if (len >= osz) len = osz - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return 1;
}

int64_t json_int(const char *obj, const char *key)
{
    const char *p;
    p = find_key(obj, key);
    if (!p) return INT64_MIN;
    p = strchr(p, ':');
    if (!p) return INT64_MIN;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '-' && (*p < '0' || *p > '9')) return INT64_MIN;
    return (int64_t)strtoll(p, NULL, 10);
}

int json_bool(const char *obj, const char *key)
{
    const char *p;
    p = find_key(obj, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "true",  4) == 0) return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    return -1;
}

AgentMsg agent_make_result(AgentMsg src, Err err, const char *detail)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = src.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", src.user_id);
    snprintf(out.payload, sizeof(out.payload), "%s", detail);
    out.err = err;
    return out;
}

int payload_has_content(const char *payload)
{
    size_t i;
    for (i = 0; payload[i] != '\0'; i++) {
        if (payload[i] != ' ' && payload[i] != '\t' &&
            payload[i] != '\n')
            return 1;
    }
    return 0;
}
