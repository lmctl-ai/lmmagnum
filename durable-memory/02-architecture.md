# Architecture

## Layout

| Path | Role |
|---|---|
| `schema/lmmagnum.dsl.json` | the DSL surface (types, methods, functions, context `scene`); embedded into the binary at configure time (`build/generated/schema_embedded.hpp`) |
| `cpp/dslvm.hpp` | vendored lmcode-cpp VM, pinned by `sdk.lock`; never edit, refresh with `scripts/sync-sdk.sh` |
| `cpp/main.cpp` | `Host` state (shapes, materials, camera, light, accumulator), `register_host()` binding every schema member, CLI (`--version`, `--dsl-schema [--json]`, `--dsl-signatures`, `--dsl FILE...`) |
| `cpp/scene.{hpp,cpp}` | `Mesh/Material/Shape/Group/Camera/Light/Image` structs (`Group` = scene-graph transform, `Shape.parent`); conversion of `Trade::MeshData` primitives into plain triangle lists; `make_tube`, `make_ring`, `make_text` (5x7 voxel font) |
| `cpp/render.cpp` | software rasterizer, PNG/PPM writers, STL/OBJ/PLY exporters |
| `cpp/texture.cpp`, `cpp/stb_image.h` | texture decode (vendored stb_image, PNG/JPEG/BMP/TGA only), mip chain, trilinear/nearest sampling, wrap modes, PNG save |
| `cpp/deflate.hpp`, `cpp/atomic-file.*` | PNG zlib stream and crash-safe file writes (shared with lmcad) |
| `third_party/{magnum,corrade}` | pinned shallow clones, git-ignored |

## Call path

`lmscript` compiles the `_dsl_` block against the schema into bytecode (`.dslb`), then runs `lmmagnum --dsl file.dslb`. The VM resolves each import against the registry (signature parity is checked: `diff <(lmscript --dsl-signatures schema/lmmagnum.dsl.json) <(lmmagnum --dsl-signatures)`), so schema and `register_host` must change together. `checked_method` validates argument kinds and converts C++ exceptions into `dslvm::Error`.

## Where Magnum is used

- `Magnum::Primitives::{cubeSolid, uvSphereSolid, icosphereSolid, cylinderSolid, coneSolid, capsule3DSolid, planeSolid}` generate geometry in `scene.cpp`; `scaled()` bakes size into positions/normals.
- `Magnum::Math` (`Vector3`, `Matrix4::lookAt/perspectiveProjection/translation/rotation*/scaling`, `Color3`, `cross`, `normalMatrix`) is used everywhere, including in the rasterizer.
- Not used: GL, SceneGraph, Shaders, Text, Audio, Vk, plugins. Image output is not Magnum's converters (those are plugins from magnum-plugins).

## Rendering

Per triangle: model transform, view-projection, near-plane clip (z >= -w), perspective-correct interpolation of world position and normal, z-buffer, then Lambert (key light + fixed fill light + `ambient`) plus Blinn-Phong specular, two-sided. Supersample factor 2 by default, box-filtered. Optional shadow map (`Scene.shadows(true)`): orthographic depth pass along the directional light over all visible shapes, 2048x2048, slope-scaled bias, 3x3 PCF; it scales the key light's diffuse and specular only. No textures, reflections or per-vertex colours.
