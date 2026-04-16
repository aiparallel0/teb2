#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "api/json.h"

HttpResp json_error(int status, const char *msg)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = status;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                  "{\"error\":\"%s\"}", msg);
    snprintf(r.content_type, sizeof(r.content_type), "%s",
             "application/json");
    return r;
}

HttpResp json_ok(const char *body)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 200;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body), "%s", body);
    snprintf(r.content_type, sizeof(r.content_type), "%s",
             "application/json");
    return r;
}

int extract_json_str(const char *body, const char *key,
                     char *out, size_t outsz)
{
    const char *k = strstr(body, key);
    const char *v;
    size_t i;
    if (!k) return 0;
    k += strlen(key);
    while (*k == ' ' || *k == ':' || *k == '"') k++;
    v = k;
    for (i = 0; i < outsz - 1 && v[i] != '\0' && v[i] != '"'; i++)
        out[i] = v[i];
    out[i] = '\0';
    return i > 0 ? 1 : 0;
}

int64_t extract_cursor(const char *query)
{
    const char *p;

    if (!query || !*query) return 0;
    if (strncmp(query, "cursor=", 7) == 0)
        return strtoll(query + 7, NULL, 10);
    p = strstr(query, "&cursor=");
    if (!p) return 0;
    return strtoll(p + 8, NULL, 10);
}
