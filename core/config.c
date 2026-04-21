#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/config.h"

#define ENV_LINE_MAX 512

static void copy_strip(char *dst, size_t dsz, const char *src)
{
    size_t i;
    for (i = 0; i < dsz - 1 && src[i] != '\0' && src[i] != '\r' && src[i] != '\n'; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

static void apply_env_line(const char *line, Config *cfg)
{
    const char *eq = strchr(line, '=');
    char key[64];
    size_t klen;

    if (!eq) return;
    klen = (size_t)(eq - line);
    if (klen == 0 || klen >= sizeof(key)) return;
    memcpy(key, line, klen);
    key[klen] = '\0';

    if (strcmp(key, "DB_PATH") == 0)
        copy_strip(cfg->db_path, sizeof(cfg->db_path), eq + 1);
    else if (strcmp(key, "SECRET") == 0)
        copy_strip(cfg->secret,  sizeof(cfg->secret),  eq + 1);
    else if (strcmp(key, "PORT") == 0)
        cfg->port = atoi(eq + 1);
    else if (strcmp(key, "SMTP_HOST") == 0)
        copy_strip(cfg->smtp_host, sizeof(cfg->smtp_host), eq + 1);
    else if (strcmp(key, "SMTP_PORT") == 0)
        cfg->smtp_port = atoi(eq + 1);
    else if (strcmp(key, "SMTP_USER") == 0)
        copy_strip(cfg->smtp_user, sizeof(cfg->smtp_user), eq + 1);
    else if (strcmp(key, "SMTP_PASS") == 0)
        copy_strip(cfg->smtp_pass, sizeof(cfg->smtp_pass), eq + 1);
    else if (strcmp(key, "OPENAI_API_KEY") == 0)
        copy_strip(cfg->openai_key, sizeof(cfg->openai_key), eq + 1);
    else if (strcmp(key, "OPENAI_MODEL") == 0)
        copy_strip(cfg->openai_model, sizeof(cfg->openai_model), eq + 1);
    else if (strcmp(key, "LLM_BASE_URL") == 0)
        copy_strip(cfg->llm_base_url, sizeof(cfg->llm_base_url), eq + 1);
    else if (strcmp(key, "LLM_MAX_TOKENS") == 0)
        cfg->llm_max_tokens = atoi(eq + 1);
    else if (strcmp(key, "LLM_RETRIES") == 0)
        cfg->llm_retries = atoi(eq + 1);
    else if (strcmp(key, "RUN_TIMEOUT_SEC") == 0)
        cfg->run_timeout_sec = atoi(eq + 1);
    else if (strcmp(key, "WORKERS") == 0)
        cfg->workers = atoi(eq + 1);
    else if (strcmp(key, "ADMIN_EMAIL") == 0)
        copy_strip(cfg->admin_email, sizeof(cfg->admin_email), eq + 1);
    else if (strcmp(key, "ADMIN_PASSWORD") == 0)
        copy_strip(cfg->admin_password, sizeof(cfg->admin_password), eq + 1);
}

Config load_config(const char *env_path)
{
    Config cfg;
    const char *v;
    FILE *f;
    char line[ENV_LINE_MAX];

    memset(&cfg, 0, sizeof(cfg));
    cfg.port = 8080;
    cfg.smtp_port = 587;
    cfg.workers = 4;
    snprintf(cfg.db_path,      sizeof(cfg.db_path),      "%s", "teb2.db");
    snprintf(cfg.secret,       sizeof(cfg.secret),       "%s", "change_me_in_production");
    snprintf(cfg.openai_model, sizeof(cfg.openai_model), "%s", "gpt-4o-mini");
    snprintf(cfg.llm_base_url, sizeof(cfg.llm_base_url), "%s", "api.openai.com");
    cfg.llm_max_tokens = 1024;
    cfg.llm_retries = 3;
    cfg.run_timeout_sec = 300;

    f = fopen(env_path, "r");
    if (f) {
        while (fgets(line, ENV_LINE_MAX, f))
            apply_env_line(line, &cfg);
        fclose(f);
    }

    if ((v = getenv("DB_PATH")))        snprintf(cfg.db_path,      sizeof(cfg.db_path),      "%s", v);
    if ((v = getenv("SECRET")))         snprintf(cfg.secret,       sizeof(cfg.secret),        "%s", v);
    if ((v = getenv("PORT")))           cfg.port = atoi(v);
    if ((v = getenv("SMTP_HOST")))      snprintf(cfg.smtp_host,    sizeof(cfg.smtp_host),     "%s", v);
    if ((v = getenv("SMTP_PORT")))      cfg.smtp_port = atoi(v);
    if ((v = getenv("SMTP_USER")))      snprintf(cfg.smtp_user,    sizeof(cfg.smtp_user),     "%s", v);
    if ((v = getenv("SMTP_PASS")))      snprintf(cfg.smtp_pass,    sizeof(cfg.smtp_pass),     "%s", v);
    if ((v = getenv("OPENAI_API_KEY"))) snprintf(cfg.openai_key,   sizeof(cfg.openai_key),    "%s", v);
    if ((v = getenv("OPENAI_MODEL")))   snprintf(cfg.openai_model, sizeof(cfg.openai_model),  "%s", v);
    if ((v = getenv("LLM_BASE_URL")))  snprintf(cfg.llm_base_url, sizeof(cfg.llm_base_url),  "%s", v);
    if ((v = getenv("LLM_MAX_TOKENS")))cfg.llm_max_tokens = atoi(v);
    if ((v = getenv("LLM_RETRIES")))   cfg.llm_retries = atoi(v);
    if ((v = getenv("RUN_TIMEOUT_SEC")))cfg.run_timeout_sec = atoi(v);
    if ((v = getenv("WORKERS")))        cfg.workers = atoi(v);
    if ((v = getenv("ADMIN_EMAIL")))    snprintf(cfg.admin_email,    sizeof(cfg.admin_email),    "%s", v);
    if ((v = getenv("ADMIN_PASSWORD"))) snprintf(cfg.admin_password, sizeof(cfg.admin_password), "%s", v);

    return cfg;
}

/*
 * Fail-closed startup validation. Required secrets must be present and
 * non-default. Returns ERR_AUTH on missing/default SECRET, ERR_IO on
 * malformed numeric fields. main.c must abort startup if this returns
 * anything other than ERR_OK.
 */
Err validate_config(const Config *cfg)
{
    size_t slen;
    if (!cfg) return ERR_IO;
    if (!cfg->db_path[0]) return ERR_IO;
    if (cfg->port <= 0 || cfg->port > 65535) return ERR_IO;
    if (cfg->workers < 1 || cfg->workers > 1024) return ERR_IO;
    slen = strlen(cfg->secret);
    if (slen < 32) return ERR_AUTH;
    if (strcmp(cfg->secret, "change_me_in_production") == 0) return ERR_AUTH;
    if (strcmp(cfg->secret, "CHANGE_ME_TO_A_RANDOM_SECRET") == 0)
        return ERR_AUTH;
    return ERR_OK;
}
