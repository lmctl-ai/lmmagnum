#pragma once
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector2.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lmmagnum {
using namespace Magnum::Math::Literals;
using Magnum::Float; using Magnum::UnsignedInt; using Magnum::Matrix4; using Magnum::Vector2; using Magnum::Vector3; using Magnum::Color3;

// Local-space triangle soup taken from a Magnum::Trade::MeshData primitive.
// `uvs` is empty or parallel to `positions` (origin bottom-left, v up, as in Magnum).
struct Mesh { std::vector<Vector3> positions, normals; std::vector<Vector2> uvs; std::vector<UnsignedInt> indices; };
// 8-bit RGB image sampled by the rasterizer with a lazily built mip chain. Alpha in source files is dropped.
struct Texture {
  enum class Wrap { Repeat, Clamp, Mirror };
  int width=0, height=0; std::vector<unsigned char> rgb;
  bool linear=true; Wrap wrap=Wrap::Repeat; float scale_u=1.f, scale_v=1.f;
  struct Level { int w=0, h=0; std::vector<float> rgb; };
  mutable std::vector<Level> levels; mutable bool dirty=true;   // rebuilt by sample() after pixels change
  Color3 sample(float u, float v, float lod) const;
  static Texture checker(int size, int cell, Color3 a, Color3 b);
};
struct Material { Color3 color{0.72f,0.74f,0.78f}; float roughness=.7f, specular=.2f, shininess=32.f; const Texture* texture=nullptr; };   // texture multiplies color
// Scene-graph node: a transform shared by every shape (and child group) that names it as parent.
struct Group {
  Vector3 translation{}, rotation{}, scaling{1.f,1.f,1.f};
  const Group* parent=nullptr;
  Matrix4 transformation() const;   // includes the whole parent chain
};
struct Shape {
  Mesh mesh; Material material;
  Vector3 translation{}, rotation{}, scaling{1.f,1.f,1.f};   // rotation: degrees about X, Y, Z (applied Z, Y, X)
  const Group* parent=nullptr;
  bool visible=true; std::string name;
  Matrix4 transformation() const;
};
struct Camera { float fov=45.f; Vector3 eye{3.f,3.f,5.f}, target{}; };
struct Light { Vector3 direction{-.55f,-1.f,-.4f}; float intensity=.9f; float ambient=.18f; bool shadows=false; };
struct Image { int width=0, height=0; std::vector<unsigned char> rgb; };

Mesh make_triangle(Vector3 a, Vector3 b, Vector3 c); Mesh make_cube(); Mesh make_sphere(unsigned rings); Mesh make_icosphere(unsigned subdivisions);
Mesh make_cylinder(); Mesh make_cone(); Mesh make_capsule(); Mesh make_plane();
// Voxel text from a built-in 5x7 font (A-Z, 0-9 and . , ! ? - : ' / + = ( ) _; lowercase is upper-cased).
// Origin at the lower-left of the first glyph, lying in the XY plane, `height` units tall, extruded by one pixel.
Mesh make_text(const std::string& text, float height);
// Cylinder of `radius` from p0 to p1 (capped), baked in world space. Throws when p0 == p1.
Mesh make_tube(Vector3 p0, Vector3 p1, float radius);
// Flat double-sided annulus in the XZ plane (y = 0) between two radii.
Mesh make_ring(float inner, float outer, unsigned segments);
Mesh scaled(Mesh, Vector3 factor); // bakes a non-uniform scale into positions and normals
std::size_t triangle_count(const Mesh&);

// Decodes PNG/JPEG/BMP/TGA/GIF(first frame)/PSD/HDR/PNM bytes with stb_image. Throws on failure or when larger than 8192 per side.
Texture load_texture(const std::string& path);
void save_texture_png(const std::string& path, const Texture&);
Image render(const std::vector<const Shape*>& shapes, const Camera&, const Light&, Color3 background, int width, int height, int supersample=2);
void write_png(const std::string& path, const Image&);
void write_ppm(const std::string& path, const Image&);
void write_stl(const std::string& path, const std::vector<const Shape*>&);
void write_obj(const std::string& path, const std::vector<const Shape*>&);
void write_ply(const std::string& path, const std::vector<const Shape*>&);
}
