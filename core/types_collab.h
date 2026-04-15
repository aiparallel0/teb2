#ifndef TYPES_COLLAB_H
#define TYPES_COLLAB_H

#include <stdint.h>
#include "core/errors.h"

typedef struct {
    int64_t id; char owner_id[64]; char name[128]; int64_t created_at;
} Workspace;

typedef struct {
    int64_t id; char owner_id[64]; char name[128]; int limit;
} WsQuery;

typedef struct { Err err; Workspace ws; } WsResult;

typedef struct {
    int64_t id; int64_t ws_id; char user_id[64]; char role_name[32];
    int64_t created_at;
} Collaborator;

typedef struct {
    int64_t id; int64_t ws_id; char user_id[64]; char role_name[32]; int limit;
    int64_t cursor;
} CollabQuery;

typedef struct { Err err; Collaborator rows[16]; int count; } CollabResult;

typedef struct {
    int64_t id; int64_t ws_id; char sender_id[64]; char body[512];
    int64_t created_at;
} ChatMessage;

typedef struct {
    int64_t id; int64_t ws_id; char sender_id[64]; char body[512]; int limit;
    int64_t cursor;
} ChatQuery;

typedef struct { Err err; ChatMessage rows[16]; int count; } ChatResult;

typedef struct {
    int64_t id; char name[128]; char kind[32]; char webhook_url[256];
    int enabled; int64_t created_at;
} Integration;

typedef struct {
    int64_t id; char name[128]; char kind[32]; int enabled; int limit;
} IntegQuery;

typedef struct { Err err; Integration integ; } IntegResult;

typedef struct {
    int64_t id; char user_id[64]; char provider[64];
    char access_tok[256]; char refresh_tok[256]; int64_t expiry;
} OAuthToken;

typedef struct {
    int64_t id; char user_id[64]; char provider[64];
    char access_tok[256]; char refresh_tok[256]; int64_t expiry;
} OAuthQuery;

typedef struct { Err err; OAuthToken tok; } OAuthResult;

typedef struct { char channel[32]; char webhook_url[256]; char message[512]; } NotifyReq;
typedef struct { Err err; int sent; } NotifyResult;

#endif /* TYPES_COLLAB_H */
