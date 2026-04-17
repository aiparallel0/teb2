#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
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

static int is_sse_path(const char *path)
{
    return (strncmp(path, "/sse/", 5) == 0);
}

static void install_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    /* No SA_RESTART: accept() returns EINTR so the loop can observe
     * g_running and shut down cleanly instead of blocking forever. */
    sa.sa_flags = 0;
    (void)sigaction(SIGTERM, &sa, NULL);
    (void)sigaction(SIGINT,  &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
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
    if (validate_config(&cfg) != ERR_OK) {
        fprintf(stderr,
            "teb2: refusing to start: missing or default SECRET "
            "(must be >=32 chars, not placeholder). Edit .env.\n");
        return 2;
    }
    if (db_open(cfg.db_path, &db) != ERR_OK) {
        fprintf(stderr, "db_open failed: %s\n", cfg.db_path);
        return 1;
    }
    install_signals();

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
    if (listen(srv, 64) != 0) {
        close(srv); db_close(&db); return 1;
    }

    /* Pre-fork worker pool: N-1 child workers share the listening socket */
    {
        int nw = (cfg.workers > 1) ? cfg.workers : 4;
        int i;
        for (i = 1; i < nw; i++) {
            pid_t wk = fork();
            if (wk == 0) {
                /* Worker: reopen own SQLite handle (not fork-safe) */
                db_close(&db);
                if (db_open(cfg.db_path, &db) != ERR_OK) _exit(1);
                break;
            }
            if (wk < 0) break; /* fork failure: proceed with fewer workers */
        }
    }

    while (g_running) {
        ssize_t n;
        Ctx ctx;
        UserClaims uc;
        TokenResult ar;
        conn = accept(srv, NULL, NULL);
        if (conn < 0) {
            if (errno == EINTR) continue; /* signal: re-check g_running */
            continue;
        }
        /* Slowloris / idle-client guard: hard deadlines on the socket. */
        socket_set_deadlines(conn, 10, 10);
        n = slowloris_read(conn, buf, sizeof(buf) - 1, 10);
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
