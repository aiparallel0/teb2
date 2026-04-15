#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"

/*
 * Learn agent: extracts an insight from a completed measure outcome.
 * In:  AgentMsg (MSG_LEARN, payload = outcome text, id = goal_id).
 * Out: AgentMsg (MSG_RESULT, payload = "learned:<insight>").
 *
 * The insight is the first 256 chars of the payload, prefixed with
 * "learned:" so that the coordinator knows the Learn phase completed.
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

AgentMsg learn_handle(AgentMsg msg)
{
    char buf[512];

    if (msg.tag != MSG_LEARN)
        return make_result(msg, ERR_UNKNOWN, "not_a_learn_request");

    if (!payload_has_content(msg.payload))
        return make_result(msg, ERR_NOT_FOUND, "learned:nothing:empty_input");

    snprintf(buf, sizeof(buf), "learned:%.480s", msg.payload);
    return make_result(msg, ERR_OK, buf);
}
