#ifndef API_H
#define API_H

#include "core/types.h"

/* api/server.c */
HttpReq  parse_request(const char *raw, size_t len);
HttpResp dispatch(HttpReq req, Ctx *ctx);
void     write_response(int fd, HttpResp resp);

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

TokenResult authenticate_request(HttpReq req, const char *secret)
    __attribute__((warn_unused_result));

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
HttpResp handle_run_get(HttpReq req, Ctx *ctx);

/* api/search.c */
HttpResp handle_search(HttpReq req, Ctx *ctx);

/* api/metrics.c */
void     metrics_inc(const char *method, const char *path, int status);
HttpResp handle_healthz(HttpReq req, Ctx *ctx);
HttpResp handle_metrics(HttpReq req, Ctx *ctx);

/* api/oauth.c */
HttpResp handle_oauth_redirect(HttpReq req, Ctx *ctx);
HttpResp handle_oauth_callback(HttpReq req, Ctx *ctx);

#endif /* API_H */
