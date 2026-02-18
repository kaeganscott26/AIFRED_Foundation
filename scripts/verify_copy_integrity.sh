#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="/home/north3rnlight3r/Projects/Free_API"
DST="$ROOT_DIR/apps/aifred_chat"

if [[ ! -d "$SRC" ]]; then
  echo "[FAIL] Source missing: $SRC" >&2
  exit 1
fi

if [[ ! -d "$DST" ]]; then
  echo "[FAIL] Copied app missing: $DST" >&2
  exit 1
fi

DIFF_OUTPUT="$(rsync -an --delete --checksum --exclude='.git' "$SRC/" "$DST/")"
if [[ -n "$DIFF_OUTPUT" ]]; then
  echo "[FAIL] Copy integrity mismatch detected. First differences:" >&2
  echo "$DIFF_OUTPUT" | sed -n '1,20p' >&2
  exit 1
fi

echo "[OK] Copy integrity verified: $DST matches $SRC (excluding .git)."
