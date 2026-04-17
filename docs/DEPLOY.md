# Deploying teb2

This document is the operational envelope for running teb2 outside a
developer laptop. It is intentionally short because teb2 is one static
binary plus one SQLite file: most "deployment complexity" other tools
carry is not relevant here.

## Target topology

```
   Internet -- TLS --> nginx (443)  -->  teb2 (localhost:8080, HTTP)
                                              |
                                              +-- /var/lib/teb2/teb2.db   (SQLite + WAL)
                                              +-- /var/lib/teb2/backups/  (snapshots)
```

* **nginx terminates TLS.** teb2 itself speaks plain HTTP on its
  internal port. The service binds to `127.0.0.1` behind nginx; it is
  never exposed directly. See `nginx/` for an example server block.
* **teb2 initiates TLS outbound.** As of this PR, `exec/tls.c` is a
  verified OpenSSL 1.2+ client used by `exec/http.c` whenever a URL
  uses `https://`. OAuth token exchange (`exec/oauth_http.c`) now uses
  the real provider URLs over HTTPS.
* **One SQLite file.** WAL mode is enabled in `db_open`. Multi-process
  reads are safe; writes serialize through SQLite's internal locking.
  Pre-forked workers each open their own `Db` handle after `fork()` so
  no handle is shared across processes.

## Secret provisioning

teb2 refuses to start if:

* `SECRET` is missing, shorter than 32 characters, or still the
  template value (`change_me_in_production` /
  `CHANGE_ME_TO_A_RANDOM_SECRET`).
* `DB_PATH` is empty, `PORT` is out of range, or `WORKERS` is absurd.

This is `validate_config()` in `core/config.c`; startup fails with
exit code 2 and a message to stderr. Generate a real secret with:

```sh
head -c 48 /dev/urandom | base64 > /etc/teb2/secret
chmod 600 /etc/teb2/secret
```

Then set `SECRET=$(cat /etc/teb2/secret)` in your environment (e.g.
the systemd unit's `EnvironmentFile=`).

`.env.production.example` is the template. `.env.production` is
`.gitignore`d and must never be committed.

## systemd unit (sketch)

```
[Unit]
Description=teb2 planning agent
After=network.target

[Service]
User=teb2
Group=teb2
EnvironmentFile=/etc/teb2/env
ExecStart=/usr/local/bin/teb2 /etc/teb2/env
Restart=on-failure
RestartSec=2
# teb2 installs SIGTERM/SIGINT handlers that drain the accept loop
# and close the DB cleanly; systemd's default TERM+KILL works.
TimeoutStopSec=15
ProtectSystem=strict
ReadWritePaths=/var/lib/teb2
NoNewPrivileges=true
PrivateTmp=yes

[Install]
WantedBy=multi-user.target
```

Graceful shutdown: teb2 installs SIGTERM/SIGINT without
`SA_RESTART`, so the blocking `accept()` returns `EINTR` and the main
loop falls through to `db_close()` before exiting. WAL is flushed on
close, so `systemctl restart teb2` is safe.

## Backups

`ops/backup.sh` wraps `sqlite3 .backup` (online snapshot API, safe
against concurrent writers) and writes `/var/lib/teb2/backups/teb2_<ts>.db`.
Install as a timer:

```
# /etc/systemd/system/teb2-backup.service
[Service]
Type=oneshot
User=teb2
EnvironmentFile=/etc/teb2/env
ExecStart=/usr/local/bin/teb2-backup.sh

# /etc/systemd/system/teb2-backup.timer
[Timer]
OnCalendar=hourly
Persistent=true
[Install]
WantedBy=timers.target
```

Rotate off-box with your preferred tool (restic / borg / s3 cp). The
script itself prunes anything older than `$KEEP_DAYS` days (default 14).

## Observability

* `/healthz` returns `200 ok` when the DB is reachable.
* `/metrics` exposes the in-process counters incremented by
  `metrics_inc`.
* Every mutating request is appended to the `audit_log` table with
  `(user_id, action, path, status, src_ip, created_at)`. Query in prod with:
  ```
  SELECT created_at, user_id, action, path, status
    FROM audit_log
   WHERE created_at > strftime('%s','now','-1 hour')
   ORDER BY id DESC LIMIT 50;
  ```

## Rate limiting envelope

* Per-IP: 120 req/min (10/min for `/auth/register` and `/auth/login`).
  Keyed on `X-Forwarded-For` as supplied by nginx.
* Per-user: 300 mutating req/min, keyed on the authenticated user id
  on top of the IP limit. Protects against token-share storms.

## What teb2 is NOT

teb2 is deliberately small. It has no test suite (the compiler is the
test suite: `-std=c99 -pedantic -Wall -Wextra -Werror -fanalyzer`
must stay green). It has no plugin marketplace, no CRDT editor, no
workflow canvas. Do not add those. See `README.md` and `orders.txt`
for the architectural charter that keeps the project maintainable.
