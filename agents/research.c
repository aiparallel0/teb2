#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

AgentMsg research_handle(AgentMsg msg)
{
    LlmReq      lreq; LlmReply lrep;
    OutcomeQuery oq;  OutcomeResult ores;
    char safe[1024];

    if (msg.tag != MSG_EXEC_REQ && msg.tag != MSG_RESEARCH) return msg;
    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_IO,
            "{\"error\":\"llm_disabled\",\"reason\":\"no api key\"}");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "research");
    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
    /* NOTE: this MVP path does not yet fetch web snippets; the
     * research prompt will reply with confidence=low when snippets
     * are absent. Follow-up work (docs/FOLLOWUPS.md §E) adds
     * grounding via exec/http.c + a search provider. */
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO,
            "{\"error\":\"llm_error\"}");

    if (msg.db) {
        memset(&oq, 0, sizeof(oq));
        oq.task_id = msg.id;
        snprintf(oq.result, sizeof(oq.result), "%.511s", lrep.reply);
        ores = store_outcome(msg.db, oq); (void)ores;
    }
    return agent_make_result(msg, ERR_OK, lrep.reply);
}

