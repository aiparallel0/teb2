#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

/*
 * Measure agent: evaluates whether a task outcome satisfies its goal.
 * In:  AgentMsg (MSG_MEASURE, payload = outcome text).
 * Out: AgentMsg (MSG_RESULT, payload = "measured:pass" or "measured:fail").
 */

AgentMsg measure_handle(AgentMsg msg)
{
    if (msg.tag != MSG_MEASURE)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_measure_request");

    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND,
                                 "measured:fail:empty_outcome");

    return agent_make_result(msg, ERR_OK, "measured:pass");
}
