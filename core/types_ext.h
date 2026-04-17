#ifndef TYPES_EXT_H
#define TYPES_EXT_H

#include <stdint.h>
#include "core/errors.h"

/* --- moved from types.h --- */
typedef struct { int64_t id; int64_t task_id; int64_t run_at; } SchedEntry;
typedef struct { int64_t id; int64_t task_id; int64_t run_at; int limit; } SchedQuery;
typedef struct { Err err; SchedEntry entry; } SchedResult;

typedef struct { int64_t id; char user_id[64]; int64_t limit_cents; int64_t spent_cents; } Budget;
typedef struct { int64_t id; char user_id[64]; int64_t amount_cents; } BudgetQuery;
typedef struct { Err err; Budget budget; } BudgetResult;

typedef struct { int64_t id; char agent[64]; char key[128]; char val[512]; int64_t ts; } MemEntry;
typedef struct { int64_t id; char agent[64]; char key[128]; char val[512]; int limit; } MemQuery;
typedef struct { Err err; MemEntry entry; } MemResult;
typedef struct { Err err; MemEntry rows[8]; int count; } MemListResult;

/* --- approvals (finance HITL) --- */
typedef struct {
    int64_t id; char user_id[64]; char kind[32]; int64_t amount_cents;
    char    payload[512]; char status[16]; char risk[16]; int64_t created_at;
} Approval;
typedef struct {
    int64_t id; char user_id[64]; char kind[32]; int64_t amount_cents;
    char    payload[512]; char status[16]; char risk[16]; int limit;
} ApprovalQuery;
typedef struct { Err err; Approval ap; Approval rows[16]; int count; } ApprovalResult;

/* --- enterprise --- */
typedef struct {
    int64_t id; char name[128]; char domain[128]; int64_t created_at;
} Org;
typedef struct { int64_t id; char name[128]; char domain[128]; int limit; } OrgQuery;
typedef struct { Err err; Org org; } OrgResult;

typedef struct {
    int64_t id; int64_t org_id; char provider[64]; char endpoint[256];
    char cert[512];
} SsoConfig;
typedef struct { int64_t id; int64_t org_id; char provider[64]; } SsoQuery;
typedef struct { Err err; SsoConfig sso; } SsoResult;

typedef struct { int64_t id; int64_t org_id; char cidr[64]; } IpEntry;
typedef struct { int64_t id; int64_t org_id; char cidr[64]; int limit; } IpQuery;
typedef struct { Err err; IpEntry ip; } IpResult;

/* --- analytics --- */
typedef struct {
    int64_t id; int64_t goal_id; int pct; int64_t created_at;
} ProgressSnapshot;
typedef struct { int64_t id; int64_t goal_id; int pct; int limit; } SnapQuery;
typedef struct { Err err; ProgressSnapshot snap; } SnapResult;

typedef struct {
    int64_t id; int64_t task_id; char user_id[64]; int minutes;
    int64_t created_at;
} TimeEntry;
typedef struct {
    int64_t id; int64_t task_id; char user_id[64]; int minutes; int limit;
} TimeQuery;
typedef struct { Err err; TimeEntry entry; } TimeResult;

typedef struct {
    int64_t id; int64_t goal_id; int64_t value_cents; int64_t cost_cents;
    int64_t created_at;
} RoiMetric;
typedef struct { int64_t id; int64_t goal_id; int limit; } RoiQuery;
typedef struct { Err err; RoiMetric roi; } RoiResult;

/* --- gamification --- */
typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128];
    int64_t created_at;
} XpEvent;
typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128]; int limit;
    int64_t cursor;
} XpQuery;
typedef struct { Err err; XpEvent xp; } XpResult;

typedef struct {
    int64_t id; char user_id[64]; int current; int longest; int64_t updated_at;
} Streak;
typedef struct { int64_t id; char user_id[64]; } StreakQuery;
typedef struct { Err err; Streak streak; } StreakResult;

typedef struct {
    int64_t id; char name[128]; int target; int64_t created_at;
} Challenge;
typedef struct { int64_t id; char name[128]; int target; int limit; } ChallengeQuery;
typedef struct { Err err; Challenge ch; } ChallengeResult;

/* --- community --- */
typedef struct {
    int64_t id; char user_id[64]; char title[256]; char body[512];
    int64_t created_at;
} BlogPost;
typedef struct {
    int64_t id; char user_id[64]; char title[256]; char body[512]; int limit;
} BlogQuery;
typedef struct { Err err; BlogPost post; } BlogResult;

typedef struct {
    int64_t id; char user_id[64]; char feature[128]; int64_t created_at;
} FeatureVote;
typedef struct { int64_t id; char user_id[64]; char feature[128]; } VoteQuery;
typedef struct { Err err; FeatureVote vote; int count; } VoteResult;

/* --- assets --- */
typedef struct {
    int64_t id; char user_id[64]; char filename[256];
    char mime_type[128]; int64_t size_bytes; char path[512]; int64_t created_at;
} Asset;
typedef struct {
    int64_t id; char user_id[64]; char filename[256];
    char mime_type[128]; int64_t size_bytes; char path[512]; int limit;
} AssetQuery;
typedef struct { Err err; Asset asset; } AssetResult;

/* --- workflow --- */
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

/* --- search --- */
typedef struct { char query[256]; char entity[32]; int limit; } SearchQuery;
typedef struct { char entity[32]; int64_t entity_id; char snippet[256]; } SearchHit;
typedef struct { Err err; SearchHit hits[32]; int count; } SearchResult;

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

#endif /* TYPES_EXT_H */
