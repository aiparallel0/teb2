#ifndef API_ESCAPE_H
#define API_ESCAPE_H

#include <stddef.h>

/*
 * Escape a string for safe embedding inside a JSON quoted value.
 * Handles: " \ control-chars and <>&  (XSS-safe in HTML contexts).
 * Returns bytes written (excluding NUL), or 0 on error / empty src.
 */
size_t json_escape(const char *src, char *dst, size_t dstsz);

#endif /* API_ESCAPE_H */
