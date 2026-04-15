#include <string.h>
#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"
#include "exec/exec.h"

/*
 * Outreach agent:
 *   MSG_NUDGE  — generate motivational nudge via LLM, store in nudges.
 *   MSG_NOTIFY — compose notification, send via SMTP if configured.
 * In:  AgentMsg (MSG_NUDGE | MSG_NOTIFY).
 * Out: AgentMsg (MSG_RESULT).
 */

static AgentMsg handle_nudge(AgentMsg msg)
{
    LlmReq     lreq;
    LlmReply   lrep;
    NudgeQuery nq;
    NudgeResult nr;
    const char *text = msg.payload;

    if (msg.cfg && msg.cfg->openai_key[0]) {
        memset(&lreq, 0, sizeof(lreq));
        snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
        snprintf(lreq.system, sizeof(lreq.system),
            "You are a motivational coach. Generate a short, encouraging "
            "message (1-2 sentences) for someone working toward their goal. "
            "Be specific and positive.");
        snprintf(lreq.user, sizeof(lreq.user),
            "Goal context: %.900s", msg.payload);
        lrep = llm_call(lreq, msg.cfg);
        if (lrep.err == ERR_OK) text = lrep.reply;
    }

    if (msg.db) {
        memset(&nq, 0, sizeof(nq));
        snprintf(nq.user_id, sizeof(nq.user_id), "%s", msg.user_id);
        snprintf(nq.message, sizeof(nq.message), "%.511s", text);
        nr = store_nudge(msg.db, nq);
        (void)nr;
    }

    return agent_make_result(msg, ERR_OK, text);
}

static AgentMsg handle_notify(AgentMsg msg)
{
    MailReq   mreq;
    MailResult mr;

    if (!msg.cfg || !msg.cfg->smtp_host[0])
        return agent_make_result(msg, ERR_OK, "notify:no_smtp");

    memset(&mreq, 0, sizeof(mreq));
    snprintf(mreq.to,      sizeof(mreq.to),      "%s", msg.user_id);
    snprintf(mreq.subject, sizeof(mreq.subject),  "teb2 notification");
    snprintf(mreq.body,    sizeof(mreq.body),     "%.1000s", msg.payload);

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
