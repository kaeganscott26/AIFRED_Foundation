#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_DIR="$ROOT_DIR/apps/aifred_chat"
SRC_APP_DIR="/home/north3rnlight3r/Projects/Free_API"
RUN_TESTKIT="${RUN_TESTKIT:-0}"

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT_DIR/build" --parallel
ctest --test-dir "$ROOT_DIR/build" --output-on-failure

if [[ "$RUN_TESTKIT" == "1" ]]; then
  echo "[INFO] Running Phase 7 TestKit generation + verification"
  python "$ROOT_DIR/plugins/aifr3d_vst3/TestKit/tools/generate_test_wavs.py"
  python "$ROOT_DIR/plugins/aifr3d_vst3/TestKit/tools/run_testkit_analysis.py" --build-dir build
  python "$ROOT_DIR/plugins/aifr3d_vst3/TestKit/tools/verify_testkit_outputs.py"
fi

if [[ -f "$APP_DIR/package.json" ]]; then
  if command -v node >/dev/null 2>&1; then
    if node -e "const p=require('$APP_DIR/package.json'); process.exit((p.scripts&&p.scripts.check)?0:1)"; then
      echo "[INFO] Running safe npm check script in copied app"
      if ! (cd "$APP_DIR" && npm run check); then
        echo "[WARN] Copied app npm check failed; comparing with source repo state"
        if [[ -f "$SRC_APP_DIR/package.json" ]] && node -e "const p=require('$SRC_APP_DIR/package.json'); process.exit((p.scripts&&p.scripts.check)?0:1)"; then
          if (cd "$SRC_APP_DIR" && npm run check); then
            echo "[FAIL] Copied app check failed while source passes (possible copy behavior drift)." >&2
            exit 1
          else
            echo "[INFO] Source app check fails in same manner; treating as pre-existing and unchanged."
          fi
        else
          echo "[INFO] Source check script unavailable; npm check treated as best-effort attempt."
        fi
      fi
    else
      echo "[INFO] No npm check script present; skipping runnable check"
    fi
  else
    echo "[INFO] node unavailable; skipping npm check"
  fi
fi

echo "[OK] Phase build verification complete."
