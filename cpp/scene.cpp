#include "scene.hpp"
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Matrix3.h>
#include <cmath>
#include <stdexcept>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Trade/MeshData.h>

namespace lmmagnum {
namespace {
Matrix4 trs(Vector3 t,Vector3 r,Vector3 s) {
  return Matrix4::translation(t)*Matrix4::rotationZ(Magnum::Deg(r.z()))*Matrix4::rotationY(Magnum::Deg(r.y()))*
         Matrix4::rotationX(Magnum::Deg(r.x()))*Matrix4::scaling(s);
}
}
Matrix4 Group::transformation() const { Matrix4 m=trs(translation,rotation,scaling); return parent?parent->transformation()*m:m; }
Matrix4 Shape::transformation() const { Matrix4 m=trs(translation,rotation,scaling); return parent?parent->transformation()*m:m; }
namespace {
Mesh convert(const Magnum::Trade::MeshData& data) {
  Mesh m;
  auto p=data.positions3DAsArray(), n=data.normalsAsArray();
  m.positions.assign(p.begin(),p.end()); m.normals.assign(n.begin(),n.end());
  if(data.hasAttribute(Magnum::Trade::MeshAttribute::TextureCoordinates)) { auto t=data.textureCoordinates2DAsArray(); m.uvs.assign(t.begin(),t.end()); }
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
  m.positions={a,b,c}; m.normals={n,n,n}; m.indices={0,1,2}; m.uvs={{0.f,0.f},{1.f,0.f},{.5f,1.f}}; return m;
}

namespace {
// 5x7 glyphs, rows top to bottom, '#' = lit.
struct Glyph { char c; const char* rows[7]; };
const Glyph kFont[] = {
 {'A',{".###.","#...#","#...#","#####","#...#","#...#","#...#"}},{'B',{"####.","#...#","#...#","####.","#...#","#...#","####."}},
 {'C',{".####","#....","#....","#....","#....","#....",".####"}},{'D',{"####.","#...#","#...#","#...#","#...#","#...#","####."}},
 {'E',{"#####","#....","#....","####.","#....","#....","#####"}},{'F',{"#####","#....","#....","####.","#....","#....","#...."}},
 {'G',{".####","#....","#....","#.###","#...#","#...#",".###."}},{'H',{"#...#","#...#","#...#","#####","#...#","#...#","#...#"}},
 {'I',{"#####","..#..","..#..","..#..","..#..","..#..","#####"}},{'J',{"..###","...#.","...#.","...#.","...#.","#..#.",".##.."}},
 {'K',{"#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"}},{'L',{"#....","#....","#....","#....","#....","#....","#####"}},
 {'M',{"#...#","##.##","#.#.#","#.#.#","#...#","#...#","#...#"}},{'N',{"#...#","##..#","#.#.#","#..##","#...#","#...#","#...#"}},
 {'O',{".###.","#...#","#...#","#...#","#...#","#...#",".###."}},{'P',{"####.","#...#","#...#","####.","#....","#....","#...."}},
 {'Q',{".###.","#...#","#...#","#...#","#.#.#","#..#.",".##.#"}},{'R',{"####.","#...#","#...#","####.","#.#..","#..#.","#...#"}},
 {'S',{".####","#....","#....",".###.","....#","....#","####."}},{'T',{"#####","..#..","..#..","..#..","..#..","..#..","..#.."}},
 {'U',{"#...#","#...#","#...#","#...#","#...#","#...#",".###."}},{'V',{"#...#","#...#","#...#","#...#","#...#",".#.#.","..#.."}},
 {'W',{"#...#","#...#","#...#","#.#.#","#.#.#","##.##","#...#"}},{'X',{"#...#","#...#",".#.#.","..#..",".#.#.","#...#","#...#"}},
 {'Y',{"#...#","#...#",".#.#.","..#..","..#..","..#..","..#.."}},{'Z',{"#####","....#","...#.","..#..",".#...","#....","#####"}},
 {'0',{".###.","#...#","#..##","#.#.#","##..#","#...#",".###."}},{'1',{"..#..",".##..","..#..","..#..","..#..","..#..",".###."}},
 {'2',{".###.","#...#","....#","...#.","..#..",".#...","#####"}},{'3',{"####.","....#","....#",".###.","....#","....#","####."}},
 {'4',{"...#.","..##.",".#.#.","#..#.","#####","...#.","...#."}},{'5',{"#####","#....","####.","....#","....#","#...#",".###."}},
 {'6',{"..##.",".#...","#....","####.","#...#","#...#",".###."}},{'7',{"#####","....#","...#.","..#..",".#...",".#...",".#..."}},
 {'8',{".###.","#...#","#...#",".###.","#...#","#...#",".###."}},{'9',{".###.","#...#","#...#",".####","....#","...#.",".##.."}},
 {'.',{".....",".....",".....",".....",".....",".##..",".##.."}},{',',{".....",".....",".....",".....",".##..","..#..",".#..."}},
 {'!',{"..#..","..#..","..#..","..#..","..#..",".....","..#.."}},{'?',{".###.","#...#","....#","...#.","..#..",".....","..#.."}},
 {'-',{".....",".....",".....","#####",".....",".....","....."}},{':',{".....",".##..",".##..",".....",".##..",".##..","....."}},
 {'\'',{"..#..","..#..",".#...",".....",".....",".....","....."}},{'/',{"....#","....#","...#.","..#..",".#...","#....","#...."}},
 {'+',{".....","..#..","..#..","#####","..#..","..#..","....."}},{'=',{".....",".....","#####",".....","#####",".....","....."}},
 {'(',{"...#.","..#..",".#...",".#...",".#...","..#..","...#."}},{')',{".#...","..#..","...#.","...#.","...#.","..#..",".#..."}},
 {'_',{".....",".....",".....",".....",".....",".....","#####"}},
};
const Glyph* find_glyph(char c) {
  if(c>='a'&&c<='z') c=char(c-'a'+'A');
  for(const auto& g:kFont) if(g.c==c) return &g;
  return nullptr;
}
void add_box(Mesh& m,Vector3 lo,Vector3 hi) {
  static const Vector3 N[6]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
  for(int f=0;f<6;f++) {
    int a=f/2; int u=(a+1)%3, v=(a+2)%3; float s=(f%2==0)?1.f:-1.f;
    Vector3 q[4];
    for(int i=0;i<4;i++){ q[i]=Vector3(); q[i][a]=s>0?hi[a]:lo[a]; q[i][u]=(i==1||i==2)?hi[u]:lo[u]; q[i][v]=(i>=2)?hi[v]:lo[v]; }
    UnsignedInt base=UnsignedInt(m.positions.size());
    for(auto& p:q){ m.positions.push_back(p); m.normals.push_back(N[f]); }
    // Wind counter-clockwise seen from outside.
    bool ccw=Magnum::Math::dot(Magnum::Math::cross(q[1]-q[0],q[2]-q[0]),N[f])>0;
    if(ccw) m.indices.insert(m.indices.end(),{base,base+1,base+2,base,base+2,base+3});
    else m.indices.insert(m.indices.end(),{base,base+2,base+1,base,base+3,base+2});
  }
}
}
Mesh make_text(const std::string& text,float height) {
  Mesh m; const float px=height/7.f;
  float x=0;
  const float total=float(text.size())*6*px;   // planar UVs: u along the string, v up the glyph row
  for(char ch:text) {
    if(ch==' ') { x+=6*px; continue; }
    const Glyph* g=find_glyph(ch); if(!g) g=find_glyph('?');
    for(int row=0;row<7;row++) for(int col=0;col<5;col++) if(g->rows[row][col]=='#')
      add_box(m,{x+col*px,(6-row)*px,0.f},{x+(col+1)*px,(7-row)*px,px});
    x+=6*px;
  }
  m.uvs.reserve(m.positions.size());
  for(const auto& p:m.positions) m.uvs.push_back({total>0.f?p.x()/total:0.f,p.y()/height});
  return m;
}
Mesh make_cube() { return convert(Magnum::Primitives::cubeSolid(Magnum::Primitives::CubeFlag::TextureCoordinatesAllSame)); }
Mesh make_sphere(unsigned rings) { return convert(Magnum::Primitives::uvSphereSolid(rings,rings*2,Magnum::Primitives::UVSphereFlag::TextureCoordinates)); }
Mesh make_icosphere(unsigned subdivisions) { return convert(Magnum::Primitives::icosphereSolid(subdivisions)); }
Mesh make_cylinder() { return convert(Magnum::Primitives::cylinderSolid(1,48,1.f,Magnum::Primitives::CylinderFlag::CapEnds|Magnum::Primitives::CylinderFlag::TextureCoordinates)); }
Mesh make_cone() { return convert(Magnum::Primitives::coneSolid(1,48,1.f,Magnum::Primitives::ConeFlag::CapEnd|Magnum::Primitives::ConeFlag::TextureCoordinates)); }
Mesh make_capsule() { return convert(Magnum::Primitives::capsule3DSolid(8,1,48,1.f,Magnum::Primitives::CapsuleFlag::TextureCoordinates)); }
Mesh make_plane() { return convert(Magnum::Primitives::planeSolid(Magnum::Primitives::PlaneFlag::TextureCoordinates)); }
Mesh scaled(Mesh m, Vector3 f) {
  for(auto& v:m.positions) v*=f;
  for(auto& n:m.normals) n=(n/f).normalized();
  return m;
}
Mesh make_tube(Vector3 p0,Vector3 p1,float radius) {
  Vector3 d=p1-p0; float len=d.length();
  if(len==0.f) throw std::runtime_error("tube endpoints must differ");
  Mesh m=scaled(make_cylinder(),{radius,len/2,radius});   // unit cylinder spans y in [-1, 1]
  Vector3 dir=d/len;
  Matrix4 rot=dir.y()<-0.9999f?Matrix4::rotationX(Magnum::Deg(180.f)):Matrix4::from(Magnum::Quaternion::rotation(Vector3::yAxis(),dir).toMatrix(),Vector3{});
  Matrix4 xf=Matrix4::translation((p0+p1)*0.5f)*rot; Magnum::Matrix3 nm=xf.normalMatrix();
  for(auto& v:m.positions) v=xf.transformPoint(v);
  for(auto& n:m.normals) n=(nm*n).normalized();
  return m;
}
Mesh make_ring(float inner,float outer,unsigned segments) {
  if(segments<3) throw std::runtime_error("ring needs at least 3 segments");
  Mesh m; const float two_pi=6.28318530718f;
  for(int side=0;side<2;side++) {
    UnsignedInt base=UnsignedInt(m.positions.size()); Vector3 n{0.f,side?-1.f:1.f,0.f};
    for(unsigned k=0;k<=segments;k++) { float a=two_pi*float(k)/float(segments),c=std::cos(a),s=std::sin(a);
      m.positions.push_back({inner*c,0.f,inner*s}); m.positions.push_back({outer*c,0.f,outer*s}); m.normals.push_back(n); m.normals.push_back(n);
      // u runs around the ring, v from the inner to the outer edge
      m.uvs.push_back({float(k)/float(segments),0.f}); m.uvs.push_back({float(k)/float(segments),1.f}); }
    for(unsigned k=0;k<segments;k++) { UnsignedInt i=base+2*k;
      if(side==0) m.indices.insert(m.indices.end(),{i,i+1,i+2,i+2,i+1,i+3}); else m.indices.insert(m.indices.end(),{i,i+2,i+1,i+2,i+3,i+1}); }
  }
  return m;
}
std::size_t triangle_count(const Mesh& m) { return m.indices.size()/3; }
}
