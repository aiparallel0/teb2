#!/bin/sh
# teb2 online SQLite backup.
#
# Intended to be invoked by a systemd timer (see docs/DEPLOY.md). Uses
# sqlite3 .backup which is safe against a running writer -- unlike a
# raw cp of the WAL file, .backup acquires a read transaction and
# produces a consistent snapshot.
#
# Environment:
#   DB_PATH      -- path to the live teb2.db  (default: /var/lib/teb2/teb2.db)
#   BACKUP_DIR   -- where to write snapshots  (default: /var/lib/teb2/backups)
#   KEEP_DAYS    -- delete snapshots older than this many days (default: 14)
#
# Exit non-zero on any error so the systemd timer surfaces failures.

set -eu

DB_PATH="${DB_PATH:-/var/lib/teb2/teb2.db}"
BACKUP_DIR="${BACKUP_DIR:-/var/lib/teb2/backups}"
KEEP_DAYS="${KEEP_DAYS:-14}"

if [ ! -r "$DB_PATH" ]; then
    echo "teb2-backup: $DB_PATH not readable" >&2
    exit 1
fi

if ! command -v sqlite3 >/dev/null 2>&1; then
    echo "teb2-backup: sqlite3 CLI not installed" >&2
    exit 1
fi

mkdir -p "$BACKUP_DIR"
chmod 700 "$BACKUP_DIR"

TS=$(date -u +%Y%m%dT%H%M%SZ)
OUT="$BACKUP_DIR/teb2_${TS}.db"

# Atomically create a consistent snapshot using SQLite's backup API.
# Write to a .tmp then rename so a reader never sees a partial file.
sqlite3 "$DB_PATH" ".backup '${OUT}.tmp'"
sync
mv "${OUT}.tmp" "$OUT"
chmod 600 "$OUT"

# Prune old backups. -mtime +N matches files strictly older than N*24h.
find "$BACKUP_DIR" -maxdepth 1 -type f -name 'teb2_*.db' \
    -mtime "+${KEEP_DAYS}" -print -delete

echo "teb2-backup: wrote $OUT"
