#include <string.h>
#include "core/types.h"
#include <stdio.h>
#include "api/api.h"

HttpResp dispatch(HttpReq req, Ctx *ctx)
{
    const char *p = req.path;

    if (strncmp(p, "/goals", 6) == 0) {
        if (strcmp(req.method, "POST") == 0)   return handle_goal_create(req, ctx);
        if (strcmp(req.method, "GET") == 0)    return handle_goal_list(req, ctx);
    }
    if (strncmp(p, "/goal/", 6) == 0) {
        if (strcmp(req.method, "GET") == 0)    return handle_goal_get(req, ctx);
        if (strcmp(req.method, "POST") == 0)   return handle_goal_decompose(req, ctx);
    }
    if (strncmp(p, "/decompose/", 11) == 0 && strcmp(req.method, "POST") == 0)
        return handle_decompose_run(req, ctx);

    if (strncmp(p, "/tasks/goal/", 12) == 0 && strcmp(req.method, "GET") == 0)
        return handle_task_list(req, ctx);
    if (strcmp(p, "/tasks") == 0 && strcmp(req.method, "POST") == 0)
        return handle_task_create(req, ctx);
    if (strncmp(p, "/tasks/", 7) == 0) {
        if (strcmp(req.method, "PUT") == 0)    return handle_task_update(req, ctx);
        if (strcmp(req.method, "POST") == 0)   return handle_task_execute(req, ctx);
        if (strcmp(req.method, "GET") == 0)    return handle_task_status(req, ctx);
    }
    if (strncmp(p, "/exec/", 6) == 0 && strcmp(req.method, "POST") == 0)
        return handle_exec_run(req, ctx);

    if (strcmp(p, "/auth/register") == 0)      return handle_register(req, ctx);
    if (strcmp(p, "/auth/login")    == 0)      return handle_login(req, ctx);
    if (strcmp(p, "/auth/refresh")  == 0)      return handle_refresh(req, ctx);

    if (strcmp(p, "/outcomes") == 0 && strcmp(req.method, "POST") == 0)
        return handle_outcome_store(req, ctx);
    if (strncmp(p, "/outcome/", 9) == 0 && strcmp(req.method, "GET") == 0)
        return handle_outcome_get(req, ctx);

    if (strcmp(p, "/schedules") == 0 && strcmp(req.method, "POST") == 0)
        return handle_sched_create(req, ctx);
    if (strncmp(p, "/schedules/", 11) == 0 && strcmp(req.method, "GET") == 0)
        return handle_sched_list(req, ctx);

    if (strcmp(p, "/budgets") == 0 && strcmp(req.method, "POST") == 0)
        return handle_budget_create(req, ctx);
    if (strcmp(p, "/budgets") == 0 && strcmp(req.method, "GET") == 0)
        return handle_budget_get(req, ctx);
    if (strcmp(p, "/spending") == 0 && strcmp(req.method, "POST") == 0)
        return handle_spend_record(req, ctx);

    if (strcmp(p, "/nudges") == 0 && strcmp(req.method, "POST") == 0)
        return handle_nudge_create(req, ctx);
    if (strcmp(p, "/nudges") == 0 && strcmp(req.method, "GET") == 0)
        return handle_nudge_list(req, ctx);

    {
        HttpResp r;
        memset(&r, 0, sizeof(r));
        r.status   = 404;
        r.body_len = (size_t)snprintf(r.body, sizeof(r.body),
                                      "{\"error\":\"not_found\"}");
        snprintf(r.content_type, sizeof(r.content_type), "%s",
                 "application/json");
        return r;
    }
}
