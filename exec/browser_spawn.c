#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "core/types.h"
#include "core/errors.h"
#include "exec/exec.h"

#define BROWSER_JS "./browser_worker.js"

typedef struct { int rp[2]; int wp[2]; } Pipes;

/* Suppress analyzer fd-leak in the child process path: _exit() closes all fds */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
static void __attribute__((noreturn)) child_exec(Pipes p)
{
    close(p.wp[1]);
    close(p.rp[0]);

    if (dup2(p.wp[0], STDIN_FILENO) < 0) {
        close(p.wp[0]); close(p.rp[1]); _exit(1);
    }
    close(p.wp[0]);

    if (dup2(p.rp[1], STDOUT_FILENO) < 0) {
        close(p.rp[1]); close(STDIN_FILENO); _exit(1);
    }
    close(p.rp[1]);

    execlp("node", "node", BROWSER_JS, (char *)NULL);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    _exit(1);
}
#pragma GCC diagnostic pop

/*
 * browser_spawn: fork a Node.js Playwright worker, wire pipes.
 * In:  BrowserProc *proc.
 * Out: 0 on success, -1 on failure.
 */
int browser_spawn(BrowserProc *proc)
{
    Pipes p;

    if (!proc) return -1;
    memset(proc, 0, sizeof(*proc));

    if (pipe(p.rp) != 0) return -1;
    if (pipe(p.wp) != 0) {
        close(p.rp[0]); close(p.rp[1]);
        return -1;
    }

    proc->pid = (int)fork();
    if (proc->pid < 0) {
        close(p.rp[0]); close(p.rp[1]);
        close(p.wp[0]); close(p.wp[1]);
        return -1;
    }

    if (proc->pid == 0) child_exec(p); /* noreturn in child */

    /* Parent */
    close(p.wp[0]);
    close(p.rp[1]);
    proc->rfd = p.rp[0];
    proc->wfd = p.wp[1];
    return 0;
}
