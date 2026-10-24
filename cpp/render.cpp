#include "scene.hpp"
#include "atomic-file.hpp"
#include "deflate.hpp"
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector4.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace lmmagnum {
namespace {
using Magnum::Vector4; using Magnum::Matrix3; using Magnum::Vector2;
struct Vertex { Vector4 clip; Vector3 world, normal; Vector2 uv; };
unsigned char channel(float v) { return static_cast<unsigned char>(std::lround(std::clamp(v,0.f,1.f)*255.f)); }
void put32(std::vector<unsigned char>& o,uint32_t v){o.push_back(v>>24);o.push_back(v>>16);o.push_back(v>>8);o.push_back(v);}
uint32_t crc32(const unsigned char* p,size_t n){uint32_t c=0xffffffffu;for(size_t i=0;i<n;i++){c^=p[i];for(int b=0;b<8;b++)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c^0xffffffffu;}
void chunk(std::vector<unsigned char>& out,const char* type,const std::vector<unsigned char>& data){put32(out,uint32_t(data.size()));size_t b=out.size();for(int i=0;i<4;i++)out.push_back(type[i]);out.insert(out.end(),data.begin(),data.end());put32(out,crc32(out.data()+b,out.size()-b));}
void save(const std::string& path,const void* data,size_t n){
  if(path.empty()||path.find('\0')!=std::string::npos) throw std::runtime_error("output path must be nonempty and contain no NUL bytes");
  AtomicFile f(path); f.write(std::vector<unsigned char>(static_cast<const unsigned char*>(data),static_cast<const unsigned char*>(data)+n)); f.commit();
}
void save(const std::string& path,const std::string& text){ save(path,text.data(),text.size()); }
void validate(const Image& im){
  if(im.width<=0||im.height<=0||im.width>8192||im.height>8192||uint64_t(im.width)*im.height>16777216||im.rgb.size()!=size_t(im.width)*im.height*3) throw std::runtime_error("invalid image buffer");
}
// Clip a polygon against the near plane w > eps, in clip space.
std::vector<Vertex> clip_near(const std::vector<Vertex>& in) {
  const float eps=1e-4f; std::vector<Vertex> out; if(in.empty()) return out;
  auto d=[&](const Vertex& v){ return v.clip.z()+v.clip.w(); }; // OpenGL near plane: z >= -w
  Vertex a=in.back(); float da=d(a);
  for(const auto& b:in) { float db=d(b);
    if((da>=0)!=(db>=0)) { float t=da/(da-db); Vertex v;
      v.clip=a.clip*(1-t)+b.clip*t; v.world=a.world*(1-t)+b.world*t; v.normal=a.normal*(1-t)+b.normal*t; v.uv=a.uv*(1-t)+b.uv*t; out.push_back(v); }
    if(db>=0) out.push_back(b);
    a=b; da=db; }
  std::vector<Vertex> kept; for(auto& v:out) if(v.clip.w()>eps) kept.push_back(v);
  return kept;
}
}

Image render(const std::vector<const Shape*>& shapes,const Camera& cam,const Light& light,Color3 bg,int width,int height,int ss) {
  if(width<1||height<1||width>8192||height>8192||uint64_t(width)*height>16777216) throw std::runtime_error("image dimensions exceed limits");
  ss=std::clamp(ss,1,4);
  const int W=width*ss, H=height*ss;
  if(uint64_t(W)*H>67108864) throw std::runtime_error("supersampled image dimensions exceed limits");
  std::vector<Color3> color(size_t(W)*H,bg);
  std::vector<float> depth(size_t(W)*H,std::numeric_limits<float>::infinity());
  Matrix4 view=Matrix4::lookAt(cam.eye,cam.target,Vector3::yAxis()).invertedRigid();
  Matrix4 proj=Matrix4::perspectiveProjection(Magnum::Deg(cam.fov),float(width)/float(height),.05f,10000.f);
  Matrix4 vp=proj*view;
  Vector3 to_light=-light.direction.normalized();
  // Optional shadow map: orthographic depth along the light direction over the bounds of every visible shape.
  struct ShadowMap { bool on=false; int R=0; std::vector<float> d; Vector3 r,u,f; float x0=0,y0=0,sx=0,sy=0,texel=0; } sm;
  if(light.shadows) {
    sm.f=light.direction.normalized(); Vector3 ref=std::fabs(sm.f.y())>.99f?Vector3::xAxis():Vector3::yAxis();
    sm.r=Magnum::Math::cross(sm.f,ref).normalized(); sm.u=Magnum::Math::cross(sm.r,sm.f);
    std::vector<Vector3> lp; std::vector<UnsignedInt> li;
    float mnx=1e30f,mny=1e30f,mxx=-1e30f,mxy=-1e30f;
    for(const Shape* s:shapes) { if(!s->visible) continue; Matrix4 model=s->transformation(); UnsignedInt base=UnsignedInt(lp.size());
      for(const auto& p:s->mesh.positions){ Vector3 w=model.transformPoint(p); Vector3 q{Magnum::Math::dot(w,sm.r),Magnum::Math::dot(w,sm.u),Magnum::Math::dot(w,sm.f)};
        lp.push_back(q); mnx=std::min(mnx,q.x()); mxx=std::max(mxx,q.x()); mny=std::min(mny,q.y()); mxy=std::max(mxy,q.y()); }
      for(auto i:s->mesh.indices) li.push_back(base+i); }
    if(!lp.empty()) {
      sm.on=true; sm.R=2048; sm.d.assign(size_t(sm.R)*sm.R,std::numeric_limits<float>::infinity());
      float ext=std::max(mxx-mnx,mxy-mny)*1.02f+1e-4f; sm.x0=mnx-(ext-(mxx-mnx))*.5f; sm.y0=mny-(ext-(mxy-mny))*.5f; sm.sx=sm.sy=float(sm.R)/ext; sm.texel=ext/float(sm.R);
      for(size_t t=0;t+2<li.size();t+=3) {
        Vector3 v[3]={lp[li[t]],lp[li[t+1]],lp[li[t+2]]}; float X[3],Y[3];
        for(int k=0;k<3;k++){ X[k]=(v[k].x()-sm.x0)*sm.sx; Y[k]=(v[k].y()-sm.y0)*sm.sy; }
        float area=(X[1]-X[0])*(Y[2]-Y[0])-(Y[1]-Y[0])*(X[2]-X[0]); if(std::fabs(area)<1e-12f) continue;
        int ax=std::max(0,int(std::floor(std::min({X[0],X[1],X[2]})))), bx=std::min(sm.R-1,int(std::ceil(std::max({X[0],X[1],X[2]}))));
        int ay=std::max(0,int(std::floor(std::min({Y[0],Y[1],Y[2]})))), by=std::min(sm.R-1,int(std::ceil(std::max({Y[0],Y[1],Y[2]}))));
        for(int y=ay;y<=by;y++) for(int x=ax;x<=bx;x++) { float px=x+.5f,py=y+.5f;
          float w0=((X[1]-px)*(Y[2]-py)-(Y[1]-py)*(X[2]-px))/area, w1=((X[2]-px)*(Y[0]-py)-(Y[2]-py)*(X[0]-px))/area, w2=1.f-w0-w1;
          if(w0<-1e-5f||w1<-1e-5f||w2<-1e-5f) continue;
          float z=w0*v[0].z()+w1*v[1].z()+w2*v[2].z(); float& m=sm.d[size_t(y)*sm.R+x]; if(z<m) m=z; } }
    }
  }
  auto lit=[&](const Vector3& pos,float ndotl)->float {
    if(!sm.on) return 1.f;
    float lx=(Magnum::Math::dot(pos,sm.r)-sm.x0)*sm.sx, ly=(Magnum::Math::dot(pos,sm.u)-sm.y0)*sm.sy, lz=Magnum::Math::dot(pos,sm.f);
    float bias=sm.texel*(2.5f+14.f*(1.f-std::clamp(ndotl,0.f,1.f))); int cx=int(std::floor(lx)), cy=int(std::floor(ly)); float s=0; int n=0;
    for(int j=-1;j<=1;j++) for(int i=-1;i<=1;i++){ int x=cx+i,y=cy+j; if(x<0||y<0||x>=sm.R||y>=sm.R){ s+=1.f; n++; continue; }
      s+=(lz-bias<=sm.d[size_t(y)*sm.R+x])?1.f:0.f; n++; }
    return s/float(n); };
  Vector3 fill=Vector3{.8f,-.2f,.9f}.normalized()*-1.f;
  for(const Shape* shape:shapes) { if(!shape->visible) continue;
    Matrix4 model=shape->transformation(); Matrix3 nm=model.normalMatrix();
    const Mesh& m=shape->mesh; const Material& mat=shape->material;
    const Texture* tex=mat.texture;
    if(tex&&m.uvs.size()!=m.positions.size()) throw std::runtime_error("shape '"+shape->name+"' has no texture coordinates (supported: cube, box, sphere, cylinder, cone, capsule, plane, text, ring, tube, triangle)");
    for(size_t t=0;t+2<m.indices.size();t+=3) {
      std::vector<Vertex> poly(3);
      Vector3 wp[3];
      for(int k=0;k<3;k++){ auto i=m.indices[t+k]; wp[k]=model.transformPoint(m.positions[i]);
        poly[k].world=wp[k]; poly[k].normal=(nm*m.normals[i]); poly[k].clip=vp*Vector4{wp[k],1.f}; if(tex) poly[k].uv=m.uvs[i]; }
      Vector3 fn=Magnum::Math::cross(wp[1]-wp[0],wp[2]-wp[0]);
      if(fn.dot()==0.f) continue;
      fn=fn.normalized();
      poly=clip_near(poly); if(poly.size()<3) continue;
      struct S { float x,y,z; Vector3 world,normal; Vector2 uv; float invw; };
      std::vector<S> s; for(auto& v:poly){ float iw=1.f/v.clip.w(); s.push_back({(v.clip.x()*iw*.5f+.5f)*W,(1.f-(v.clip.y()*iw*.5f+.5f))*H,v.clip.z()*iw,v.world*iw,v.normal*iw,v.uv*iw,iw}); }
      for(size_t f=1;f+1<s.size();f++) {
        const S &a=s[0],&b=s[f],&c=s[f+1];
        float area=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); if(std::fabs(area)<1e-9f) continue;
        int x0=std::max(0,int(std::floor(std::min({a.x,b.x,c.x})))), x1=std::min(W-1,int(std::ceil(std::max({a.x,b.x,c.x}))));
        int y0=std::max(0,int(std::floor(std::min({a.y,b.y,c.y})))), y1=std::min(H-1,int(std::ceil(std::max({a.y,b.y,c.y}))));
        for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) {
          float px=x+.5f, py=y+.5f;
          float w0=((b.x-px)*(c.y-py)-(b.y-py)*(c.x-px))/area, w1=((c.x-px)*(a.y-py)-(c.y-py)*(a.x-px))/area, w2=1.f-w0-w1;
          if(w0<-1e-6f||w1<-1e-6f||w2<-1e-6f) continue;
          float z=w0*a.z+w1*b.z+w2*c.z; size_t at=size_t(y)*W+x;
          if(z<-1.f||z>=depth[at]) continue;
          float iw=w0*a.invw+w1*b.invw+w2*c.invw;
          Vector3 pos=(a.world*w0+b.world*w1+c.world*w2)/iw, n=(a.normal*w0+b.normal*w1+c.normal*w2)/iw;
          n=n.dot()>0?n.normalized():fn;
          Vector3 vdir=(cam.eye-pos).normalized();
          if(Magnum::Math::dot(n,vdir)<0) n=-n; // two-sided shading
          float ndl=Magnum::Math::dot(n,to_light); float shade=ndl>0.f?lit(pos,ndl):0.f;
          float diff=std::max(0.f,ndl)*light.intensity*shade+std::max(0.f,Magnum::Math::dot(n,fill))*.22f+light.ambient;
          Vector3 h=(to_light+vdir).normalized();
          float spec=mat.specular*std::pow(std::max(0.f,Magnum::Math::dot(n,h)),mat.shininess)*shade;
          Color3 albedo=mat.color;
          if(tex) {   // perspective-correct UV and its screen-space derivatives (finite differences) pick the mip level
            auto uv_at=[&](float qx,float qy){ float u0=((b.x-qx)*(c.y-qy)-(b.y-qy)*(c.x-qx))/area, u1=((c.x-qx)*(a.y-qy)-(c.y-qy)*(a.x-qx))/area, u2=1.f-u0-u1;
              float q=u0*a.invw+u1*b.invw+u2*c.invw; return (a.uv*u0+b.uv*u1+c.uv*u2)/q; };
            Vector2 uv=uv_at(px,py), dx=uv_at(px+1.f,py)-uv, dy=uv_at(px,py+1.f)-uv;
            float tw=float(tex->width)*tex->scale_u, th=float(tex->height)*tex->scale_v;
            float rho=std::max(std::hypot(dx.x()*tw,dx.y()*th),std::hypot(dy.x()*tw,dy.y()*th))/float(ss);   // texels per output pixel
            albedo=albedo*tex->sample(uv.x(),uv.y(),rho>1.f?std::log2(rho):0.f);
          }
          depth[at]=z; color[at]=Color3{albedo*diff+Color3{spec}};
        }
      }
    }
  }
  Image im; im.width=width; im.height=height; im.rgb.resize(size_t(width)*height*3);
  for(int y=0;y<height;y++) for(int x=0;x<width;x++) { Vector3 acc{};
    for(int j=0;j<ss;j++) for(int i=0;i<ss;i++) acc+=Vector3(color[size_t(y*ss+j)*W+x*ss+i]);
    acc/=float(ss*ss); size_t o=(size_t(y)*width+x)*3; im.rgb[o]=channel(acc.x()); im.rgb[o+1]=channel(acc.y()); im.rgb[o+2]=channel(acc.z()); }
  return im;
}

