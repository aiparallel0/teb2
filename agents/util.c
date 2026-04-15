#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/util.h"

AgentMsg agent_make_result(AgentMsg src, Err err, const char *detail)
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

int payload_has_content(const char *payload)
{
    size_t i;
    for (i = 0; payload[i] != '\0'; i++) {
        if (payload[i] != ' ' && payload[i] != '\t' &&
            payload[i] != '\n')
            return 1;
    }
    return 0;
}
