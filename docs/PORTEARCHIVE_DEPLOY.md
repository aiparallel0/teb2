# Deploying teb2 at portearchive.com/teb2/

End-to-end runbook for mounting this repo at `https://portearchive.com/teb2/`
on the same Digital Ocean droplet that already serves `/teb` and `/fixie`
via a host nginx.

## Honesty disclaimer

Before anything else: **none of this has been executed against a live
server** from within this repo's history. Every PR up to #29 is Copilot
code churn with no evidence of a real end-to-end smoke test. The scripts
below are best-effort and defensive, but the first real run IS the test.
Budget 30 minutes the first time to watch logs and be ready to run
`cp $BACKUP $TARGET && nginx -t && systemctl reload nginx` by hand.

## Architecture

```
  Internet ──TLS──▶ host nginx :443 ──┬──▶ /teb     (existing, untouched)
                                      ├──▶ /fixie   (existing, untouched)
                                      ├──▶ /teb2/*  ──▶ 127.0.0.1:18082  (teb2 app container)
                                      └──▶ /hooks/teb2-deploy ──▶ 127.0.0.1:9001 (adnanh/webhook)
```

The container's own nginx service (`docker-compose.yml`'s `nginx`
service) is **not** started on this droplet. It would try to bind :80
and :443 and fight the host nginx.

## Prerequisites on the droplet

- Debian or Ubuntu, root shell.
- Host nginx already serving `portearchive.com` on :443 with a valid
  Let's Encrypt cert.
- Docker CE + compose plugin.
- `python3`, `openssl`, `curl`, `git` (all usually already present).
- The `webhook` apt package (installed by the script if missing).

## What you need to do

### 1. First install

**Important — the DigitalOcean web console SIGHUPs your shell when it
re-authenticates or idles out.** That will kill any foreground install.
Run the install detached from the tty and tail the log file instead:

```sh
# One-time: fetch the installer and run it fully detached.
# Survives the DO console disconnecting on you.
mkdir -p /opt && cd /opt
git clone https://github.com/aiparallel0/teb2 teb2 2>/dev/null || true
cd /opt/teb2
git fetch origin
git checkout main   # or: claude/deploy-detection-site-W4VTy before merge
git pull --ff-only
setsid nohup bash ops/portearchive/install.sh \
    </dev/null >/var/log/teb2-install.log 2>&1 &
disown
# Watch progress:
tail -f /var/log/teb2-install.log
# Ctrl-C out of the tail whenever; the install continues regardless.
# Re-attach at any time with: tail -n+1 /var/log/teb2-install.log
```

The installer itself also writes to `/var/log/teb2-install.log` via
`tee`, so even if you forget the `nohup` dance, the output is
recoverable — provided the shell didn't die before the script did.

Alternative, if you already have the repo cloned somewhere else (for
example at `~/teb2`), just run from inside `/opt/teb2` — the script
only touches `/opt/teb2`, `/etc/nginx/snippets/teb2.conf`,
`/etc/teb2/`, `/etc/systemd/system/teb2-webhook.service`, and whichever
single nginx sites-enabled file contains the portearchive :443 server
block.

The script:
1. preflights nginx, docker, python3, openssl, curl;
2. clones or fast-forwards `/opt/teb2`;
3. generates `SECRET` and writes `/opt/teb2/.env`;
4. generates `TEB2_WEBHOOK_SECRET` and writes `/etc/teb2/webhook.env`
   (only if it does not already exist — re-runs never rotate the secret);
5. installs `adnanh/webhook` via apt;
6. renders the nginx snippet to `/etc/nginx/snippets/teb2.conf`;
7. auto-inserts `include /etc/nginx/snippets/teb2.conf;` into the
   existing `server { … server_name portearchive.com … listen 443 … }`
   block. Backs the original file up, runs `nginx -t`, and rolls back
   automatically if the test fails;
8. `docker compose up -d --build app` with the portearchive override
   so the container publishes only to `127.0.0.1:18082`;
9. installs + enables the `teb2-webhook.service` systemd unit;
10. waits for `/healthz` on the loopback port;
11. prints the exact values to paste into the GitHub webhook form.

### 2. Register the GitHub webhook (one-time, manual)

Go to https://github.com/aiparallel0/teb2/settings/hooks and click
**Add webhook**. Fill in exactly:

| Field            | Value                                             |
|------------------|---------------------------------------------------|
| Payload URL      | `https://portearchive.com/hooks/teb2-deploy`      |
| Content type     | `application/json`                                |
| Secret           | the value of `TEB2_WEBHOOK_SECRET` in `/etc/teb2/webhook.env` on the droplet |
| SSL verification | Enabled                                           |
| Events           | **Just the push event**                           |
| Active           | ✓                                                 |

