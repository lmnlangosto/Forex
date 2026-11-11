#include "epe_doc6_chain.hpp"
#include <cmath>

namespace epe {
namespace doc6 {

namespace {

float Luma(const Rgba& c) { return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b; }

}  // namespace

Rgba ApplyChromaticChain(Rgba in, const EpeDoc6Profile& p) {
  // Trabajo en lineal; A02 default linear-work (identidad aquí).
  // A01 lut_id=0 → identidad.
  Rgba c = in;

  // A08/A09 black/white points
  const float bp = p.black_point;
  const float wp = p.white_point > bp ? p.white_point : bp + 1e-4f;
  auto map_bw = [&](float x) { return (x - bp) / (wp - bp); };
  c.r = map_bw(c.r);
  c.g = map_bw(c.g);
  c.b = map_bw(c.b);

  // A07 offset
  c.r += p.offset[0];
  c.g += p.offset[1];
  c.b += p.offset[2];

  // A06 gain
  c.r *= p.color_gain_rgb[0];
  c.g *= p.color_gain_rgb[1];
  c.b *= p.color_gain_rgb[2];

  // A04 contrast pivot 0.5
  c.r = (c.r - 0.5f) * p.contrast + 0.5f;
  c.g = (c.g - 0.5f) * p.contrast + 0.5f;
  c.b = (c.b - 0.5f) * p.contrast + 0.5f;

  // A05 saturation
  const float y = Luma(c);
  c.r = y + (c.r - y) * p.saturation;
  c.g = y + (c.g - y) * p.saturation;
  c.b = y + (c.b - y) * p.saturation;

  // A03 gamma (espacio de trabajo; gamma=1 identidad)
  if (std::fabs(p.gamma - 1.f) > 1e-6f && p.gamma > 0.f) {
    auto g = [&](float x) {
      x = Saturate(x);
      return std::pow(x, 1.f / p.gamma);
    };
    c.r = g(c.r);
    c.g = g(c.g);
    c.b = g(c.b);
  }

  // A10 clamp_exposure
  const float exp_mul = std::pow(2.f, p.exposure);
  c.r = Saturate(c.r * exp_mul);
  c.g = Saturate(c.g * exp_mul);
  c.b = Saturate(c.b * exp_mul);
  c.a = Saturate(c.a);
  return c;
}

void ApplyProfileToRgba8(std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                         const EpeDoc6Profile& p) {
  // Ordered 4×4 Bayer (A11) — 1 LSB cuando dither_enable.
  static const int bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
  for (std::uint32_t y = 0; y < height; ++y) {
    for (std::uint32_t x = 0; x < width; ++x) {
      std::uint8_t* px = rgba + (static_cast<std::size_t>(y) * width + x) * 4;
      Rgba c{px[0] / 255.f, px[1] / 255.f, px[2] / 255.f, px[3] / 255.f};
      // Des-premultiplicar aproximado para cadena; re-premultiplicar al final.
      if (c.a > 1e-6f) {
        c.r /= c.a;
        c.g /= c.a;
        c.b /= c.a;
      }
      c = ApplyChromaticChain(c, p);
      c.a *= p.opacity_default;  // A13
      // Premult A12 convención a salida
      c.r *= c.a;
      c.g *= c.a;
      c.b *= c.a;
      float rf = c.r * 255.f, gf = c.g * 255.f, bf = c.b * 255.f, af = c.a * 255.f;
      if (p.dither_enable) {
        const float d = (bayer[y & 3][x & 3] / 16.f - 0.5f);
        rf += d;
        gf += d;
        bf += d;
      }
      auto q = [](float v) -> std::uint8_t {
        if (v < 0.f) return 0;
        if (v > 255.f) return 255;
        return static_cast<std::uint8_t>(v + 0.5f);
      };
      px[0] = q(rf);
      px[1] = q(gf);
      px[2] = q(bf);
      px[3] = q(af);
    }
  }
}

}  // namespace doc6
}  // namespace epe
