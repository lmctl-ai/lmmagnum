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
for png in texture-modes textured-planet textured-primitives triangle textured-quad primitives primitives-scene-graph text model-viewer area-lights raytracing octree motionblur pbr-spheres depthtesting material-sandbox lightbulb caffeine-molecule ball-stick buckyball dna-double-helix cornell-box saturn-rings truss-bridge shadows multidraw cubemap bullet fluidsimulation3d fluidsimulation2d textured-triangle level-1-sessions level-2-orchestration level-3-hands-off-lead level-4-automation; do
  [ "$(head -c 4 "$WORK/run/images/$png.png" | tail -c 3)" = "PNG" ] || fail "images/$png.png is not a PNG"; done
[ -s "$WORK/run/build/primitives.obj" ] || fail "OBJ export"; ok "all examples ran and wrote valid output"
# Textures: decoded pixels land where the UVs say, and bad input is a located one-line error (exit 3).
png_px() { python3 -I - "$@" <<'PY'
import sys, zlib, struct
data = open(sys.argv[1], 'rb').read(); pos = 8; idat = b''; w = h = 0
while pos < len(data):
    n, t = struct.unpack('>I4s', data[pos:pos+8]); body = data[pos+8:pos+8+n]; pos += 12 + n
    if t == b'IHDR': w, h = struct.unpack('>II', body[:8])
    elif t == b'IDAT': idat += body
raw = zlib.decompress(idat); stride = 1 + w * 3
assert all(raw[y * stride] == 0 for y in range(h)), 'unsupported PNG filter'
for xy in sys.argv[2:]:
    x, y = map(int, xy.split(',')); o = y * stride + 1 + x * 3; print(*raw[o:o+3])
PY
}
run_dsl() { # NAME BODY: compile and run one DSL snippet in $WORK/run; prints stdout+stderr, returns the host exit code
  local name=$1 body=$2
  printf 'p = _dsl_ cli="lmmagnum" install="github:lmctl-ai/lmmagnum@latest";\n%s\nend _dsl_\nsavedsl(p, "%s.dslb");\nr = system(concat(dsl_cli(p), ["--dsl", "%s.dslb"]), 60000);\nprintln(r.stdout + r.stderr);\n' "$body" "$name" "$name" > "$WORK/run/$name.lms"
  ( cd "$WORK/run" && "$LMSCRIPT" "$name.lms" ) 2>&1
}
SETUP='scene.background(0.0, 0.0, 0.0); scene.ambient(1.0); scene.directional_light(0.0, 0.0, -1.0, 0.0); scene.camera(40.0, 0.0, 0.0, 3.0, 0.0, 0.0, 0.0);'
out="$(run_dsl tex1 "$SETUP
let t = scene.texture(2, 2).filter(\"nearest\");
t.pixel(0, 0, 1.0, 0.0, 0.0); t.pixel(1, 0, 0.0, 1.0, 0.0); t.pixel(0, 1, 0.0, 0.0, 1.0); t.pixel(1, 1, 1.0, 1.0, 0.0);
let quad = scene.plane(2.0, material().color(1.0, 1.0, 1.0).specular(0.0).texture(t));
scene.render_png(\"tex1.png\", 100, 100);
t.save_png(\"tex1-src.png\");
quad.visible(false);
scene.plane(2.0, material().color(1.0, 1.0, 1.0).specular(0.0).texture(scene.texture_file(\"tex1-src.png\").filter(\"nearest\")));
scene.render_png(\"tex1b.png\", 100, 100);")"
case "$out" in *'"ok": true'*) ;; *) echo "$out"; fail "texture script";; esac
cmp "$WORK/run/tex1.png" "$WORK/run/tex1b.png" || fail "a texture saved with save_png and loaded with texture_file renders differently"
got="$(png_px "$WORK/run/tex1.png" 30,30 70,30 30,70 70,70 | tr '\n' '|')"
# image row 0 is the top of the quad (v = 1): red top-left, green top-right, blue bottom-left, yellow bottom-right; ambient 1.0 scales by 1.0 + fill light
python3 -I - "$got" <<'PY' || fail "textured quad corners (got $got)"
import sys
px = [list(map(int, p.split())) for p in sys.argv[1].strip('|').split('|')]
want = [(1,0,0),(0,1,0),(0,0,1),(1,1,0)]
for got, w in zip(px, want):
    assert all((v > 200) if c else (v < 60) for v, c in zip(got, w)), (px, w)
PY
ok "textures: 2x2 nearest quad shows red, green, blue and yellow where the UVs put them; PNG save/load round trip"
for case in 'scene.texture(0, 4);|between 1 and 4096' 'scene.texture(2, 2).pixel(2, 0, 0.0, 0.0, 0.0);|outside the texture' 'scene.texture(2, 2).filter("cubic");|filter must be' 'scene.texture_file("missing-file.png");|cannot open texture file' 'scene.icosphere(1.0, 2, material().texture(scene.texture(2, 2))); scene.render_png("x.png", 40, 40);|no texture coordinates'; do
  code="${case%%|*}"; want="${case##*|}"
  out="$(run_dsl texerr "$SETUP $code" || true)"
  case "$out" in *'"ok": false'*"$want"*) ;; *) echo "$out"; fail "texture error '$want'";; esac
done
ok "texture misuse is a one-line error: size, pixel range, filter name, missing file, mesh without UVs"
if [ "${LMMAGNUM_FULL_STATIC:-ON}" = ON ] && [ "$(uname -s)" = Linux ]; then
  out="$(ldd "$HOST" 2>&1 || true)"; case "$out" in *"not a dynamic executable"*|*"statically linked"*) ;; *) fail "binary is not static";; esac; ok "binary is fully static"
fi
echo "all $pass checks passed"
