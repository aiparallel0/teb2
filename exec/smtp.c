#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

static const char b64c[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void b64_encode(const unsigned char *in, size_t inlen,
                       char *out, size_t outlen)
{
    size_t i, o = 0;
    unsigned int v;
    for (i = 0; i + 2 < inlen && o + 4 < outlen; i += 3) {
        v = ((unsigned int)in[i] << 16) | ((unsigned int)in[i+1] << 8) | in[i+2];
        out[o++] = b64c[(v >> 18) & 0x3F]; out[o++] = b64c[(v >> 12) & 0x3F];
        out[o++] = b64c[(v >> 6)  & 0x3F]; out[o++] = b64c[v & 0x3F];
    }
    if (i < inlen && o + 4 < outlen) {
        v = (unsigned int)in[i] << 16;
        if (i + 1 < inlen) v |= (unsigned int)in[i+1] << 8;
        out[o++] = b64c[(v >> 18) & 0x3F]; out[o++] = b64c[(v >> 12) & 0x3F];
        out[o++] = (i + 1 < inlen) ? b64c[(v >> 6) & 0x3F] : '=';
        out[o++] = '=';
    }
    out[o] = '\0';
}

static int smtp_recv(int fd, char *buf, size_t len)
{
    ssize_t n = read(fd, buf, len - 1);
    if (n <= 0) return -1;
    buf[n] = '\0';
    if (n >= 3) return (buf[0] - '0') * 100 + (buf[1] - '0') * 10 + (buf[2] - '0');
    return -1;
}

static int smtp_send(int fd, const char *line)
{
    char tmp[1300];
    int len = snprintf(tmp, sizeof(tmp), "%s\r\n", line);
    if (len < 0) return -1;
    return (write(fd, tmp, (size_t)len) > 0) ? 0 : -1;
}

static void smtp_set_timeouts(int fd)
{
    struct timeval tv;
    tv.tv_sec = 10; tv.tv_usec = 0;
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static int smtp_connect(int fd, struct sockaddr *sa, socklen_t salen)
{
    fd_set wf;
    struct timeval tv;
    int flags, err = 0;
    socklen_t elen = sizeof(err);
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
    if (connect(fd, sa, salen) == 0) goto ok;
    if (errno != EINPROGRESS) return -1;
    FD_ZERO(&wf); FD_SET(fd, &wf);
    tv.tv_sec = 10; tv.tv_usec = 0;
    if (select(fd + 1, NULL, &wf, NULL, &tv) <= 0) return -1;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) < 0 || err != 0)
        return -1;
ok:
    (void)fcntl(fd, F_SETFL, flags);
    return 0;
}

MailResult send_mail(MailReq req, Config *cfg)
{
    MailResult r;
    struct addrinfo hints, *res = NULL;
    char portstr[8], buf[1024], b64buf[256], hdr[2048];
    int fd, code;

    memset(&r, 0, sizeof(r));
    if (!cfg || !cfg->smtp_host[0]) { r.err = ERR_IO; return r; }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", cfg->smtp_port);
    if (getaddrinfo(cfg->smtp_host, portstr, &hints, &res) != 0) {
        r.err = ERR_IO; return r;
    }
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        r.err = ERR_IO; return r;
    }
    if (smtp_connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
        freeaddrinfo(res);
        close(fd);
        r.err = ERR_IO; return r;
    }
    freeaddrinfo(res);
    smtp_set_timeouts(fd);
    code = smtp_recv(fd, buf, sizeof(buf));
    if (code < 0 || code >= 500) { close(fd); r.err = ERR_IO; return r; }
    if (smtp_send(fd, "EHLO teb") < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    if (code >= 500) { close(fd); r.err = ERR_IO; return r; }
    if (smtp_send(fd, "AUTH LOGIN") < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    (void)code;
    b64_encode((const unsigned char *)cfg->smtp_user, strlen(cfg->smtp_user),
               b64buf, sizeof(b64buf));
    if (smtp_send(fd, b64buf) < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    (void)code;
    b64_encode((const unsigned char *)cfg->smtp_pass, strlen(cfg->smtp_pass),
               b64buf, sizeof(b64buf));
    if (smtp_send(fd, b64buf) < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    if (code >= 500) { close(fd); r.err = ERR_IO; return r; }
    snprintf(hdr, sizeof(hdr), "MAIL FROM:<%s>", cfg->smtp_user);
    if (smtp_send(fd, hdr) < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    if (code >= 500) { close(fd); r.err = ERR_IO; return r; }
    snprintf(hdr, sizeof(hdr), "RCPT TO:<%s>", req.to);
    if (smtp_send(fd, hdr) < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    if (code >= 500) { close(fd); r.err = ERR_IO; return r; }
    if (smtp_send(fd, "DATA") < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    (void)code;
    snprintf(hdr, sizeof(hdr),
             "Subject: %s\r\nTo: %s\r\nFrom: %s\r\n\r\n%s\r\n.",
             req.subject, req.to, cfg->smtp_user, req.body);
    if (smtp_send(fd, hdr) < 0) { close(fd); r.err = ERR_IO; return r; }
    code = smtp_recv(fd, buf, sizeof(buf));
    (void)smtp_send(fd, "QUIT");
    (void)smtp_recv(fd, buf, sizeof(buf));
    close(fd);
    r.err = ERR_OK;
    r.sent = 1;
    return r;
}
