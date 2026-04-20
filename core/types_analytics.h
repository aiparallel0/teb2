#ifndef TYPES_ANALYTICS_H
#define TYPES_ANALYTICS_H

#include <stdint.h>
#include "core/errors.h"

/* --- analytics (measure phase) --- */
typedef struct {
    int64_t id; int64_t goal_id; int pct; int64_t created_at;
} ProgressSnapshot;
typedef struct { int64_t id; int64_t goal_id; int pct; int limit; } SnapQuery;
typedef struct { Err err; ProgressSnapshot snap; } SnapResult;

typedef struct {
    int64_t id; int64_t task_id; char user_id[64]; int minutes;
    int64_t created_at;
} TimeEntry;
typedef struct {
    int64_t id; int64_t task_id; char user_id[64]; int minutes; int limit;
} TimeQuery;
typedef struct { Err err; TimeEntry entry; } TimeResult;

typedef struct {
    int64_t id; int64_t goal_id; int64_t value_cents; int64_t cost_cents;
    int64_t created_at;
} RoiMetric;
typedef struct { int64_t id; int64_t goal_id; int limit; } RoiQuery;
typedef struct { Err err; RoiMetric roi; } RoiResult;

/* --- gamification (measurement-adjacent) --- */
typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128];
    int64_t created_at;
} XpEvent;
typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128]; int limit;
    int64_t cursor;
} XpQuery;
typedef struct { Err err; XpEvent xp; } XpResult;

typedef struct {
    int64_t id; char user_id[64]; int current; int longest; int64_t updated_at;
} Streak;
typedef struct { int64_t id; char user_id[64]; } StreakQuery;
typedef struct { Err err; Streak streak; } StreakResult;

typedef struct {
    int64_t id; char name[128]; int target; int64_t created_at;
} Challenge;
typedef struct { int64_t id; char name[128]; int target; int limit; } ChallengeQuery;
typedef struct { Err err; Challenge ch; } ChallengeResult;

#endif /* TYPES_ANALYTICS_H */
