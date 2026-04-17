#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/types_ext.h"
#include "core/llm.h"
#include "core/sanitize.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"

/* All monetary amounts are int64_t cents. No floating point. */
#define TIER_AUTO     100LL    /* under $1.00: auto-approve */
#define TIER_CONFIRM  10000LL  /* under $100.00: single confirmation */

static int64_t parse_cents(const char *payload)
{
    int64_t cents = 0;
    const char *p = payload;
    while (*p >= '0' && *p <= '9') {
        int64_t digit = *p++ - '0';
        if (cents > (INT64_MAX - digit) / 10) return INT64_MAX;
        cents = cents * 10 + digit;
    }
    return cents;
}

static void find_risk(const char *reply, char *out, size_t osz)
{
    const char *p = strstr(reply, "\"risk\"");
    const char *e;
    size_t len;
    out[0] = '\0';
    if (!p) return;
    p = strchr(p + 6, '"'); if (!p) return;
    p++;
    e = strchr(p, '"'); if (!e) return;
    len = (size_t)(e - p);
    if (len >= osz) len = osz - 1;
    memcpy(out, p, len); out[len] = '\0';
}

/* LLM risk assessment via prompts/finance.risk.md; stored in
 * agent_memory and on the approval row when one is created. */
static void assess_risk(AgentMsg msg, int64_t cents, char *risk, size_t rsz)
{
    LlmReq   lreq; LlmReply lrep;
    MemQuery mq;   MemResult mr;
    char user[1024], safe[512];

    risk[0] = '\0';
    if (!msg.cfg || !msg.cfg->openai_key[0] || !msg.db) return;

    sanitize_untrusted(msg.payload, safe, sizeof(safe));
    snprintf(user, sizeof(user),
        "<spend>amount_cents=%lld; context=%s</spend>",
        (long long)cents, safe);

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.prompt_name, sizeof(lreq.prompt_name), "finance.risk");
    snprintf(lreq.user, sizeof(lreq.user), "%s", user);
    lreq.want_json = 1;

    lrep = llm_call(lreq, msg.cfg);
    if (lrep.err == ERR_OK) find_risk(lrep.reply, risk, rsz);

    memset(&mq, 0, sizeof(mq));
    snprintf(mq.agent, sizeof(mq.agent), "finance");
    snprintf(mq.key,   sizeof(mq.key),   "risk_%lld", (long long)msg.id);
    snprintf(mq.val,   sizeof(mq.val),   "%.500s",
             lrep.err == ERR_OK ? lrep.reply : "{\"risk\":\"UNKNOWN\"}");
    mr = store_mem(msg.db, mq); (void)mr;
}

static AgentMsg open_approval(AgentMsg msg, int64_t cents, const char *risk)
{
    ApprovalQuery aq; ApprovalResult ar;
    char body[1024];

    if (!msg.db || !msg.user_id[0])
        return agent_make_result(msg, ERR_AUTH,
            "{\"status\":\"pending\",\"reason\":\"awaiting confirmation\"}");

    memset(&aq, 0, sizeof(aq));
    snprintf(aq.user_id, sizeof(aq.user_id), "%s", msg.user_id);
    snprintf(aq.kind,    sizeof(aq.kind),    "finance");
    aq.amount_cents = cents;
    snprintf(aq.payload, sizeof(aq.payload), "%.500s", msg.payload);
    snprintf(aq.risk,    sizeof(aq.risk),    "%s", risk ? risk : "");
    ar = store_approval(msg.db, aq);

    snprintf(body, sizeof(body),
        "{\"status\":\"pending\",\"approval_id\":%lld,"
        "\"amount_cents\":%lld,\"risk\":\"%s\"}",
        (long long)(ar.err == ERR_OK ? ar.ap.id : 0),
        (long long)cents, risk ? risk : "");
    return agent_make_result(msg, ERR_AUTH, body);
}

AgentMsg finance_handle(AgentMsg msg)
{
    int64_t cents;
    char    risk[16];

    if (msg.tag != MSG_FINANCE_REQ)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_finance_request");

    cents = parse_cents(msg.payload);
    assess_risk(msg, cents, risk, sizeof(risk));

    if (cents < TIER_AUTO) return agent_make_result(msg, ERR_OK,
        "{\"status\":\"approved\",\"auto\":true}");
    if (cents < TIER_CONFIRM) return open_approval(msg, cents, risk);
    return agent_make_result(msg, ERR_AUTH,
        "{\"status\":\"denied\",\"reason\":\"amount exceeds confirm tier\"}");
}

