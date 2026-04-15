#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include "core/errors.h"

typedef enum { ROLE_USER = 0, ROLE_ADMIN } UserRole;

typedef enum {
    PERM_GOAL_READ = 0,
    PERM_GOAL_WRITE,
    PERM_TASK_READ,
    PERM_TASK_WRITE,
    PERM_ADMIN
} Permission;

typedef struct { unsigned char data[64]; size_t len; } Bytes;
typedef struct { unsigned char k[8]; } Key8;

typedef struct {
    char db_path[256];
    char secret[128];
    int  port;
} Config;

typedef struct {
    int64_t  id;
    char     user_id[64];
    char     email[128];
    char     password_hash[128];
    UserRole role;
} User;

typedef struct {
    int64_t id;
    char    user_id[64];
    char    title[256];
    char    description[512];
    char    status[32];
    int64_t parent_id;
    int64_t created_at;
} Goal;

typedef struct {
    int64_t id;
    int64_t goal_id;
    char    user_id[64];
    char    title[256];
    char    description[512];
    char    status[32];
    char    agent[64];
    int64_t created_at;
} Task;

typedef struct {
    int64_t      user_id;
    UserRole     role;
    int64_t      expiry;
    unsigned char mac[8];
} Ticket;

typedef struct {
    int64_t  user_id;
    char     email[128];
    UserRole role;
} UserClaims;

typedef struct { unsigned int rounds; char salt[32]; } HashConfig;
typedef struct { Err err; char hash[128]; int match; } HashResult;
typedef struct { Err err; Ticket ticket; UserClaims claims; } TokenResult;
typedef struct { Err err; Bytes data; } CipherResult;

typedef struct {
    char   method[8];
    char   path[256];
    char   body[4096];
    size_t body_len;
    char   auth_header[256];
} HttpReq;

typedef struct {
    int    status;
    char   body[4096];
    size_t body_len;
    char   content_type[64];
} HttpResp;

typedef struct { char url[512]; char method[8]; char body[4096]; } SerialReq;
typedef struct { char action[64]; char target[256]; char value[512]; } SerialCmd;

typedef struct { Err err; int status; char body[4096]; size_t body_len; } HttpResult;
typedef struct { Err err; char output[4096]; size_t output_len; } BrowserResult;

typedef struct {
    int64_t id; char user_id[64]; char title[256]; char status[32]; int limit; int offset;
} GoalQuery;
typedef struct { Err err; Goal rows[16]; int count; } GoalResult;

typedef struct {
    int64_t id; int64_t goal_id; char user_id[64]; char title[256]; char status[32]; int limit;
} TaskQuery;
typedef struct { Err err; Task rows[16]; int count; } TaskResult;

typedef struct {
    int64_t id;
    char email[128]; char password_hash[128]; UserRole role;
} UserQuery;
typedef struct { Err err; User user; } UserResult;

typedef struct {
    int64_t id;
    int64_t task_id;
    char    result[512];
    int64_t created_at;
} Outcome;

typedef struct {
    int64_t id; int64_t task_id; char result[512]; int limit;
} OutcomeQuery;
typedef struct { Err err; Outcome outcome; } OutcomeResult;

typedef struct { char login[128]; char password[128]; } Cred;

struct sqlite3; /* forward declaration; completed by <sqlite3.h> in db/ files */
typedef struct { struct sqlite3 *handle; } Db;

typedef struct { Db *db; Config *cfg; UserClaims *user; } Ctx;


typedef struct {
    int64_t id; int64_t task_id; int64_t run_at;
} SchedEntry;

typedef struct { int64_t id; int64_t task_id; int64_t run_at; int limit; } SchedQuery;
typedef struct { Err err; SchedEntry entry; } SchedResult;

typedef struct {
    int64_t id; char user_id[64]; int64_t limit_cents; int64_t spent_cents;
} Budget;

typedef struct { int64_t id; char user_id[64]; int64_t amount_cents; } BudgetQuery;
typedef struct { Err err; Budget budget; } BudgetResult;

typedef struct {
    int64_t id; char agent[64]; char key[128]; char val[512]; int64_t ts;
} MemEntry;

typedef struct {
    int64_t id; char agent[64]; char key[128]; char val[512]; int limit;
} MemQuery;
typedef struct { Err err; MemEntry entry; } MemResult;

typedef struct {
    int64_t id; char user_id[64]; char message[512]; int64_t created_at;
} Nudge;

typedef struct { int64_t id; char user_id[64]; char message[512]; int limit; } NudgeQuery;
typedef struct { Err err; Nudge nudge; int count; } NudgeResult;

#endif /* TYPES_H */
