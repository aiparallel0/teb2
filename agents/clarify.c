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
 * Clarify agent — named-prompt + sanitized input.
 * In:  AgentMsg (MSG_CLARIFY, payload = goal text, cfg, db, user_id).
 * Out: AgentMsg (MSG_RESULT, payload = JSON per prompts/clarify.md).
 */

#define CLARITY_MIN 8

static int count_non_ws(const char *s)
{
    int n = 0; size_t i;
    for (i = 0; s[i] != '\0'; i++)
        if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n') n++;
    return n;
}

static void append_prior_learnings(const AgentMsg *msg, char *ctx, size_t csz)
{
    LearnListResult ll;
    int i, n;
    if (!msg->db || !msg->user_id[0]) return;
    ll = list_learnings(msg->db, msg->user_id, 5);
    if (ll.err != ERR_OK || ll.count == 0) return;
    n = (int)strlen(ctx);
    n += snprintf(ctx + n, (n < (int)csz) ? csz - n : 0,
                  "<prior_learnings>\n");
    for (i = 0; i < ll.count && n < (int)csz - 32; i++) {
        n += snprintf(ctx + n, csz - n, "%lld: %.200s\n",
                      (long long)ll.rows[i].id, ll.rows[i].insight);
    }
    snprintf(ctx + n, (n < (int)csz) ? csz - n : 0,
             "</prior_learnings>\n");
}

AgentMsg clarify_handle(AgentMsg msg)
{
    LlmReq   lreq;
    LlmReply lrep;
    char     safe[1024];

    if (msg.tag != MSG_CLARIFY)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_clarify_request");
    if (count_non_ws(msg.payload) < CLARITY_MIN)
        return agent_make_result(msg, ERR_OK,
            "{\"status\":\"needs_info\",\"readiness_score\":10,"
            "\"questions\":[\"Please describe your goal in a full sentence.\"],"
            "\"missing_dimensions\":[\"scope\"],"
            "\"reasoning\":\"input too short\"}");
    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_OK,
            "{\"status\":\"ready\",\"readiness_score\":70,"
            "\"questions\":[],\"missing_dimensions\":[],"
            "\"reasoning\":\"llm_disabled\"}");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "clarify");
    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
    append_prior_learnings(&msg, lreq.context, sizeof(lreq.context));
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err == ERR_OK)
        return agent_make_result(msg, ERR_OK, lrep.reply);
    return agent_make_result(msg, ERR_IO,
        "{\"error\":\"llm_error\",\"reason\":\"upstream failure\"}");
}

