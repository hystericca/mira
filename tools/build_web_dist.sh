#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DAWN_ROOT="${DAWN_ROOT:-$HOME/Developer/dawn}"
OUT_DIR="${OUT_DIR:-$DAWN_ROOT/out/mira-release}"
BUILD="${BUILD:-1}"

if [[ "$BUILD" != "0" ]]; then
	"$repo_root/tools/build.sh" "$OUT_DIR" release
fi

web_root="$OUT_DIR/wasm"
dist_root="$repo_root/web-dist"
asset_files=(mira_web.html mira_web.js mira_web.wasm)

for file in "${asset_files[@]}"; do
	if [[ ! -f "$web_root/$file" ]]; then
		echo "missing expected build artifact: $web_root/$file" >&2
		exit 1
	fi
done

rm -rf "$dist_root"
mkdir -p "$dist_root"
for file in "${asset_files[@]}"; do
	cp "$web_root/$file" "$dist_root/$file"
done

{
	source_hash="$(
		cd "$repo_root"
		{
			git ls-files
			if [[ -f DAWN_REVISION ]]; then
				printf 'DAWN_REVISION\n'
			fi
		} |
			grep -v '^web-dist/' |
			sort -u |
			xargs shasum -a 256 |
			shasum -a 256 |
			awk '{print $1}'
	)"
	printf 'mira_source_hash=%s\n' "$source_hash"
	printf 'dawn_commit=%s\n' "$(git -C "$DAWN_ROOT" rev-parse HEAD)"
	printf 'mode=release\n'
} > "$dist_root/build.txt"

find "$dist_root" -maxdepth 1 -type f -print0 |
	xargs -0 stat -f "%N %z bytes" |
	sort
