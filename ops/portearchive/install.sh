#!/usr/bin/env bash
# ops/portearchive/install.sh
#
# One-shot installer that wires teb2 into an existing portearchive.com
# droplet that ALREADY serves /teb and /fixie via host nginx. Designed to
# be copy-pasted into a root shell. Idempotent: safe to re-run.
#
# What it does:
#   1. Preflight (root, OS, nginx/docker/python3 present).
#   2. Pick/confirm loopback ports for the app and webhook daemon.
#   3. Clone or fast-forward /opt/teb2.
#   4. Render .env with a fresh 64-char SECRET and 64-char WEBHOOK_SECRET
#      (only if placeholders are present; never overwrites real secrets).
#   5. Install adnanh/webhook package (Debian/Ubuntu).
#   6. Render /etc/nginx/snippets/teb2.conf from the template.
#   7. Auto-insert `include /etc/nginx/snippets/teb2.conf;` into the
#      existing portearchive.com server block (with backup + rollback if
#      `nginx -t` fails).
#   8. Build+start the teb2 app container on 127.0.0.1:$LOOPBACK_PORT.
#   9. Install + enable the systemd webhook receiver unit.
#  10. Wait for /healthz on the loopback port.
#  11. Print the EXACT GitHub webhook settings the operator must paste
#      into the repo's Settings > Webhooks page.
#
# This script will NOT:
#   - Edit DNS or TLS certs (portearchive.com already has those).
#   - Stop or restart anything unrelated to teb2.
#   - Bind port 80 or 443 (the host nginx already owns them).
#
# Re-running after the first successful install pulls new code, rebuilds
# the container, reloads nginx only if the snippet changed, and reports.
# It does NOT re-issue the webhook secret unless /etc/teb2/webhook.env
# is deleted first.
#
# REQUIREMENTS (fail fast if missing):
#   - Debian/Ubuntu with systemd
#   - Host nginx already serving portearchive.com on 443 with a valid cert
#   - Docker CE + compose plugin
#   - python3 (used for safe in-place nginx config insertion)
#   - openssl (for random secrets)
#   - curl

set -euo pipefail

# ── Logging — survive console disconnect ────────────────────────────────
# Everything below goes to both stdout AND a permanent logfile, so
# reconnecting after a DigitalOcean web-console timeout lets you
# `tail -n+1 /var/log/teb2-install.log` to see what happened.
LOGFILE="${TEB2_INSTALL_LOG:-/var/log/teb2-install.log}"
mkdir -p "$(dirname "$LOGFILE")" 2>/dev/null || true
# Using `tee -a` via process substitution; `exec >` installs it for the
# rest of the script. Works when run foreground, background, or via
# `nohup … &`. Does NOT protect against SIGHUP by itself — use nohup
# or `setsid … < /dev/null &` for that (see docs/PORTEARCHIVE_DEPLOY.md).
exec > >(tee -a "$LOGFILE") 2>&1
echo "=== teb2 install started at $(date -Iseconds) (pid $$) ==="

# ── Tunables ────────────────────────────────────────────────────────────
REPO_URL="${REPO_URL:-https://github.com/aiparallel0/teb2.git}"
REPO_BRANCH="${REPO_BRANCH:-main}"
# If the repo is private, set GITHUB_TOKEN to a Personal Access Token
# with at least `repo:read` (fine-grained: contents=read). The token is
# embedded into the remote URL ONLY for this repo via `git remote
# set-url`, persisted to /opt/teb2/.git/config (chmod 600 by git), and
# also written to /etc/teb2/webhook.env so redeploy.sh can fetch.
GITHUB_TOKEN="${GITHUB_TOKEN:-}"
INSTALL_DIR="${INSTALL_DIR:-/opt/teb2}"
DOMAIN_HINT="${DOMAIN_HINT:-portearchive.com}"
LOOPBACK_PORT="${TEB2_LOOPBACK_PORT:-18082}"
WEBHOOK_PORT="${TEB2_WEBHOOK_PORT:-9001}"
HEALTH_TIMEOUT="${HEALTH_TIMEOUT:-120}"
NGINX_SNIPPET="/etc/nginx/snippets/teb2.conf"
WEBHOOK_ENV="/etc/teb2/webhook.env"
WEBHOOK_UNIT="/etc/systemd/system/teb2-webhook.service"
# ────────────────────────────────────────────────────────────────────────

