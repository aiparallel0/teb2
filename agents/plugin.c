#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"

AgentMsg plugin_handle(AgentMsg msg)
{
    if (msg.tag != MSG_PLUGIN)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_plugin_request");

    if (strncmp(msg.payload, "enable:", 7) == 0)
        return agent_make_result(msg, ERR_OK, "plugin_enabled");
    if (strncmp(msg.payload, "disable:", 8) == 0)
        return agent_make_result(msg, ERR_OK, "plugin_disabled");
    if (strncmp(msg.payload, "webhook:", 8) == 0)
        return agent_make_result(msg, ERR_OK, "webhook_dispatched");

    return agent_make_result(msg, ERR_OK, "plugin_registered");
}
