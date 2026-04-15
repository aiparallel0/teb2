#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

/*
 * Learn agent: extracts an insight from a completed measure outcome.
 * In:  AgentMsg (MSG_LEARN, payload = outcome text, id = goal_id).
 * Out: AgentMsg (MSG_RESULT, payload = "learned:<insight>").
 *
 * The insight is the first 256 chars of the payload, prefixed with
 * "learned:" so that the coordinator knows the Learn phase completed.
 */

AgentMsg learn_handle(AgentMsg msg)
{
    char buf[512];

    if (msg.tag != MSG_LEARN)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_learn_request");

    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND,
                                 "learned:nothing:empty_input");

    snprintf(buf, sizeof(buf), "learned:%.480s", msg.payload);
    return agent_make_result(msg, ERR_OK, buf);
}
