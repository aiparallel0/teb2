#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"

static AgentMsg decompose_goal(AgentMsg msg)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = msg.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", msg.user_id);
    snprintf(out.payload, sizeof(out.payload), "decomposed:%lld",
             (long long)msg.id);
    out.err = ERR_OK;
    return out;
}

static AgentMsg next_task(AgentMsg msg)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = msg.id;
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

AgentMsg coord_handle(AgentMsg msg)
{
    switch (msg.tag) {
    case MSG_GOAL_NEW:    return decompose_goal(msg);
    case MSG_TASK_DONE:   return next_task(msg);
    case MSG_EXEC_REQ:    return research_handle(msg);
    case MSG_FINANCE_REQ: return finance_handle(msg);
    case MSG_NUDGE:       return outreach_handle(msg);
    case MSG_CHECKIN:     return next_task(msg);
    case MSG_MEASURE:     return measure_handle(msg);
    case MSG_LEARN:       return learn_handle(msg);
    case MSG_RESULT:      return msg;
    }
    return unknown_msg(msg);
}
