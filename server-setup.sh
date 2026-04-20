#!/usr/bin/env bash
# server-setup.sh — one-shot teb2 deployment for a fresh (or existing) server.
#
# Run as root or a user with sudo+docker rights:
#   curl -fsSL https://raw.githubusercontent.com/aiparallel0/teb2/main/server-setup.sh | sudo bash
#   — or —
#   sudo bash server-setup.sh
#
# The script is idempotent: safe to re-run. It will never stop services that it
# did not start, never overwrite a .env that already contains a real SECRET, and
# never bind a port that is already in use by another process.
#
# What it does (in order):
#   1. Preflight  — OS check, sudo, required commands
#   2. Docker     — install Docker CE + Compose plugin if absent (Debian/Ubuntu)
#   3. Repo       — clone or update aiparallel0/teb2 to INSTALL_DIR
#   4. Env        — create .env with a generated SECRET if not already set
#   5. Port check — warn and abort if LISTEN_PORT (default 80) is occupied
#   6. Backup     — snapshot any existing teb2 DB volume before rebuild
#   7. Launch     — docker compose up --build -d
#   8. Health     — poll /healthz until healthy (up to 120 s)
#   9. Summary    — print URL and next-step hints

set -euo pipefail

# ── Configuration (override via environment) ─────────────────────────────────
REPO_URL="${REPO_URL:-https://github.com/aiparallel0/teb2.git}"
INSTALL_DIR="${INSTALL_DIR:-/opt/teb2}"
LISTEN_PORT="${LISTEN_PORT:-80}"
LISTEN_TLS_PORT="${LISTEN_TLS_PORT:-443}"
HEALTH_TIMEOUT="${HEALTH_TIMEOUT:-120}"      # seconds to wait for /healthz
# ─────────────────────────────────────────────────────────────────────────────

RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'; NC='\033[0m'
info()  { echo -e "${GREEN}[teb2]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC} $*" >&2; }
abort() { echo -e "${RED}[error]${NC} $*" >&2; exit 1; }

# ── 1. Preflight ─────────────────────────────────────────────────────────────
info "Step 1/9 — preflight checks"

if [[ $EUID -ne 0 ]]; then
    abort "Run this script as root or via: sudo bash $0"
fi

# Detect distro family (we only automate Docker install on Debian/Ubuntu)
DISTRO_FAMILY="unknown"
if [[ -f /etc/os-release ]]; then
    # shellcheck source=/dev/null
    . /etc/os-release
    case "${ID_LIKE:-$ID}" in
        *debian*|*ubuntu*) DISTRO_FAMILY="debian" ;;
        *rhel*|*fedora*)   DISTRO_FAMILY="rhel"   ;;
    esac
fi

# ── 2. Docker ─────────────────────────────────────────────────────────────────
info "Step 2/9 — ensure Docker is installed"

install_docker_debian() {
    info "  Installing Docker CE (Debian/Ubuntu)…"
    apt-get update -qq
    apt-get install -y -qq ca-certificates curl gnupg lsb-release
    install -m 0755 -d /etc/apt/keyrings
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
        | gpg --dearmor --yes -o /etc/apt/keyrings/docker.gpg
    chmod a+r /etc/apt/keyrings/docker.gpg
    # shellcheck disable=SC1091
    CODENAME=$(. /etc/os-release; echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}")
    echo "deb [arch=$(dpkg --print-architecture) \
signed-by=/etc/apt/keyrings/docker.gpg] \
https://download.docker.com/linux/ubuntu ${CODENAME} stable" \
        > /etc/apt/sources.list.d/docker.list
    apt-get update -qq
    apt-get install -y -qq \
        docker-ce docker-ce-cli containerd.io \
        docker-buildx-plugin docker-compose-plugin
    systemctl enable --now docker
}

if ! command -v docker &>/dev/null; then
    if [[ "$DISTRO_FAMILY" == "debian" ]]; then
        install_docker_debian
    else
        abort "Docker not found. Install Docker CE manually then re-run this script."
    fi
else
    info "  Docker already installed: $(docker --version)"
fi

# Resolve compose command
if docker compose version &>/dev/null 2>&1; then
    COMPOSE="docker compose"
elif command -v docker-compose &>/dev/null; then
    COMPOSE="docker-compose"
else
    abort "docker compose plugin not found. Install docker-compose-plugin and retry."
fi
info "  Compose: $($COMPOSE version --short 2>/dev/null || echo 'ok')"

