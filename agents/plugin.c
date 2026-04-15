#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_collab.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"
#include "exec/exec.h"

/*
 * Plugin agent: fetches integration config, POSTs to webhook_url if enabled.
 * In:  AgentMsg (MSG_PLUGIN, payload="webhook:<integ_id>:<data>"|"enable:"|...).
 * Out: AgentMsg (MSG_RESULT).
 */

static AgentMsg webhook_dispatch(AgentMsg msg)
{
    IntegQuery  iq;
    IntegResult ir;
    HttpReq     hreq;
    HttpResult  hres;
    Cred        cred;
    const char *colon;
    char        idstr[32];
    size_t      idlen;

    /* payload format: "webhook:<integ_id>:<json_data>" */
    colon = strchr(msg.payload + 8, ':');
    if (!colon)
        return agent_make_result(msg, ERR_UNKNOWN, "bad_webhook_payload");

    idlen = (size_t)(colon - (msg.payload + 8));
    if (idlen >= sizeof(idstr)) idlen = sizeof(idstr) - 1;
    memcpy(idstr, msg.payload + 8, idlen);
    idstr[idlen] = '\0';

    if (!msg.db)
        return agent_make_result(msg, ERR_DB, "no_db");

    memset(&iq, 0, sizeof(iq));
    iq.id = (int64_t)strtoll(idstr, NULL, 10);
    ir = fetch_integ(msg.db, iq);

    if (ir.err != ERR_OK || !ir.integ.enabled || !ir.integ.webhook_url[0])
        return agent_make_result(msg, ERR_NOT_FOUND, "integ_not_found_or_disabled");

    memset(&hreq, 0, sizeof(hreq));
    memset(&cred, 0, sizeof(cred));
    snprintf(hreq.method, sizeof(hreq.method), "POST");
    snprintf(hreq.path,   sizeof(hreq.path),   "%s", ir.integ.webhook_url);
    snprintf(hreq.body,   sizeof(hreq.body),   "%s", colon + 1);
    hreq.body_len = strlen(hreq.body);
    hres = send_request(hreq, &cred);
    (void)hres;

    return agent_make_result(msg, ERR_OK, "webhook_dispatched");
}

AgentMsg plugin_handle(AgentMsg msg)
{
    if (msg.tag != MSG_PLUGIN)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_plugin_request");
    if (strncmp(msg.payload, "enable:", 7) == 0)
        return agent_make_result(msg, ERR_OK, "plugin_enabled");
    if (strncmp(msg.payload, "disable:", 8) == 0)
        return agent_make_result(msg, ERR_OK, "plugin_disabled");
    if (strncmp(msg.payload, "webhook:", 8) == 0)
        return webhook_dispatch(msg);
    return agent_make_result(msg, ERR_OK, "plugin_registered");
}
