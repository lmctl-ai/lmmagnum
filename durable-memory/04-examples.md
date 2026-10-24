# Examples

Each file in `examples/` is self-executing lmscript: `lmscript examples/NAME.lms` compiles its `_dsl_` block, runs it with `lmmagnum` from PATH and writes `images/` (pictures) or `build/` (intermediates). `examples/render-all.sh` runs all of them (run it from an empty scratch directory with `build/` on PATH; it does this itself).

The Magnum index (https://magnum.graphics/doc/magnum/example-index.html) is mostly GPU, audio, physics and UI code. The headless analogues below keep the subject and drop the part that needs a device. Examples that cannot be reproduced meaningfully are listed at the end.

| File | Magnum example | What is reproduced | What is dropped |
|---|---|---|---|
| `triangle.lms` | Your First Triangle | one flat triangle | per-vertex RGB colours |
| `textured-quad.lms` | Textured Quad | quad of two triangles; checker tiles | image import, UVs, shaders |
| `primitives.lms` | Primitives | all solid primitives, OBJ export | mouse rotation |
| `model-viewer.lms` | Model Viewer | posed multi-shape model, OBJ export | file importers, scene graph UI |
| `area-lights.lms` | Area Lights | spheres and floor under soft light | real area lights (only directional + ambient) |
| `raytracing.lms` | Ray Tracing | sphere field | reflections, shadows (rasterizer) |
| `octree.lms` | Octree | 4x4x4 cell lattice | collision queries |
| `arcball.lms` | ArcBall Camera | scripted orbit, 12 PNG frames | interaction |
| `animated-gif.lms` | Animated GIF | 24-frame bounce; GIF via ffmpeg if present | GIF decoding |
| `motionblur.lms` | Motion blur | 16-frame accumulation | GPU accumulation buffer |
| `primitives-scene-graph.lms` | Primitives, using a scene graph (Python examples) | the same solids parented to `Group`s, one nested group, one root rotation | mouse input |
| `text.lms` | Text (Python examples) | voxel text from a 5x7 font, two lines | distance-field glyphs, font import |
| `pbr-spheres.lms` | lmfilament `hellopbr` | roughness/specular sweep over five spheres | metallic, IBL |
| `depthtesting.lms` | lmfilament `depthtesting` | two overlapping triangles, nearer wins | per-vertex colors |
| `material-sandbox.lms` | lmfilament `material_sandbox` | 3x3 grid of color x gloss | refraction, cloth, subsurface |
| `lightbulb.lms` | lmfilament `lightbulb` | glossy sphere, floor, glowing bulb under one key light | point/spot lights, shadows |

Not reproduced: Audio, Bullet/DART/Box2D physics, 2D/3D fluid simulation, ImGui, Mouse Interaction, Object Picking, Text rendering, Cube Map, Shadow map, Multi Draw, Vulkan and WebXR examples, plain-GLFW and sokol triangles. They require a GL/Vulkan device, windowing, input, audio, fonts, or third-party libraries that the one-static-binary goal excludes.
