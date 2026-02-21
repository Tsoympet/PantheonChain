#!/usr/bin/env bash
set -euo pipefail
ARCHIVE="${1:-}"
DEST_DIR="${2:-.}"
[[ -n "$ARCHIVE" ]] || { echo "usage: restore.sh <archive.tar.gz> [dest_dir]"; exit 1; }
[[ -f "$ARCHIVE" ]] || { echo "archive not found: $ARCHIVE"; exit 1; }
mkdir -p "$DEST_DIR"
tar -xzf "$ARCHIVE" -C "$DEST_DIR"
echo "Restore completed into $DEST_DIR"
