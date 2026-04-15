#ifndef RATELIMIT_H
#define RATELIMIT_H

#define RL_BUCKETS 256

typedef struct { char key[128]; int count; int window_start; } RlBucket;

int rl_check(const char *key, int max_per_minute);

#endif /* RATELIMIT_H */
