#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sqlite3.h>
#include "core/types.h"
#include "core/errors.h"
#include "db/db.h"

static const char *SCHEMA_A =
    "CREATE TABLE IF NOT EXISTS workspaces("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "owner_id TEXT NOT NULL,name TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS collaborators("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "ws_id INTEGER NOT NULL,user_id TEXT NOT NULL,"
    "role_name TEXT NOT NULL DEFAULT 'member',"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "FOREIGN KEY (ws_id) REFERENCES workspaces(id));"

    "CREATE TABLE IF NOT EXISTS chat_messages("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "ws_id INTEGER NOT NULL,sender_id TEXT NOT NULL,"
    "body TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS integrations("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL UNIQUE,kind TEXT NOT NULL,"
    "webhook_url TEXT NOT NULL DEFAULT '',"
    "enabled INTEGER NOT NULL DEFAULT 0,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS oauth_tokens("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,provider TEXT NOT NULL,"
    "access_tok TEXT NOT NULL,refresh_tok TEXT NOT NULL DEFAULT '',"
    "expiry INTEGER NOT NULL DEFAULT 0,UNIQUE(user_id,provider));"

    "CREATE TABLE IF NOT EXISTS orgs("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,domain TEXT NOT NULL UNIQUE,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS sso_configs("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "org_id INTEGER NOT NULL,provider TEXT NOT NULL,"
    "endpoint TEXT NOT NULL,cert TEXT NOT NULL DEFAULT '',"
    "FOREIGN KEY (org_id) REFERENCES orgs(id));"

    "CREATE TABLE IF NOT EXISTS ip_allowlist("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "org_id INTEGER NOT NULL,cidr TEXT NOT NULL,"
    "FOREIGN KEY (org_id) REFERENCES orgs(id));";

static const char *SCHEMA_B =
    "CREATE TABLE IF NOT EXISTS progress_snapshots("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "goal_id INTEGER NOT NULL,pct INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS time_entries("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "task_id INTEGER NOT NULL,user_id TEXT NOT NULL,"
    "minutes INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS roi_metrics("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "goal_id INTEGER NOT NULL,value_cents INTEGER NOT NULL,"
    "cost_cents INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS xp_events("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,amount INTEGER NOT NULL,"
    "reason TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS streaks("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL UNIQUE,"
    "current INTEGER NOT NULL DEFAULT 0,"
    "longest INTEGER NOT NULL DEFAULT 0,"
    "updated_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS challenges("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,target INTEGER NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS blog_posts("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,title TEXT NOT NULL,"
    "body TEXT NOT NULL DEFAULT '',"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS feature_votes("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,feature TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "UNIQUE(user_id,feature));";

static const char *SCHEMA_C =
    "CREATE TABLE IF NOT EXISTS assets("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,filename TEXT NOT NULL,"
    "mime_type TEXT NOT NULL DEFAULT 'application/octet-stream',"
    "size_bytes INTEGER NOT NULL DEFAULT 0,path TEXT NOT NULL,"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')));"

    "CREATE TABLE IF NOT EXISTS workflow_runs("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "goal_id INTEGER NOT NULL,"
    "status TEXT NOT NULL DEFAULT 'pending',"
    "started_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "finished_at INTEGER NOT NULL DEFAULT 0,"
    "error_msg TEXT NOT NULL DEFAULT '',"
    "pid INTEGER NOT NULL DEFAULT 0,"
    "token_spend INTEGER NOT NULL DEFAULT 0);"

    "CREATE TABLE IF NOT EXISTS workflow_steps("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "run_id INTEGER NOT NULL,step_index INTEGER NOT NULL,"
    "agent TEXT NOT NULL,payload TEXT NOT NULL DEFAULT '',"
    "status TEXT NOT NULL DEFAULT 'pending',"
    "result TEXT NOT NULL DEFAULT '',"
    "created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "FOREIGN KEY (run_id) REFERENCES workflow_runs(id));"

    "CREATE VIRTUAL TABLE IF NOT EXISTS search_idx USING fts5("
    "entity,entity_id UNINDEXED,content);"

    "CREATE TABLE IF NOT EXISTS prompt_overrides("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "user_id TEXT NOT NULL,name TEXT NOT NULL,"
    "body TEXT NOT NULL,updated_at INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
    "UNIQUE(user_id,name));";

static Err run_sql(sqlite3 *h, const char *sql)
{
    char *e = NULL;
    int rc = sqlite3_exec(h, sql, NULL, NULL, &e);
    if (rc != SQLITE_OK) { sqlite3_free(e); return ERR_DB; }
    return ERR_OK;
}

Err db_init_ext(Db *db)
{
    if (!db || !db->handle) return ERR_DB;
    if (run_sql(db->handle, SCHEMA_A) != ERR_OK) return ERR_DB;
    if (run_sql(db->handle, SCHEMA_B) != ERR_OK) return ERR_DB;
    if (run_sql(db->handle, SCHEMA_C) != ERR_OK) return ERR_DB;
    return ERR_OK;
}
