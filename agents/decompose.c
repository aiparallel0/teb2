#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/*
 * Decompose agent — produces a typed DAG of tasks per prompts/decompose.md.
 * Parses the JSON reply and inserts each task with its routed agent.
 * In:  AgentMsg (MSG_DECOMPOSE | MSG_GOAL_NEW, id=goal_id, payload=goal).
 * Out: AgentMsg (MSG_RESULT, payload="decomposed:<count>").
 */

#define MAX_TASKS 7

/* Find a quoted string value after a key ("key":"value"). Returns 1 on success. */
static int find_kv_str(const char *obj, const char *key, char *out, size_t osz)
{
    char pat[64];
    const char *p, *e;
    size_t len;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(obj, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), '"');
    if (!p) return 0;
    p++;
    e = p;
    while (*e && !(*e == '"' && (e == p || *(e - 1) != '\\'))) e++;
    if (!*e) return 0;
    len = (size_t)(e - p);
    if (len >= osz) len = osz - 1;
    memcpy(out, p, len); out[len] = '\0';
    return 1;
}

/* Split the reply into individual task objects at depth 1 in "tasks":[ ... ]. */
static int split_tasks(const char *reply, const char *pos[MAX_TASKS], int ends[MAX_TASKS])
{
    const char *p = strstr(reply, "\"tasks\"");
    int depth = 0, n = 0;
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    p++;
    while (*p && n < MAX_TASKS) {
        if (*p == '{') {
            if (depth == 0) pos[n] = p;
            depth++;
        } else if (*p == '}') {
            depth--;
            if (depth == 0) { ends[n] = (int)(p - pos[n]) + 1; n++; }
        } else if (*p == ']' && depth == 0) {
            break;
        }
        p++;
    }
    return n;
}

AgentMsg decompose_handle(AgentMsg msg)
{
    LlmReq   lreq;
    LlmReply lrep;
    TaskQuery tq; TaskResult tr;
    const char *pos[MAX_TASKS]; int ends[MAX_TASKS];
    char obj[1024], title[256], agent[64], desc[512], safe[1024];
    char buf[64];
    int  n, i, stored = 0;

    if (msg.tag != MSG_DECOMPOSE && msg.tag != MSG_GOAL_NEW)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_decompose_request");
    if (!msg.cfg || !msg.cfg->openai_key[0])
        return agent_make_result(msg, ERR_OK, "decomposed:no_key");

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "decompose");
    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(lreq.user, sizeof(lreq.user), "%s", safe);
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err != ERR_OK)
        return agent_make_result(msg, ERR_IO, "llm_error");

    n = split_tasks(lrep.reply, pos, ends);
    for (i = 0; i < n && msg.db; i++) {
        size_t ol = (size_t)ends[i];
        if (ol >= sizeof(obj)) ol = sizeof(obj) - 1;
        memcpy(obj, pos[i], ol); obj[ol] = '\0';
        title[0] = agent[0] = desc[0] = '\0';
        (void)find_kv_str(obj, "title", title, sizeof(title));
        (void)find_kv_str(obj, "agent", agent, sizeof(agent));
        (void)find_kv_str(obj, "description", desc, sizeof(desc));
        if (!title[0]) continue;
        memset(&tq, 0, sizeof(tq));
        tq.goal_id = msg.id;
        snprintf(tq.user_id,     sizeof(tq.user_id),     "%s", msg.user_id);
        snprintf(tq.title,       sizeof(tq.title),       "%.255s", title);
        snprintf(tq.description, sizeof(tq.description), "%.511s", desc);
        snprintf(tq.agent,       sizeof(tq.agent),       "%s", agent[0] ? agent : "exec");
        tr = store_task(msg.db, tq);
        if (tr.err == ERR_OK) stored++;
    }
    snprintf(buf, sizeof(buf), "decomposed:%d", stored);
    return agent_make_result(msg, ERR_OK, buf);
}

