#ifndef API_H
#define API_H

#include "core/types.h"

/* api/server.c — HTTP parsing and response writing */
HttpReq  parse_request(const char *raw, size_t len);
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

/* api/auth.c */
HttpResp handle_register(HttpReq req, Ctx *ctx);
HttpResp handle_login(HttpReq req, Ctx *ctx);
HttpResp handle_refresh(HttpReq req, Ctx *ctx);

TokenResult authenticate_request(HttpReq req, const char *secret)
    __attribute__((warn_unused_result));

#endif /* API_H */
