#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Exec agent — handles tasks routed with agent="exec" by the
 * decompose phase. Picks an exec.* prompt from a lightweight
 * keyword match over the task payload, then delegates to llm_call.
 * This closes the long-standing dead-letter bug where MSG_EXEC_REQ
 * silently ran the research prompt for every "exec" task.
 *
 * Keyword rules are deliberately short and local; richer routing
 * belongs to router_handle (agents/router.c) which the caller may
 * invoke explicitly when a confident classification is required.
 */

struct rule { const char *needle; const char *prompt; };

static const struct rule RULES[] = {
    { "review",     "exec.code_review" },
    { "diff",       "exec.code_review" },
    { "refactor",   "exec.refactor"    },
    { "sql",        "exec.sql"         },
    { "query",      "exec.sql"         },
    { "translate",  "exec.translate"   },
    { "rewrite",    "exec.rewrite"     },
    { "summari",    "exec.summarize"   },  /* summarise / summarize */
    { "extract",    "exec.extract"     },
    { "classif",    "exec.classify"    },  /* classify / classification */
    { "sentiment",  "exec.sentiment"   },
    { "plan",       "exec.plan"        },
    { "write",      "exec.write"       },
    { "draft",      "exec.write"       },
    { "blog",       "exec.write"       },
    { "post",       "exec.write"       },
    { "code",       "exec.code"        },
    { "implement",  "exec.code"        },
    { "function",   "exec.code"        },
};

static const char *pick_prompt(const char *payload)
{
    char low[512];
    size_t i, n = sizeof(RULES) / sizeof(RULES[0]);
    size_t j;
    for (j = 0; j < sizeof(low) - 1 && payload[j]; j++) {
        char c = payload[j];
        low[j] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    low[j] = '\0';
    for (i = 0; i < n; i++)
        if (strstr(low, RULES[i].needle)) return RULES[i].prompt;
    return "exec.plan";  /* safe default: plan, don't guess */
}

AgentMsg exec_handle(AgentMsg msg)
{
    LlmReq       lreq;
    LlmReply     lrep;
    OutcomeQuery oq;
    OutcomeResult ores;
    char         safe[1024];
    const char  *prompt;

    if (msg.tag != MSG_EXEC_RUN)
        return agent_make_result(msg, ERR_UNKNOWN, "not_an_exec_request");
    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_IO,
            "{\"error\":\"llm_disabled\",\"reason\":\"no api key\"}");

    memset(&lreq, 0, sizeof(lreq));
    prompt = pick_prompt(msg.payload);
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "%s", prompt);
    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO,
            "{\"error\":\"llm_error\",\"reason\":\"upstream failure\"}");

    if (msg.db) {
        memset(&oq, 0, sizeof(oq));
        oq.task_id = msg.id;
        snprintf(oq.result, sizeof(oq.result), "%.511s", lrep.reply);
        ores = store_outcome(msg.db, oq); (void)ores;
    }
    return agent_make_result(msg, ERR_OK, lrep.reply);
}
