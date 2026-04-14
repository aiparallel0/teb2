#ifndef API_H
#define API_H

#include "core/types.h"

HttpResp handle_goal_create(HttpReq req, Ctx *ctx);
HttpResp handle_goal_get(HttpReq req, Ctx *ctx);
HttpResp handle_goal_list(HttpReq req, Ctx *ctx);
HttpResp handle_goal_decompose(HttpReq req, Ctx *ctx);

HttpResp handle_task_create(HttpReq req, Ctx *ctx);
HttpResp handle_task_update(HttpReq req, Ctx *ctx);
HttpResp handle_task_execute(HttpReq req, Ctx *ctx);
HttpResp handle_task_status(HttpReq req, Ctx *ctx);
HttpResp handle_task_list(HttpReq req, Ctx *ctx);

HttpResp handle_outcome_store(HttpReq req, Ctx *ctx);
HttpResp handle_outcome_get(HttpReq req, Ctx *ctx);

HttpResp handle_register(HttpReq req, Ctx *ctx);
HttpResp handle_login(HttpReq req, Ctx *ctx);
HttpResp handle_refresh(HttpReq req, Ctx *ctx);

TokenResult authenticate_request(HttpReq req, const char *secret)
    __attribute__((warn_unused_result));

#endif /* API_H */
