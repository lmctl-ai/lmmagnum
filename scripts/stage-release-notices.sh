#!/usr/bin/env bash
# Stage license files beside a release binary: scripts/stage-release-notices.sh <dist-dir>
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST="${1:?dist directory}"
mkdir -p "$DIST"
cp "$ROOT/LICENSE" "$DIST/LICENSE"
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$DIST/THIRD_PARTY_NOTICES"
cp "$ROOT/third_party/magnum/COPYING" "$DIST/Magnum-COPYING"
cp "$ROOT/third_party/corrade/COPYING" "$DIST/Corrade-COPYING"
