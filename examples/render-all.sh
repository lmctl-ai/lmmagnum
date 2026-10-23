#!/usr/bin/env bash
# Run every example from an empty scratch directory (outputs go to ./images and ./build).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PATH="$HERE/../build:$PATH"
for f in "$HERE"/*.lms; do lmscript "$f"; done
