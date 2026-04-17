#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/prompts.h"
#include "exec/exec.h"

/* Oversized buffers so persona + guardrails + a phase prompt (up to
 * ~6 KB each) can all be composed into a single system message
 * without truncation. main.c pre-forks workers, each worker is
 * single-threaded, so static storage is safe. */
#define REQ_CAP  49152u
#define SYS_CAP  24576u
#define USR_CAP   8192u
#define BODY_CAP 40960u

static void esc_json(const char *src, char *dst, size_t dsz)
{
    size_t i = 0, o = 0;
    if (!src || !dst || dsz == 0) { if (dst && dsz) dst[0] = '\0'; return; }
    while (src[i] && o + 2 < dsz) {
        unsigned char c = (unsigned char)src[i];
        if (c == '\n')      { dst[o++] = '\\'; dst[o++] = 'n'; }
        else if (c == '\r') { dst[o++] = '\\'; dst[o++] = 'r'; }
        else if (c == '\t') { dst[o++] = '\\'; dst[o++] = 't'; }
        else if (c == '"' || c == '\\') { dst[o++] = '\\'; dst[o++] = (char)c; }
        else if (c < 0x20) { i++; continue; }
        else dst[o++] = (char)c;
        i++;
    }
    dst[o] = '\0';
}

/* Extract the first top-level "content":"..." value. */
static void parse_content(const char *body, char *out, size_t osz)
{
    const char *p = strstr(body, "\"content\"");
    size_t i = 0;
    if (!p) { if (osz) out[0] = '\0'; return; }
    p += 9;
    while (*p == ' ' || *p == ':') p++;
    if (*p != '"') { if (osz) out[0] = '\0'; return; }
    p++;
    while (i + 1 < osz && *p && *p != '"') {
        if (*p == '\\' && *(p + 1)) {
            p++;
            if (*p == 'n')      { out[i++] = '\n'; p++; continue; }
            else if (*p == 'r') { out[i++] = '\r'; p++; continue; }
            else if (*p == 't') { out[i++] = '\t'; p++; continue; }
            else if (*p == '"' || *p == '\\') { out[i++] = *p++; continue; }
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
}

static int parse_total_tokens(const char *body)
{
    const char *p = strstr(body, "\"total_tokens\"");
    if (!p) return 0;
    p += 14;
    while (*p == ' ' || *p == ':') p++;
    return atoi(p);
}

/* Compose persona + guardrails + named prompt + optional context. */
static void build_system(const LlmReq *req, char *out, size_t osz)
{
    const char *pe = prompt_get("system/persona");
    const char *gu = prompt_get("system/guardrails");
    const char *ph = req->prompt_name[0] ? prompt_get(req->prompt_name) : "";
    int n = snprintf(out, osz,
        "%s\n---\n%s\n---\n%s\n%s%s",
        pe ? pe : "", gu ? gu : "", ph ? ph : "",
        req->context[0] ? "---\n" : "",
        req->context[0] ? req->context : "");
    if (n < 0 || (size_t)n >= osz) out[osz - 1] = '\0';
}

LlmReply llm_call(LlmReq req, Config *cfg)
{
    static char sysraw[SYS_CAP], esys[SYS_CAP * 2];
    static char eusr[USR_CAP * 2], body[BODY_CAP], reqbuf[REQ_CAP];
    static char resp[REQ_CAP];
    LlmReply r;
    const char *model;
    ssize_t n;
    int hlen;
    const char *bodyp;

    memset(&r, 0, sizeof(r));
    if (!cfg || !cfg->openai_key[0]) { r.err = ERR_IO; return r; }

    build_system(&req, sysraw, sizeof(sysraw));
    esc_json(sysraw,   esys, sizeof(esys));
    esc_json(req.user, eusr, sizeof(eusr));
    model = req.model[0] ? req.model : cfg->openai_model;

    hlen = snprintf(body, sizeof(body),
        "{\"model\":\"%s\","
        "\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}]"
        "%s,\"max_tokens\":1024}",
        model, esys, eusr,
        req.want_json ? ",\"response_format\":{\"type\":\"json_object\"}" : "");
    if (hlen <= 0 || (size_t)hlen >= sizeof(body)) { r.err = ERR_LIMIT; return r; }

    hlen = snprintf(reqbuf, sizeof(reqbuf),
        "POST /v1/chat/completions HTTP/1.0\r\n"
        "Host: api.openai.com\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n\r\n%s",
        cfg->openai_key, (int)strlen(body), body);
    if (hlen <= 0 || (size_t)hlen >= sizeof(reqbuf)) { r.err = ERR_LIMIT; return r; }

    n = tls_request("api.openai.com", 443, reqbuf, (size_t)hlen,
                    resp, sizeof(resp) - 1);
    if (n < 0) { r.err = ERR_IO; return r; }
    resp[n] = '\0';

    r.status = (n > 9) ? atoi(resp + 9) : 0;
    bodyp = strstr(resp, "\r\n\r\n");
    bodyp = bodyp ? bodyp + 4 : resp;
    if (r.status < 200 || r.status >= 300) { r.err = ERR_IO; return r; }

    parse_content(bodyp, r.reply, sizeof(r.reply));
    r.total_tokens = parse_total_tokens(bodyp);
    r.err = ERR_OK;
    return r;
}

