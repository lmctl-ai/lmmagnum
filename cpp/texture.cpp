#include "scene.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

namespace lmmagnum {
namespace {
constexpr int kMaxSide = 8192;
// Wrap an integer texel coordinate into [0, n).
int wrap_index(int i, int n, Texture::Wrap w) {
  switch(w) {
    case Texture::Wrap::Clamp: return std::clamp(i, 0, n-1);
    case Texture::Wrap::Mirror: { int period = 2*n; int m = ((i % period) + period) % period; return m < n ? m : period-1-m; }
    default: return ((i % n) + n) % n;
  }
}
Color3 fetch(const Texture::Level& l, int x, int y, Texture::Wrap w) {
  x = wrap_index(x, l.w, w); y = wrap_index(y, l.h, w);
  const float* p = &l.rgb[(size_t(y)*l.w + x)*3]; return Color3{p[0], p[1], p[2]};
}
void build_levels(const Texture& t) {
  t.levels.clear();
  Texture::Level base; base.w = t.width; base.h = t.height; base.rgb.resize(t.rgb.size());
  for(size_t i=0;i<t.rgb.size();i++) base.rgb[i] = float(t.rgb[i])/255.f;
  t.levels.push_back(std::move(base));
  while(t.levels.back().w > 1 || t.levels.back().h > 1) {
    const auto& s = t.levels.back(); Texture::Level n; n.w = std::max(1, s.w/2); n.h = std::max(1, s.h/2); n.rgb.assign(size_t(n.w)*n.h*3, 0.f);
    for(int y=0;y<n.h;y++) for(int x=0;x<n.w;x++) for(int c=0;c<3;c++) {
      int x0 = std::min(2*x, s.w-1), x1 = std::min(2*x+1, s.w-1), y0 = std::min(2*y, s.h-1), y1 = std::min(2*y+1, s.h-1);
      n.rgb[(size_t(y)*n.w+x)*3+c] = .25f*(s.rgb[(size_t(y0)*s.w+x0)*3+c] + s.rgb[(size_t(y0)*s.w+x1)*3+c] + s.rgb[(size_t(y1)*s.w+x0)*3+c] + s.rgb[(size_t(y1)*s.w+x1)*3+c]);
    }
    t.levels.push_back(std::move(n));
  }
  t.dirty = false;
}
Color3 bilinear(const Texture::Level& l, float u, float v, Texture::Wrap w) {
  float x = u*float(l.w) - .5f, y = (1.f-v)*float(l.h) - .5f;
  float fx = std::floor(x), fy = std::floor(y); float tx = x-fx, ty = y-fy; int ix = int(fx), iy = int(fy);
  Color3 a = fetch(l, ix, iy, w)*(1-tx) + fetch(l, ix+1, iy, w)*tx, b = fetch(l, ix, iy+1, w)*(1-tx) + fetch(l, ix+1, iy+1, w)*tx;
  return a*(1-ty) + b*ty;
}
}

Color3 Texture::sample(float u, float v, float lod) const {
  if(width <= 0 || height <= 0) return Color3{1.f};
  if(dirty || levels.empty()) build_levels(*this);
  u *= scale_u; v *= scale_v;
  if(!std::isfinite(u) || !std::isfinite(v)) return Color3{1.f};
  const float top = float(levels.size()-1);
  lod = std::isfinite(lod) ? std::clamp(lod, 0.f, top) : 0.f;
  if(!linear) {
    const Level& l = levels[size_t(std::lround(lod))];
    return fetch(l, int(std::floor(u*float(l.w))), int(std::floor((1.f-v)*float(l.h))), wrap);
  }
  int lo = int(std::floor(lod)); float f = lod - float(lo); int hi = std::min(lo+1, int(top));
  Color3 a = bilinear(levels[size_t(lo)], u, v, wrap);
  if(f <= 0.f || hi == lo) return a;
  return a*(1.f-f) + bilinear(levels[size_t(hi)], u, v, wrap)*f;
}

Texture Texture::checker(int size, int cell, Color3 a, Color3 b) {
  Texture t; t.width = t.height = size; t.rgb.resize(size_t(size)*size*3);
  auto q = [](float x){ return static_cast<unsigned char>(std::lround(std::clamp(x, 0.f, 1.f)*255.f)); };
  for(int y=0;y<size;y++) for(int x=0;x<size;x++) { const Color3& c = ((x/cell + y/cell) % 2 == 0) ? a : b; size_t o = (size_t(y)*size + x)*3; t.rgb[o]=q(c.r()); t.rgb[o+1]=q(c.g()); t.rgb[o+2]=q(c.b()); }
  return t;
}

Texture load_texture(const std::string& path) {
  if(path.empty() || path.find('\0') != std::string::npos) throw std::runtime_error("texture path must be nonempty and contain no NUL bytes");
  std::ifstream in(path, std::ios::binary); if(!in) throw std::runtime_error("cannot open texture file: " + path);
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if(bytes.empty() || bytes.size() > (256u<<20)) throw std::runtime_error("texture file is empty or larger than 256 MiB: " + path);
  int w=0, h=0, n=0;
  if(!stbi_info_from_memory(bytes.data(), int(bytes.size()), &w, &h, &n)) throw std::runtime_error("unsupported or corrupt texture (PNG, JPEG, BMP and TGA are supported): " + path);
  if(w < 1 || h < 1 || w > kMaxSide || h > kMaxSide || uint64_t(w)*uint64_t(h) > 67108864ull) throw std::runtime_error("texture dimensions exceed limits (8192 per side, 64 Mpixel): " + path);
  unsigned char* px = stbi_load_from_memory(bytes.data(), int(bytes.size()), &w, &h, &n, 4);
  if(!px) throw std::runtime_error(std::string("cannot decode texture: ") + path + ": " + stbi_failure_reason());
  Texture t; t.width = w; t.height = h; t.rgb.resize(size_t(w)*h*3);
  for(size_t i=0;i<size_t(w)*h;i++) {   // composite over white so transparent texels do not turn black
    float a = px[i*4+3]/255.f;
    for(int c=0;c<3;c++) t.rgb[i*3+c] = static_cast<unsigned char>(std::lround(px[i*4+c]*a + 255.f*(1.f-a)));
  }
  stbi_image_free(px);
  return t;
}
void save_texture_png(const std::string& path, const Texture& t) {
  if(t.width < 1 || t.height < 1) throw std::runtime_error("texture is empty");
  Image im; im.width = t.width; im.height = t.height; im.rgb = t.rgb; write_png(path, im);
}
}