# ── 3. Repo ───────────────────────────────────────────────────────────────────
info "Step 3/9 — clone or update repository → $INSTALL_DIR"

if [[ -d "$INSTALL_DIR/.git" ]]; then
    info "  Repository exists — pulling latest changes"
    git -C "$INSTALL_DIR" fetch --quiet origin
    # Only fast-forward; never rebase over local edits
    if git -C "$INSTALL_DIR" merge --ff-only origin/main &>/dev/null; then
        info "  Updated to $(git -C "$INSTALL_DIR" rev-parse --short HEAD)"
    else
        warn "  Could not fast-forward (local edits?). Skipping pull — using existing code."
    fi
else
    git clone --quiet "$REPO_URL" "$INSTALL_DIR"
    info "  Cloned → $INSTALL_DIR"
fi

cd "$INSTALL_DIR"

# ── 4. Env ────────────────────────────────────────────────────────────────────
info "Step 4/9 — configure .env"

if [[ ! -f .env ]]; then
    cp .env.production.example .env
    info "  Created .env from .env.production.example"
fi

# Replace placeholder SECRET with a cryptographically random one
if grep -qE '^SECRET=(change_me|CHANGE_ME|)' .env; then
    NEW_SECRET=$(openssl rand -hex 32)
    # Use | as sed delimiter to avoid / conflicts with hex string
    sed -i "s|^SECRET=.*|SECRET=${NEW_SECRET}|" .env
    info "  Generated secure SECRET ($(wc -c <<<"$NEW_SECRET" | tr -d ' ') hex chars)"
else
    info "  SECRET already set — keeping existing value"
fi

# Inject LISTEN_PORT / LISTEN_TLS_PORT if not already in .env
grep -q '^LISTEN_PORT=' .env       || echo "LISTEN_PORT=${LISTEN_PORT}"       >> .env
grep -q '^LISTEN_TLS_PORT=' .env   || echo "LISTEN_TLS_PORT=${LISTEN_TLS_PORT}" >> .env

# Re-export so later steps see the values
set -a
# shellcheck source=/dev/null
. .env
set +a

# ── 5. Port conflict check ────────────────────────────────────────────────────
info "Step 5/9 — check port ${LISTEN_PORT} availability"

port_in_use() {
    # ss is preferred; fall back to netstat or lsof
    if command -v ss &>/dev/null; then
        ss -tlnH "sport = :$1" 2>/dev/null | grep -q .
    elif command -v netstat &>/dev/null; then
        netstat -tlnp 2>/dev/null | grep -q ":$1 "
    elif command -v lsof &>/dev/null; then
        lsof -iTCP:"$1" -sTCP:LISTEN &>/dev/null
    else
        return 1  # cannot detect — proceed with caution
    fi
}

if port_in_use "$LISTEN_PORT"; then
    # Check if it's OUR own nginx container already running — that's fine
    EXISTING_NGINX=$(docker ps --filter "name=teb2-nginx" --filter "status=running" \
                     --format '{{.Names}}' 2>/dev/null || true)
    if [[ -z "$EXISTING_NGINX" ]]; then
        warn "Port ${LISTEN_PORT} is already in use by another process."
        warn "To avoid corrupting existing websites, this script will NOT rebind that port."
        warn "Options:"
        warn "  a) Stop the process on port ${LISTEN_PORT} first, then re-run."
        warn "  b) Set LISTEN_PORT=8080 (or any free port) and re-run:"
        warn "     LISTEN_PORT=8080 bash server-setup.sh"
        abort "Aborting to protect existing services on port ${LISTEN_PORT}."
    else
        info "  Port ${LISTEN_PORT} already held by our teb2-nginx container — continuing"
    fi
else
    info "  Port ${LISTEN_PORT} is free"
fi

# ── 6. Backup existing DB ─────────────────────────────────────────────────────
info "Step 6/9 — backup any existing database"

BACKUP_DIR="$INSTALL_DIR/backups"
mkdir -p "$BACKUP_DIR"

EXISTING_APP=$(docker ps --filter "name=teb2-app" --filter "status=running" \
               --format '{{.ID}}' 2>/dev/null | head -1 || true)
if [[ -n "$EXISTING_APP" ]]; then
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    BACKUP_FILE="$BACKUP_DIR/teb2_${TIMESTAMP}.db"
    if docker cp "${EXISTING_APP}:/app/data/teb2.db" "$BACKUP_FILE" 2>/dev/null; then
        info "  DB backup saved → $BACKUP_FILE"
    else
        info "  No DB file found in running container — skipping backup"
    fi
