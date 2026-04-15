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

static void parse_request(const char *raw, size_t len, HttpReq *req)
{
    const char *end = raw + len;
    const char *p   = raw;
    const char *sp1, *sp2, *body, *auth;
    memset(req, 0, sizeof(*req));
    sp1 = memchr(p, ' ', (size_t)(end - p));
    if (!sp1) return;
    snprintf(req->method, sizeof(req->method), "%.*s", (int)(sp1 - p), p);
    p = sp1 + 1;
    sp2 = memchr(p, ' ', (size_t)(end - p));
    if (!sp2) return;
    snprintf(req->path, sizeof(req->path), "%.*s", (int)(sp2 - p), p);
    auth = strstr(raw, "\nAuthorization: ");
    if (auth) {
        auth += 16;
        snprintf(req->auth_header, sizeof(req->auth_header),
                 "%.*s", (int)(strcspn(auth, "\r\n")), auth);
    }
    body = strstr(raw, "\r\n\r\n");
    if (body) {
        body += 4;
        req->body_len = (size_t)(end - body);
        if (req->body_len >= sizeof(req->body))
            req->body_len = sizeof(req->body) - 1;
        memcpy(req->body, body, req->body_len);
    }
}

static void write_response(int fd, HttpResp resp)
{
    char    hdr[256];
    int     hlen;
    ssize_t nw;
    hlen = snprintf(hdr, sizeof(hdr),
                    "HTTP/1.1 %d OK\r\nContent-Type: %s\r\n"
                    "Content-Length: %zu\r\n\r\n",
                    resp.status, resp.content_type, resp.body_len);
    nw = write(fd, hdr, (size_t)hlen);
    if (nw < 0) return;
    if (resp.body_len > 0) {
        nw = write(fd, resp.body, resp.body_len);
        (void)nw;
    }
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
    (void)setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
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
            parse_request(buf, (size_t)n, &req);
            memset(&ctx, 0, sizeof(ctx));
            ctx.db  = &db;
            ctx.cfg = &cfg;
            ar = authenticate_request(req, cfg.secret);
            if (ar.err == ERR_OK) { uc = ar.claims; ctx.user = &uc; }
            resp = dispatch(req, &ctx);
            write_response(conn, resp);
        }
        close(conn);
    }
    close(srv);
    db_close(&db);
    return 0;
}
