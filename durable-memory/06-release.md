# CI and releases

`.github/workflows/ci.yml` builds and tests on linux-x64 (ubuntu-24.04), linux-arm64 (ubuntu-24.04-arm), macos-arm64 (macos-15), macos-x64 (macos-15-intel) and windows-x64 (windows-2022 + MinGW-w64 via choco). Each job: install the published `@lmctl-ai/lmctl` (provides `lmscript`), run `./test.sh`, stage the binary as `lmmagnum-OS-ARCH[.exe]` (the naming lmscript's `install="github:lmctl-ai/lmmagnum@latest"` expects) with license files, upload an artifact.

Pushing a `v*` tag runs the `release` job, which collects the five binaries, license files and `SHA256SUMS` and creates the GitHub release. To release: bump `project(lmmagnum VERSION x.y.z)` in CMakeLists.txt, commit, `git tag vx.y.z && git push --tags`. `--version` must equal the tag (test.sh checks it against CMakeLists.txt, not the tag).

Static linking: Linux `-static`; macOS cannot link libSystem statically so only libstdc++/libgcc concerns are absent (Apple clang uses system libc++); Windows `-static` with MinGW.

Status: all five platforms build and pass test.sh in CI (first green run on main, 2026-10-23).

License: MIT (Magnum, Corrade, cpp-dsl-sdk and lmcad are MIT, so the combined binary is MIT-compatible with no copyleft obligations beyond the glibc/GCC runtime notes in THIRD_PARTY_NOTICES.md).

## Automatic releases

Every push to `main` that CI accepts is released automatically. The `version` job in `.github/workflows/ci.yml` picks `MAJOR.MINOR.<run number>` (MAJOR.MINOR come from the version source in the repo, the patch is the workflow run number), every build job runs `scripts/stamp-version.sh` to write that version into the working tree before building (never committed), so the binary reports it. When all platform builds and tests pass, the `release` job creates the tag `vMAJOR.MINOR.N` on the pushed commit and publishes the assets, marked latest. Pull requests and manual runs never release. Pushing a `v*` tag by hand still releases exactly that version. To move to a new MAJOR.MINOR, change the version source (`project(... VERSION x.y.z)` in `CMakeLists.txt`, or the `VERSION` file for lmblender) in a commit.
