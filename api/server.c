#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "core/types.h"
#include "api/api.h"

/*
 * HTTP/1.0 request parser and response writer.
 *
 * Parsing is best-effort and defensive: the accept loop calls
 * slowloris_read() which enforces both wall-clock and byte-count limits,
 * so parse_request() only needs to cope with well-formed (but possibly
 * hostile) input up to 8 KiB. Anything exotic is silently dropped.
 *
 * write_response always emits HTTP/1.1 with Content-Length and then
 * closes the socket (no keep-alive). Nginx in front terminates TLS and
 * keeps client-side keep-alive; teb2 itself is single-request-per-conn
 * so worker accounting stays trivial.
 */

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
                         "%.*s", (int)strcspn(tk + 6, "&"), tk + 6);
            snprintf(req.query, sizeof(req.query), "%s", qm + 1);
            *qm = '\0';
        }
    }
    auth = strstr(raw, "\nAuthorization: ");
    if (auth) {
        auth += 16;
        if (strncmp(auth, "Bearer ", 7) == 0) auth += 7;
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
