#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$ROOT_DIR/src/renderer"
DEST_DIR="$ROOT_DIR/android/app/src/main/assets/www"

rm -rf "$DEST_DIR"
mkdir -p "$DEST_DIR/assets"

cp "$SRC_DIR/index.html" "$DEST_DIR/index.html"
cp "$SRC_DIR/app.js" "$DEST_DIR/app.js"
cp "$SRC_DIR/styles.css" "$DEST_DIR/styles.css"

if [ -d "$SRC_DIR/assets" ]; then
  cp -R "$SRC_DIR/assets/." "$DEST_DIR/assets/"
fi

echo "Android web assets synced to $DEST_DIR"
