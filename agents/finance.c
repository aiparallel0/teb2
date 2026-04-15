#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "agents/util.h"
#include "db/db.h"
#include "exec/exec.h"

/* All monetary amounts are int64_t cents. No floating point. */
#define TIER_AUTO     100LL    /* under 1.00: auto-approve */
#define TIER_CONFIRM  10000LL  /* under 100.00: single confirmation */

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
    return agent_make_result(msg, ERR_AUTH,
                             "denied:requires_authorization");
}

static AgentMsg needs_confirm(AgentMsg msg)
{
    return agent_make_result(msg, ERR_AUTH,
                             "pending:awaiting_confirmation");
}

AgentMsg finance_handle(AgentMsg msg)
{
    int64_t cents;

    if (msg.tag != MSG_FINANCE_REQ)
        return agent_make_result(msg, ERR_UNKNOWN,
                                 "not_a_finance_request");

    cents = parse_cents(msg.payload);

    switch (cents < TIER_AUTO ? 0
          : cents < TIER_CONFIRM ? 1
          : 2) {
    case 0:  return approve(msg);
    case 1:  return needs_confirm(msg);
    default: return deny(msg);
    }
}
