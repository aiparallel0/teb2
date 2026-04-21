#ifndef API_H
#define API_H
#include <sys/types.h>
#include "core/types.h"

/* HttpReq.body max is 8191 bytes; larger payloads are truncated. */
/* api/server.c */
HttpReq  parse_request(const char *raw, size_t len);
HttpResp dispatch(HttpReq req, Ctx *ctx);
void     write_response(int fd, HttpResp resp);

/* api/limits.c */
void     socket_set_deadlines(int fd, int read_sec, int write_sec);
ssize_t  slowloris_read(int fd, char *buf, size_t cap, int deadline_sec) __attribute__((warn_unused_result));

/* api/goals.c */
HttpResp handle_goal_create(HttpReq req, Ctx *ctx);
HttpResp handle_goal_get(HttpReq req, Ctx *ctx);
HttpResp handle_goal_list(HttpReq req, Ctx *ctx);
HttpResp handle_goal_decompose(HttpReq req, Ctx *ctx);

/* api/tasks.c */
HttpResp handle_task_create(HttpReq req, Ctx *ctx);
HttpResp handle_task_update(HttpReq req, Ctx *ctx);
HttpResp handle_task_execute(HttpReq req, Ctx *ctx);
HttpResp handle_task_status(HttpReq req, Ctx *ctx);
HttpResp handle_task_list(HttpReq req, Ctx *ctx);

/* api/outcomes.c */
HttpResp handle_outcome_store(HttpReq req, Ctx *ctx);
HttpResp handle_outcome_get(HttpReq req, Ctx *ctx);

/* api/nudges.c */
HttpResp handle_nudge_store(HttpReq req, Ctx *ctx);
HttpResp handle_nudge_get(HttpReq req, Ctx *ctx);

/* api/learn.c */
HttpResp handle_learn_store(HttpReq req, Ctx *ctx);
HttpResp handle_learn_get(HttpReq req, Ctx *ctx);

/* api/exec.c */
HttpResp handle_exec_run(HttpReq req, Ctx *ctx);

/* api/decompose.c */
HttpResp handle_decompose_run(HttpReq req, Ctx *ctx);

/* api/approvals.c */
HttpResp handle_approval_list(HttpReq req, Ctx *ctx);
HttpResp handle_approval_update(HttpReq req, Ctx *ctx);

/* api/prompts.c */
HttpResp handle_prompt_list(HttpReq req, Ctx *ctx);
HttpResp handle_prompt_get(HttpReq req, Ctx *ctx);

/* api/schedules.c */
HttpResp handle_sched_create(HttpReq req, Ctx *ctx);
HttpResp handle_sched_list(HttpReq req, Ctx *ctx);

/* api/budgets.c */
HttpResp handle_budget_create(HttpReq req, Ctx *ctx);
HttpResp handle_budget_get(HttpReq req, Ctx *ctx);
HttpResp handle_spend_record(HttpReq req, Ctx *ctx);

/* api/auth.c */
HttpResp handle_register(HttpReq req, Ctx *ctx);
HttpResp handle_login(HttpReq req, Ctx *ctx);
HttpResp handle_refresh(HttpReq req, Ctx *ctx);
void     auth_email_normalize(char *s);

TokenResult authenticate_request(HttpReq req, const char *secret) __attribute__((warn_unused_result));

/* api/forgot.c */
HttpResp handle_forgot(HttpReq req, Ctx *ctx);
HttpResp handle_reset(HttpReq req, Ctx *ctx);

/* api/collab.c */
HttpResp handle_ws_create(HttpReq req, Ctx *ctx);
HttpResp handle_ws_get(HttpReq req, Ctx *ctx);
HttpResp handle_collab_add(HttpReq req, Ctx *ctx);
HttpResp handle_collab_list(HttpReq req, Ctx *ctx);
HttpResp handle_chat_send(HttpReq req, Ctx *ctx);
HttpResp handle_chat_list(HttpReq req, Ctx *ctx);

/* api/integrations.c */
HttpResp handle_integ_toggle(HttpReq req, Ctx *ctx);
HttpResp handle_integ_get(HttpReq req, Ctx *ctx);
HttpResp handle_webhook_route(HttpReq req, Ctx *ctx);

/* api/enterprise.c */
HttpResp handle_org_create(HttpReq req, Ctx *ctx);
HttpResp handle_org_get(HttpReq req, Ctx *ctx);
HttpResp handle_sso_validate(HttpReq req, Ctx *ctx);
HttpResp handle_ip_check(HttpReq req, Ctx *ctx);

/* api/analytics.c */
HttpResp handle_snap_store(HttpReq req, Ctx *ctx);
HttpResp handle_roi_get(HttpReq req, Ctx *ctx);
HttpResp handle_time_store(HttpReq req, Ctx *ctx);

/* api/gamification.c */
HttpResp handle_xp_credit(HttpReq req, Ctx *ctx);
HttpResp handle_streak_get(HttpReq req, Ctx *ctx);
HttpResp handle_leaderboard(HttpReq req, Ctx *ctx);

/* api/community.c */
HttpResp handle_blog_store(HttpReq req, Ctx *ctx);
HttpResp handle_blog_get(HttpReq req, Ctx *ctx);
HttpResp handle_vote_upsert(HttpReq req, Ctx *ctx);

/* api/sse.c */
HttpResp handle_sse_subscribe(HttpReq req, Ctx *ctx);

/* api/routes.c */
HttpResp dispatch_ext(HttpReq req, Ctx *ctx);

/* api/assets.c */
HttpResp handle_asset_upload(HttpReq req, Ctx *ctx);
HttpResp handle_asset_get(HttpReq req, Ctx *ctx);

/* api/notify.c */
HttpResp handle_notify_email(HttpReq req, Ctx *ctx);

/* api/workflow.c */
HttpResp handle_run_create(HttpReq req, Ctx *ctx);

/* api/run_cancel.c */
HttpResp handle_run_get(HttpReq req, Ctx *ctx);
HttpResp handle_run_cancel(HttpReq req, Ctx *ctx);

/* api/prompt_edit.c */
HttpResp handle_prompt_edit(HttpReq req, Ctx *ctx);
HttpResp handle_prompt_delete(HttpReq req, Ctx *ctx);

/* api/search.c */
HttpResp handle_search(HttpReq req, Ctx *ctx);

/* api/metrics.c */
void     metrics_inc(const char *method, const char *path, int status);
void     metrics_observe_llm(long tokens, long latency_ms);
HttpResp handle_healthz(HttpReq req, Ctx *ctx);
HttpResp handle_metrics(HttpReq req, Ctx *ctx);

/* api/oauth.c */
HttpResp handle_oauth_redirect(HttpReq req, Ctx *ctx);
HttpResp handle_oauth_callback(HttpReq req, Ctx *ctx);

/* api/static.c */
HttpResp handle_ui_index(HttpReq req, Ctx *ctx);
HttpResp handle_ui_appjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_style(HttpReq req, Ctx *ctx);
HttpResp handle_ui_goalsjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_tasksjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_financejs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_collabjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_dashjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_enterprisejs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_analyticsjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_approvalsjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_promptsjs(HttpReq req, Ctx *ctx);
HttpResp handle_ui_workflowsjs(HttpReq req, Ctx *ctx);

/* api/server_loop.c */
void server_install_signals(void);
int  server_start(Config *cfg, Db *db) __attribute__((warn_unused_result));

#endif /* API_H */
