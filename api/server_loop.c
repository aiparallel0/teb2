#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include "core/types.h"
#include "core/errors.h"
#include "auth/auth.h"
#include "db/db.h"
#include "api/api.h"

/* Wall-clock budget a worker will spend on one client, from accept()
 * through complete request receipt. Kept small so a saturated pool
 * reclaims workers quickly even under slowloris; nginx in front of
 * teb2 keeps honest clients from ever hitting this. */
#define REQUEST_DEADLINE_SEC 10

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) { (void)sig; g_running = 0; }

static void reap_children(int sig)
{
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

void server_install_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; /* no SA_RESTART so accept() returns EINTR */
    (void)sigaction(SIGTERM, &sa, NULL);
    (void)sigaction(SIGINT,  &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = reap_children;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    (void)sigaction(SIGCHLD, &sa, NULL);
}

static int is_sse_path(const char *path)
{
    return (strncmp(path, "/sse/", 5) == 0);
}

/* Pre-fork N-1 worker children sharing the listen socket. The caller
 * returns on fork failure with a smaller pool. */
static void prefork_workers(Config *cfg, Db *db)
{
    int nw = (cfg->workers > 1) ? cfg->workers : 4;
    int i;
    for (i = 1; i < nw; i++) {
        pid_t wk = fork();
        if (wk == 0) {
            /* Worker: reopen own SQLite handle (not fork-safe) */
            db_close(db);
            if (db_open(cfg->db_path, db) != ERR_OK) _exit(1);
            return;
        }
        if (wk < 0) break;
    }
}

static void serve_one(int srv, int conn, Db *db, Config *cfg)
{
    char buf[8192];
    ssize_t n;
    HttpReq  req;
    HttpResp resp;
    Ctx ctx;
    UserClaims uc;
    TokenResult ar;

    socket_set_deadlines(conn, REQUEST_DEADLINE_SEC, REQUEST_DEADLINE_SEC);
    n = slowloris_read(conn, buf, sizeof(buf) - 1, REQUEST_DEADLINE_SEC);
    if (n <= 0) return;
    buf[n] = '\0';
    req = parse_request(buf, (size_t)n);
    req.fd = conn;
    memset(&ctx, 0, sizeof(ctx));
    ctx.db  = db;
    ctx.cfg = cfg;
    ar = authenticate_request(req, cfg->secret);
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
            db_close(db);
            _exit(0);
        }
    } else {
        resp = dispatch(req, &ctx);
        write_response(conn, resp);
    }
}

int server_start(Config *cfg, Db *db)
{
    union {
        struct sockaddr    base;
        struct sockaddr_in in4;
    } sa;
    int srv, conn, optval;

    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) return 1;
    optval = 1;
    (void)setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                     &optval, sizeof(optval));
    memset(&sa, 0, sizeof(sa));
    sa.in4.sin_family      = AF_INET;
    sa.in4.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.in4.sin_port        = htons((uint16_t)cfg->port);
    if (bind(srv, &sa.base, sizeof(sa.in4)) != 0) {
        close(srv); return 1;
    }
    if (listen(srv, 64) != 0) {
        close(srv); return 1;
    }

    prefork_workers(cfg, db);

    while (g_running) {
        conn = accept(srv, NULL, NULL);
        if (conn < 0) {
            if (errno == EINTR) continue; /* signal: re-check g_running */
            continue;
        }
        serve_one(srv, conn, db, cfg);
        close(conn);
    }
    close(srv);
    return 0;
}
