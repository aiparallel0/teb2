#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

AgentMsg oauth_handle(AgentMsg msg)
{
    if (msg.tag != MSG_OAUTH)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_an_oauth_request");

    if (strncmp(msg.payload, "exchange:", 9) == 0)
        return agent_make_result(msg, ERR_OK, "token_exchanged");
    if (strncmp(msg.payload, "refresh:", 8) == 0)
        return agent_make_result(msg, ERR_OK, "token_refreshed");

    return agent_make_result(msg, ERR_OK, "state_generated");
}
