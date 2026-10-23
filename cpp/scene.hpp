#pragma once
#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <cstdint>
#include <string>
#include <vector>

namespace lmmagnum {
using namespace Magnum::Math::Literals;
using Magnum::Float; using Magnum::UnsignedInt; using Magnum::Matrix4; using Magnum::Vector3; using Magnum::Color3;

// Local-space triangle soup taken from a Magnum::Trade::MeshData primitive.
struct Mesh { std::vector<Vector3> positions, normals; std::vector<UnsignedInt> indices; };
struct Material { Color3 color{0.72f,0.74f,0.78f}; float roughness=.7f, specular=.2f, shininess=32.f; };
struct Shape {
  Mesh mesh; Material material;
  Vector3 translation{}, rotation{}, scaling{1.f,1.f,1.f};   // rotation: degrees about X, Y, Z (applied Z, Y, X)
  bool visible=true; std::string name;
  Matrix4 transformation() const;
};
struct Camera { float fov=45.f; Vector3 eye{3.f,3.f,5.f}, target{}; };
struct Light { Vector3 direction{-.55f,-1.f,-.4f}; float intensity=.9f; float ambient=.18f; };
struct Image { int width=0, height=0; std::vector<unsigned char> rgb; };

Mesh make_triangle(Vector3 a, Vector3 b, Vector3 c); Mesh make_cube(); Mesh make_sphere(unsigned rings); Mesh make_icosphere(unsigned subdivisions);
Mesh make_cylinder(); Mesh make_cone(); Mesh make_capsule(); Mesh make_plane();
Mesh scaled(Mesh, Vector3 factor); // bakes a non-uniform scale into positions and normals
std::size_t triangle_count(const Mesh&);

Image render(const std::vector<const Shape*>& shapes, const Camera&, const Light&, Color3 background, int width, int height, int supersample=2);
void write_png(const std::string& path, const Image&);
void write_ppm(const std::string& path, const Image&);
void write_stl(const std::string& path, const std::vector<const Shape*>&);
void write_obj(const std::string& path, const std::vector<const Shape*>&);
void write_ply(const std::string& path, const std::vector<const Shape*>&);
}