RED='\033[0;31m'; YEL='\033[1;33m'; GRN='\033[0;32m'; CYA='\033[0;36m'; NC='\033[0m'
info() { echo -e "${GRN}[teb2]${NC} $*"; }
warn() { echo -e "${YEL}[warn]${NC} $*" >&2; }
err()  { echo -e "${RED}[error]${NC} $*" >&2; }
abort(){ err "$*"; exit 1; }

# ── 1. Preflight ────────────────────────────────────────────────────────
info "Step 1/11 — preflight"
[[ $EUID -eq 0 ]] || abort "must run as root (try: sudo bash $0)"
for c in nginx docker python3 openssl curl git awk sed; do
    command -v "$c" >/dev/null || abort "missing required command: $c"
done
if ! docker compose version >/dev/null 2>&1; then
    command -v docker-compose >/dev/null \
        || abort "docker compose plugin (or docker-compose) required"
fi
COMPOSE="docker compose"; docker compose version >/dev/null 2>&1 \
    || COMPOSE="docker-compose"
systemctl --version >/dev/null || abort "systemd required"

# Docker daemon must actually be running. The CLI being installed is
# not enough — `docker info` fails with "Cannot connect to the Docker
# daemon" when dockerd is stopped.
if ! docker info >/dev/null 2>&1; then
    info "  docker daemon not running — starting and enabling at boot"
    systemctl enable --now docker || abort "systemctl enable --now docker failed"
    # daemon takes a beat to start accepting connections
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        docker info >/dev/null 2>&1 && break
        sleep 1
    done
    docker info >/dev/null 2>&1 \
        || abort "docker daemon still not reachable after start; see: journalctl -u docker -n 50"
fi
info "  docker daemon: $(docker info --format '{{.ServerVersion}}' 2>/dev/null || echo 'unknown')"

# Sanity: host nginx is running and serving portearchive.com somewhere.
if ! nginx -T 2>/dev/null | grep -Eq "server_name[^;]*${DOMAIN_HINT}"; then
    warn "did not find a server_name matching '${DOMAIN_HINT}' in nginx -T"
    warn "continuing anyway, but step 7 (auto-insert) will probably fail"
fi

# Warn if ports are taken.
port_free() {
    ! (ss -tlnH "sport = :$1" 2>/dev/null | grep -q .)
}
port_free "$LOOPBACK_PORT" \
    || abort "loopback port $LOOPBACK_PORT is already in use — set TEB2_LOOPBACK_PORT=... and re-run"
port_free "$WEBHOOK_PORT" \
    || warn "webhook port $WEBHOOK_PORT already in use (might be a previous teb2-webhook — will be reclaimed by systemctl)"

# ── 2. Repo ─────────────────────────────────────────────────────────────
info "Step 2/11 — clone or update repo at $INSTALL_DIR"

# Build an auth-bearing URL only if a token was supplied. Embedding the
# token in the remote URL is the simplest way to make every subsequent
# `git fetch` (including from redeploy.sh, run by systemd) work without
# stdin or a credential helper. Git stores .git/config mode 600.
auth_url() {
    if [[ -n "$GITHUB_TOKEN" ]]; then
        # Strip any existing user:pass@ prefix, then re-insert oauth2:TOKEN@
        echo "$1" | sed -E "s#^(https?://)([^@]+@)?#\1oauth2:${GITHUB_TOKEN}@#"
    else
        echo "$1"
    fi
}
CLONE_URL="$(auth_url "$REPO_URL")"

if [[ -d "$INSTALL_DIR/.git" ]]; then
    # Re-set the remote URL each run so a freshly-supplied token replaces
    # an expired one without requiring `rm -rf /opt/teb2`.
    git -C "$INSTALL_DIR" remote set-url origin "$CLONE_URL"
    if ! git -C "$INSTALL_DIR" fetch --quiet origin 2>/dev/null; then
        if [[ -z "$GITHUB_TOKEN" ]]; then
            abort "git fetch failed and GITHUB_TOKEN is empty — set GITHUB_TOKEN=ghp_… and re-run"
        fi
        abort "git fetch failed even with GITHUB_TOKEN set — token may be expired or scoped wrong"
    fi
    git -C "$INSTALL_DIR" checkout --quiet "$REPO_BRANCH"
    if ! git -C "$INSTALL_DIR" merge --ff-only "origin/$REPO_BRANCH" >/dev/null 2>&1; then
        warn "cannot fast-forward (local edits?) — leaving repo as is"
    fi
