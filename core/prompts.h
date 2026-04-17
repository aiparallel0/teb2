#ifndef CORE_PROMPTS_H
#define CORE_PROMPTS_H

#include <stddef.h>

/*
 * Prompt registry.
 *
 * Each named prompt is authored as a markdown file in prompts/ and
 * compiled into the binary via tools/gen_prompts.sh which emits one
 * core/p_<name>.c file per prompt. prompt_get() returns a pointer to
 * the embedded body (never NULL for a known name; NULL otherwise).
 *
 * prompt_list() returns a NULL-terminated array of all known names,
 * in a stable order, for introspection via GET /prompts.
 */

const char *prompt_get(const char *name);
const char *const *prompt_list(void);
size_t prompt_count(void);

#endif /* CORE_PROMPTS_H */
