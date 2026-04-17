#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "api/api.h"

/*
 * Connection-level safety net sitting between accept() and parse_request().
 * Rationale: the accept loop reads once into an 8 KiB buffer, then hands
 * the bytes to parse_request(). Without deadlines a slow client can hold
 * a worker forever (slowloris), or a client can dribble headers so the
 * single read() never sees \r\n\r\n. These two helpers together cap both
 * wall-clock time and total bytes per request so a worker cannot starve.
 * Bodies larger than cap are rejected (returning -1 / closing) rather
 * than silently truncated, which is what the raw read() previously did.
 */

/* Set SO_RCVTIMEO / SO_SNDTIMEO on an accepted connection. Values in
 * seconds; 0 means "leave unchanged". Best-effort: failure is ignored
 * because the outer read deadline below is the real enforcement. */
void socket_set_deadlines(int fd, int read_sec, int write_sec)
{
    struct timeval tv;
    if (fd < 0) return;
    if (read_sec > 0) {
        tv.tv_sec = read_sec; tv.tv_usec = 0;
        (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
    if (write_sec > 0) {
        tv.tv_sec = write_sec; tv.tv_usec = 0;
        (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }
}

/* Case-insensitive search for a header on its own line, matching
 * "\n<name>:" (preserving the leading newline so we don't match
 * mid-line). Returns pointer just after ':' or NULL. */
static const char *find_header(const char *hay, const char *name)
{
    size_t nlen = strlen(name);
    const char *p = hay;
    while ((p = strchr(p, '\n')) != NULL) {
        p++;
        if (strncasecmp(p, name, nlen) == 0 && p[nlen] == ':')
            return p + nlen + 1;
    }
    return NULL;
}

/*
 * Read an HTTP request with a wall-clock deadline. Returns the number of
 * bytes read (>0) on success, 0 if the client closed cleanly with no data,
 * or -1 on timeout / error / oversize request. The caller owns buf; we
 * read up to cap bytes total. We stop early once \r\n\r\n is seen and the
 * advertised Content-Length (if any) has arrived, so we don't waste a
 * round-trip waiting for the client's FIN.
 */
ssize_t slowloris_read(int fd, char *buf, size_t cap, int deadline_sec)
{
    time_t start = time(NULL);
    size_t off = 0;
    int headers_done = 0;
    size_t body_target = 0; /* 0 = unknown until headers parsed */

    if (!buf || cap == 0 || fd < 0) return -1;
    for (;;) {
        ssize_t n;
        if ((time(NULL) - start) >= deadline_sec) return -1;
        if (off >= cap - 1) return -1; /* oversize request */
        n = read(fd, buf + off, cap - 1 - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break; /* client closed */
        off += (size_t)n;
        buf[off] = '\0';
        {
            const char *hend = strstr(buf, "\r\n\r\n");
            if (!hend) continue;
            if (!headers_done) {
                const char *cl = find_header(buf, "Content-Length");
                headers_done = 1;
                if (cl) {
                    while (*cl == ' ' || *cl == '\t') cl++;
                    body_target = (size_t)strtoul(cl, NULL, 10);
                    if (body_target > cap) return -1;
                }
            }
            {
                size_t have_body = off - (size_t)(hend + 4 - buf);
                if (have_body >= body_target) break;
            }
        }
    }
    return (ssize_t)off;
}
