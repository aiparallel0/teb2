#ifndef TYPES_EXT_H
#define TYPES_EXT_H

#include <stdint.h>
#include "core/errors.h"

typedef struct {
    int64_t id; char name[128]; char domain[128]; int64_t created_at;
} Org;

typedef struct { int64_t id; char name[128]; char domain[128]; int limit; } OrgQuery;
typedef struct { Err err; Org org; } OrgResult;

typedef struct {
    int64_t id; int64_t org_id; char provider[64]; char endpoint[256];
    char cert[512];
} SsoConfig;

typedef struct { int64_t id; int64_t org_id; char provider[64]; } SsoQuery;
typedef struct { Err err; SsoConfig sso; } SsoResult;

typedef struct { int64_t id; int64_t org_id; char cidr[64]; } IpEntry;
typedef struct { int64_t id; int64_t org_id; char cidr[64]; int limit; } IpQuery;
typedef struct { Err err; IpEntry ip; } IpResult;

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

typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128];
    int64_t created_at;
} XpEvent;

typedef struct {
    int64_t id; char user_id[64]; int amount; char reason[128]; int limit;
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

typedef struct {
    int64_t id; char user_id[64]; char title[256]; char body[512];
    int64_t created_at;
} BlogPost;

typedef struct {
    int64_t id; char user_id[64]; char title[256]; char body[512]; int limit;
} BlogQuery;

typedef struct { Err err; BlogPost post; } BlogResult;

typedef struct {
    int64_t id; char user_id[64]; char feature[128]; int64_t created_at;
} FeatureVote;

typedef struct { int64_t id; char user_id[64]; char feature[128]; } VoteQuery;
typedef struct { Err err; FeatureVote vote; int count; } VoteResult;

#endif /* TYPES_EXT_H */
