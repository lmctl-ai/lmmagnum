#!/usr/bin/env bash
# Regenerate the README pictures: run every example in a scratch directory and copy the results to docs/images/.
# Usage (after building): scripts/update-readme-images.sh
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build}"
export PATH="$HERE/$BUILD_DIR:$PATH"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmmagnum-images-XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
( cd "$WORK" && for f in "$HERE"/examples/*.lms; do lmscript "$f" >/dev/null; done )
mkdir -p "$HERE/docs/images"
cp "$WORK"/images/*.png "$HERE/docs/images/"
[ -f "$WORK/images/animated-gif.gif" ] && cp "$WORK/images/animated-gif.gif" "$HERE/docs/images/"
ls "$HERE/docs/images"
