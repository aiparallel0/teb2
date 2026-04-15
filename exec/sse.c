#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

void sse_write(SseConn *c, const char *event, const char *data)
{
    char buf[1024];
    int len;
    ssize_t nw;

    if (!c || !c->open || c->fd < 0) return;
    if (!event || !data) return;
    len = snprintf(buf, sizeof(buf),
                   "event: %s\ndata: %s\n\n", event, data);
    if (len < 0 || (size_t)len >= sizeof(buf)) {
        c->open = 0;
        return;
    }
    nw = write(c->fd, buf, (size_t)len);
    if (nw < 0) {
        c->open = 0;
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
