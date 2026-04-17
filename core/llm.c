#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/prompts.h"
#include "core/log.h"
#include "exec/exec.h"
#include "db/db.h"
#include "api/api.h"

/* Static buffers — safe because each worker is single-threaded. */
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
/* Jittered backoff: 1s, 2s, 4s with ±25% jitter */
static void backoff_sleep(int attempt)
{
    struct timespec ts;
    unsigned int base = (1u << (unsigned)attempt);
    unsigned int jitter_ms = (unsigned int)(rand() % ((int)base * 250 + 1));
    ts.tv_sec = (time_t)base;
    ts.tv_nsec = (long)jitter_ms * 1000000L;
    nanosleep(&ts, NULL);
}

LlmReply llm_call(LlmReq req, Config *cfg)
{
    static char sysraw[SYS_CAP], esys[SYS_CAP * 2];
    static char eusr[USR_CAP * 2], body[BODY_CAP], reqbuf[REQ_CAP];
    static char resp[REQ_CAP];
    LlmReply r;
    const char *model, *host;
    int hlen, max_tok, retries, attempt;

    memset(&r, 0, sizeof(r));
    if (!cfg || !cfg->openai_key[0]) { r.err = ERR_IO; return r; }
    build_system(&req, sysraw, sizeof(sysraw));
    esc_json(sysraw, esys, sizeof(esys));
    esc_json(req.user, eusr, sizeof(eusr));
    model   = req.model[0] ? req.model : cfg->openai_model;
    host    = cfg->llm_base_url[0] ? cfg->llm_base_url : "api.openai.com";
    max_tok = req.max_tokens > 0 ? req.max_tokens : cfg->llm_max_tokens;
    if (max_tok <= 0) max_tok = 1024;
    retries = cfg->llm_retries > 0 ? cfg->llm_retries : 3;

    hlen = snprintf(body, sizeof(body),
        "{\"model\":\"%s\","
        "\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}]"
        "%s,\"max_tokens\":%d}",
        model, esys, eusr,
        req.want_json ? ",\"response_format\":{\"type\":\"json_object\"}" : "",
        max_tok);
    if (hlen <= 0 || (size_t)hlen >= sizeof(body)) { r.err = ERR_LIMIT; return r; }

    hlen = snprintf(reqbuf, sizeof(reqbuf),
        "POST /v1/chat/completions HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Authorization: Bearer %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n\r\n%s",
        host, cfg->openai_key, (int)strlen(body), body);
    if (hlen <= 0 || (size_t)hlen >= sizeof(reqbuf)) { r.err = ERR_LIMIT; return r; }

    for (attempt = 0; attempt <= retries; attempt++) {
        ssize_t n;
        const char *bodyp;
        struct timespec t0, t1;
        long latency_ms;
        if (attempt > 0) {
            teb_log_warn("llm", "retry %d/%d for %s",
                         attempt, retries, req.prompt_name);
            backoff_sleep(attempt - 1);
        }
        clock_gettime(CLOCK_MONOTONIC, &t0);
        n = tls_request(host, 443, reqbuf, (size_t)hlen,
                        resp, sizeof(resp) - 1);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        latency_ms = (long)(t1.tv_sec - t0.tv_sec) * 1000L
                     + (long)(t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (latency_ms < 0) latency_ms = 0;
        if (n < 0) { r.err = ERR_IO; continue; }
        resp[n] = '\0';
        r.status = (n > 9) ? atoi(resp + 9) : 0;
        bodyp = strstr(resp, "\r\n\r\n");
        bodyp = bodyp ? bodyp + 4 : resp;
        if (r.status == 429 || r.status >= 500) {
            teb_log_warn("llm", "status %d from %s", r.status, host);
            r.err = ERR_IO; continue;
        }
        if (r.status < 200 || r.status >= 300) { r.err = ERR_IO; return r; }
        parse_content(bodyp, r.reply, sizeof(r.reply));
        r.total_tokens = parse_total_tokens(bodyp);
        r.err = ERR_OK;
        metrics_observe_llm((long)r.total_tokens, latency_ms);
        teb_log_info("llm", "prompt=%s tokens=%d latency_ms=%ld",
                     req.prompt_name, r.total_tokens, latency_ms);
        return r;
    }
    teb_log_error("llm", "exhausted %d retries for %s",
                  retries, req.prompt_name);
    return r;
}
