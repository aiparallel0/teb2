#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

AgentMsg decompose_handle(AgentMsg msg)
{
    if (msg.tag != MSG_DECOMPOSE)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_decompose_request");

    return agent_make_result(msg, ERR_OK, "decomposed");
}
