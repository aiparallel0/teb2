#ifndef DB_H
#define DB_H

#include "core/types.h"
#include "core/errors.h"
#include "core/types_collab.h"
#include "core/types_ext.h"

Err      db_open(const char *path, Db *out)
    __attribute__((warn_unused_result));
Err      db_init_ext(Db *db)
    __attribute__((warn_unused_result));
void     db_close(Db *db);

GoalResult fetch_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult store_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult list_goals(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult update_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));

TaskResult fetch_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult store_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult list_tasks(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult update_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));

UserResult fetch_user(Db *db, UserQuery q) __attribute__((warn_unused_result));
UserResult store_user(Db *db, UserQuery q) __attribute__((warn_unused_result));

OutcomeResult store_outcome(Db *db, OutcomeQuery q) __attribute__((warn_unused_result));
OutcomeResult fetch_outcome(Db *db, OutcomeQuery q) __attribute__((warn_unused_result));

NudgeResult store_nudge(Db *db, NudgeQuery q) __attribute__((warn_unused_result));
NudgeResult fetch_nudge(Db *db, NudgeQuery q) __attribute__((warn_unused_result));

LearnResult store_learning(Db *db, LearnQuery q) __attribute__((warn_unused_result));
LearnResult fetch_learning(Db *db, LearnQuery q) __attribute__((warn_unused_result));
LearnListResult list_learnings(Db *db, const char *user_id, int limit)
    __attribute__((warn_unused_result));

SchedResult store_sched(Db *db, SchedQuery q) __attribute__((warn_unused_result));
SchedResult fetch_sched(Db *db, SchedQuery q) __attribute__((warn_unused_result));

BudgetResult store_budget(Db *db, BudgetQuery q) __attribute__((warn_unused_result));
BudgetResult fetch_budget(Db *db, BudgetQuery q) __attribute__((warn_unused_result));
BudgetResult record_spend(Db *db, BudgetQuery q) __attribute__((warn_unused_result));

MemResult store_mem(Db *db, MemQuery q) __attribute__((warn_unused_result));
MemResult fetch_mem(Db *db, MemQuery q) __attribute__((warn_unused_result));
MemListResult list_mem(Db *db, const char *agent_name, const char *key_prefix,
                       int limit) __attribute__((warn_unused_result));

/* approvals (finance HITL) */
ApprovalResult store_approval(Db *db, ApprovalQuery q) __attribute__((warn_unused_result));
ApprovalResult list_approvals(Db *db, ApprovalQuery q) __attribute__((warn_unused_result));
ApprovalResult update_approval(Db *db, ApprovalQuery q) __attribute__((warn_unused_result));

/* collab */
WsResult     store_ws(Db *db, WsQuery q) __attribute__((warn_unused_result));
WsResult     fetch_ws(Db *db, WsQuery q) __attribute__((warn_unused_result));
CollabResult store_collab(Db *db, CollabQuery q) __attribute__((warn_unused_result));
CollabResult list_collabs(Db *db, CollabQuery q) __attribute__((warn_unused_result));
ChatResult   store_chat(Db *db, ChatQuery q) __attribute__((warn_unused_result));
ChatResult   list_chats(Db *db, ChatQuery q) __attribute__((warn_unused_result));

/* integrations */
IntegResult store_integ(Db *db, IntegQuery q) __attribute__((warn_unused_result));
IntegResult fetch_integ(Db *db, IntegQuery q) __attribute__((warn_unused_result));
OAuthResult store_oauth(Db *db, OAuthQuery q) __attribute__((warn_unused_result));
OAuthResult fetch_oauth(Db *db, OAuthQuery q) __attribute__((warn_unused_result));

/* enterprise */
OrgResult store_org(Db *db, OrgQuery q) __attribute__((warn_unused_result));
OrgResult fetch_org(Db *db, OrgQuery q) __attribute__((warn_unused_result));
SsoResult store_sso(Db *db, SsoQuery q) __attribute__((warn_unused_result));
IpResult  check_ip(Db *db, IpQuery q) __attribute__((warn_unused_result));

/* analytics */
SnapResult store_snap(Db *db, SnapQuery q) __attribute__((warn_unused_result));
TimeResult store_time(Db *db, TimeQuery q) __attribute__((warn_unused_result));
RoiResult  fetch_roi(Db *db, RoiQuery q) __attribute__((warn_unused_result));

/* gamification */
XpResult     credit_xp(Db *db, XpQuery q) __attribute__((warn_unused_result));
StreakResult check_streak(Db *db, StreakQuery q) __attribute__((warn_unused_result));
XpResult     list_xp(Db *db, XpQuery q) __attribute__((warn_unused_result));

/* community */
BlogResult store_blog(Db *db, BlogQuery q) __attribute__((warn_unused_result));
BlogResult fetch_blog(Db *db, BlogQuery q) __attribute__((warn_unused_result));
VoteResult store_vote(Db *db, VoteQuery q) __attribute__((warn_unused_result));

/* assets */
AssetResult store_asset(Db *db, AssetQuery q) __attribute__((warn_unused_result));
AssetResult fetch_asset(Db *db, AssetQuery q) __attribute__((warn_unused_result));

/* workflow */
RunResult  store_run(Db *db, RunQuery q) __attribute__((warn_unused_result));
RunResult  fetch_run(Db *db, RunQuery q) __attribute__((warn_unused_result));
RunResult  update_run_status(Db *db, int64_t run_id,
               const char *status, const char *error_msg)
    __attribute__((warn_unused_result));
StepResult store_step(Db *db, StepQuery q) __attribute__((warn_unused_result));
StepResult list_steps(Db *db, int64_t run_id) __attribute__((warn_unused_result));
StepResult update_step_status(Db *db, int64_t step_id,
               const char *status, const char *result)
    __attribute__((warn_unused_result));

/* search */
SearchResult full_text_search(Db *db, SearchQuery q)
    __attribute__((warn_unused_result));
Err index_entity(Db *db, const char *entity,
                 int64_t entity_id, const char *content)
    __attribute__((warn_unused_result));

/* audit */
Err audit_log(Db *db, const char *user_id, const char *action,
              const char *path, int status, const char *src_ip)
    __attribute__((warn_unused_result));

#endif /* DB_H */
