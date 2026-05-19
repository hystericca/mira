#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DAWN_ROOT="${DAWN_ROOT:-$HOME/Developer/dawn}"
OUT_DIR="${OUT_DIR:-$DAWN_ROOT/out/mira-release}"
REMOTE="${REMOTE:-aqua}"
REMOTE_ROOT="${REMOTE_ROOT:-/opt/mira-web}"
BUILD="${BUILD:-1}"

if [[ "$BUILD" != "0" ]]; then
    "$repo_root/tools/build.sh" "$OUT_DIR" release
fi

web_root="$OUT_DIR/wasm"
asset_files=(mira_web.html mira_web.js mira_web.wasm)
for file in "${asset_files[@]}"; do
    if [[ ! -f "$web_root/$file" ]]; then
        echo "missing expected build artifact: $web_root/$file" >&2
        exit 1
    fi
done

release="$(date -u +%Y%m%d%H%M%S)"
remote_tmp="/tmp/mira-web-$release"

ssh "$REMOTE" bash -s -- "$remote_tmp" <<'REMOTE'
set -euo pipefail
remote_tmp="$1"
rm -rf "$remote_tmp"
mkdir -p "$remote_tmp"
REMOTE

tar -C "$web_root" -cf - "${asset_files[@]}" | ssh "$REMOTE" tar -C "$remote_tmp" -xf -
ssh "$REMOTE" "cat > '$remote_tmp/Caddyfile'" < "$repo_root/deploy/dokploy/Caddyfile"

ssh "$REMOTE" bash -s -- "$REMOTE_ROOT" "$release" "$remote_tmp" <<'REMOTE'
set -euo pipefail
remote_root="$1"
release="$2"
remote_tmp="$3"
remote_release="$remote_root/releases/$release"

sudo mkdir -p "$remote_root/releases"
sudo rm -rf "$remote_release"
sudo mkdir -p "$remote_release"
sudo cp -a "$remote_tmp/." "$remote_release/"
sudo install -m 0644 "$remote_tmp/Caddyfile" "$remote_root/Caddyfile"
sudo rm -f "$remote_release/Caddyfile"
sudo ln -sfn "releases/$release" "$remote_root/current"
rm -rf "$remote_tmp"

echo "published $remote_release"
sudo find -L "$remote_root/current" -maxdepth 1 -type f -printf "%f %s bytes\n" | sort
REMOTE
