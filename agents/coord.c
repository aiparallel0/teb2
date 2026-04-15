#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"

static AgentMsg next_task(AgentMsg msg)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = msg.id;
    out.db  = msg.db;
    out.cfg = msg.cfg;
    snprintf(out.user_id, sizeof(out.user_id), "%s", msg.user_id);
    snprintf(out.payload, sizeof(out.payload), "next:%lld", (long long)msg.id);
    out.err = ERR_OK;
    return out;
}

static AgentMsg unknown_msg(AgentMsg msg)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = msg.id;
    out.err = ERR_UNKNOWN;
    return out;
}

/*
 * coord_handle: central dispatcher — ensures db/cfg propagate, routes by tag.
 * In:  AgentMsg (any tag; db and cfg must be set by the API layer).
 * Out: AgentMsg (MSG_RESULT with outcome).
 */
AgentMsg coord_handle(AgentMsg msg)
{
    switch (msg.tag) {
    case MSG_GOAL_NEW:    return decompose_handle(msg);
    case MSG_CLARIFY:     return clarify_handle(msg);
    case MSG_TASK_DONE:   return next_task(msg);
    case MSG_EXEC_REQ:    return research_handle(msg);
    case MSG_FINANCE_REQ: return finance_handle(msg);
    case MSG_NUDGE:       return outreach_handle(msg);
    case MSG_CHECKIN:     return next_task(msg);
    case MSG_MEASURE:     return measure_handle(msg);
    case MSG_LEARN:       return learn_handle(msg);
    case MSG_RESULT:      return msg;
    case MSG_DECOMPOSE:   return decompose_handle(msg);
    case MSG_PLUGIN:      return plugin_handle(msg);
    case MSG_OAUTH:       return oauth_handle(msg);
    case MSG_NOTIFY:      return outreach_handle(msg);
    }
    return unknown_msg(msg);
}
