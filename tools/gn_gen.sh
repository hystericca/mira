#!/usr/bin/env bash
set -euo pipefail

# hi if ur reading change this to ur own directory
DAWN_ROOT="${DAWN_ROOT:-$HOME/Developer/dawn}"
DEPOT_TOOLS="${DEPOT_TOOLS:-$HOME/Developer/depot_tools}"
OUT_DIR="${1:-$DAWN_ROOT/out/mira-debug}"
MODE="${2:-debug}"

if [[ ! -x "$DEPOT_TOOLS/gn" ]]; then
  echo "missing gn at $DEPOT_TOOLS/gn" >&2
  exit 1
fi

if [[ ! -d "$DAWN_ROOT/.git" ]]; then
  echo "missing Dawn checkout at $DAWN_ROOT" >&2
  exit 1
fi

if [[ ! -x "$DAWN_ROOT/third_party/emsdk/upstream/emscripten/em++" ]]; then
  echo "missing Emscripten in Dawn checkout; run gclient sync in $DAWN_ROOT" >&2
  exit 1
fi

if [[ ! -f "$DAWN_ROOT/third_party/emdawnwebgpu/pkg/webgpu/src/library_webgpu.js" ||
      ! -f "$DAWN_ROOT/third_party/emdawnwebgpu/pkg/webgpu/src/webgpu.cpp" ]]; then
  echo "missing emdawnwebgpu sources in Dawn checkout; run gclient sync in $DAWN_ROOT" >&2
  exit 1
fi

MIRA_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -L "$DAWN_ROOT/mira" || ! -e "$DAWN_ROOT/mira" ]]; then
  rm -f "$DAWN_ROOT/mira"
else
  echo "$DAWN_ROOT/mira exists and is not a symlink" >&2
  exit 1
fi
ln -s "$MIRA_ROOT" "$DAWN_ROOT/mira"

EXCLUDE="$DAWN_ROOT/.git/info/exclude"
if ! grep -qxF "/mira" "$EXCLUDE"; then
  printf '\n/mira\n' >> "$EXCLUDE"
fi

if [[ "$MODE" == "release" ]]; then
  IS_DEBUG=false
  SYMBOL_LEVEL=1
else
  IS_DEBUG=true
  SYMBOL_LEVEL=2
fi

PATH="$DEPOT_TOOLS:$DAWN_ROOT/buildtools/mac:$PATH" "$DEPOT_TOOLS/gn" gen "$OUT_DIR" \
  --root="$DAWN_ROOT" \
  --root-target=//mira:gn_root \
  --add-export-compile-commands=//mira/* \
  --args="
is_debug = $IS_DEBUG
symbol_level = $SYMBOL_LEVEL
use_system_xcode = true
use_siso = false
dawn_use_swiftshader = false
dawn_enable_vulkan = false
dawn_enable_vulkan_validation_layers = false
dawn_enable_vulkan_loader = false
dawn_enable_metal = true
dawn_enable_null = true
dawn_enable_desktop_gl = false
dawn_enable_opengles = false
dawn_enable_webgpu_on_webgpu = false
dawn_build_node_bindings = false
"

python3 "$MIRA_ROOT/tools/write_clangd_database.py" \
  "$OUT_DIR/compile_commands.json" \
  "$MIRA_ROOT/compile_commands.json"
