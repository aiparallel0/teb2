#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

#define HTTP_BUFSIZE 8192

static int open_connection(const char *host, int port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    char portstr[8];
    int fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (getaddrinfo(host, portstr, &hints, &res) != 0) return -1;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

HttpResult send_request(HttpReq req, Cred *cred)
{
    HttpResult r;
    char buf[HTTP_BUFSIZE];
    char host[128];
    const char *reqpath;
    int  port = 80;
    int  fd;
    ssize_t n;
    int hlen;
    const char *path_start;
    const char *slash;
    const char *host_end;
    size_t hlen_h;

    memset(&r,   0, sizeof(r));
    memset(buf,  0, sizeof(buf));
    (void)cred;

    /* Parse URL: strip scheme, extract host[:port] and /path */
    path_start = req.path;
    if (strncmp(req.path, "https://", 8) == 0) {
        path_start = req.path + 8;
        port = 443;
    } else if (strncmp(req.path, "http://", 7) == 0) {
        path_start = req.path + 7;
    }

    slash    = strchr(path_start, '/');
    host_end = slash ? slash : (path_start + strlen(path_start));
    hlen_h   = (size_t)(host_end - path_start);
    if (hlen_h >= sizeof(host)) hlen_h = sizeof(host) - 1;
    memcpy(host, path_start, hlen_h);
    host[hlen_h] = '\0';

    {
        char *colon = strchr(host, ':');
        if (colon) { port = atoi(colon + 1); *colon = '\0'; }
    }

    fd = open_connection(host, port);
    if (fd < 0) { r.err = ERR_IO; return r; }

    reqpath = slash ? slash : "/";
    hlen = snprintf(buf, sizeof(buf),
             "%s %s HTTP/1.0\r\nHost: %s\r\nContent-Length: %zu\r\n",
             req.method, reqpath, host, req.body_len);
    if (hlen > 0 && req.auth_header[0] && (size_t)hlen < sizeof(buf) - 4)
        hlen += snprintf(buf + hlen, sizeof(buf) - (size_t)hlen,
                         "Authorization: %s\r\n", req.auth_header);
    if (hlen > 0 && req.body_len > 0 && (size_t)hlen < sizeof(buf) - 4)
        hlen += snprintf(buf + hlen, sizeof(buf) - (size_t)hlen,
                         "Content-Type: application/json\r\n");
    if (hlen > 0 && (size_t)hlen < sizeof(buf) - 3)
        hlen += snprintf(buf + hlen, sizeof(buf) - (size_t)hlen, "\r\n");

    if (hlen <= 0 || write(fd, buf, (size_t)hlen) < 0) {
        close(fd); r.err = ERR_IO; return r;
    }
    if (req.body_len > 0 && write(fd, req.body, req.body_len) < 0) {
        close(fd); r.err = ERR_IO; return r;
    }

    n = read(fd, buf, (ssize_t)(sizeof(buf) - 1));
    close(fd);
    if (n < 0) { r.err = ERR_IO; return r; }
    buf[n] = '\0';

    r.status = 0;
    if (n > 9) r.status = atoi(buf + 9); /* "HTTP/1.x NNN " */
    {
        const char *body = strstr(buf, "\r\n\r\n");
        if (body) {
            body += 4;
            r.body_len = (size_t)(n - (body - buf));
            if (r.body_len > sizeof(r.body) - 1) r.body_len = sizeof(r.body) - 1;
            memcpy(r.body, body, r.body_len);
        }
    }
    r.err = ERR_OK;
    return r;
}
