#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Decompose agent: uses LLM to split a goal into 3-7 tasks, stores them.
 * In:  AgentMsg (MSG_DECOMPOSE | MSG_GOAL_NEW, id=goal_id, payload=description).
 * Out: AgentMsg (MSG_RESULT, payload="decomposed:<count>").
 */

#define MAX_TASKS 7

/* Extract quoted strings from a JSON array ["t1","t2",...] */
static int parse_task_array(const char *text, char tasks[][256], int max)
{
    int n = 0;
    const char *p = text;
    const char *end;
    size_t len;

    p = strchr(p, '[');
    if (!p) p = text;
    while (n < max && (p = strchr(p, '"')) != NULL) {
        p++;
        end = strchr(p, '"');
        if (!end) break;
        len = (size_t)(end - p);
        if (len > 0 && len < 256) {
            memcpy(tasks[n], p, len);
            tasks[n][len] = '\0';
            n++;
        }
        p = end + 1;
    }
    return n;
}

AgentMsg decompose_handle(AgentMsg msg)
{
    LlmReq   lreq;
    LlmReply lrep;
    char     tasks[MAX_TASKS][256];
    TaskQuery tq;
    TaskResult tr;
    char buf[64];
    int  n, i;

    if (msg.tag != MSG_DECOMPOSE && msg.tag != MSG_GOAL_NEW)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_decompose_request");

    if (!msg.cfg || !msg.cfg->openai_key[0]) {
        snprintf(buf, sizeof(buf), "decomposed:no_key");
        return agent_make_result(msg, ERR_OK, buf);
    }

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
    snprintf(lreq.system, sizeof(lreq.system),
        "Decompose the goal into 3-7 specific, actionable tasks. "
        "Output ONLY a JSON array of task title strings: "
        "[\"task1\",\"task2\",\"task3\"]");
    snprintf(lreq.user, sizeof(lreq.user), "Goal: %.900s", msg.payload);

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO, "llm_error");

    memset(tasks, 0, sizeof(tasks));
    n = parse_task_array(lrep.reply, tasks, MAX_TASKS);

    if (msg.db) {
        for (i = 0; i < n; i++) {
            memset(&tq, 0, sizeof(tq));
            tq.goal_id = msg.id;
            snprintf(tq.user_id, sizeof(tq.user_id), "%s", msg.user_id);
            snprintf(tq.title, sizeof(tq.title), "%.255s", tasks[i]);
            snprintf(tq.agent,   sizeof(tq.agent),   "exec");
            tr = store_task(msg.db, tq);
            (void)tr;
        }
    }

    snprintf(buf, sizeof(buf), "decomposed:%d", n);
    return agent_make_result(msg, ERR_OK, buf);
}
