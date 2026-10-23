# lmmagnum developer notes

lmmagnum is a single static executable that exposes parts of Magnum (Math, Primitives, MeshTools, Trade) to lmscript `_dsl_` blocks, with a software renderer so it needs no GPU, window system or shared library.

1. [01-build-static.md](01-build-static.md): clone, configure, build, verify static.
2. [02-architecture.md](02-architecture.md): source layout, how a DSL call reaches Magnum, how rendering works.
3. [03-dsl-reference.md](03-dsl-reference.md): every DSL call and the language subset.
4. [04-examples.md](04-examples.md): the examples and how each maps to a Magnum sample.
5. [05-extending.md](05-extending.md): adding a call/primitive; gotchas found so far.
6. [06-release.md](06-release.md): CI matrix and publishing releases.
