#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "exec/exec.h"

static AgentMsg compose_message(AgentMsg src, const char *channel)
{
    AgentMsg out;
    HttpReq  req;
    HttpResult res;
    Cred cred;

    memset(&out,  0, sizeof(out));
    memset(&req,  0, sizeof(req));
    memset(&cred, 0, sizeof(cred));

    snprintf(req.method,   sizeof(req.method),   "%s", "POST");
    snprintf(req.path,     sizeof(req.path),      "%s", channel);
    snprintf(req.body,     sizeof(req.body),      "%s", src.payload);
    req.body_len = strlen(src.payload);

    res = send_request(req, &cred);

    out.tag = MSG_RESULT;
    out.id  = src.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", src.user_id);
    if (res.err == ERR_OK) {
        snprintf(out.payload, sizeof(out.payload), "sent:%d", res.status);
        out.err = ERR_OK;
    } else {
        snprintf(out.payload, sizeof(out.payload), "send_failed");
        out.err = res.err;
    }
    return out;
}

AgentMsg outreach_handle(AgentMsg msg)
{
    if (msg.tag != MSG_NUDGE)
        return msg;
    return compose_message(msg, "http://localhost:9000/notify");
}
