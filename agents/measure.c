#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_ext.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Measure agent: uses LLM to score task completion 0-100%, stores snapshot.
 * In:  AgentMsg (MSG_MEASURE, id=goal_id, payload=outcome text).
 * Out: AgentMsg (MSG_RESULT, payload="measured:<pct>%").
 */

static int parse_score(const char *reply)
{
    const char *p = reply;
    int score = 0;
    /* Find first run of digits */
    while (*p && (*p < '0' || *p > '9')) p++;
    while (*p >= '0' && *p <= '9') {
        score = score * 10 + (*p - '0');
        if (score > 100) { score = 100; break; }
        p++;
    }
    return score;
}

AgentMsg measure_handle(AgentMsg msg)
{
    LlmReq    lreq;
    LlmReply  lrep;
    SnapQuery sq;
    SnapResult sr;
    int  score = 0;
    char buf[64];

    if (msg.tag != MSG_MEASURE)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_measure_request");

    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND, "measured:fail:empty_outcome");

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
        snprintf(lreq.system, sizeof(lreq.system),
            "Score the completion of this task from 0 to 100. "
            "Reply with a single integer only, nothing else.");
        snprintf(lreq.user, sizeof(lreq.user),
            "Outcome: %.900s", msg.payload);
        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK)
            score = parse_score(lrep.reply);
        else
            score = payload_has_content(msg.payload) ? 50 : 0;
    } else {
        score = payload_has_content(msg.payload) ? 50 : 0;
    }

    if (msg.db) {
        memset(&sq, 0, sizeof(sq));
        sq.goal_id = msg.id;
        sq.pct     = score;
        sr = store_snap(msg.db, sq);
        (void)sr;
    }

    snprintf(buf, sizeof(buf), "measured:%d%%", score);
    return agent_make_result(msg, ERR_OK, buf);
}
