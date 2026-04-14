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
}

Config load_config(const char *env_path)
{
    Config cfg;
    const char *v;
    FILE *f;
    char line[ENV_LINE_MAX];

    memset(&cfg, 0, sizeof(cfg));
    cfg.port = 8080;
    snprintf(cfg.db_path, sizeof(cfg.db_path), "%s", "teb2.db");
    snprintf(cfg.secret,  sizeof(cfg.secret),  "%s", "change_me_in_production");

    f = fopen(env_path, "r");
    if (f) {
        while (fgets(line, ENV_LINE_MAX, f))
            apply_env_line(line, &cfg);
        fclose(f);
    }

    if ((v = getenv("DB_PATH"))) snprintf(cfg.db_path, sizeof(cfg.db_path), "%s", v);
    if ((v = getenv("SECRET")))  snprintf(cfg.secret,  sizeof(cfg.secret),  "%s", v);
    if ((v = getenv("PORT")))    cfg.port = atoi(v);

    return cfg;
}