void write_png(const std::string& path,const Image& im) {
  validate(im);
  std::vector<unsigned char> raw; raw.reserve(size_t(im.height)*(size_t(im.width)*3+1));
  for(int y=0;y<im.height;y++){ raw.push_back(0); auto b=im.rgb.begin()+size_t(y)*im.width*3; raw.insert(raw.end(),b,b+size_t(im.width)*3); }
  std::vector<unsigned char> out{137,80,78,71,13,10,26,10}, ihdr; put32(ihdr,im.width); put32(ihdr,im.height); ihdr.insert(ihdr.end(),{8,2,0,0,0});
  chunk(out,"IHDR",ihdr); chunk(out,"sRGB",{0}); chunk(out,"IDAT",deflate_zlib(raw)); chunk(out,"IEND",{});
  save(path,out.data(),out.size());
}
void write_ppm(const std::string& path,const Image& im) {
  validate(im);
  std::string head="P6\n"+std::to_string(im.width)+" "+std::to_string(im.height)+"\n255\n";
  std::string body=head; body.append(reinterpret_cast<const char*>(im.rgb.data()),im.rgb.size());
  save(path,body);
}

namespace {
struct World { std::vector<Vector3> p; std::vector<Vector3> n; std::vector<UnsignedInt> idx; };
World gather(const std::vector<const Shape*>& shapes) {
  World w;
  for(const Shape* s:shapes) { if(!s->visible) continue;
    Matrix4 model=s->transformation(); auto nm=model.normalMatrix(); UnsignedInt base=UnsignedInt(w.p.size());
    for(size_t i=0;i<s->mesh.positions.size();i++){ w.p.push_back(model.transformPoint(s->mesh.positions[i])); w.n.push_back((nm*s->mesh.normals[i]).normalized()); }
    for(auto i:s->mesh.indices) w.idx.push_back(base+i); }
  return w;
}
}
void write_stl(const std::string& path,const std::vector<const Shape*>& shapes) {
  World w=gather(shapes); uint32_t tris=uint32_t(w.idx.size()/3);
  std::vector<unsigned char> out(84+size_t(tris)*50,0);
  std::memcpy(out.data(),"lmmagnum binary STL",19); std::memcpy(out.data()+80,&tris,4);
  unsigned char* o=out.data()+84;
  for(uint32_t t=0;t<tris;t++,o+=50) {
    Vector3 v[3]={w.p[w.idx[3*t]],w.p[w.idx[3*t+1]],w.p[w.idx[3*t+2]]};
    Vector3 n=Magnum::Math::cross(v[1]-v[0],v[2]-v[0]); if(n.dot()>0) n=n.normalized();
    float f[12]={n.x(),n.y(),n.z(),v[0].x(),v[0].y(),v[0].z(),v[1].x(),v[1].y(),v[1].z(),v[2].x(),v[2].y(),v[2].z()};
    std::memcpy(o,f,48); }
  save(path,out.data(),out.size());
}
void write_obj(const std::string& path,const std::vector<const Shape*>& shapes) {
  World w=gather(shapes); std::ostringstream o; o.precision(9); o<<"# lmmagnum\n";
  for(auto& p:w.p) o<<"v "<<p.x()<<' '<<p.y()<<' '<<p.z()<<'\n';
  for(auto& n:w.n) o<<"vn "<<n.x()<<' '<<n.y()<<' '<<n.z()<<'\n';
  for(size_t t=0;t+2<w.idx.size();t+=3) { o<<'f'; for(int k=0;k<3;k++) o<<' '<<w.idx[t+k]+1<<"//"<<w.idx[t+k]+1; o<<'\n'; }
  save(path,o.str());
}
void write_ply(const std::string& path,const std::vector<const Shape*>& shapes) {
  World w=gather(shapes); std::ostringstream o; o.precision(9);
  o<<"ply\nformat ascii 1.0\ncomment lmmagnum\nelement vertex "<<w.p.size()<<"\nproperty float x\nproperty float y\nproperty float z\nproperty float nx\nproperty float ny\nproperty float nz\nelement face "<<w.idx.size()/3<<"\nproperty list uchar uint vertex_indices\nend_header\n";
  for(size_t i=0;i<w.p.size();i++) o<<w.p[i].x()<<' '<<w.p[i].y()<<' '<<w.p[i].z()<<' '<<w.n[i].x()<<' '<<w.n[i].y()<<' '<<w.n[i].z()<<'\n';
  for(size_t t=0;t+2<w.idx.size();t+=3) o<<"3 "<<w.idx[t]<<' '<<w.idx[t+1]<<' '<<w.idx[t+2]<<'\n';
  save(path,o.str());
}
}
