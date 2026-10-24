# Extending and gotchas

## Add a DSL call
1. Add the member to `schema/lmmagnum.dsl.json` (types -> methods, or functions).
2. Bind it in `register_host()` in `cpp/main.cpp` with the same parameter and return type strings.
3. Rebuild (CMake re-embeds the schema) and run the signature diff from 02-architecture.md.
4. Update 03-dsl-reference.md and add or adjust an example.

## Add a primitive
Call the `Magnum::Primitives::*Solid` function in `scene.cpp`'s `make_*`, then bind a Scene method that bakes size via `scaled()`. This revision of Magnum has no torus.

## Texture coordinates
`Mesh::uvs` is empty or parallel to `positions`. `convert()` takes Magnum's own (`UVSphereFlag`, `CylinderFlag`, `ConeFlag`, `CapsuleFlag`, `PlaneFlag`, `CubeFlag::TextureCoordinatesAllSame`); text, ring and triangle generate theirs. The icosphere has none (shared vertices cannot carry a seam). A new primitive that should be texturable must fill `uvs`, or rendering a textured shape of that kind errors.

## Gotchas
- Static linking needs both `CORRADE_BUILD_STATIC` and `MAGNUM_BUILD_STATIC`; the bare `BUILD_STATIC` is not enough.
- `Trade` requires Corrade `PluginManager` even though no plugin is loaded.
- Some primitives are not indexed and/or are triangle strips (`planeSolid`); `convert()` in `scene.cpp` normalises both to indexed triangle lists. Missing this renders only half a plane.
- Magnum headers must be included individually (`Matrix3.h`, `Vector4.h`); forward declarations give "incomplete type" errors.
- DSL `Value::real` is double; Magnum math is float, so cast at the boundary.
- Magnum primitives are unit-sized (cube edge 2, radius 1, half-length 1); sizes passed from the DSL are converted accordingly.
- Open items: capsule length, per-vertex colours. CI: `.github/workflows/ci.yml` builds five platforms; a `v*` tag publishes the release (see 06-release.md).
