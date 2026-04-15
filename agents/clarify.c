#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

/*
 * Clarify agent: validates a goal has enough detail to be decomposed.
 * In:  AgentMsg (MSG_CLARIFY, payload = goal description).
 * Out: AgentMsg (MSG_RESULT, payload = "clarified:ok" or
 *      "clarify:need_more:<reason>").
 *
 * A goal is considered sufficiently clear if the payload contains
 * at least 8 non-whitespace characters (short phrases like "profit"
 * are too vague to decompose into actionable tasks).
 */

#define CLARITY_MIN 8

static int count_non_ws(const char *s)
{
    int n = 0;
    size_t i;
    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n')
            n++;
    }
    return n;
}

AgentMsg clarify_handle(AgentMsg msg)
{
    if (msg.tag != MSG_CLARIFY)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_clarify_request");

    if (count_non_ws(msg.payload) < CLARITY_MIN)
        return agent_make_result(msg, ERR_OK,
                                 "clarify:need_more:too_vague");

    return agent_make_result(msg, ERR_OK, "clarified:ok");
}
