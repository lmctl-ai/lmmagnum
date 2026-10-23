#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; cd "$HERE"
BUILD_DIR="${BUILD_DIR:-build}"; EXE=""
case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) EXE=".exe";; esac
HOST="$BUILD_DIR/lmmagnum$EXE"
pass=0; ok(){ pass=$((pass+1)); echo "ok $pass - $1"; }; fail(){ echo "FAIL: $*" >&2; exit 1; }
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lmmagnum-test-XXXXXX")"; trap 'rm -rf "$WORK"' EXIT
scripts/sync-sdk.sh --verify >/dev/null || fail "SDK header pin mismatch"; ok "SDK header matches sdk.lock"
scripts/sync-deps.sh >/dev/null
cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMMAGNUM_FULL_STATIC="${LMMAGNUM_FULL_STATIC:-ON}"
cmake --build "$BUILD_DIR" -j3; ok "build"
want="lmmagnum $(sed -n 's/^project(lmmagnum VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
[ "$("$HOST" --version | tr -d '\r')" = "$want" ] || fail "--version is not '$want'"; ok "--version prints '$want'"
"$HOST" --dsl-schema --json > "$WORK/schema.json"; cmp schema/lmmagnum.dsl.json "$WORK/schema.json" || fail "embedded schema differs"; ok "embedded schema is byte-identical"
LMSCRIPT="$(scripts/select-compiler.sh)"
diff -u <("$LMSCRIPT" --dsl-signatures schema/lmmagnum.dsl.json | tr -d '\r') <("$HOST" --dsl-signatures | tr -d '\r') || fail "signatures differ"; ok "host and compiler signatures match"
# Run every example in a scratch directory and check each produced a PNG/GIF/OBJ.
export PATH="$HERE/$BUILD_DIR:$PATH"
mkdir "$WORK/run"; ( cd "$WORK/run"; for f in "$HERE"/examples/*.lms; do "$LMSCRIPT" "$f" >/dev/null || fail "example $(basename "$f")"; done )
for png in triangle textured-quad primitives model-viewer area-lights raytracing octree motionblur; do
  [ "$(head -c 4 "$WORK/run/images/$png.png" | tail -c 3)" = "PNG" ] || fail "images/$png.png is not a PNG"; done
[ -s "$WORK/run/build/primitives.obj" ] || fail "OBJ export"; ok "all examples ran and wrote valid output"
if [ "${LMMAGNUM_FULL_STATIC:-ON}" = ON ] && [ "$(uname -s)" = Linux ]; then
  out="$(ldd "$HOST" 2>&1 || true)"; case "$out" in *"not a dynamic executable"*|*"statically linked"*) ;; *) fail "binary is not static";; esac; ok "binary is fully static"
fi
echo "all $pass checks passed"
