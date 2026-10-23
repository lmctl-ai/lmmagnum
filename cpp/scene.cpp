#include "scene.hpp"
#include <Magnum/Math/Functions.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Trade/MeshData.h>

namespace lmmagnum {
Matrix4 Shape::transformation() const {
  using namespace Magnum::Math::Literals;
  return Matrix4::translation(translation)*Matrix4::rotationZ(Magnum::Deg(rotation.z()))*Matrix4::rotationY(Magnum::Deg(rotation.y()))*
         Matrix4::rotationX(Magnum::Deg(rotation.x()))*Matrix4::scaling(scaling);
}
namespace {
Mesh convert(const Magnum::Trade::MeshData& data) {
  Mesh m;
  auto p=data.positions3DAsArray(), n=data.normalsAsArray();
  m.positions.assign(p.begin(),p.end()); m.normals.assign(n.begin(),n.end());
  std::vector<UnsignedInt> seq;
  if(data.isIndexed()) { auto i=data.indicesAsArray(); seq.assign(i.begin(),i.end()); }
  else for(UnsignedInt i=0;i<UnsignedInt(p.size());i++) seq.push_back(i);
  // Primitives such as planeSolid() are triangle strips; the host stores plain triangle lists.
  using Magnum::MeshPrimitive;
  if(data.primitive()==MeshPrimitive::TriangleStrip) {
    for(size_t k=2;k<seq.size();k++) { if(k%2==0) m.indices.insert(m.indices.end(),{seq[k-2],seq[k-1],seq[k]}); else m.indices.insert(m.indices.end(),{seq[k-1],seq[k-2],seq[k]}); }
  } else if(data.primitive()==MeshPrimitive::TriangleFan) {
    for(size_t k=2;k<seq.size();k++) m.indices.insert(m.indices.end(),{seq[0],seq[k-1],seq[k]});
  } else m.indices=seq;
  return m;
}
}
Mesh make_triangle(Vector3 a,Vector3 b,Vector3 c) {
  Mesh m; Vector3 n=Magnum::Math::cross(b-a,c-a); n=n.dot()>0?n.normalized():Vector3::zAxis();
  m.positions={a,b,c}; m.normals={n,n,n}; m.indices={0,1,2}; return m;
}
Mesh make_cube() { return convert(Magnum::Primitives::cubeSolid()); }
Mesh make_sphere(unsigned rings) { return convert(Magnum::Primitives::uvSphereSolid(rings,rings*2)); }
Mesh make_icosphere(unsigned subdivisions) { return convert(Magnum::Primitives::icosphereSolid(subdivisions)); }
Mesh make_cylinder() { return convert(Magnum::Primitives::cylinderSolid(1,48,1.f,Magnum::Primitives::CylinderFlag::CapEnds)); }
Mesh make_cone() { return convert(Magnum::Primitives::coneSolid(1,48,1.f,Magnum::Primitives::ConeFlag::CapEnd)); }
Mesh make_capsule() { return convert(Magnum::Primitives::capsule3DSolid(8,1,48,1.f)); }
Mesh make_plane() { return convert(Magnum::Primitives::planeSolid()); }
Mesh scaled(Mesh m, Vector3 f) {
  for(auto& v:m.positions) v*=f;
  for(auto& n:m.normals) n=(n/f).normalized();
  return m;
}
std::size_t triangle_count(const Mesh& m) { return m.indices.size()/3; }
}
