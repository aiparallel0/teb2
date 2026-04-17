#!/bin/sh
set -eu

# teb2 deployment script
# Usage: ./deploy.sh [up|down|restart|status|logs|backup|update]

APP_NAME="teb2"
COMPOSE_FILE="docker-compose.yml"
BACKUP_DIR="./backups"

check_deps() {
    for cmd in docker; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            echo "Error: $cmd is required but not installed." >&2
            exit 1
        fi
    done
    # Use docker compose (v2) or docker-compose (v1)
    if docker compose version >/dev/null 2>&1; then
        COMPOSE="docker compose"
    elif command -v docker-compose >/dev/null 2>&1; then
        COMPOSE="docker-compose"
    else
        echo "Error: docker compose is required." >&2
        exit 1
    fi
}

ensure_env() {
    if [ ! -f .env ]; then
        if [ -f .env.production.example ]; then
            cp .env.production.example .env
            echo "Created .env from .env.production.example — edit before running." >&2
            echo "In particular, set SECRET to a random 32+ character string." >&2
            exit 1
        elif [ -f .env.example ]; then
            cp .env.example .env
            echo "Created .env from .env.example — edit before running." >&2
            exit 1
        else
            echo "Error: no .env template found." >&2
            exit 1
        fi
    fi
}

cmd_up() {
    ensure_env
    $COMPOSE -f "$COMPOSE_FILE" up -d --build
    echo "$APP_NAME is running."
    echo "Health: curl -s http://localhost:${LISTEN_PORT:-80}/healthz"
}

cmd_down() {
    $COMPOSE -f "$COMPOSE_FILE" down
    echo "$APP_NAME stopped."
}

cmd_restart() {
    $COMPOSE -f "$COMPOSE_FILE" restart
    echo "$APP_NAME restarted."
}

cmd_status() {
    $COMPOSE -f "$COMPOSE_FILE" ps
}

cmd_logs() {
    $COMPOSE -f "$COMPOSE_FILE" logs --tail=100 -f
}

cmd_backup() {
    mkdir -p "$BACKUP_DIR"
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    CONTAINER=$($COMPOSE -f "$COMPOSE_FILE" ps -q app)
    if [ -z "$CONTAINER" ]; then
        echo "Error: app container not running." >&2
        exit 1
    fi
    docker cp "$CONTAINER:/app/data/teb2.db" \
        "$BACKUP_DIR/teb2_${TIMESTAMP}.db"
    echo "Backup saved: $BACKUP_DIR/teb2_${TIMESTAMP}.db"
}

cmd_update() {
    cmd_backup
    git pull --ff-only
    $COMPOSE -f "$COMPOSE_FILE" up -d --build
    echo "$APP_NAME updated."
}

check_deps

case "${1:-help}" in
    up)      cmd_up      ;;
    down)    cmd_down    ;;
    restart) cmd_restart ;;
    status)  cmd_status  ;;
    logs)    cmd_logs    ;;
    backup)  cmd_backup  ;;
    update)  cmd_update  ;;
    *)
        echo "Usage: $0 {up|down|restart|status|logs|backup|update}"
        echo ""
        echo "  up       Build and start all services"
        echo "  down     Stop all services"
        echo "  restart  Restart all services"
        echo "  status   Show service status"
        echo "  logs     Tail service logs"
        echo "  backup   Backup SQLite database"
        echo "  update   Pull latest code and redeploy"
        exit 0
        ;;
esac
