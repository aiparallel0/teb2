#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

AgentMsg learn_handle(AgentMsg msg)
{
    LlmReq     lreq; LlmReply lrep;
    LearnQuery lq;   LearnResult lr;
    char buf[512], safe[1024];

    if (msg.tag != MSG_LEARN)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_learn_request");
    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND,
            "{\"error\":\"not_enough_context\",\"reason\":\"empty outcome\"}");

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "learn");
        sanitize_untrusted(msg.payload, safe, sizeof(safe));
        snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
        lreq.want_json = 1;
        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK) {
            char insight[512];
            /* Extract the scalar insight sentence. Storing the full
             * envelope back into learnings.insight turns every future
             * context injection into a JSON string blob — the model
             * then treats its own schema as data and collapses. */
            if (!json_str(lrep.reply, "insight", insight, sizeof(insight)) ||
                insight[0] == '\0')
                snprintf(insight, sizeof(insight), "%.*s", 480, lrep.reply);
            if (msg.db) {
                memset(&lq, 0, sizeof(lq));
                lq.goal_id = msg.id;
                snprintf(lq.insight, sizeof(lq.insight), "%.511s", insight);
                lr = store_learning(msg.db, lq); (void)lr;
            }
            snprintf(buf, sizeof(buf), "%.499s", lrep.reply);
            return agent_make_result(msg, ERR_OK, buf);
        }
    }
    if (msg.db) {
        memset(&lq, 0, sizeof(lq));
        lq.goal_id = msg.id;
        snprintf(lq.insight, sizeof(lq.insight), "%.511s", msg.payload);
        lr = store_learning(msg.db, lq); (void)lr;
    }
    snprintf(buf, sizeof(buf), "%.499s", msg.payload);
    return agent_make_result(msg, ERR_OK, buf);
}

