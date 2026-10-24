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
  for(char ch:text) {
    if(ch==' ') { x+=6*px; continue; }
    const Glyph* g=find_glyph(ch); if(!g) g=find_glyph('?');
    for(int row=0;row<7;row++) for(int col=0;col<5;col++) if(g->rows[row][col]=='#')
      add_box(m,{x+col*px,(6-row)*px,0.f},{x+(col+1)*px,(7-row)*px,px});
    x+=6*px;
  }
  return m;
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
