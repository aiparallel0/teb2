#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "core/types.h"
#include "api/api.h"

static HttpResp serve_file(const char *path, const char *ctype)
{
    HttpResp resp;
    int fd;
    ssize_t nr;

    memset(&resp, 0, sizeof(resp));
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        resp.status = 404;
        snprintf(resp.content_type, sizeof(resp.content_type),
                 "text/plain");
        snprintf(resp.body, sizeof(resp.body), "not found");
        resp.body_len = 9;
        return resp;
    }
    nr = read(fd, resp.body, sizeof(resp.body) - 1);
    close(fd);
    if (nr < 0) nr = 0;
    resp.body[nr] = '\0';
    resp.body_len = (size_t)nr;
    resp.status = 200;
    snprintf(resp.content_type, sizeof(resp.content_type), "%s", ctype);
    return resp;
}

HttpResp handle_ui_index(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/index.html", "text/html");
}

HttpResp handle_ui_appjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/app.js", "application/javascript");
}

HttpResp handle_ui_style(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/style.css", "text/css");
}

HttpResp handle_ui_goalsjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/goals.js", "application/javascript");
}

HttpResp handle_ui_tasksjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/tasks.js", "application/javascript");
}

HttpResp handle_ui_financejs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/finance.js", "application/javascript");
}

HttpResp handle_ui_collabjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/collab.js", "application/javascript");
}

HttpResp handle_ui_dashjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/dash.js", "application/javascript");
}

HttpResp handle_ui_enterprisejs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/enterprise.js", "application/javascript");
}

HttpResp handle_ui_analyticsjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/analytics.js", "application/javascript");
}

HttpResp handle_ui_approvalsjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/approvals.js", "application/javascript");
}

HttpResp handle_ui_promptsjs(HttpReq req, Ctx *ctx)
{
    (void)req; (void)ctx;
    return serve_file("ui/prompts.js", "application/javascript");
}
