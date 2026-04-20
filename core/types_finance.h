#ifndef TYPES_FINANCE_H
#define TYPES_FINANCE_H

#include <stdint.h>
#include "core/errors.h"

/* --- budgets --- */
typedef struct { int64_t id; char user_id[64]; int64_t limit_cents; int64_t spent_cents; } Budget;
typedef struct { int64_t id; char user_id[64]; int64_t amount_cents; } BudgetQuery;
typedef struct { Err err; Budget budget; } BudgetResult;

/* --- approvals (finance HITL + workflow gates) --- */
typedef struct {
    int64_t id; char user_id[64]; char kind[32]; int64_t amount_cents;
    char    payload[512]; char status[16]; char risk[16]; int64_t created_at;
} Approval;
typedef struct {
    int64_t id; char user_id[64]; char kind[32]; int64_t amount_cents;
    char    payload[512]; char status[16]; char risk[16]; int limit;
} ApprovalQuery;
typedef struct { Err err; Approval ap; Approval rows[16]; int count; } ApprovalResult;

#endif /* TYPES_FINANCE_H */
