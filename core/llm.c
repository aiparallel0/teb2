#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "exec/exec.h"

/* Minimal JSON string escaping — handles backslash and double-quote */
static void esc_json(const char *src, char *dst, size_t dsz)
{
    size_t i = 0, o = 0;
    while (src[i] && o + 2 < dsz) {
        if (src[i] == '"' || src[i] == '\\') {
            if (o + 3 >= dsz) break;
            dst[o++] = '\\';
        } else if (src[i] == '\n') {
            if (o + 3 >= dsz) break;
            dst[o++] = '\\'; dst[o++] = 'n'; i++; continue;
        } else if (src[i] == '\r') {
            if (o + 3 >= dsz) break;
            dst[o++] = '\\'; dst[o++] = 'r'; i++; continue;
        }
        dst[o++] = src[i++];
    }
    dst[o] = '\0';
}

/* Extract the first "content":"..." value from an OpenAI JSON response */
static LlmReply parse_openai(const char *body)
{
    LlmReply r;
    const char *p;
    size_t i;

    memset(&r, 0, sizeof(r));
    p = strstr(body, "\"content\"");
    if (!p) { r.err = ERR_IO; return r; }
    p += 9;
    while (*p == ' ' || *p == ':') p++;
    if (*p != '"') { r.err = ERR_IO; return r; }
    p++; /* skip opening quote */
    for (i = 0; i < sizeof(r.reply) - 1 && *p && *p != '"'; i++) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            if (*p == 'n')      { r.reply[i] = '\n'; p++; continue; }
            else if (*p == 'r') { r.reply[i] = '\r'; p++; continue; }
        }
        r.reply[i] = *p++;
    }
    r.reply[i] = '\0';
    r.err = ERR_OK;
    return r;
}

LlmReply llm_call(LlmReq req, Config *cfg)
{
    LlmReply r;
    HttpReq  hreq;
    HttpResult hres;
    Cred cred;
    char esys[1024], eusr[2048];
    int n;

    memset(&r,    0, sizeof(r));
    memset(&hreq, 0, sizeof(hreq));
    memset(&cred, 0, sizeof(cred));

    if (!cfg || !cfg->openai_key[0]) { r.err = ERR_IO; return r; }

    esc_json(req.system, esys, sizeof(esys));
    esc_json(req.user,   eusr, sizeof(eusr));

    snprintf(hreq.method, sizeof(hreq.method), "POST");
    snprintf(hreq.path,   sizeof(hreq.path),
             "https://api.openai.com/v1/chat/completions");
    snprintf(hreq.auth_header, sizeof(hreq.auth_header),
             "Bearer %s", cfg->openai_key);

    n = snprintf(hreq.body, sizeof(hreq.body),
        "{\"model\":\"%s\","
        "\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}],"
        "\"max_tokens\":512}",
        req.model[0] ? req.model : cfg->openai_model,
        esys, eusr);
    hreq.body_len = (n > 0 && (size_t)n < sizeof(hreq.body))
                    ? (size_t)n : sizeof(hreq.body) - 1;

    hres = send_request(hreq, &cred);
    if (hres.err != ERR_OK || hres.status < 200 || hres.status >= 300) {
        r.err = ERR_IO;
        return r;
    }
    return parse_openai(hres.body);
}