else
    info "  No running teb2-app container — skipping backup"
fi

# ── 6b. TLS certificates (Let's Encrypt via certbot one-shot) ────────────────
info "Step 6b/9 — ensure TLS certificates for \${DOMAIN:-<unset>}"

if [[ -n "${DOMAIN:-}" ]]; then
    LIVE_DIR="/etc/letsencrypt/live/${DOMAIN}"
    if [[ -s "${LIVE_DIR}/fullchain.pem" && -s "${LIVE_DIR}/privkey.pem" ]]; then
        info "  Certificates already present at ${LIVE_DIR} — skipping certbot"
    else
        info "  No certificates found for ${DOMAIN}; bootstrapping"
        mkdir -p "${LIVE_DIR}" "${INSTALL_DIR}/nginx/acme"
        # Self-signed placeholder so nginx can start; certbot will overwrite it.
        if [[ ! -s "${LIVE_DIR}/fullchain.pem" ]]; then
            openssl req -x509 -nodes -newkey rsa:2048 -days 1 \
                -keyout "${LIVE_DIR}/privkey.pem" \
                -out    "${LIVE_DIR}/fullchain.pem" \
                -subj   "/CN=${DOMAIN}" >/dev/null 2>&1
            info "  Wrote self-signed placeholder for ${DOMAIN}"
        fi
        # Bring nginx up so /.well-known/acme-challenge/ is reachable on :80,
        # then run the one-shot certbot service to obtain a real cert.
        $COMPOSE -f docker-compose.yml up -d --build nginx app
        info "  Requesting Let's Encrypt certificate via webroot challenge"
        if DOMAIN="${DOMAIN}" CERTBOT_EMAIL="${CERTBOT_EMAIL:-}" \
             $COMPOSE -f docker-compose.yml run --rm certbot; then
            info "  Certificate obtained; reloading nginx"
            docker exec "$(docker ps -qf name=nginx | head -n1)" nginx -s reload || true
        else
            warn "  certbot failed — leaving placeholder in place. Re-run after DNS + :80 reachable."
        fi
    fi
else
    info "  DOMAIN not set — skipping TLS bootstrap (HTTP-only setup)"
fi

# ── 7. Build and launch ───────────────────────────────────────────────────────
info "Step 7/9 — build image and start services"

$COMPOSE -f docker-compose.yml up -d --build

# ── 8. Health check ───────────────────────────────────────────────────────────
info "Step 8/9 — waiting for /healthz (timeout ${HEALTH_TIMEOUT}s)"

ELAPSED=0
INTERVAL=5
APP_PORT="${PORT:-8080}"
HEALTH_URL="http://127.0.0.1:${LISTEN_PORT}/healthz"

# Also try the internal app port directly, in case nginx isn't in this script's scope
HEALTH_URL_APP="http://127.0.0.1:${APP_PORT}/healthz"

until curl -sf "$HEALTH_URL" &>/dev/null || curl -sf "$HEALTH_URL_APP" &>/dev/null; do
    if [[ $ELAPSED -ge $HEALTH_TIMEOUT ]]; then
        warn "Health check timed out after ${HEALTH_TIMEOUT}s."
        warn "Check logs with: $COMPOSE -f $INSTALL_DIR/docker-compose.yml logs --tail=50"
        abort "Deployment may have failed — see logs above."
    fi
    echo -n "."
    sleep $INTERVAL
    ELAPSED=$((ELAPSED + INTERVAL))
done
echo ""
info "  Service is healthy (${ELAPSED}s)"

# ── 9. Summary ────────────────────────────────────────────────────────────────
info "Step 9/9 — deployment complete"
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  teb2 is running                                 ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════╝${NC}"
echo ""
echo "  URL  → http://$(hostname -f 2>/dev/null || hostname):${LISTEN_PORT}"
echo "  Dir  → $INSTALL_DIR"
echo "  Logs → $COMPOSE -f $INSTALL_DIR/docker-compose.yml logs -f"
echo ""
echo "  Useful commands (run from $INSTALL_DIR):"
echo "    ./deploy.sh status   — show running containers"
echo "    ./deploy.sh logs     — tail live logs"
echo "    ./deploy.sh backup   — snapshot the database"
echo "    ./deploy.sh update   — pull latest code and redeploy"
echo ""
echo "  To add TLS, edit nginx/nginx.conf and docker-compose.yml,"
echo "  then run: ./deploy.sh restart"
echo ""
