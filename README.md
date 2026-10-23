# lmmagnum

**One static executable that drives [Magnum](https://magnum.graphics) from [lmscript](https://lmctl.com/tools/lmscript/skill.html) `_dsl_` blocks.** Build a scene from Magnum's primitives, render it with a built-in software rasterizer, and export PNG, PPM, STL, OBJ or PLY. No GPU, no window system, no shared libraries.

```sh
npm install -g @lmctl-ai/lmctl
lmscript examples/primitives.lms      # writes images/primitives.png
```

Native binaries (`lmmagnum-linux-x64`, `-linux-arm64`, `-macos-arm64`, `-macos-x64`, `-windows-x64.exe`) are on the [releases page](https://github.com/lmctl-ai/lmmagnum/releases).

## Build

```sh
scripts/sync-deps.sh                      # Magnum + Corrade at the commits in deps.lock
cmake -S . -B build -G Ninja && cmake --build build
./test.sh                                 # build, schema/signature parity, all examples, static check
```

## Docs and examples

- [`durable-memory/`](durable-memory/00-index.md): developer documentation (build, architecture, DSL reference, extending).
- [`examples/`](examples/): headless analogues of the [Magnum example index](https://magnum.graphics/doc/magnum/example-index.html), one `.lms` each.

## License

MIT, see [LICENSE](LICENSE). Magnum, Corrade and cpp-dsl-sdk are MIT as well; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
