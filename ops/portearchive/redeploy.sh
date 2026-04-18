#!/usr/bin/env bash
# ops/portearchive/redeploy.sh
#
# Invoked by adnanh/webhook when GitHub POSTs a push-to-main event
# (HMAC-verified by the webhook daemon; by the time we run, the push
# is authentic).
#
# Steps:
#   1. fetch origin/main
#   2. snapshot the SQLite DB (so a bad code change cannot lose data)
#   3. fast-forward to origin/main
#   4. docker compose up -d --build app (with portearchive override)
#   5. wait for /healthz on the loopback port; if it fails, roll back to
#      the previous commit and restart.
#
# Exit codes:
#   0 success
#   1 deploy failed, roll-back succeeded
#   2 deploy failed AND roll-back failed — operator action required

set -euo pipefail

REPO_DIR="${REPO_DIR:-/opt/teb2}"
LOOPBACK_PORT="${TEB2_LOOPBACK_PORT:-18082}"
COMPOSE_BASE="docker-compose.yml"
COMPOSE_OVERRIDE="ops/portearchive/compose.yml"
HEALTH_TIMEOUT="${HEALTH_TIMEOUT:-90}"
BACKUP_DIR="$REPO_DIR/backups"
TS="$(date +%Y%m%d_%H%M%S)"

log() { logger -t teb2-redeploy "$*"; echo "[teb2-redeploy] $*"; }

cd "$REPO_DIR"

COMPOSE=""
if docker compose version >/dev/null 2>&1; then
    COMPOSE="docker compose"
elif command -v docker-compose >/dev/null 2>&1; then
    COMPOSE="docker-compose"
else
    log "ERROR: docker compose not found"; exit 2
fi

# If GITHUB_TOKEN is supplied (via the webhook systemd EnvironmentFile),
# re-set the remote URL each run so a rotated token takes effect on
# the next push without manual intervention. The git config file is
# created mode 600 by git; the token is only ever stored there.
if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    CUR_URL="$(git remote get-url origin)"
    NEW_URL="$(echo "$CUR_URL" | sed -E "s#^(https?://)([^@]+@)?#\1oauth2:${GITHUB_TOKEN}@#")"
    if [[ "$CUR_URL" != "$NEW_URL" ]]; then
        git remote set-url origin "$NEW_URL"
    fi
fi

log "fetching origin"
git fetch --quiet origin main

OLD_SHA="$(git rev-parse HEAD)"
NEW_SHA="$(git rev-parse origin/main)"
if [[ "$OLD_SHA" == "$NEW_SHA" ]]; then
    log "already at $NEW_SHA — nothing to do"; exit 0
fi

log "snapshotting DB before deploy"
mkdir -p "$BACKUP_DIR"
CID="$(docker ps --filter 'name=teb2-app' --format '{{.ID}}' | head -1 || true)"
if [[ -n "$CID" ]]; then
    docker cp "$CID:/app/data/teb2.db" "$BACKUP_DIR/teb2_${TS}_pre_${OLD_SHA:0:7}.db" \
        2>/dev/null || log "WARN: DB backup skipped (no file in container)"
fi

log "fast-forwarding $OLD_SHA -> $NEW_SHA"
git reset --hard "$NEW_SHA"

log "rebuilding app container"
$COMPOSE -f "$COMPOSE_BASE" -f "$COMPOSE_OVERRIDE" up -d --build app

log "waiting up to ${HEALTH_TIMEOUT}s for /healthz on 127.0.0.1:${LOOPBACK_PORT}"
SECS=0
until curl -sf --max-time 3 "http://127.0.0.1:${LOOPBACK_PORT}/healthz" >/dev/null 2>&1; do
    if (( SECS >= HEALTH_TIMEOUT )); then
        log "ERROR: health check timed out; rolling back to ${OLD_SHA:0:7}"
        if git reset --hard "$OLD_SHA" \
           && $COMPOSE -f "$COMPOSE_BASE" -f "$COMPOSE_OVERRIDE" up -d --build app; then
            log "rolled back successfully"; exit 1
        fi
        log "CRITICAL: roll-back FAILED — manual intervention required"; exit 2
    fi
    sleep 3; SECS=$((SECS + 3))
done

log "deploy OK; now at ${NEW_SHA:0:7}"