else
    if ! git clone --quiet --branch "$REPO_BRANCH" "$CLONE_URL" "$INSTALL_DIR" 2>/dev/null; then
        if [[ -z "$GITHUB_TOKEN" ]]; then
            abort "git clone failed and GITHUB_TOKEN is empty — repo is private; set GITHUB_TOKEN=ghp_… and re-run"
        fi
        abort "git clone failed even with GITHUB_TOKEN set — token may be expired or scoped wrong"
    fi
fi
cd "$INSTALL_DIR"
CURRENT_SHA="$(git rev-parse --short HEAD)"
info "  repo at $CURRENT_SHA"

# ── 3. .env ─────────────────────────────────────────────────────────────
info "Step 3/11 — generate .env"
if [[ ! -f .env ]]; then
    cp .env.production.example .env
    info "  created .env from .env.production.example"
fi
if grep -qE '^SECRET=(change_me|CHANGE_ME|$)' .env; then
    sed -i "s|^SECRET=.*|SECRET=$(openssl rand -hex 32)|" .env
    info "  SECRET generated"
fi
grep -q '^TEB2_LOOPBACK_PORT=' .env \
    || echo "TEB2_LOOPBACK_PORT=${LOOPBACK_PORT}" >> .env
# Remove any stale LISTEN_PORT=80/443 lines — we do NOT bind public ports
sed -i '/^LISTEN_PORT=/d; /^LISTEN_TLS_PORT=/d' .env

# ── 4. Webhook env + secret ─────────────────────────────────────────────
info "Step 4/11 — webhook secret"
mkdir -p "$(dirname "$WEBHOOK_ENV")"
if [[ ! -f "$WEBHOOK_ENV" ]]; then
    WHS="$(openssl rand -hex 32)"
    cat > "$WEBHOOK_ENV" <<EOF
TEB2_WEBHOOK_PORT=${WEBHOOK_PORT}
TEB2_WEBHOOK_SECRET=${WHS}
TEB2_LOOPBACK_PORT=${LOOPBACK_PORT}
EOF
    chmod 600 "$WEBHOOK_ENV"
    info "  wrote $WEBHOOK_ENV (secret: ${WHS:0:8}…${WHS: -4})"
else
    info "  $WEBHOOK_ENV already exists — keeping existing secret"
fi

# Persist GITHUB_TOKEN into webhook.env so redeploy.sh (run by systemd
# with EnvironmentFile=$WEBHOOK_ENV) can re-set the remote URL on each
# auto-redeploy. Idempotent: only writes if a token is supplied.
if [[ -n "$GITHUB_TOKEN" ]]; then
    if grep -q '^GITHUB_TOKEN=' "$WEBHOOK_ENV"; then
        sed -i "s|^GITHUB_TOKEN=.*|GITHUB_TOKEN=${GITHUB_TOKEN}|" "$WEBHOOK_ENV"
    else
        echo "GITHUB_TOKEN=${GITHUB_TOKEN}" >> "$WEBHOOK_ENV"
    fi
fi
# shellcheck source=/dev/null
set -a; . "$WEBHOOK_ENV"; set +a

# ── 5. adnanh/webhook package ───────────────────────────────────────────
info "Step 5/11 — install adnanh/webhook"
if ! command -v webhook >/dev/null; then
    apt-get update -qq
    apt-get install -y -qq webhook \
        || abort "apt-get install webhook failed — the 'webhook' package should be available on Debian/Ubuntu"
fi
info "  $(webhook -version 2>&1 | head -1)"

# Render hooks.json placeholders with real secret + port
HOOKS_SRC="$INSTALL_DIR/ops/portearchive/hooks.json"
HOOKS_RENDERED="$INSTALL_DIR/ops/portearchive/.hooks.rendered.json"
sed \
    -e "s|__TEB2_WEBHOOK_SECRET_PLACEHOLDER__|${TEB2_WEBHOOK_SECRET}|g" \
    "$HOOKS_SRC" > "$HOOKS_RENDERED"
chmod 600 "$HOOKS_RENDERED"

# ── 6. Nginx snippet ────────────────────────────────────────────────────
info "Step 6/11 — render $NGINX_SNIPPET"
mkdir -p "$(dirname "$NGINX_SNIPPET")"
sed \
    -e "s|__TEB2_LOOPBACK_PORT__|${TEB2_LOOPBACK_PORT}|g" \
    -e "s|__TEB2_WEBHOOK_PORT__|${TEB2_WEBHOOK_PORT}|g" \
    "$INSTALL_DIR/ops/portearchive/nginx-teb2.conf" > "$NGINX_SNIPPET"

