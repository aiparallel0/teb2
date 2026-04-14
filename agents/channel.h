#ifndef CHANNEL_H
#define CHANNEL_H

#include "core/types.h"

typedef enum {
    MSG_GOAL_NEW    = 0,
    MSG_TASK_DONE,
    MSG_EXEC_REQ,
    MSG_FINANCE_REQ,
    MSG_NUDGE,
    MSG_CHECKIN,
    MSG_RESULT
} MsgTag;

typedef struct {
    MsgTag  tag;
    int64_t id;          /* goal_id or task_id depending on tag */
    char    user_id[64];
    char    payload[512]; /* serialized data for this message type */
    Err     err;
} AgentMsg;

#endif /* CHANNEL_H */
