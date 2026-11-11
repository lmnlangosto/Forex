#include "epe_paint_soft.hpp"
#include "../Effects/epe_doc6_chain.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace epe {
namespace render {

Status SoftPainter::Init(std::uint32_t width, std::uint32_t height, const EpeDoc6Profile& profile) {
  if (width < 64 || height < 64) return Status::InvalidArg;
  w_ = width;
  h_ = height;
  profile_ = profile;
  fb_.assign(static_cast<std::size_t>(w_) * h_ * 4, 0);
  stats_ = {};
  ready_ = true;
  return Status::Ok;
}

void SoftPainter::SetProfile(const EpeDoc6Profile& profile) { profile_ = profile; }

void SoftPainter::Shutdown() {
  ready_ = false;
  fb_.clear();
}

void SoftPainter::Put(std::int32_t x, std::int32_t y, float r, float g, float b, float a) {
  if (x < 0 || y < 0 || x >= static_cast<std::int32_t>(w_) || y >= static_cast<std::int32_t>(h_))
    return;
  // A12 premult: dst = src + dst*(1-src.a)
  std::uint8_t* p = fb_.data() + (static_cast<std::size_t>(y) * w_ + static_cast<std::size_t>(x)) * 4;
  const float da = p[3] / 255.f;
  const float dr = p[0] / 255.f, dg = p[1] / 255.f, db = p[2] / 255.f;
  const float out_a = a + da * (1.f - a);
  const float out_r = r * a + dr * (1.f - a);
  const float out_g = g * a + dg * (1.f - a);
  const float out_b = b * a + db * (1.f - a);
  auto q = [](float v) -> std::uint8_t {
    v *= 255.f;
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<std::uint8_t>(v + 0.5f);
  };
  p[0] = q(out_r);
  p[1] = q(out_g);
  p[2] = q(out_b);
  p[3] = q(out_a);
}

void SoftPainter::FillRect(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1,
                           float r, float g, float b, float a) {
  if (x0 > x1) std::swap(x0, x1);
  if (y0 > y1) std::swap(y0, y1);
  for (std::int32_t y = y0; y <= y1; ++y)
    for (std::int32_t x = x0; x <= x1; ++x) Put(x, y, r, g, b, a);
}

void SoftPainter::Line(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, float r,
                       float g, float b, float a, float width_px) {
  // A17 grosor; A22 AA básico por cobertura en vecinos.
  const float dx = static_cast<float>(x1 - x0);
  const float dy = static_cast<float>(y1 - y0);
  const float len = std::sqrt(dx * dx + dy * dy);
  const int steps = std::max(1, static_cast<int>(len * 2.f));
  const float half = std::max(0.5f, width_px * 0.5f);
  for (int i = 0; i <= steps; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(steps);
    const float fx = x0 + dx * t;
    const float fy = y0 + dy * t;
    const int cx = static_cast<int>(std::floor(fx));
    const int cy = static_cast<int>(std::floor(fy));
    const int rad = static_cast<int>(std::ceil(half)) + (profile_.aa_primitives ? 1 : 0);
    for (int oy = -rad; oy <= rad; ++oy) {
      for (int ox = -rad; ox <= rad; ++ox) {
        const float d = std::sqrt(static_cast<float>(ox * ox + oy * oy));
        float cov = 1.f;
        if (profile_.aa_primitives) {
          cov = std::max(0.f, 1.f - (d - half));
          if (cov <= 0.f) continue;
          if (cov > 1.f) cov = 1.f;
        } else if (d > half) {
          continue;
        }
        Put(cx + ox, cy + oy, r, g, b, a * cov);
      }
    }
  }
}

void SoftPainter::Candle(std::int32_t x, std::int32_t y_open, std::int32_t y_close,
                         std::int32_t y_high, std::int32_t y_low, bool up) {
  // Estímulo visual Doc_6 §1.4 (no dato de mercado real).
  const float r = up ? 0.15f : 0.85f;
  const float g = up ? 0.75f : 0.20f;
  const float b = up ? 0.35f : 0.20f;
  Line(x, y_high, x, y_low, 0.7f, 0.7f, 0.75f, 1.f, 1.f);  // mecha A16/A17
  const std::int32_t top = std::min(y_open, y_close);
  const std::int32_t bot = std::max(y_open, y_close);
  FillRect(x - 4, top, x + 4, bot, r, g, b, 1.f);           // A14 relleno
  // Contorno A16
  Line(x - 4, top, x + 4, top, 0.05f, 0.05f, 0.08f, 1.f, 1.f);
  Line(x - 4, bot, x + 4, bot, 0.05f, 0.05f, 0.08f, 1.f, 1.f);
}

void SoftPainter::DrawScene(std::uint64_t generation) {
  // Fondo (clear base Doc_4) + degradado suave A15 (dos stops verticales).
  for (std::uint32_t y = 0; y < h_; ++y) {
    const float t = static_cast<float>(y) / static_cast<float>(h_ - 1);
    const float r = 0.05f * (1.f - t) + 0.08f * t;
    const float g = 0.07f * (1.f - t) + 0.10f * t;
    const float b = 0.10f * (1.f - t) + 0.14f * t;
    for (std::uint32_t x = 0; x < w_; ++x) {
      std::uint8_t* p = fb_.data() + (static_cast<std::size_t>(y) * w_ + x) * 4;
      p[0] = static_cast<std::uint8_t>(r * 255);
      p[1] = static_cast<std::uint8_t>(g * 255);
      p[2] = static_cast<std::uint8_t>(b * 255);
      p[3] = 255;
    }
  }

  // Rejilla
  const int step = 40;
  for (std::uint32_t x = 0; x < w_; x += step)
    Line(static_cast<int>(x), 0, static_cast<int>(x), static_cast<int>(h_ - 1), 0.15f, 0.35f, 0.40f,
         0.55f, 1.f);
  for (std::uint32_t y = 0; y < h_; y += step)
    Line(0, static_cast<int>(y), static_cast<int>(w_ - 1), static_cast<int>(y), 0.15f, 0.35f, 0.40f,
         0.55f, 1.f);

  // Zona (rectángulo semitransparente) A14+A13
  FillRect(static_cast<int>(w_ * 0.1f), static_cast<int>(h_ * 0.25f), static_cast<int>(w_ * 0.9f),
           static_cast<int>(h_ * 0.35f), 0.1f, 0.4f, 0.5f, 0.25f);

  // Nivel horizontal A18 dash
  {
    const int y = static_cast<int>(h_ * 0.55f);
    for (int x = 20; x + 12 < static_cast<int>(w_) - 20; x += 16)
      Line(x, y, x + 8, y, 0.9f, 0.75f, 0.2f, 0.9f, 2.f);
  }

  // Velas estímulo (patrón estable + variación por generation)
  const int base_y = static_cast<int>(h_ * 0.7f);
  const int n = 24;
  for (int i = 0; i < n; ++i) {
    const int x = 60 + i * static_cast<int>((w_ - 120) / n);
    const int wobble = static_cast<int>((generation * 3 + static_cast<unsigned>(i) * 17) % 40);
    const int mid = base_y - wobble;
    const bool up = ((i + static_cast<int>(generation)) % 2) == 0;
    Candle(x, mid + 10, mid - 15, mid - 25, mid + 20, up);
  }

  // Línea tendencia A16/A17
  Line(40, base_y + 30, static_cast<int>(w_) - 40, base_y - 50, 0.2f, 0.85f, 0.9f, 0.95f, 2.f);

  // Sombra UI A19 (offset sin blur; radio=0 · R-6-06) luego marcador A20
  FillRect(static_cast<int>(w_) - 76, 24, static_cast<int>(w_) - 16, 54, 0.f, 0.f, 0.f, 0.35f);
  FillRect(static_cast<int>(w_) - 80, 20, static_cast<int>(w_) - 20, 50, 1.f, 0.85f, 0.2f, 0.7f);
}

Status SoftPainter::PaintAndPresent(std::uint64_t generation, const std::uint8_t* /*payload*/,
                                    std::uint32_t /*payload_bytes*/, bool present) {
  if (!ready_) return Status::NotReady;
  ++stats_.paints;
  stats_.last_generation = generation;
  if (!present) {
    ++stats_.omits;
    return Status::OmittedPresent;
  }
  DrawScene(generation);
  doc6::ApplyProfileToRgba8(fb_.data(), w_, h_, profile_);

  stats_.nonzero_pixels = 0;
  stats_.rgba_checksum = 0;
  for (std::size_t i = 0; i < fb_.size(); ++i) {
    if (fb_[i] != 0) ++stats_.nonzero_pixels;
    stats_.rgba_checksum =
        stats_.rgba_checksum * 1315423911ull + static_cast<std::uint64_t>(fb_[i]) + 1ull;
  }
  ++stats_.presents;
  return Status::Ok;
}

Status SoftPainter::WritePpm(const std::string& path) const {
  if (!ready_ || fb_.empty()) return Status::NotReady;
  std::ofstream out(path, std::ios::binary);
  if (!out) return Status::FailClosed;
  out << "P6\n" << w_ << " " << h_ << "\n255\n";
  for (std::uint32_t y = 0; y < h_; ++y) {
    for (std::uint32_t x = 0; x < w_; ++x) {
      const std::uint8_t* p = fb_.data() + (static_cast<std::size_t>(y) * w_ + x) * 4;
      // PPM RGB; des-premult aproximado para visualización
      float a = p[3] / 255.f;
      if (a < 1e-6f) a = 1.f;
      const std::uint8_t r = static_cast<std::uint8_t>(std::min(255.f, p[0] / a));
      const std::uint8_t g = static_cast<std::uint8_t>(std::min(255.f, p[1] / a));
      const std::uint8_t b = static_cast<std::uint8_t>(std::min(255.f, p[2] / a));
      out.put(static_cast<char>(r));
      out.put(static_cast<char>(g));
      out.put(static_cast<char>(b));
    }
  }
  return Status::Ok;
}

}  // namespace render
}  // namespace epe
