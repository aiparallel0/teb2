#ifndef LLM_H
#define LLM_H

#include "core/types.h"
#include "core/errors.h"

/* LLM gateway — thin wrapper over send_request() */

typedef struct {
    char model[64];
    char system[512];
    char user[1024];
} LlmReq;

typedef struct {
    Err  err;
    char reply[2048];
} LlmReply;

LlmReply llm_call(LlmReq req, Config *cfg)
    __attribute__((warn_unused_result));

#endif /* LLM_H */
