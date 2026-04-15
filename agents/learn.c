#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Learn agent: uses LLM to extract insights from outcomes, stores learnings.
 * In:  AgentMsg (MSG_LEARN, id=goal_id, payload=outcome text).
 * Out: AgentMsg (MSG_RESULT, payload="learned:<insight>").
 */

AgentMsg learn_handle(AgentMsg msg)
{
    LlmReq     lreq;
    LlmReply   lrep;
    LearnQuery lq;
    LearnResult lr;
    char buf[512];

    if (msg.tag != MSG_LEARN)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_learn_request");

    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND, "learned:nothing:empty_input");

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
        snprintf(lreq.system, sizeof(lreq.system),
            "Extract 1-2 key learnings or actionable insights from this "
            "task outcome. Be specific and concise (under 200 words).");
        snprintf(lreq.user, sizeof(lreq.user),
            "Outcome: %.900s", msg.payload);

        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK) {
            if (msg.db) {
                memset(&lq, 0, sizeof(lq));
                lq.goal_id = msg.id;
                snprintf(lq.insight, sizeof(lq.insight), "%.511s", lrep.reply);
                lr = store_learning(msg.db, lq);
                (void)lr;
            }
            snprintf(buf, sizeof(buf), "learned:%.450s", lrep.reply);
            return agent_make_result(msg, ERR_OK, buf);
        }
    }

    /* Fallback: store raw payload as learning */
    if (msg.db) {
        memset(&lq, 0, sizeof(lq));
        lq.goal_id = msg.id;
        snprintf(lq.insight, sizeof(lq.insight), "%.511s", msg.payload);
        lr = store_learning(msg.db, lq);
        (void)lr;
    }

    snprintf(buf, sizeof(buf), "learned:%.480s", msg.payload);
    return agent_make_result(msg, ERR_OK, buf);
}
