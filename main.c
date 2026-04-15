#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/config.h"
#include "auth/auth.h"
#include "db/db.h"
#include "exec/exec.h"
#include "agents/channel.h"
#include "api/api.h"

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static HttpResp not_found(void)
{
    HttpResp r;
    memset(&r, 0, sizeof(r));
    r.status   = 404;
    r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                  "{\"error\":\"not_found\"}");
    snprintf(r.content_type, sizeof(r.content_type), "%s",
             "application/json");
    return r;
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

static HttpResp dispatch(HttpReq req, Ctx *ctx)
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
    return not_found();
}

int main(int argc, char **argv)
{
    Config cfg;
    Db     db;
    union {
        struct sockaddr     base;
        struct sockaddr_in  in4;
    } sa;
    int    srv, conn, optval;
    char   buf[8192];
    HttpReq  req;
    HttpResp resp;

    cfg = load_config(argc > 1 ? argv[1] : ".env");
    if (db_open(cfg.db_path, &db) != ERR_OK) {
        fprintf(stderr, "db_open failed\n");
        return 1;
    }
    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { db_close(&db); return 1; }
    optval = 1;
    (void)setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                     &optval, sizeof(optval));
    memset(&sa, 0, sizeof(sa));
    sa.in4.sin_family      = AF_INET;
    sa.in4.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.in4.sin_port        = htons((uint16_t)cfg.port);
    if (bind(srv, &sa.base, sizeof(sa.in4)) != 0) {
        close(srv); db_close(&db); return 1;
    }
    if (listen(srv, 16) != 0) {
        close(srv); db_close(&db); return 1;
    }

    while (g_running) {
        ssize_t n;
        Ctx ctx;
        UserClaims uc;
        TokenResult ar;
        conn = accept(srv, NULL, NULL);
        if (conn < 0) continue;
        n = read(conn, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            req = parse_request(buf, (size_t)n);
            memset(&ctx, 0, sizeof(ctx));
            ctx.db  = &db;
            ctx.cfg = &cfg;
            ar = authenticate_request(req, cfg.secret);
            if (ar.err == ERR_OK) {
                uc = ar.claims;
                ctx.user = &uc;
            }
            resp = dispatch(req, &ctx);
            write_response(conn, resp);
        }
        close(conn);
    }
    close(srv);
    db_close(&db);
    return 0;
}
