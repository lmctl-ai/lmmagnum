# lmmagnum developer notes

lmmagnum is a single static executable that exposes parts of Magnum (Math, Primitives, MeshTools, Trade) to lmscript `_dsl_` blocks, with a software renderer so it needs no GPU, window system or shared library.

1. [01-build-static.md](01-build-static.md): clone, configure, build, verify static.
2. [02-architecture.md](02-architecture.md): source layout, how a DSL call reaches Magnum, how rendering works.
3. [03-dsl-reference.md](03-dsl-reference.md): every DSL call and the language subset.
4. [04-examples.md](04-examples.md): the examples and how each maps to a Magnum sample.
5. [05-extending.md](05-extending.md): adding a call/primitive; gotchas found so far.
6. [06-release.md](06-release.md): CI matrix and publishing releases.
7. [20-sibling-3d-hosts-and-conventions.md](20-sibling-3d-hosts-and-conventions.md): the four 3D repos side by side, DSL cheat sheet, porting rules, README conventions, **mandatory working conventions** (commit and push, identity, automatic releases).

## 60-second takeover

```sh
scripts/sync-deps.sh && cmake -S . -B build -G Ninja && cmake --build build
./test.sh        # SDK pin, build, signatures, every example, static link
scripts/update-readme-images.sh   # regenerate docs/images and docs/videos
```

Change flow: schema + `register_host()` (+ `scene.cpp`/`render.cpp`), rebuild, `./test.sh`, docs, `git commit && git push origin main`.
