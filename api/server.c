#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "core/types.h"
#include "core/ratelimit.h"
#include "api/api.h"
#include "api/json.h"

HttpReq parse_request(const char *raw, size_t len)
{
    HttpReq req;
    const char *end = raw + len;
    const char *p   = raw;
    const char *sp1, *sp2, *body, *auth, *xff;

    memset(&req, 0, sizeof(req));
    sp1 = memchr(p, ' ', (size_t)(end - p));
    if (!sp1) return req;
    snprintf(req.method, sizeof(req.method), "%.*s",
             (int)(sp1 - p), p);
    p = sp1 + 1;
    sp2 = memchr(p, ' ', (size_t)(end - p));
    if (!sp2) return req;
    snprintf(req.path, sizeof(req.path), "%.*s",
             (int)(sp2 - p), p);
    {
        char *qm = strchr(req.path, '?');
        if (qm) {
            const char *tk = strstr(qm, "token=");
            if (tk && !req.auth_header[0])
                snprintf(req.auth_header, sizeof(req.auth_header),
                         "Bearer %s", tk + 6);
            snprintf(req.query, sizeof(req.query), "%s", qm + 1);
            *qm = '\0';
        }
    }
    auth = strstr(raw, "\nAuthorization: ");
    if (auth) {
        auth += 16;
        snprintf(req.auth_header, sizeof(req.auth_header),
                 "%.*s", (int)(strcspn(auth, "\r\n")), auth);
    }
    xff = strstr(raw, "\nX-Forwarded-For: ");
    if (xff) {
        xff += 18;
        snprintf(req.fwd_for, sizeof(req.fwd_for),
                 "%.*s", (int)(strcspn(xff, "\r\n,")), xff);
    }
    body = strstr(raw, "\r\n\r\n");
    if (body) {
        body += 4;
        req.body_len = (size_t)(end - body);
        if (req.body_len >= sizeof(req.body))
            req.body_len = sizeof(req.body) - 1;
        memcpy(req.body, body, req.body_len);
    }
    return req;
}

#define CORS_HDRS \
    "Access-Control-Allow-Origin: *\r\n" \
    "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n" \
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"

void write_response(int fd, HttpResp resp)
{
    char    hdr[512];
    int     hlen;
    ssize_t nw;

    hlen = snprintf(hdr, sizeof(hdr),
                    "HTTP/1.1 %d OK\r\nContent-Type: %s\r\n"
                    CORS_HDRS
                    "Content-Length: %zu\r\n\r\n",
                    resp.status, resp.content_type, resp.body_len);
    nw = write(fd, hdr, (size_t)hlen);
    if (nw < 0) return;
    if (resp.body_len > 0) {
        nw = write(fd, resp.body, resp.body_len);
        (void)nw;
    }
}

static HttpResp options_resp(void)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 204;
    r.body_len = 0;
    snprintf(r.content_type, sizeof(r.content_type), "%s",
             "application/json");
    return r;
}

static HttpResp dispatch_internal(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;
    const char *rlkey;
    if (strcmp(req.method, "OPTIONS") == 0) return options_resp();
    if (strcmp(req.method, "GET") == 0) {
        if (strcmp(p, "/") == 0) return handle_ui_index(req, ctx);
        if (strcmp(p, "/app.js") == 0) return handle_ui_appjs(req, ctx);
        if (strcmp(p, "/style.css") == 0) return handle_ui_style(req, ctx);
        if (strcmp(p, "/goals.js") == 0) return handle_ui_goalsjs(req, ctx);
        if (strcmp(p, "/tasks.js") == 0) return handle_ui_tasksjs(req, ctx);
        if (strcmp(p, "/finance.js") == 0) return handle_ui_financejs(req, ctx);
        if (strcmp(p, "/collab.js") == 0) return handle_ui_collabjs(req, ctx);
        if (strcmp(p, "/dash.js") == 0) return handle_ui_dashjs(req, ctx);
        if (strcmp(p, "/enterprise.js") == 0) return handle_ui_enterprisejs(req, ctx);
        if (strcmp(p, "/analytics.js") == 0) return handle_ui_analyticsjs(req, ctx);
        if (strcmp(p, "/healthz") == 0) return handle_healthz(req, ctx);
        if (strcmp(p, "/metrics") == 0) return handle_metrics(req, ctx);
    }
    rlkey = req.fwd_for[0] ? req.fwd_for : "unknown";
    if (strcmp(p, "/auth/register") == 0 || strcmp(p, "/auth/login") == 0) {
        if (!rl_check(rlkey, 10))
            return json_error(429, "rate_limited");
    } else {
        if (!rl_check(rlkey, 120))
            return json_error(429, "rate_limited");
    }
    if (strncmp(p, "/goals", 6) == 0) {
        if (strcmp(req.method, "POST") == 0)
            return handle_goal_create(req, ctx);
        if (strcmp(req.method, "GET") == 0)
            return handle_goal_list(req, ctx);
    }
    if (strncmp(p, "/goal/", 6) == 0) {
        if (strcmp(req.method, "GET") == 0)
            return handle_goal_get(req, ctx);
        if (strcmp(req.method, "POST") == 0)
            return handle_goal_decompose(req, ctx);
    }
    if (strncmp(p, "/tasks/goal/", 12) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_task_list(req, ctx);
    if (strcmp(p, "/tasks") == 0 && strcmp(req.method, "POST") == 0)
        return handle_task_create(req, ctx);
    if (strncmp(p, "/tasks/", 7) == 0) {
        if (strcmp(req.method, "PUT") == 0)
            return handle_task_update(req, ctx);
        if (strcmp(req.method, "POST") == 0)
            return handle_task_execute(req, ctx);
        if (strcmp(req.method, "GET") == 0)
            return handle_task_status(req, ctx);
    }
    if (strcmp(p, "/auth/register") == 0) return handle_register(req, ctx);
    if (strcmp(p, "/auth/login") == 0)    return handle_login(req, ctx);
    if (strcmp(p, "/auth/refresh") == 0)  return handle_refresh(req, ctx);
    if (strncmp(p, "/sse/chat/", 10) == 0 && strcmp(req.method, "GET") == 0)
        return handle_sse_subscribe(req, ctx);
    return dispatch_ext(req, ctx);
}

HttpResp dispatch(HttpReq req, Ctx *ctx)
{
    HttpResp resp = dispatch_internal(req, ctx);
    metrics_inc(req.method, req.path, resp.status);
    return resp;
}
