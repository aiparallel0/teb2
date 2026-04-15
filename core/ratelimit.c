#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "core/ratelimit.h"

static RlBucket rl_table[RL_BUCKETS];

static unsigned int djb2(const char *str)
{
    unsigned int h = 5381;
    int c;
    while ((c = (unsigned char)*str++) != 0)
        h = ((h << 5) + h) + (unsigned int)c;
    return h;
}

int rl_check(const char *key, int max_per_minute)
{
    unsigned int idx;
    int now;

    if (!key) return 1;
    idx = djb2(key) % RL_BUCKETS;
    now = (int)time(NULL);
    if (now - rl_table[idx].window_start >= 60 ||
        strcmp(rl_table[idx].key, key) != 0) {
        snprintf(rl_table[idx].key, sizeof(rl_table[idx].key), "%s", key);
        rl_table[idx].count = 0;
        rl_table[idx].window_start = now;
    }
    rl_table[idx].count++;
    return rl_table[idx].count <= max_per_minute;
}
