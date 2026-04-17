#ifndef CORE_SANITIZE_H
#define CORE_SANITIZE_H

#include <stddef.h>

/*
 * Prompt-injection hardening. sanitize_untrusted wraps user-supplied
 * text in <untrusted_input>…</untrusted_input> after stripping ASCII
 * control characters (0x00–0x1F except TAB/LF), zero-width and
 * direction-override sequences in the UTF-8 BOM/RLO/LRO/PDI range,
 * and neutralising any closing </untrusted_input> tag inside the
 * body. The guardrails prompt tells every model that content inside
 * this envelope is data, never instructions.
 *
 * Returns the number of bytes written to dst (excluding the NUL), or
 * 0 on invalid input (NULL dst, dsz < 64).
 */
size_t sanitize_untrusted(const char *src, char *dst, size_t dsz);

#endif /* CORE_SANITIZE_H */
