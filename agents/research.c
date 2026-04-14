#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "exec/exec.h"

static AgentMsg run_browser(AgentMsg src)
{
    AgentMsg out;
    SerialCmd cmd;
    BrowserResult br;
    Cred cred;

    memset(&out,  0, sizeof(out));
    memset(&cmd,  0, sizeof(cmd));
    memset(&cred, 0, sizeof(cred));

    snprintf(cmd.action, sizeof(cmd.action), "%s", "navigate");
    snprintf(cmd.target, sizeof(cmd.target), "%.*s",
             (int)(sizeof(cmd.target) - 1), src.payload);
    snprintf(cmd.value,  sizeof(cmd.value),  "%s", "");

    /* cred.login = "rfd,wfd" for the Playwright pipe */
    snprintf(cred.login, sizeof(cred.login), "%s", "");

    br = browser_send(cmd, &cred);

    out.tag = MSG_RESULT;
    out.id  = src.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", src.user_id);
    if (br.err == ERR_OK) {
        snprintf(out.payload, sizeof(out.payload), "%.*s",
                 (int)br.output_len, br.output);
        out.err = ERR_OK;
    } else {
        snprintf(out.payload, sizeof(out.payload), "browser_error");
        out.err = br.err;
    }
    return out;
}

AgentMsg research_handle(AgentMsg msg)
{
    if (msg.tag != MSG_EXEC_REQ)
        return msg;
    return run_browser(msg);
}
