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
    snprintf(req.method, sizeof(req.method), "%.*s", (int)(sp1 - p), p);
    p = sp1 + 1;
    sp2 = memchr(p, ' ', (size_t)(end - p));
    if (!sp2) return req;
    snprintf(req.path, sizeof(req.path), "%.*s", (int)(sp2 - p), p);
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

void write_response(int fd, HttpResp resp)
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
