#pragma once
// Pintor software · MATERIALIZACIÓN Doc_6 §1.4 (superficie de prueba) + Doc_4 Present.
// Emite framebuffer RGBA8; demuestra embellecedores Doc_6 (no solo clear).
#include "../../Public/epe_doc6_profile.h"
#include "epe_status.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace epe {
namespace render {

struct PaintStats {
  std::uint64_t paints = 0;
  std::uint64_t presents = 0;
  std::uint64_t omits = 0;
  std::uint64_t last_generation = 0;
  std::uint32_t nonzero_pixels = 0;
  std::uint64_t rgba_checksum = 0;
};

class SoftPainter {
 public:
  Status Init(std::uint32_t width, std::uint32_t height, const EpeDoc6Profile& profile);
  void SetProfile(const EpeDoc6Profile& profile);
  // generation/session: contexto Doc_5; payload opcional (raíz mínima → escena §1.4).
  Status PaintAndPresent(std::uint64_t generation, const std::uint8_t* payload,
                         std::uint32_t payload_bytes, bool present);
  Status WritePpm(const std::string& path) const;
  const PaintStats& Stats() const { return stats_; }
  const std::vector<std::uint8_t>& Framebuffer() const { return fb_; }
  bool Ready() const { return ready_; }
  void Shutdown();

 private:
  void DrawScene(std::uint64_t generation);
  void Put(std::int32_t x, std::int32_t y, float r, float g, float b, float a);
  void FillRect(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, float r,
                float g, float b, float a);
  void Line(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, float r, float g,
            float b, float a, float width_px);
  void Candle(std::int32_t x, std::int32_t y_open, std::int32_t y_close, std::int32_t y_high,
              std::int32_t y_low, bool up);

  bool ready_ = false;
  std::uint32_t w_ = 0, h_ = 0;
  EpeDoc6Profile profile_{};
  std::vector<std::uint8_t> fb_;
  PaintStats stats_{};
};

}  // namespace render
}  // namespace epe
