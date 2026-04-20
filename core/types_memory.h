#ifndef TYPES_MEMORY_H
#define TYPES_MEMORY_H

#include <stdint.h>
#include "core/errors.h"

/* --- agent memory (key/value + timestamp, learn phase) --- */
typedef struct { int64_t id; char agent[64]; char key[128]; char val[512]; int64_t ts; } MemEntry;
typedef struct { int64_t id; char agent[64]; char key[128]; char val[512]; int limit; } MemQuery;
typedef struct { Err err; MemEntry entry; } MemResult;
typedef struct { Err err; MemEntry rows[8]; int count; } MemListResult;

/* --- task_plan: per-task DAG/HITL metadata from decompose/measure --- */
typedef struct {
    int64_t task_id;           /* FK -> tasks.id (PK of task_plan) */
    char    depends_on[128];   /* CSV of sibling indices from decompose */
    int     effort_minutes;
    int     est_cost_cents;
    int     requires_hitl;     /* 0/1 */
    char    success_criteria[256];
    char    next_action[16];   /* measure's next_action: done|retry|escalate */
    int     score_0_100;       /* latest measure score */
    int     attempts;          /* incremented on retry */
} TaskPlan;
typedef struct { int64_t task_id; TaskPlan plan; } TaskPlanQuery;
typedef struct { Err err; TaskPlan plan; } TaskPlanResult;

/* --- prompt_overrides: per-user runtime prompt customization --- */
typedef struct {
    int64_t id; char user_id[64]; char name[64]; char body[4096]; int64_t updated_at;
} PromptOverride;
typedef struct { int64_t id; char user_id[64]; char name[64]; char body[4096]; } OverrideQuery;
typedef struct { Err err; PromptOverride ov; } OverrideResult;

#endif /* TYPES_MEMORY_H */
