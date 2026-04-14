#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "core/types.h"
#include "core/errors.h"
#include "agents/channel.h"
#include "db/db.h"
#include "exec/exec.h"

/* All monetary amounts are int64_t cents. No floating point. */
#define TIER_AUTO     100LL    /* under 1.00: auto-approve */
#define TIER_CONFIRM  10000LL  /* under 100.00: single confirmation */

static int64_t parse_cents(const char *payload)
{
    int64_t cents = 0;
    const char *p = payload;
    while (*p >= '0' && *p <= '9')
        cents = cents * 10 + (*p++ - '0');
    return cents;
}

static AgentMsg make_result(AgentMsg src, Err err, const char *detail)
{
    AgentMsg out;
    memset(&out, 0, sizeof(out));
    out.tag = MSG_RESULT;
    out.id  = src.id;
    snprintf(out.user_id, sizeof(out.user_id), "%s", src.user_id);
    snprintf(out.payload, sizeof(out.payload), "%s", detail);
    out.err = err;
    return out;
}

static AgentMsg approve(AgentMsg msg)
{
    return make_result(msg, ERR_OK, "approved");
}

static AgentMsg deny(AgentMsg msg)
{
    return make_result(msg, ERR_AUTH, "denied:requires_authorization");
}

static AgentMsg needs_confirm(AgentMsg msg)
{
    return make_result(msg, ERR_AUTH, "pending:awaiting_confirmation");
}

AgentMsg finance_handle(AgentMsg msg)
{
    int64_t cents;

    if (msg.tag != MSG_FINANCE_REQ)
        return make_result(msg, ERR_UNKNOWN, "not_a_finance_request");

    cents = parse_cents(msg.payload);

    switch (cents < TIER_AUTO ? 0
          : cents < TIER_CONFIRM ? 1
          : 2) {
    case 0:  return approve(msg);
    case 1:  return needs_confirm(msg);
    default: return deny(msg);
    }
}
