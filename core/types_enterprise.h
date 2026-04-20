#ifndef TYPES_ENTERPRISE_H
#define TYPES_ENTERPRISE_H

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

#endif /* TYPES_ENTERPRISE_H */
