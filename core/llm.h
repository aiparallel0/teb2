#ifndef LLM_H
#define LLM_H

#include "core/types.h"
#include "core/errors.h"

/* LLM gateway — thin wrapper over send_request() */

typedef struct {
    char model[64];
    char prompt_name[64];   /* registry key, e.g. "decompose" */
    char user[1024];        /* untrusted user text; caller must sanitize */
    char context[1536];     /* trusted extra context (snippets, prior learnings) */
    int  want_json;         /* 1 ⇒ request JSON object response_format */
    int  max_tokens;        /* per-call override; 0 ⇒ use cfg->llm_max_tokens */
} LlmReq;

typedef struct {
    Err  err;
    int  status;            /* upstream HTTP status (0 if transport error) */
    int  total_tokens;      /* parsed from usage.total_tokens (0 if absent) */
    char reply[2048];
} LlmReply;

LlmReply llm_call(LlmReq req, Config *cfg)
    __attribute__((warn_unused_result));

#endif /* LLM_H */
