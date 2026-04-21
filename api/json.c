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

/* Extract a JSON string value. Decodes the common backslash escapes
 * (\", \\, \/, \n, \r, \t, \b, \f) so a password containing a quote or
 * backslash round-trips correctly through /auth/register + /auth/login
 * instead of being silently truncated at the first literal '"' byte.
 * \uXXXX is not decoded; it passes through as-is, which is fine for
 * ASCII emails and passwords. */
int extract_json_str(const char *body, const char *key,
                     char *out, size_t outsz)
{
    const char *k = strstr(body, key);
    size_t i = 0;
    if (!k || outsz == 0) return 0;
    k += strlen(key);
    while (*k == ' ' || *k == ':' || *k == '"') k++;
    while (*k && *k != '"' && i + 1 < outsz) {
        if (*k == '\\' && k[1]) {
            char c = k[1], d = c;
            if (c == 'n') d = '\n'; else if (c == 'r') d = '\r';
            else if (c == 't') d = '\t'; else if (c == 'b') d = '\b';
            else if (c == 'f') d = '\f';
            out[i++] = d; k += 2; continue;
        }
        out[i++] = *k++;
    }
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
