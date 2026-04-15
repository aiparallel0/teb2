#ifndef DB_H
#define DB_H

#include "core/types.h"
#include "core/errors.h"

Err      db_open(const char *path, Db *out)
    __attribute__((warn_unused_result));

void     db_close(Db *db);

GoalResult fetch_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult store_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult list_goals(Db *db, GoalQuery q) __attribute__((warn_unused_result));
GoalResult update_goal(Db *db, GoalQuery q) __attribute__((warn_unused_result));

TaskResult fetch_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult store_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult list_tasks(Db *db, TaskQuery q) __attribute__((warn_unused_result));
TaskResult update_task(Db *db, TaskQuery q) __attribute__((warn_unused_result));

UserResult fetch_user(Db *db, UserQuery q) __attribute__((warn_unused_result));
UserResult store_user(Db *db, UserQuery q) __attribute__((warn_unused_result));

OutcomeResult store_outcome(Db *db, OutcomeQuery q) __attribute__((warn_unused_result));
OutcomeResult fetch_outcome(Db *db, OutcomeQuery q) __attribute__((warn_unused_result));

#endif /* DB_H */
