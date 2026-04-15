#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Research agent: uses LLM to summarize findings for a task, stores outcome.
 * In:  AgentMsg (MSG_EXEC_REQ, id=task_id, payload=topic/description).
 * Out: AgentMsg (MSG_RESULT, payload=research summary or error).
 */

AgentMsg research_handle(AgentMsg msg)
{
    LlmReq      lreq;
    LlmReply    lrep;
    OutcomeQuery oq;
    OutcomeResult ores;

    if (msg.tag != MSG_EXEC_REQ)
        return msg;

    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_IO, "research_unavailable");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
    snprintf(lreq.system, sizeof(lreq.system),
        "You are a research assistant. Summarize key facts and findings "
        "relevant to the given topic in 2-3 concise sentences.");
    snprintf(lreq.user, sizeof(lreq.user),
        "Research topic: %.900s", msg.payload);

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO, "llm_error");

    if (msg.db) {
        memset(&oq, 0, sizeof(oq));
        oq.task_id = msg.id;
        snprintf(oq.result, sizeof(oq.result), "%.511s", lrep.reply);
        ores = store_outcome(msg.db, oq);
        (void)ores;
    }

    return agent_make_result(msg, ERR_OK, lrep.reply);
}
