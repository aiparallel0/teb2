#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "exec/exec.h"
#include "db/db.h"

static const char *token_url(const char *p)
{
    if (strcmp(p, "google") == 0) return "http://accounts.google.com/o/oauth2/token";
    if (strcmp(p, "github") == 0) return "http://github.com/login/oauth/access_token";
    if (strcmp(p, "slack") == 0)  return "http://slack.com/api/oauth.v2.access";
    return "";
}

static void url_encode(const char *in, char *out, size_t n)
{
    size_t o = 0;
    while (*in && o + 3 < n) {
        unsigned char c = (unsigned char)*in;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out[o++] = (char)c;
        } else if (c == ' ') {
            out[o++] = '+';
        } else {
            int w = snprintf(out + o, n - o, "%%%02X", c);
            if (w > 0) o += (size_t)w;
        }
        in++;
    }
    out[o] = '\0';
}

static void uppercase(const char *in, char *out, size_t n)
{
    size_t i;
    for (i = 0; i < n - 1 && in[i]; i++)
        out[i] = (in[i] >= 'a' && in[i] <= 'z') ? (char)(in[i] - 32) : in[i];
    out[i] = '\0';
}

OAuthResult oauth_exchange(const char *provider, const char *code,
    const char *redirect_uri, Config *cfg, Db *db, const char *user_id)
{
    OAuthResult r;
    char env_id[64], env_sec[64], up[32];
    const char *cid, *csec;
    HttpReq hr;
    HttpResult resp;
    OAuthQuery oq;
    char enc_code[256], enc_uri[256];

    memset(&r, 0, sizeof(r));
    (void)cfg;
    uppercase(provider, up, sizeof(up));
    snprintf(env_id,  sizeof(env_id),  "%s_CLIENT_ID", up);
    snprintf(env_sec, sizeof(env_sec), "%s_CLIENT_SECRET", up);
    cid  = getenv(env_id);
    csec = getenv(env_sec);
    if (!cid || !csec) { r.err = ERR_AUTH; return r; }
    url_encode(code, enc_code, sizeof(enc_code));
    url_encode(redirect_uri, enc_uri, sizeof(enc_uri));
    memset(&hr, 0, sizeof(hr));
    snprintf(hr.method, sizeof(hr.method), "POST");
    snprintf(hr.path, sizeof(hr.path), "%s", token_url(provider));
    hr.body_len = (size_t)snprintf(hr.body, sizeof(hr.body),
        "grant_type=authorization_code&code=%s&redirect_uri=%s"
        "&client_id=%s&client_secret=%s",
        enc_code, enc_uri, cid, csec);
    resp = send_request(hr, NULL);
    if (resp.err != ERR_OK || resp.status < 200 || resp.status >= 400) {
        r.err = ERR_IO; return r;
    }
    memset(&oq, 0, sizeof(oq));
    snprintf(oq.user_id, sizeof(oq.user_id), "%s", user_id);
    snprintf(oq.provider, sizeof(oq.provider), "%s", provider);
    {
        const char *at, *rt;
        char a[256], rv[256];
        memset(a, 0, sizeof(a)); memset(rv, 0, sizeof(rv));
        at = strstr(resp.body, "access_token");
        if (at) {
            at = strchr(at, ':');
            if (at) { at++; while (*at == ' ' || *at == '"') at++;
                snprintf(a, sizeof(a), "%.*s", (int)strcspn(at, "\""), at); }
        }
        rt = strstr(resp.body, "refresh_token");
        if (rt) {
            rt = strchr(rt, ':');
            if (rt) { rt++; while (*rt == ' ' || *rt == '"') rt++;
                snprintf(rv, sizeof(rv), "%.*s", (int)strcspn(rt, "\""), rt); }
        }
        snprintf(oq.access_tok, sizeof(oq.access_tok), "%s", a);
        snprintf(oq.refresh_tok, sizeof(oq.refresh_tok), "%s", rv);
    }
    r = store_oauth(db, oq);
    return r;
}

OAuthResult oauth_refresh(const char *provider, const char *refresh_tok,
    Config *cfg, Db *db, const char *user_id)
{
    OAuthResult r;
    char env_id[64], env_sec[64], up[32];
    const char *cid, *csec;
    HttpReq hr;
    HttpResult resp;
    OAuthQuery oq;

    memset(&r, 0, sizeof(r));
    (void)cfg;
    uppercase(provider, up, sizeof(up));
    snprintf(env_id,  sizeof(env_id),  "%s_CLIENT_ID", up);
    snprintf(env_sec, sizeof(env_sec), "%s_CLIENT_SECRET", up);
    cid  = getenv(env_id);
    csec = getenv(env_sec);
    if (!cid || !csec) { r.err = ERR_AUTH; return r; }
    memset(&hr, 0, sizeof(hr));
    snprintf(hr.method, sizeof(hr.method), "POST");
    snprintf(hr.path, sizeof(hr.path), "%s", token_url(provider));
    hr.body_len = (size_t)snprintf(hr.body, sizeof(hr.body),
        "grant_type=refresh_token&refresh_token=%s"
        "&client_id=%s&client_secret=%s",
        refresh_tok, cid, csec);
    resp = send_request(hr, NULL);
    if (resp.err != ERR_OK || resp.status < 200 || resp.status >= 400) {
        r.err = ERR_IO; return r;
    }
    memset(&oq, 0, sizeof(oq));
    snprintf(oq.user_id, sizeof(oq.user_id), "%s", user_id);
    snprintf(oq.provider, sizeof(oq.provider), "%s", provider);
    {
        const char *at;
        char a[256];
        memset(a, 0, sizeof(a));
        at = strstr(resp.body, "access_token");
        if (at) {
            at = strchr(at, ':');
            if (at) { at++; while (*at == ' ' || *at == '"') at++;
                snprintf(a, sizeof(a), "%.*s", (int)strcspn(at, "\""), at); }
        }
        snprintf(oq.access_tok, sizeof(oq.access_tok), "%s", a);
        snprintf(oq.refresh_tok, sizeof(oq.refresh_tok), "%s", refresh_tok);
    }
    r = store_oauth(db, oq);
    return r;
}
