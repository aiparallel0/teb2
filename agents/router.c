#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"

/*
 * Router agent — classifies a single task description into one of
 * the five execution agents (research, outreach, finance, browser,
 * exec). Thin wrapper around the `router` prompt; returns the raw
 * classification JSON unchanged so callers can make their own
 * confidence-threshold decisions.
 *
 * This closes FOLLOWUPS §D partially: the router is now callable
 * via MSG_ROUTE. Wiring decompose to *consult* it on low-confidence
 * routing is still deferred — it changes the decompose loop's
 * contract and deserves its own focused PR.
 */

AgentMsg router_handle(AgentMsg msg)
{
    LlmReq   lreq;
    LlmReply lrep;
    char     safe[1024];

    if (msg.tag != MSG_ROUTE)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_route_request");
    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_OK,
            "{\"agent\":\"exec\",\"confidence\":0.5,"
            "\"rationale\":\"empty payload — default bucket\"}");
    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_OK,
            "{\"agent\":\"exec\",\"confidence\":0.5,"
            "\"rationale\":\"llm_disabled — default bucket\"}");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "router");
    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO,
            "{\"error\":\"llm_error\",\"reason\":\"upstream failure\"}");
    return agent_make_result(msg, ERR_OK, lrep.reply);
}
