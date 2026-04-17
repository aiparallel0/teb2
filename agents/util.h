#ifndef AGENTS_UTIL_H
#define AGENTS_UTIL_H

#include <stddef.h>
#include <stdint.h>
#include "core/types.h"

AgentMsg agent_make_result(AgentMsg src, Err err, const char *detail);
int payload_has_content(const char *payload);

/* Minimal JSON field extractors over a single object buffer. They
 * are permissive by design — scan for "key" then a separator, then
 * read the value. Suitable for the small, well-shaped envelopes our
 * prompts produce; not a replacement for a real parser. */
int     json_str (const char *obj, const char *key, char *out, size_t osz);
int64_t json_int (const char *obj, const char *key); /* INT64_MIN on miss */
int     json_bool(const char *obj, const char *key); /* -1 miss, 0/1 otherwise */

#endif /* AGENTS_UTIL_H */
