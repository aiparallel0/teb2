#ifndef TYPES_CONTENT_H
#define TYPES_CONTENT_H

#include <stdint.h>
#include "core/errors.h"

/* --- community --- */
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

/* --- assets --- */
typedef struct {
    int64_t id; char user_id[64]; char filename[256];
    char mime_type[128]; int64_t size_bytes; char path[512]; int64_t created_at;
} Asset;
typedef struct {
    int64_t id; char user_id[64]; char filename[256];
    char mime_type[128]; int64_t size_bytes; char path[512]; int limit;
} AssetQuery;
typedef struct { Err err; Asset asset; } AssetResult;

/* --- search --- */
typedef struct { char query[256]; char entity[32]; int limit; } SearchQuery;
typedef struct { char entity[32]; int64_t entity_id; char snippet[256]; } SearchHit;
typedef struct { Err err; SearchHit hits[32]; int count; } SearchResult;

#endif /* TYPES_CONTENT_H */
