#ifndef TYPES_WORKFLOW_H
#define TYPES_WORKFLOW_H

#include <stdint.h>
#include "core/errors.h"

/* --- scheduling (workflow timing) --- */
typedef struct { int64_t id; int64_t task_id; int64_t run_at; } SchedEntry;
typedef struct { int64_t id; int64_t task_id; int64_t run_at; int limit; } SchedQuery;
typedef struct { Err err; SchedEntry entry; } SchedResult;

/* --- workflow runs + steps --- */
typedef struct {
    int64_t id; int64_t goal_id; char status[16];
    int64_t started_at; int64_t finished_at; char error_msg[256];
} WorkflowRun;
typedef struct {
    int64_t id; int64_t run_id; int step_index; char agent[32];
    char payload[512]; char status[16]; char result[512]; int64_t created_at;
} WorkflowStep;
typedef struct { int64_t id; int64_t goal_id; char status[16]; int limit; } RunQuery;
typedef struct { Err err; WorkflowRun run; } RunResult;
typedef struct {
    int64_t id; int64_t run_id; char agent[32]; char payload[512]; int limit;
} StepQuery;
typedef struct { Err err; WorkflowStep step; WorkflowStep steps[32]; int count; } StepResult;

#endif /* TYPES_WORKFLOW_H */