After saving, GitHub sends a `ping` delivery. `journalctl -u teb2-webhook -f`
should show it arriving. The script's `trigger-rule` rejects anything
that isn't a push to `refs/heads/main`, so the `ping` intentionally
returns "Hook rules were not satisfied" — that's OK.

### 3. Smoke test the auto-update

```sh
git commit --allow-empty -m "poke redeploy"
git push origin main
```

Within ~20 s the droplet should:
1. receive `/hooks/teb2-deploy`,
2. verify the HMAC signature,
3. run `ops/portearchive/redeploy.sh` which fetches, snapshots the DB,
   `reset --hard origin/main`, rebuilds, waits for health, and rolls
   back automatically on failure.

Watch it:

```sh
journalctl -u teb2-webhook -f
docker logs -f teb2-app
```

## Environment variables reference

### Consumed by the teb2 app (`.env` at repo root)

| Var            | Purpose                                                  |
|----------------|----------------------------------------------------------|
| `SECRET`       | HMAC-SHA256 key for session tickets. ≥32 chars, random.  |
| `DB_PATH`      | `/app/data/teb2.db` inside the container.                |
| `PORT`         | Internal app port (stays at `8080`; don't change).       |
| `WORKERS`      | Pre-forked worker count. `4` is fine for a small droplet.|
| `OPENAI_API_KEY` / `OPENAI_MODEL` | LLM calls. Required for real agent work.  |
| `TEB2_LOOPBACK_PORT` | Host port the container publishes on (default `18082`). Must match the port in `nginx-teb2.conf`. |

**Do not set** `LISTEN_PORT` or `LISTEN_TLS_PORT` in `.env` on this
droplet. The `ports:` line in the override already handles that, and
the container-side nginx is not running.

### Consumed by the webhook receiver (`/etc/teb2/webhook.env`)

| Var                    | Purpose                                               |
|------------------------|-------------------------------------------------------|
| `TEB2_WEBHOOK_PORT`    | Loopback port the `webhook` daemon binds. `9001` default. |
| `TEB2_WEBHOOK_SECRET`  | Must match the Secret field in GitHub's webhook form. |
| `TEB2_LOOPBACK_PORT`   | Passed to `redeploy.sh` for its post-deploy health-check. |

## Rolling back

If an auto-deploy breaks production, `redeploy.sh` already tried once
to roll back. If it failed, do it by hand:

```sh
cd /opt/teb2
git log --oneline -5                  # find the last good SHA
git reset --hard <GOOD_SHA>
docker compose -f docker-compose.yml -f ops/portearchive/compose.yml \
  up -d --build app
```

To restore a DB snapshot taken by `redeploy.sh`:

```sh
docker exec -i teb2-app sqlite3 /app/data/teb2.db '.exit' && \
  docker cp backups/teb2_<timestamp>_pre_<sha>.db teb2-app:/app/data/teb2.db && \
  docker restart teb2-app
```

## If you need to pause auto-deploys

```sh
systemctl stop teb2-webhook.service
```

Re-enable with `systemctl start teb2-webhook.service`.
GitHub will queue deliveries for a short while and then give up.

## Uninstall

```sh
systemctl disable --now teb2-webhook.service
rm /etc/systemd/system/teb2-webhook.service
rm -f /etc/nginx/snippets/teb2.conf
# Remove the `include /etc/nginx/snippets/teb2.conf;` line from
# whichever sites-enabled file the install script edited. The backup
# it made is named <file>.bak.<unix_ts>.
nginx -t && systemctl reload nginx
docker compose -f /opt/teb2/docker-compose.yml \
               -f /opt/teb2/ops/portearchive/compose.yml down
# Optionally: rm -rf /opt/teb2 /etc/teb2
```

## Known limitations / things I did NOT do

- **No TLS cert work.** I assume the host nginx already terminates TLS
  for `portearchive.com`. If it doesn't, this will serve plaintext.
- **No DNS work.** Subpath routing uses the existing `portearchive.com`
  A record.
- **No rate-limiting on `/hooks/teb2-deploy`.** GitHub does not hammer
  it, but if the webhook secret ever leaks, an attacker could. Consider
  adding `limit_req` to the nginx location if this bothers you.
- **No per-environment isolation.** The same `SECRET` persists across
  redeploys because it's in `.env` on disk — otherwise every redeploy
  would invalidate every session. If you want rotation, move `SECRET`
  to a systemd drop-in and rotate it manually.
- **No multi-commit queue.** If two pushes arrive 2 seconds apart the
  second one blocks on the first inside `redeploy.sh` (git and compose
  serialize). No queue is needed for normal push cadence.
- **No staging.** There is one environment and it is production. Add a
  second compose project name + second loopback port + second nginx
  location if you want staging.
