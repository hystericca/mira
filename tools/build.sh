#!/usr/bin/env bash
set -euo pipefail

DAWN_ROOT="${DAWN_ROOT:-$HOME/Developer/dawn}"
DEPOT_TOOLS="${DEPOT_TOOLS:-$HOME/Developer/depot_tools}"
OUT_DIR="${1:-$DAWN_ROOT/out/mira-debug}"
MODE="${2:-debug}"

"$(dirname "${BASH_SOURCE[0]}")/gn_gen.sh" "$OUT_DIR" "$MODE"

ant exec tsgo --noEmit -p tsconfig.json

PATH="$DEPOT_TOOLS:$DAWN_ROOT/buildtools/mac:$PATH" "$DEPOT_TOOLS/autoninja" -C "$OUT_DIR" \
  mira:mira_tests \
  mira:mira_draw_bench \
  mira:mira \
  wasm/mira_web.html

"$OUT_DIR/mira_tests"
"$OUT_DIR/mira_draw_bench"
"$OUT_DIR/mira"
