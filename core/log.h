#ifndef CORE_LOG_H
#define CORE_LOG_H

/* Structured JSON logging to stderr.
 * Each call emits one line: {"ts":EPOCH,"level":"…","comp":"…","msg":"…"} */

void teb_log_info(const char *component, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void teb_log_warn(const char *component, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void teb_log_error(const char *component, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

#endif /* CORE_LOG_H */
