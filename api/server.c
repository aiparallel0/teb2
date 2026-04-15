#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "core/types.h"
#include "api/api.h"

HttpReq parse_request(const char *raw, size_t len)
{
    HttpReq req;
    const char *end = raw + len;
    const char *p   = raw;
    const char *sp1, *sp2, *body, *auth;

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
    auth = strstr(raw, "\nAuthorization: ");
    if (auth) {
        auth += 16;
        snprintf(req.auth_header, sizeof(req.auth_header),
                 "%.*s", (int)(strcspn(auth, "\r\n")), auth);
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

HttpResp dispatch(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;
    if (strcmp(req.method, "OPTIONS") == 0) return options_resp();
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
    if (strcmp(p, "/auth/register") == 0)
        return handle_register(req, ctx);
    if (strcmp(p, "/auth/login") == 0)
        return handle_login(req, ctx);
    if (strcmp(p, "/auth/refresh") == 0)
        return handle_refresh(req, ctx);
    if (strcmp(p, "/outcomes") == 0 && strcmp(req.method, "POST") == 0)
        return handle_outcome_store(req, ctx);
    if (strncmp(p, "/outcome/", 9) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_outcome_get(req, ctx);
    if (strcmp(p, "/nudges") == 0 && strcmp(req.method, "POST") == 0)
        return handle_nudge_store(req, ctx);
    if (strncmp(p, "/nudge/", 7) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_nudge_get(req, ctx);
    if (strcmp(p, "/learnings") == 0
        && strcmp(req.method, "POST") == 0)
        return handle_learn_store(req, ctx);
    if (strncmp(p, "/learning/", 10) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_learn_get(req, ctx);
    if (strncmp(p, "/exec/", 6) == 0
        && strcmp(req.method, "POST") == 0)
        return handle_exec_run(req, ctx);
    if (strncmp(p, "/decompose/", 11) == 0
        && strcmp(req.method, "POST") == 0)
        return handle_decompose_run(req, ctx);
    if (strcmp(p, "/schedules") == 0
        && strcmp(req.method, "POST") == 0)
        return handle_sched_create(req, ctx);
    if (strncmp(p, "/schedules/", 11) == 0
        && strcmp(req.method, "GET") == 0)
        return handle_sched_list(req, ctx);
    if (strcmp(p, "/budgets") == 0
        && strcmp(req.method, "POST") == 0)
        return handle_budget_create(req, ctx);
    if (strcmp(p, "/budgets") == 0
        && strcmp(req.method, "GET") == 0)
        return handle_budget_get(req, ctx);
    if (strcmp(p, "/spending") == 0
        && strcmp(req.method, "POST") == 0)
        return handle_spend_record(req, ctx);
    return dispatch_ext(req, ctx);
}
