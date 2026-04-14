#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

#define FRAME_MAX 4096

/*
 * browser_send: sends a serialized command frame to a Playwright subprocess
 * over a pre-opened pipe.  The frame format is:
 *   <action>\t<target>\t<value>\n
 * The subprocess writes a JSON result line back.
 *
 * cred->login holds the path to the Playwright pipe FD pair as
 * "<read_fd>,<write_fd>" encoded in ASCII.
 */
static int parse_pipe_fds(const Cred *cred, int *rfd, int *wfd)
{
    const char *comma;
    if (!cred || cred->login[0] == '\0') return -1;
    *rfd = (int)strtol(cred->login, NULL, 10);
    comma = strchr(cred->login, ',');
    if (!comma) return -1;
    *wfd = (int)strtol(comma + 1, NULL, 10);
    return 0;
}

BrowserResult browser_send(SerialCmd cmd, Cred *cred)
{
    BrowserResult r;
    char frame[FRAME_MAX];
    char reply[FRAME_MAX];
    int rfd, wfd;
    ssize_t n;
    int flen;

    memset(&r,     0, sizeof(r));
    memset(frame,  0, sizeof(frame));
    memset(reply,  0, sizeof(reply));

    if (parse_pipe_fds(cred, &rfd, &wfd) != 0) {
        r.err = ERR_IO;
        return r;
    }

    flen = snprintf(frame, sizeof(frame), "%s\t%s\t%s\n",
                    cmd.action, cmd.target, cmd.value);
    if (flen < 0 || (size_t)flen >= sizeof(frame)) {
        r.err = ERR_LIMIT;
        return r;
    }

    if (write(wfd, frame, (size_t)flen) != (ssize_t)flen) {
        r.err = ERR_IO;
        return r;
    }

    n = read(rfd, reply, sizeof(reply) - 1);
    if (n <= 0) {
        r.err = ERR_IO;
        return r;
    }
    reply[n] = '\0';
    r.output_len = (size_t)n;
    memcpy(r.output, reply, r.output_len);
    r.err = ERR_OK;
    return r;
}
