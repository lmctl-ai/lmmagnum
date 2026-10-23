# Static build of lmmagnum

- Magnum (369e9620ab22) and Corrade are shallow clones in `third_party/` (git-ignored; re-clone `mosra/magnum`, `mosra/corrade`).
- CMake adds both via `add_subdirectory` with `CORRADE_BUILD_STATIC`/`MAGNUM_BUILD_STATIC` ON. Without those two prefixed options the libs build as `.so` and `-static` fails.
- Only Trade, Primitives, MeshTools (+ Corrade PluginManager, which Trade requires) are enabled. No GL: `cpp/render.cpp` is a software rasterizer on Magnum Math types; PNG via vendored `deflate.hpp`.
- Build: `cmake -S . -B build -G Ninja && cmake --build build` -> `build/lmmagnum` (`ldd`: not a dynamic executable, ~3.5 MB).
- Host wiring follows ../lmcad: `schema/lmmagnum.dsl.json` embedded at configure time, `cpp/dslvm.hpp` pinned by `sdk.lock` (`scripts/sync-sdk.sh --verify`).
- Check: `diff <(lmscript --dsl-signatures schema/lmmagnum.dsl.json) <(build/lmmagnum --dsl-signatures)`; run `PATH=build:$PATH lmscript examples/still-life.lms`.
- Known gaps: see 05-extending.md.
