#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"

/*
 * Clarify agent: asks the LLM to surface clarifying questions for a goal.
 * In:  AgentMsg (MSG_CLARIFY, payload = goal description, cfg = Config*).
 * Out: AgentMsg (MSG_RESULT, payload = clarifying questions or "clarified:ok").
 */

#define CLARITY_MIN 8

static int count_non_ws(const char *s)
{
    int n = 0;
    size_t i;
    for (i = 0; s[i] != '\0'; i++)
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n') n++;
    return n;
}

AgentMsg clarify_handle(AgentMsg msg)
{
    LlmReq   lreq;
    LlmReply lrep;

    if (msg.tag != MSG_CLARIFY)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_clarify_request");

    if (count_non_ws(msg.payload) < CLARITY_MIN)
        return agent_make_result(msg, ERR_OK, "clarify:need_more:too_vague");

    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_OK, "clarified:ok");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
    snprintf(lreq.system, sizeof(lreq.system),
        "You are a goal clarification assistant. "
        "Ask 1-3 specific clarifying questions to help decompose a goal "
        "into actionable tasks. Be concise. Output only the questions.");
    snprintf(lreq.user,   sizeof(lreq.user),
        "Goal: %.900s", msg.payload);

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err == ERR_OK)
        return agent_make_result(msg, ERR_OK, lrep.reply);

    return agent_make_result(msg, ERR_OK, "clarified:ok");
}
