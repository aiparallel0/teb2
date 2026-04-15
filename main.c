#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
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

static void handle_signal(int sig) { (void)sig; g_running = 0; }
static void reap_children(int sig) { (void)sig; while (waitpid(-1, NULL, WNOHANG) > 0) ; }

static int is_sse_path(const char *path)
{
    return (strncmp(path, "/sse/", 5) == 0);
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
    signal(SIGCHLD, reap_children);

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
            req.fd = conn;
            memset(&ctx, 0, sizeof(ctx));
            ctx.db  = &db;
            ctx.cfg = &cfg;
            ar = authenticate_request(req, cfg.secret);
            if (ar.err == ERR_OK) {
                uc = ar.claims;
                ctx.user = &uc;
            }
            if (is_sse_path(req.path)) {
                pid_t pid = fork();
                if (pid == 0) {
                    close(srv);
                    (void)dispatch(req, &ctx);
                    close(conn);
                    db_close(&db);
                    _exit(0);
                }
            } else {
                resp = dispatch(req, &ctx);
                write_response(conn, resp);
            }
        }
        close(conn);
    }
    close(srv);
    db_close(&db);
    return 0;
}
