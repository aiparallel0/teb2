#ifndef AGENTS_UTIL_H
#define AGENTS_UTIL_H

#include "core/types.h"

AgentMsg agent_make_result(AgentMsg src, Err err, const char *detail);
int payload_has_content(const char *payload);

#endif /* AGENTS_UTIL_H */
