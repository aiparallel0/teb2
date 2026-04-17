#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"
#include "exec/exec.h"

static AgentMsg handle_nudge(AgentMsg msg)
{
    LlmReq     lreq; LlmReply lrep;
    NudgeQuery nq;   NudgeResult nr;
    LearnListResult ll;
    char safe[1024], ctx[1536];
    const char *text = msg.payload;
    int n = 0, i;

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "outreach.nudge");
        sanitize_untrusted(msg.payload, safe, sizeof(safe));
        snprintf(lreq.user, sizeof(lreq.user),
                 "<goal>%.900s</goal>", safe);
        ctx[0] = '\0';
        if (msg.db && msg.user_id[0]) {
            ll = list_learnings(msg.db, msg.user_id, 3);
            if (ll.err == ERR_OK && ll.count > 0) {
                n += snprintf(ctx + n, sizeof(ctx) - n,
                              "<prior_learnings>\n");
                for (i = 0; i < ll.count && n < (int)sizeof(ctx) - 32; i++)
                    n += snprintf(ctx + n, sizeof(ctx) - n, "%lld: %.180s\n",
                                  (long long)ll.rows[i].id, ll.rows[i].insight);
                snprintf(ctx + n, sizeof(ctx) - n, "</prior_learnings>\n");
            }
        }
        snprintf(lreq.context, sizeof(lreq.context), "%s", ctx);
        lreq.want_json = 1;
        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK) text = lrep.reply;
    }

    if (msg.db) {
        memset(&nq, 0, sizeof(nq));
        snprintf(nq.user_id, sizeof(nq.user_id), "%s", msg.user_id);
        snprintf(nq.message, sizeof(nq.message), "%.511s", text);
        nr = store_nudge(msg.db, nq); (void)nr;
    }
    return agent_make_result(msg, ERR_OK, text);
}

static AgentMsg handle_notify(AgentMsg msg)
{
    MailReq   mreq; MailResult mr;

    if (!msg.cfg || !msg.cfg->smtp_host[0])
        return agent_make_result(msg, ERR_OK, "notify:no_smtp");

    memset(&mreq, 0, sizeof(mreq));
    snprintf(mreq.to,      sizeof(mreq.to),      "%s", msg.user_id);
    snprintf(mreq.subject, sizeof(mreq.subject), "teb2 notification");
    snprintf(mreq.body,    sizeof(mreq.body),    "%.1000s", msg.payload);
    mr = send_mail(mreq, msg.cfg);
    return agent_make_result(msg, mr.err,
                             mr.sent ? "notify:sent" : "notify:smtp_failed");
}

AgentMsg outreach_handle(AgentMsg msg)
{
    if (msg.tag == MSG_NUDGE)  return handle_nudge(msg);
    if (msg.tag == MSG_NOTIFY) return handle_notify(msg);
    return agent_make_result(msg, ERR_UNKNOWN, "not_an_outreach_request");
}

