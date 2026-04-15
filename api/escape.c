#include <string.h>
#include <stdio.h>
#include "api/escape.h"

/*
 * json_escape — escape src into dst for safe JSON embedding.
 * In:  src (NUL-terminated), dst buffer, dstsz capacity.
 * Out: bytes written (excluding NUL). 0 if src is NULL/empty or dstsz < 2.
 */
size_t json_escape(const char *src, char *dst, size_t dstsz)
{
    size_t si, di;

    if (!src || !dst || dstsz < 2) return 0;
    di = 0;
    for (si = 0; src[si] != '\0'; si++) {
        unsigned char c = (unsigned char)src[si];
        const char *esc = NULL;
        char ubuf[8];
        size_t elen;

        switch (c) {
        case '"':  esc = "\\\""; break;
        case '\\': esc = "\\\\"; break;
        case '\b': esc = "\\b";  break;
        case '\f': esc = "\\f";  break;
        case '\n': esc = "\\n";  break;
        case '\r': esc = "\\r";  break;
        case '\t': esc = "\\t";  break;
        case '<':  esc = "\\u003c"; break;
        case '>':  esc = "\\u003e"; break;
        case '&':  esc = "\\u0026"; break;
        default:
            if (c < 0x20) {
                snprintf(ubuf, sizeof(ubuf), "\\u%04x", c);
                esc = ubuf;
            }
            break;
        }
        if (esc) {
            elen = strlen(esc);
            if (di + elen >= dstsz) break;
            memcpy(dst + di, esc, elen);
            di += elen;
        } else {
            if (di + 1 >= dstsz) break;
            dst[di++] = (char)c;
        }
    }
    dst[di] = '\0';
    return di;
}
