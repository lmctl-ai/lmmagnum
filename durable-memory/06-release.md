# CI and releases

`.github/workflows/ci.yml` builds and tests on linux-x64 (ubuntu-24.04), linux-arm64 (ubuntu-24.04-arm), macos-arm64 (macos-15), macos-x64 (macos-15-intel) and windows-x64 (windows-2022 + MinGW-w64 via choco). Each job: install the published `@lmctl-ai/lmctl` (provides `lmscript`), run `./test.sh`, stage the binary as `lmmagnum-OS-ARCH[.exe]` (the naming lmscript's `install="github:lmctl-ai/lmmagnum@latest"` expects) with license files, upload an artifact.

Pushing a `v*` tag runs the `release` job, which collects the five binaries, license files and `SHA256SUMS` and creates the GitHub release. To release: bump `project(lmmagnum VERSION x.y.z)` in CMakeLists.txt, commit, `git tag vx.y.z && git push --tags`. `--version` must equal the tag (test.sh checks it against CMakeLists.txt, not the tag).

Static linking: Linux `-static`; macOS cannot link libSystem statically so only libstdc++/libgcc concerns are absent (Apple clang uses system libc++); Windows `-static` with MinGW.

Status: all five platforms build and pass test.sh in CI (first green run on main, 2026-10-23).

License: MIT (Magnum, Corrade, cpp-dsl-sdk and lmcad are MIT, so the combined binary is MIT-compatible with no copyleft obligations beyond the glibc/GCC runtime notes in THIRD_PARTY_NOTICES.md).
