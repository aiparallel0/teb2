#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/types_collab.h"
#include "core/errors.h"
#include "exec/exec.h"

NotifyResult send_notify(NotifyReq req, Cred *cred)
{
    NotifyResult nr;
    HttpReq hr;
    HttpResult hr2;

    memset(&nr, 0, sizeof(nr));
    if (req.webhook_url[0] == '\0') {
        nr.err = ERR_INVALID;
        return nr;
    }

    memset(&hr, 0, sizeof(hr));
    snprintf(hr.method, sizeof(hr.method), "POST");
    snprintf(hr.path, sizeof(hr.path), "%s", req.webhook_url);
    hr.body_len = (size_t)snprintf(hr.body, sizeof(hr.body),
        "{\"channel\":\"%s\",\"text\":\"%s\"}",
        req.channel, req.message);

    hr2 = send_request(hr, cred);
    if (hr2.err != ERR_OK) {
        nr.err = hr2.err;
        return nr;
    }
    nr.sent = 1;
    nr.err = ERR_OK;
    return nr;
}
