#!/usr/bin/env bash
set -euo pipefail
SRC_DIR="${1:-.devnet}"
OUT_FILE="${2:-backup.tar.gz}"
[[ -d "$SRC_DIR" ]] || { echo "source directory not found: $SRC_DIR"; exit 1; }
mkdir -p "$(dirname "$OUT_FILE")"
tar -czf "$OUT_FILE" "$SRC_DIR"
echo "Backup written to $OUT_FILE"