# ── 7. Insert include into portearchive server block ────────────────────
info "Step 7/11 — wire include into portearchive server block"
# Find candidate files (sites-enabled + conf.d) that contain BOTH a
# matching server_name AND a :443 listener. Multiple server blocks can
# live in one file (common: a :80 redirect + a :443 main + extras).
# We only care about the file that has the 443-ssl block, because that
# is the one we will inject the include into.
MAPFILE=()
while IFS= read -r -d '' f; do
    if grep -Eq "server_name[^;]*${DOMAIN_HINT}" "$f" \
       && grep -Eq "listen[^;]*\b443\b" "$f"; then
        MAPFILE+=("$f")
    fi
done < <(find /etc/nginx/sites-enabled /etc/nginx/conf.d -maxdepth 2 -type f -print0 2>/dev/null)

# If no file had both, fall back to any file that mentions the domain —
# the Python inserter below will still refuse to edit if no 443 block is
# present, so this is a cheap widening, not a safety hole.
if [[ ${#MAPFILE[@]} -eq 0 ]]; then
    while IFS= read -r -d '' f; do
        if grep -Eq "server_name[^;]*${DOMAIN_HINT}" "$f"; then
            MAPFILE+=("$f")
        fi
    done < <(find /etc/nginx/sites-enabled /etc/nginx/conf.d -maxdepth 2 -type f -print0 2>/dev/null)
fi

if [[ ${#MAPFILE[@]} -eq 0 ]]; then
    warn "no nginx config file references ${DOMAIN_HINT} — add manually:"
    warn "    include ${NGINX_SNIPPET};"
    warn "inside the existing portearchive server{} block, then run:"
    warn "    nginx -t && systemctl reload nginx"
    MAPFILE=()
elif [[ ${#MAPFILE[@]} -gt 1 ]]; then
    warn "multiple candidate nginx files (both have server_name+listen 443):"
    for f in "${MAPFILE[@]}"; do warn "    $f"; done
    warn "will try each in order until one contains a matching 443 server block"
fi

# Iterate through candidates. Python inserter sets done=true on the
# FIRST file whose 443 server block matches. If no file matches, we
# fall through with INSERTED=0 and print manual instructions.
INSERTED=0
for TARGET in "${MAPFILE[@]:-}"; do
    [[ -n "$TARGET" ]] || continue
    (( INSERTED == 1 )) && break
    if grep -qF "include ${NGINX_SNIPPET};" "$TARGET"; then
        info "  include already present in $TARGET"
        INSERTED=1
        continue
    fi
    BACKUP="${TARGET}.bak.$(date +%s)"
    cp -a "$TARGET" "$BACKUP"
    # Python inserter: put `include …/teb2.conf;` right before the
    # closing } of the FIRST server block in this file that has both
    # `server_name ... ${domain}` and `listen ... 443`. Exits 3 if no
    # such block exists; we restore the backup and move to the next
    # candidate file. Exits 0 on success.
    set +e
    python3 - "$TARGET" "$NGINX_SNIPPET" "$DOMAIN_HINT" <<'PY'
import re, sys
path, snippet, domain = sys.argv[1], sys.argv[2], sys.argv[3]
src = open(path).read()
out = []
i = 0
done = False
while i < len(src):
    m = re.compile(r"\bserver\s*\{").search(src, i)
    if not m:
        out.append(src[i:]); break
    out.append(src[i:m.end()])
    depth = 1
    j = m.end()
    block_start = j
    while depth > 0 and j < len(src):
        ch = src[j]
        if ch == '{': depth += 1
        elif ch == '}': depth -= 1
        j += 1
    block = src[block_start:j-1]
    matches_domain = re.search(r"server_name[^;]*\b" + re.escape(domain) + r"\b", block)
    has_443 = re.search(r"listen[^;]*\b443\b", block)
    already = ("include " + snippet) in block
    if matches_domain and has_443 and not already and not done:
        insert = "\n    include {};\n".format(snippet)
        block = block.rstrip() + insert
        done = True
    out.append(block)
    out.append("}")
    i = j
if not done:
    sys.stderr.write("no 443 server block in this file; skipping\n")
    sys.exit(3)
open(path, 'w').write("".join(out))
PY
    RC=$?
    set -e
    if [[ $RC -eq 3 ]]; then
        info "  $TARGET has no matching 443 block — restoring and trying next"
        cp -a "$BACKUP" "$TARGET"
        continue
    fi
    if [[ $RC -ne 0 ]]; then
        warn "python inserter returned $RC on $TARGET — restoring backup"
        cp -a "$BACKUP" "$TARGET"
        continue
    fi
    if ! nginx -t 2>&1; then
        err "nginx -t failed after insert into $TARGET — restoring backup"
        cp -a "$BACKUP" "$TARGET"
        nginx -t || abort "pre-install config itself fails nginx -t — manual fix required"
        warn "moving to next candidate file"
        continue
    fi
    systemctl reload nginx
    info "  inserted include into $TARGET and reloaded nginx (backup: $BACKUP)"
    INSERTED=1
done

if [[ $INSERTED -eq 0 && ${#MAPFILE[@]} -gt 0 ]]; then
    warn "could not auto-insert into any candidate file — add this line by hand"
    warn "inside the portearchive.com server{} block that listens on 443:"
    warn "    include ${NGINX_SNIPPET};"
    warn "then: nginx -t && systemctl reload nginx"
fi

# ── 8. Build + start app container ──────────────────────────────────────
info "Step 8/11 — build and start teb2 app container"
$COMPOSE \
    -f "$INSTALL_DIR/docker-compose.yml" \
    -f "$INSTALL_DIR/ops/portearchive/compose.yml" \
    up -d --build app

# ── 9. systemd webhook unit ─────────────────────────────────────────────
info "Step 9/11 — install systemd unit for webhook receiver"
install -m 644 \
    "$INSTALL_DIR/ops/portearchive/teb2-webhook.service" \
    "$WEBHOOK_UNIT"
# Point ExecStart at the RENDERED hooks file (with secret substituted),
# not the placeholder one committed to git.
sed -i "s|/opt/teb2/ops/portearchive/hooks.json|$HOOKS_RENDERED|g" "$WEBHOOK_UNIT"
systemctl daemon-reload
systemctl enable --now teb2-webhook.service
sleep 1
systemctl is-active --quiet teb2-webhook.service \
    || warn "teb2-webhook.service not active — journalctl -u teb2-webhook for details"

# ── 10. Health check ────────────────────────────────────────────────────
info "Step 10/11 — wait for /healthz on 127.0.0.1:${LOOPBACK_PORT}"
ELAPSED=0
until curl -sf --max-time 3 "http://127.0.0.1:${LOOPBACK_PORT}/healthz" >/dev/null 2>&1; do
    if (( ELAPSED >= HEALTH_TIMEOUT )); then
        err "health check timed out after ${HEALTH_TIMEOUT}s"
        err "container logs:"
        $COMPOSE -f "$INSTALL_DIR/docker-compose.yml" \
                 -f "$INSTALL_DIR/ops/portearchive/compose.yml" \
                 logs --tail=40 app || true
        abort "aborting — nginx wired but app is not healthy"
    fi
    echo -n "."
    sleep 3; ELAPSED=$((ELAPSED + 3))
done
echo ""
info "  app is healthy on 127.0.0.1:${LOOPBACK_PORT}"

# ── 11. Summary + GitHub webhook instructions ───────────────────────────
info "Step 11/11 — done"
PUBLIC_URL="https://${DOMAIN_HINT}/teb2/"
WEBHOOK_URL="https://${DOMAIN_HINT}/hooks/teb2-deploy"
echo ""
echo -e "${GRN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GRN}║  teb2 is installed at ${PUBLIC_URL}  ${NC}"
echo -e "${GRN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYA}Smoke test (from this droplet):${NC}"
echo "    curl -sf http://127.0.0.1:${LOOPBACK_PORT}/healthz && echo"
echo "    curl -sf ${PUBLIC_URL%/}/healthz && echo"
echo ""
echo -e "${CYA}GitHub webhook — paste these into${NC}"
echo -e "${CYA}  https://github.com/aiparallel0/teb2/settings/hooks${NC}"
echo ""
echo "    Payload URL    : ${WEBHOOK_URL}"
echo "    Content type   : application/json"
echo "    Secret         : (value of TEB2_WEBHOOK_SECRET in $WEBHOOK_ENV)"
echo "    SSL verification: Enable"
echo "    Events         : Just the push event"
echo "    Active         : yes"
echo ""
echo -e "${CYA}Verify webhook wiring from GitHub \"Recent Deliveries\" tab after${NC}"
echo -e "${CYA}the first push to main.${NC}"
echo ""
echo -e "${CYA}Useful:${NC}"
echo "    journalctl -u teb2-webhook -f"
echo "    $COMPOSE -f $INSTALL_DIR/docker-compose.yml -f $INSTALL_DIR/ops/portearchive/compose.yml logs -f app"
echo "    systemctl restart teb2-webhook"
echo ""
