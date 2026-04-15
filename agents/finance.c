#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/llm.h"
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

static AgentMsg approve(AgentMsg msg)
{
    return agent_make_result(msg, ERR_OK, "approved");
}

static AgentMsg deny(AgentMsg msg)
{
    return agent_make_result(msg, ERR_AUTH, "denied:requires_authorization");
}

static AgentMsg needs_confirm(AgentMsg msg)
{
    return agent_make_result(msg, ERR_AUTH, "pending:awaiting_confirmation");
}

/* LLM risk assessment stored in agent_memory after tier check */
static void store_risk(AgentMsg msg, int64_t cents)
{
    LlmReq   lreq;
    LlmReply lrep;
    MemQuery mq;
    MemResult mr;
    char assessment[512];

    if (!msg.cfg || !msg.cfg->openai_key[0] || !msg.db) return;

    memset(&lreq, 0, sizeof(lreq));
    snprintf(lreq.model,  sizeof(lreq.model),  "%s", msg.cfg->openai_model);
    snprintf(lreq.system, sizeof(lreq.system),
        "You are a financial risk assessor. "
        "Rate the risk of this transaction as LOW, MEDIUM, or HIGH. "
        "Reply with exactly one word.");
    snprintf(lreq.user, sizeof(lreq.user),
        "Amount: %lld cents. Context: %.400s",
        (long long)cents, msg.payload);

    lrep = llm_call(lreq, msg.cfg);
    snprintf(assessment, sizeof(assessment),
             "risk:%.500s", lrep.err == ERR_OK ? lrep.reply : "UNKNOWN");

    memset(&mq, 0, sizeof(mq));
    snprintf(mq.agent, sizeof(mq.agent), "finance");
    snprintf(mq.key,   sizeof(mq.key),   "risk_%lld", (long long)msg.id);
    snprintf(mq.val,   sizeof(mq.val),   "%s", assessment);
    mr = store_mem(msg.db, mq);
    (void)mr;
}

AgentMsg finance_handle(AgentMsg msg)
{
    int64_t  cents;
    AgentMsg result;

    if (msg.tag != MSG_FINANCE_REQ)
        return agent_make_result(msg, ERR_UNKNOWN, "not_a_finance_request");

    cents = parse_cents(msg.payload);

    switch (cents < TIER_AUTO ? 0 : cents < TIER_CONFIRM ? 1 : 2) {
    case 0:  result = approve(msg);       break;
    case 1:  result = needs_confirm(msg); break;
    default: result = deny(msg);          break;
    }

    store_risk(msg, cents); /* async risk assessment — result stored in memory */
    return result;
}
