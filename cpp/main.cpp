#include "dslvm.hpp"
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "scene.hpp"
#include "schema_embedded.hpp"
#include <Magnum/Math/Functions.h>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#ifndef LMMAGNUM_MAGNUM_COMMIT
#define LMMAGNUM_MAGNUM_COMMIT "unknown"
#endif

namespace {
using dslvm::Args; using dslvm::Value;
using namespace lmmagnum;
struct LightHandle { Light* light; };
struct Host {
  std::vector<std::unique_ptr<Shape>> shapes;
  std::vector<std::unique_ptr<Material>> materials;
  std::vector<std::unique_ptr<Group>> groups;
  Camera camera; Light light; LightHandle light_handle{&light};
  Color3 background{.08f,.09f,.12f};
  std::vector<double> accum; int accum_w=0, accum_h=0, accum_n=0;
  template<class T> Value hold(std::vector<std::unique_ptr<T>>& arena,const char* type,T value) {
    if(arena.size()>=4096) throw dslvm::Error(std::string(type)+" object limit exceeded (4096)");
    arena.push_back(std::make_unique<T>(std::move(value))); return Value::object(type,arena.back().get()); }
  std::vector<const Shape*> visible() const { std::vector<const Shape*> v; for(auto& s:shapes) v.push_back(s.get()); return v; }
};
void finite(double x,const char* name){ if(!std::isfinite(x)) throw dslvm::Error(std::string(name)+" must be finite"); }
void positive(double x,const char* name){ finite(x,name); if(x<=0) throw dslvm::Error(std::string(name)+" must be positive"); }
Vector3 vec(const Args& a,size_t i){ return {float(a[i].f),float(a[i+1].f),float(a[i+2].f)}; }
Material& mat(const Args& a,size_t i){ return *static_cast<Material*>(a[i].obj); }
Value make(Host& h,Mesh m,const Material& mt,const char* name){ Shape s; s.mesh=std::move(m); s.material=mt; s.name=name; return h.hold(h.shapes,"Shape",std::move(s)); }
void checked_method(dslvm::Registry& r,const char* type,const char* name,std::vector<std::string> params,std::string returns,dslvm::Method cb) {
  r.method(type,name,params,returns,[params,cb,type,name](void* object,Args& args){
    for(size_t i=0;i<params.size();i++){ const auto& e=params[i]; const auto& v=args[i];
      bool ok=e=="float"?v.kind==Value::FLOAT:e=="int"?v.kind==Value::INT:e=="string"?v.kind==Value::STR:e=="bool"?v.kind==Value::BOOL:v.kind==Value::OBJ&&v.type==e&&v.obj;
      if(!ok) throw dslvm::Error(std::string(type)+"."+name+" argument "+std::to_string(i+1)+" must be "+e); }
    try{ return cb(object,args); } catch(const dslvm::Error&){ throw; } catch(const std::exception& e){ throw dslvm::Error(e.what()); } });
}
void register_host(dslvm::Registry& r,Host& h) {
  r.context("scene","Scene");
  r.function("material",{},"Material",[&h](Args&){ return h.hold(h.materials,"Material",Material{}); });
  r.function("magnum_version",{},"string",[](Args&){ return Value::string(LMMAGNUM_MAGNUM_COMMIT); });
  // Scene: primitives come from Magnum::Primitives (unit solids), scaled to the requested size.
  checked_method(r,"Scene","cube",{"float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"cube size"); float s=float(a[0].f)/2; return make(h,scaled(make_cube(),{s,s,s}),mat(a,1),"cube"); });
  checked_method(r,"Scene","box",{"float","float","float","Material"},"Shape",[&h](void*,Args& a){ for(int i=0;i<3;i++) positive(a[i].f,"box dimension"); return make(h,scaled(make_cube(),vec(a,0)/2),mat(a,3),"box"); });
  checked_method(r,"Scene","sphere",{"float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"sphere radius"); float s=float(a[0].f); return make(h,scaled(make_sphere(24),{s,s,s}),mat(a,1),"sphere"); });
  checked_method(r,"Scene","icosphere",{"float","int","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"icosphere radius"); if(a[1].i<0||a[1].i>6) throw dslvm::Error("icosphere subdivisions must be between 0 and 6"); float s=float(a[0].f); return make(h,scaled(make_icosphere(unsigned(a[1].i)),{s,s,s}),mat(a,2),"icosphere"); });
  checked_method(r,"Scene","cylinder",{"float","float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"cylinder radius"); positive(a[1].f,"cylinder height"); return make(h,scaled(make_cylinder(),{float(a[0].f),float(a[1].f)/2,float(a[0].f)}),mat(a,2),"cylinder"); });
  checked_method(r,"Scene","cone",{"float","float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"cone radius"); positive(a[1].f,"cone height"); return make(h,scaled(make_cone(),{float(a[0].f),float(a[1].f)/2,float(a[0].f)}),mat(a,2),"cone"); });
  checked_method(r,"Scene","capsule",{"float","float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"capsule radius"); positive(a[1].f,"capsule length"); float rad=float(a[0].f); return make(h,scaled(make_capsule(),{rad,rad,rad}),mat(a,2),"capsule"); });
  checked_method(r,"Scene","tube_between",{"float","float","float","float","float","float","float","Material"},"Shape",[&h](void*,Args& a){
    for(int i=0;i<7;i++) finite(a[i].f,"tube_between argument"); positive(a[6].f,"tube radius");
    return make(h,make_tube(vec(a,0),vec(a,3),float(a[6].f)),mat(a,7),"tube"); });
  checked_method(r,"Scene","ring",{"float","float","Material"},"Shape",[&h](void*,Args& a){
    positive(a[0].f,"ring inner radius"); positive(a[1].f,"ring outer radius"); if(a[1].f<=a[0].f) throw dslvm::Error("ring outer radius must exceed inner radius");
    return make(h,make_ring(float(a[0].f),float(a[1].f),96),mat(a,2),"ring"); });
  checked_method(r,"Scene","plane",{"float","Material"},"Shape",[&h](void*,Args& a){ positive(a[0].f,"plane size"); float s=float(a[0].f)/2; return make(h,scaled(make_plane(),{s,s,1.f}),mat(a,1),"plane"); });
  checked_method(r,"Scene","text",{"string","float","Material"},"Shape",[&h](void*,Args& a){
    positive(a[1].f,"text height"); if(a[0].s.empty()||a[0].s.size()>256) throw dslvm::Error("text must be 1 to 256 characters");
    return make(h,make_text(a[0].s,float(a[1].f)),mat(a,2),"text"); });
  checked_method(r,"Scene","group",{},"Group",[&h](void*,Args&){ return h.hold(h.groups,"Group",Group{}); });
  checked_method(r,"Scene","camera",{"float","float","float","float","float","float","float"},"Camera",[&h](void*,Args& a){
    for(auto& x:a) finite(x.f,"camera argument"); if(a[0].f<=0||a[0].f>=179) throw dslvm::Error("camera field of view must be between 0 and 179 degrees");
    Vector3 eye=vec(a,1), target=vec(a,4); Vector3 f=target-eye;
    if(f.dot()==0.f) throw dslvm::Error("camera eye and target must differ");
    if(Magnum::Math::cross(f.normalized(),Vector3::yAxis()).dot()<1e-10f) throw dslvm::Error("camera view direction must not be vertical");
    h.camera={float(a[0].f),eye,target}; return Value::object("Camera",&h.camera); });
  checked_method(r,"Scene","directional_light",{"float","float","float","float"},"Light",[&h](void*,Args& a){
    for(auto& x:a) finite(x.f,"directional light argument"); if(a[3].f<0) throw dslvm::Error("light intensity must not be negative");
    Vector3 d=vec(a,0); if(d.dot()==0.f) throw dslvm::Error("light direction must be nonzero");
    h.light.direction=d.normalized(); h.light.intensity=float(a[3].f); return Value::object("Light",&h.light_handle); });
  checked_method(r,"Scene","background",{"float","float","float"},"Scene",[&h](void* self,Args& a){
    for(int i=0;i<3;i++){ finite(a[i].f,"background"); if(a[i].f<0||a[i].f>1) throw dslvm::Error("background must be between 0 and 1"); }
    h.background=Color3(vec(a,0)); return Value::object("Scene",self); });
  auto dims=[](Args& a){ if(a[1].i<1||a[2].i<1||a[1].i>8192||a[2].i>8192) throw dslvm::Error("image dimensions must be between 1 and 8192"); };
  checked_method(r,"Scene","render_png",{"string","int","int"},"void",[&h,dims](void*,Args& a){ dims(a); write_png(a[0].s,render(h.visible(),h.camera,h.light,h.background,int(a[1].i),int(a[2].i))); return Value::none(); });
  checked_method(r,"Scene","render_ppm",{"string","int","int"},"void",[&h,dims](void*,Args& a){ dims(a); write_ppm(a[0].s,render(h.visible(),h.camera,h.light,h.background,int(a[1].i),int(a[2].i))); return Value::none(); });
  checked_method(r,"Scene","triangle",{"float","float","float","float","float","float","float","float","float","Material"},"Shape",[&h](void*,Args& a){
    for(int i=0;i<9;i++) finite(a[i].f,"triangle vertex"); Vector3 p0=vec(a,0),p1=vec(a,3),p2=vec(a,6);
    if(Magnum::Math::cross(p1-p0,p2-p0).dot()==0.f) throw dslvm::Error("triangle must not be degenerate"); return make(h,make_triangle(p0,p1,p2),mat(a,9),"triangle"); });
  checked_method(r,"Scene","shadows",{"bool"},"Scene",[&h](void* self,Args& a){ h.light.shadows=a[0].b; return Value::object("Scene",self); });
  checked_method(r,"Scene","ambient",{"float"},"Scene",[&h](void* self,Args& a){ finite(a[0].f,"ambient"); if(a[0].f<0) throw dslvm::Error("ambient must not be negative"); h.light.ambient=float(a[0].f); return Value::object("Scene",self); });
  checked_method(r,"Scene","reset_accumulation",{},"void",[&h](void*,Args&){ h.accum.clear(); h.accum_n=0; return Value::none(); });
  checked_method(r,"Scene","accumulate",{"int","int"},"void",[&h,dims](void*,Args& a){
    Args d{a[0],a[0],a[1]}; dims(d); int w=int(a[0].i),hh=int(a[1].i);
    if(h.accum_n>0&&(w!=h.accum_w||hh!=h.accum_h)) throw dslvm::Error("accumulate: size changed; call reset_accumulation first");
    Image im=render(h.visible(),h.camera,h.light,h.background,w,hh);
    if(h.accum_n==0){ h.accum.assign(im.rgb.size(),0.0); h.accum_w=w; h.accum_h=hh; }
    for(size_t i=0;i<im.rgb.size();i++) h.accum[i]+=im.rgb[i];
    h.accum_n++; return Value::none(); });
  checked_method(r,"Scene","save_accumulated_png",{"string"},"void",[&h](void*,Args& a){
    if(h.accum_n==0) throw dslvm::Error("save_accumulated_png: nothing accumulated");
    Image im; im.width=h.accum_w; im.height=h.accum_h; im.rgb.resize(h.accum.size());
    for(size_t i=0;i<im.rgb.size();i++) im.rgb[i]=static_cast<unsigned char>(std::lround(h.accum[i]/h.accum_n));
    write_png(a[0].s,im); return Value::none(); });
  checked_method(r,"Scene","export_stl",{"string"},"void",[&h](void*,Args& a){ write_stl(a[0].s,h.visible()); return Value::none(); });
  checked_method(r,"Scene","export_obj",{"string"},"void",[&h](void*,Args& a){ write_obj(a[0].s,h.visible()); return Value::none(); });
  checked_method(r,"Scene","export_ply",{"string"},"void",[&h](void*,Args& a){ write_ply(a[0].s,h.visible()); return Value::none(); });
  checked_method(r,"Scene","triangle_count",{},"int",[&h](void*,Args&){ int64_t n=0; for(auto& s:h.shapes) if(s->visible) n+=int64_t(triangle_count(s->mesh)); return Value::integer(n); });
  checked_method(r,"Scene","shape_count",{},"int",[&h](void*,Args&){ return Value::integer(int64_t(h.shapes.size())); });
  auto S=[](void* p){ return static_cast<Shape*>(p); };
  checked_method(r,"Shape","translate",{"float","float","float"},"Shape",[S](void* p,Args& a){ for(int i=0;i<3;i++) finite(a[i].f,"translate"); S(p)->translation=vec(a,0); return Value::object("Shape",p); });
  checked_method(r,"Shape","rotate",{"float","float","float"},"Shape",[S](void* p,Args& a){ for(int i=0;i<3;i++) finite(a[i].f,"rotate"); S(p)->rotation=vec(a,0); return Value::object("Shape",p); });
  checked_method(r,"Shape","scale",{"float","float","float"},"Shape",[S](void* p,Args& a){ for(int i=0;i<3;i++) positive(a[i].f,"scale"); S(p)->scaling=vec(a,0); return Value::object("Shape",p); });
  checked_method(r,"Shape","visible",{"bool"},"Shape",[S](void* p,Args& a){ S(p)->visible=a[0].b; return Value::object("Shape",p); });
  checked_method(r,"Shape","name",{"string"},"Shape",[S](void* p,Args& a){ S(p)->name=a[0].s; return Value::object("Shape",p); });
  checked_method(r,"Shape","color",{"float","float","float"},"Shape",[S](void* p,Args& a){ for(int i=0;i<3;i++){ finite(a[i].f,"color"); if(a[i].f<0||a[i].f>1) throw dslvm::Error("color must be between 0 and 1"); } S(p)->material.color=Color3(vec(a,0)); return Value::object("Shape",p); });
  checked_method(r,"Shape","triangle_count",{},"int",[S](void* p,Args&){ return Value::integer(int64_t(triangle_count(S(p)->mesh))); });
  auto G=[](void* p){ return static_cast<Group*>(p); };
  checked_method(r,"Group","translate",{"float","float","float"},"Group",[G](void* p,Args& a){ for(int i=0;i<3;i++) finite(a[i].f,"translate"); G(p)->translation=vec(a,0); return Value::object("Group",p); });
  checked_method(r,"Group","rotate",{"float","float","float"},"Group",[G](void* p,Args& a){ for(int i=0;i<3;i++) finite(a[i].f,"rotate"); G(p)->rotation=vec(a,0); return Value::object("Group",p); });
  checked_method(r,"Group","scale",{"float","float","float"},"Group",[G](void* p,Args& a){ for(int i=0;i<3;i++) positive(a[i].f,"scale"); G(p)->scaling=vec(a,0); return Value::object("Group",p); });
  checked_method(r,"Group","parent",{"Group"},"Group",[G](void* p,Args& a){
    auto* np=static_cast<Group*>(a[0].obj); for(const Group* g=np;g;g=g->parent) if(g==G(p)) throw dslvm::Error("group parent would create a cycle");
    G(p)->parent=np; return Value::object("Group",p); });
  checked_method(r,"Shape","parent",{"Group"},"Shape",[S](void* p,Args& a){ S(p)->parent=static_cast<Group*>(a[0].obj); return Value::object("Shape",p); });
  auto M=[](void* p){ return static_cast<Material*>(p); };
  checked_method(r,"Material","color",{"float","float","float"},"Material",[M](void* p,Args& a){ for(int i=0;i<3;i++){ finite(a[i].f,"color"); if(a[i].f<0||a[i].f>1) throw dslvm::Error("color must be between 0 and 1"); } M(p)->color=Color3(vec(a,0)); return Value::object("Material",p); });
  checked_method(r,"Material","roughness",{"float"},"Material",[M](void* p,Args& a){ finite(a[0].f,"roughness"); M(p)->roughness=float(a[0].f); return Value::object("Material",p); });
  checked_method(r,"Material","specular",{"float"},"Material",[M](void* p,Args& a){ finite(a[0].f,"specular"); M(p)->specular=float(a[0].f); return Value::object("Material",p); });
  checked_method(r,"Material","shininess",{"float"},"Material",[M](void* p,Args& a){ positive(a[0].f,"shininess"); M(p)->shininess=float(a[0].f); return Value::object("Material",p); });
  checked_method(r,"Camera","look_at",{"float","float","float","float","float","float"},"Camera",[](void* p,Args& a){
    for(int i=0;i<6;i++) finite(a[i].f,"look_at"); auto* c=static_cast<Camera*>(p); Vector3 eye=vec(a,0),t=vec(a,3);
    if((t-eye).dot()==0.f) throw dslvm::Error("camera eye and target must differ"); c->eye=eye; c->target=t; return Value::object("Camera",p); });
  checked_method(r,"Light","intensity",{"float"},"Light",[](void* p,Args& a){ finite(a[0].f,"intensity"); if(a[0].f<0) throw dslvm::Error("light intensity must not be negative"); static_cast<LightHandle*>(p)->light->intensity=float(a[0].f); return Value::object("Light",p); });
}
void print_schema_summary(dslvm::Registry& reg) {
  std::vector<std::string> order; std::map<std::string,std::vector<std::string>> members; std::string current;
  for(const auto& sig:reg.signatures()) {
    if(sig.rfind("context ",0)==0||sig.rfind("fn ",0)==0) std::cout<<sig<<"\n";
    else if(sig.rfind("method ",0)==0) { std::string body=sig.substr(7),type=body.substr(0,body.find('.')); if(type!=current){ current=type; order.push_back(type); } members[type].push_back(body); } }
  for(const auto& type:order){ std::cout<<type<<":\n"; for(const auto& m:members[type]) std::cout<<"  "<<m<<"\n"; }
}
}
int main(int argc,char** argv) {
#ifdef _WIN32
  _setmode(_fileno(stdout),_O_BINARY); _setmode(_fileno(stderr),_O_BINARY);
#endif
  Host host; dslvm::Registry reg; register_host(reg,host);
  const std::string usage="usage: lmmagnum --version | --dsl-schema [--json] | --dsl-signatures | --dsl FILE [FILE ...]\n";
  if(argc==2&&std::string(argv[1])=="--version"){ std::cout<<"lmmagnum " LMMAGNUM_VERSION "\n"; return std::cout?0:4; }
  if(argc>=2&&std::string(argv[1])=="--dsl-schema") {
    if(argc==2){ print_schema_summary(reg); return std::cout?0:4; }
    if(argc==3&&std::string(argv[2])=="--json"){ std::cout.write(reinterpret_cast<const char*>(lmmagnum::embedded_schema),static_cast<std::streamsize>(lmmagnum::embedded_schema_size)); return std::cout?0:4; }
    std::cerr<<usage; return 2; }
  if(argc==2&&std::string(argv[1])=="--dsl-signatures"){ for(const auto& s:reg.signatures()) std::cout<<s<<"\n"; return 0; }
  if(argc<3||std::string(argv[1])!="--dsl"){ std::cerr<<usage; return 2; }
  for(int i=2;i<argc;i++) if(std::string(argv[i]).empty()||std::string(argv[i]).rfind("--",0)==0){ std::cerr<<usage; return 2; }
  const std::string schema(reinterpret_cast<const char*>(lmmagnum::embedded_schema),lmmagnum::embedded_schema_size);
  dslvm::Context context; context.set("scene",&host);
  for(int i=2;i<argc;i++) {
    dslvm::Program program;
    try{ program=dslvm::Program::load(dslvm::read_file(argv[i]),schema,reg); } catch(const dslvm::Error& e){ return dslvm::report_error("load",e); }
    try{ program.run(context,reg); } catch(const dslvm::Error& e){ return dslvm::report_error("run",e); }
    dslvm::report_ok();
  }
  return 0;
}
