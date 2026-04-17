#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_ext.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

static int extract_int_field(const char *reply, const char *key)
{
    char pat[64];
    const char *p;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(reply, pat);
    if (!p) return -1;
    p = strchr(p + strlen(pat), ':');
    if (!p) return -1;
    while (*++p == ' ') ;
    if (*p < '0' || *p > '9') return -1;
    return atoi(p);
}

AgentMsg measure_handle(AgentMsg msg)
{
    LlmReq    lreq; LlmReply lrep;
    SnapQuery sq;   SnapResult sr;
    int score = 0;
    char buf[2048], safe[1024];

    if (msg.tag != MSG_MEASURE)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_measure_request");
    if (!payload_has_content(msg.payload))
        return agent_make_result(msg, ERR_NOT_FOUND,
            "{\"error\":\"not_enough_context\",\"reason\":\"empty outcome\"}");

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "measure");
        sanitize_untrusted(msg.payload, safe, sizeof(safe));
        snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
        lreq.want_json = 1;
        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK) {
            int s = extract_int_field(lrep.reply, "score_0_100");
            score = (s >= 0 && s <= 100) ? s : 0;
            snprintf(buf, sizeof(buf), "%.2000s", lrep.reply);
        } else {
            score = 50;
            snprintf(buf, sizeof(buf),
                "{\"error\":\"llm_error\",\"score_0_100\":50}");
        }
    } else {
        score = payload_has_content(msg.payload) ? 50 : 0;
        snprintf(buf, sizeof(buf),
            "{\"score_0_100\":%d,\"rubric\":[],\"next_action\":\"retry\","
            "\"reasoning\":\"llm_disabled\"}", score);
    }

    if (msg.db) {
        memset(&sq, 0, sizeof(sq));
        sq.goal_id = msg.id;
        sq.pct     = score;
        sr = store_snap(msg.db, sq); (void)sr;
    }
    return agent_make_result(msg, ERR_OK, buf);
}
