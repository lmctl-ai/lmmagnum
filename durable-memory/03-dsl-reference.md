# DSL reference (schema v0.2)

Generated from `schema/lmmagnum.dsl.json`; context variable `scene: Scene`.

| Call | Returns |
|---|---|
| `Scene.cube(float, Material)` | `Shape` |
| `Scene.box(float, float, float, Material)` | `Shape` |
| `Scene.sphere(float, Material)` | `Shape` |
| `Scene.icosphere(float, int, Material)` | `Shape` |
| `Scene.cylinder(float, float, Material)` | `Shape` |
| `Scene.cone(float, float, Material)` | `Shape` |
| `Scene.capsule(float, float, Material)` | `Shape` |
| `Scene.plane(float, Material)` | `Shape` |
| `Scene.text(string, float, Material)` | `Shape` |
| `Scene.group()` | `Group` |
| `Scene.tube_between(float, float, float, float, float, float, float, Material)` | `Shape` |
| `Scene.ring(float, float, Material)` | `Shape` |
| `Scene.camera(float, float, float, float, float, float, float)` | `Camera` |
| `Scene.directional_light(float, float, float, float)` | `Light` |
| `Scene.background(float, float, float)` | `Scene` |
| `Scene.render_png(string, int, int)` | `void` |
| `Scene.render_ppm(string, int, int)` | `void` |
| `Scene.export_stl(string)` | `void` |
| `Scene.export_obj(string)` | `void` |
| `Scene.export_ply(string)` | `void` |
| `Scene.triangle_count()` | `int` |
| `Scene.shape_count()` | `int` |
| `Scene.triangle(float, float, float, float, float, float, float, float, float, Material)` | `Shape` |
| `Scene.ambient(float)` | `Scene` |
| `Scene.shadows(bool)` | `Scene` |
| `Scene.accumulate(int, int)` | `void` |
| `Scene.save_accumulated_png(string)` | `void` |
| `Scene.reset_accumulation()` | `void` |
| `Shape.translate(float, float, float)` | `Shape` |
| `Shape.rotate(float, float, float)` | `Shape` |
| `Shape.scale(float, float, float)` | `Shape` |
| `Shape.parent(Group)` | `Shape` |
| `Shape.visible(bool)` | `Shape` |
| `Shape.name(string)` | `Shape` |
| `Shape.color(float, float, float)` | `Shape` |
| `Shape.triangle_count()` | `int` |
| `Group.translate(float, float, float)` | `Group` |
| `Group.rotate(float, float, float)` | `Group` |
| `Group.scale(float, float, float)` | `Group` |
| `Group.parent(Group)` | `Group` |
| `Material.color(float, float, float)` | `Material` |
| `Material.roughness(float)` | `Material` |
| `Material.specular(float)` | `Material` |
| `Material.shininess(float)` | `Material` |
| `Camera.look_at(float, float, float, float, float, float)` | `Camera` |
| `Light.intensity(float)` | `Light` |
| `material()` | `Material` |
| `magnum_version()` | `string` |

## Conventions

- Lengths are scene units; angles are degrees; colours are 0..1 floats.
- Cube/box/plane sizes are full edge lengths; sphere/cone/cylinder take a radius (and a full height); capsule takes `radius, length` but `length` is currently ignored (unit capsule scaled by radius).
- `Shape.rotate` sets absolute Euler angles (applied Z, then Y, then X); `translate`/`scale` also set, not accumulate. The Shape handle returned by every constructor can be re-posed between frames.
- `render_png`/`render_ppm` render all shapes except those set `visible(false)` with 2x2 supersampling; size limit 8192 per side, 16 Mpixel.
- `accumulate(w,h)` renders and adds a frame into a float accumulator; `save_accumulated_png` writes the mean (motion blur). Call `reset_accumulation()` to start over or change size.
- Scene graph: a `Group` is a shared transform (translate/rotate/scale, same Euler order as shapes). `Shape.parent(g)` / `Group.parent(g)` attach to it; the world matrix is the product of the whole chain, used by rendering and every exporter. A parent that would form a cycle is an error.
- `Scene.text(s, h, m)` builds voxel text from a built-in 5x7 font (A-Z, 0-9 and `. , ! ? - : ' / + = ( ) _`; lowercase is upper-cased, unknown characters become `?`), `h` units tall, one pixel deep, lower-left of the first glyph at the shape origin, 1 to 256 characters.
- `tube_between(x1,y1,z1,x2,y2,z2,r,m)` is a capped cylinder between two points, baked in world space (its own `translate`/`rotate` still apply on top); `ring(inner,outer,m)` is a flat double-sided annulus in the XZ plane.
- `shadows(true)` adds a shadow map for the directional light: an orthographic depth pass over every visible shape (2048 x 2048, slope-scaled bias, 3 x 3 PCF); it darkens the key light's diffuse and specular, not ambient or the fill light.
- Limits: 4096 shapes and 4096 materials per run. Host errors surface as the standard one-line JSON, exit 3.
- Inside `_dsl_` there is no `assert`, ternary, arrays, or `println`; use `if/else`, `for (i in range(a,b))`, `str()`, string `+`, `float()`, `sin/cos` etc. Outside the block (plain lmscript) `assert`, `system`, `println` exist.
