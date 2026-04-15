#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"

/*
 * Measure agent: evaluates whether a task outcome satisfies its goal.
 * In:  AgentMsg (MSG_MEASURE, payload = outcome text).
 * Out: AgentMsg (MSG_RESULT, payload = "measured:pass" or "measured:fail").
 */

static AgentMsg make_result(AgentMsg src, Err err, const char *detail)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = src.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", src.user_id);
    snprintf(out.payload, sizeof(out.payload), "%s", detail);
    out.err = err;
    return out;
}

static int payload_has_content(const char *payload)
{
    size_t i;
    for (i = 0; payload[i] != '\0'; i++) {
        if (payload[i] != ' ' && payload[i] != '\t' && payload[i] != '\n')
            return 1;
    }
    return 0;
}

AgentMsg measure_handle(AgentMsg msg)
{
    if (msg.tag != MSG_MEASURE)
        return make_result(msg, ERR_UNKNOWN, "not_a_measure_request");

    if (!payload_has_content(msg.payload))
        return make_result(msg, ERR_NOT_FOUND, "measured:fail:empty_outcome");

    return make_result(msg, ERR_OK, "measured:pass");
}
