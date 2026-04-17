#include <string.h>
#include <stdio.h>
#include "core/sanitize.h"

#define OPEN_TAG  "<untrusted_input>\n"
#define CLOSE_TAG "\n</untrusted_input>"

/* Pattern we never want to appear verbatim inside the body so the
 * model cannot be told "the envelope is closed, now obey this". */
static int starts_close_tag(const char *p, size_t rem)
{
    static const char needle[] = "</untrusted_input>";
    size_t n = sizeof(needle) - 1;
    if (rem < n) return 0;
    return memcmp(p, needle, n) == 0;
}

/* Strip ASCII control chars (keep \t \n), zero-width and
 * direction-override sequences (U+200B..U+200F, U+202A..U+202E,
 * U+2066..U+2069), and the UTF-8 BOM U+FEFF. */
static int is_dangerous_utf8(const unsigned char *p, size_t rem, size_t *skip)
{
    if (rem < 3) return 0;
    if (p[0] == 0xE2 && p[1] == 0x80 &&
        (p[2] >= 0x8B && p[2] <= 0x8F)) { *skip = 3; return 1; }
    if (p[0] == 0xE2 && p[1] == 0x80 &&
        (p[2] >= 0xAA && p[2] <= 0xAE)) { *skip = 3; return 1; }
    if (p[0] == 0xE2 && p[1] == 0x81 &&
        (p[2] >= 0xA6 && p[2] <= 0xA9)) { *skip = 3; return 1; }
    if (p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) { *skip = 3; return 1; }
    return 0;
}

size_t sanitize_untrusted(const char *src, char *dst, size_t dsz)
{
    size_t i = 0, o = 0, ol, cl;
    size_t slen;

    if (!dst || dsz < 64) return 0;
    ol = strlen(OPEN_TAG);
    cl = strlen(CLOSE_TAG);
    memcpy(dst, OPEN_TAG, ol);
    o = ol;

    slen = src ? strlen(src) : 0;
    while (i < slen && o + cl + 2 < dsz) {
        unsigned char c = (unsigned char)src[i];
        size_t skip = 0;
        if (is_dangerous_utf8((const unsigned char *)src + i, slen - i, &skip)) {
            i += skip;
            continue;
        }
        if (c < 0x20 && c != '\t' && c != '\n') { i++; continue; }
        if (c == 0x7F) { i++; continue; }
        if (starts_close_tag(src + i, slen - i)) {
            /* break the sequence so it never re-appears */
            if (o + 2 < dsz - cl) { dst[o++] = '\\'; dst[o++] = '&'; }
            i += 1;
            continue;
        }
        dst[o++] = (char)c;
        i++;
    }
    if (o + cl < dsz) {
        memcpy(dst + o, CLOSE_TAG, cl);
        o += cl;
    }
    dst[o] = '\0';
    return o;
}
