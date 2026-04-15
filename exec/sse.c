#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include "exec/exec.h"

#define SSE_BUFSIZE 1024

void sse_write(SseConn *c, const char *event, const char *data)
{
    char buf[SSE_BUFSIZE];
    int len;
    size_t total, off;
    ssize_t nw;

    if (!c || !c->open || c->fd < 0) return;
    if (!event || !data) return;
    len = snprintf(buf, sizeof(buf),
                   "event: %s\ndata: %s\n\n", event, data);
    if (len < 0 || (size_t)len >= sizeof(buf)) {
        c->open = 0;
        return;
    }
    total = (size_t)len;
    off = 0;
    while (off < total) {
        nw = write(c->fd, buf + off, total - off);
        if (nw <= 0) {
            c->open = 0;
            return;
        }
        off += (size_t)nw;
    }
}

void sse_close(SseConn *c)
{
    if (!c) return;
    if (c->fd >= 0) {
        close(c->fd);
    }
    c->fd = -1;
    c->open = 0;
}
