#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"

/*
 * Learn agent: extracts a lesson from a task outcome and returns
 * it as a MSG_RESULT with the lesson text in payload.
 *
 * In:  AgentMsg (MSG_LEARN, payload = outcome text, id = task_id).
 * Out: AgentMsg (MSG_RESULT, payload = "learned:<summary>").
 *
 * The caller (coord or API layer) is responsible for persisting the
 * lesson into agent_memory via db/memory.c.  This agent is pure
 * computation — no I/O.
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

static int has_content(const char *s)
{
    size_t i;
    for (i = 0; s[i] != '\0'; i++)
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n') return 1;
    return 0;
}

AgentMsg learn_handle(AgentMsg msg)
{
    char lesson[512];

    if (msg.tag != MSG_LEARN)
        return make_result(msg, ERR_UNKNOWN, "not_a_learn_request");

    if (!has_content(msg.payload))
        return make_result(msg, ERR_NOT_FOUND, "learn:empty_outcome");

    snprintf(lesson, sizeof(lesson), "learned:%lld:%.*s",
             (long long)msg.id, 480, msg.payload);
    return make_result(msg, ERR_OK, lesson);
}
