#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include "core/log.h"

/*
 * Structured JSON logger.
 * Every line is a self-contained JSON object so downstream log
 * aggregators (ELK, Loki, CloudWatch) can parse without config.
 * Thread-safe: fprintf to stderr is line-buffered by POSIX.
 */

static void esc_log(const char *src, char *dst, size_t dsz)
{
    size_t i = 0, o = 0;
    if (!src || !dst || dsz == 0) { if (dst && dsz) dst[0] = '\0'; return; }
    while (src[i] && o + 6 < dsz) {
        unsigned char c = (unsigned char)src[i];
        if (c == '"')       { dst[o++] = '\\'; dst[o++] = '"'; }
        else if (c == '\\') { dst[o++] = '\\'; dst[o++] = '\\'; }
        else if (c == '\n') { dst[o++] = '\\'; dst[o++] = 'n'; }
        else if (c == '\r') { dst[o++] = '\\'; dst[o++] = 'r'; }
        else if (c < 0x20)  { i++; continue; }
        else dst[o++] = (char)c;
        i++;
    }
    dst[o] = '\0';
}

static void emit(const char *level, const char *comp, const char *fmt,
                 va_list ap)
{
    char msg[512], emsg[1024], ecomp[64];
    vsnprintf(msg, sizeof(msg), fmt, ap);
    esc_log(msg, emsg, sizeof(emsg));
    esc_log(comp, ecomp, sizeof(ecomp));
    fprintf(stderr,
            "{\"ts\":%ld,\"level\":\"%s\",\"comp\":\"%s\",\"msg\":\"%s\"}\n",
            (long)time(NULL), level, ecomp, emsg);
}

void teb_log_info(const char *comp, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); emit("info", comp, fmt, ap); va_end(ap);
}

void teb_log_warn(const char *comp, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); emit("warn", comp, fmt, ap); va_end(ap);
}

void teb_log_error(const char *comp, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt); emit("error", comp, fmt, ap); va_end(ap);
}
