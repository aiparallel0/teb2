#include <stdio.h>
#include "core/types.h"
#include "core/errors.h"
#include "core/config.h"
#include "core/log.h"
#include "db/db.h"
#include "api/api.h"

int main(int argc, char **argv)
{
    Config cfg;
    Db     db;
    int    rc;

    cfg = load_config(argc > 1 ? argv[1] : ".env");
    if (validate_config(&cfg) != ERR_OK) {
        fprintf(stderr,
            "teb2: refusing to start: missing or default SECRET "
            "(must be >=32 chars, not placeholder). Edit .env.\n");
        return 2;
    }
    if (db_open(cfg.db_path, &db) != ERR_OK) {
        fprintf(stderr, "db_open failed: %s\n", cfg.db_path);
        return 1;
    }
    server_install_signals();
    teb_log_info("main", "teb2 starting port=%d workers=%d timeout=%ds",
                 cfg.port, cfg.workers, cfg.run_timeout_sec);
    rc = server_start(&cfg, &db);
    db_close(&db);
    return rc;
}
