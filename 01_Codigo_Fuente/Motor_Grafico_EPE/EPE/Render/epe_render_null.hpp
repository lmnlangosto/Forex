#pragma once
#include "epe_status.hpp"
#include <cstdint>

namespace epe {
namespace render {

// Superficie nula: materializa ciclo Doc_4 sin GPU (ensayo cloud / headless).
// No sustituye D3D12 en producto Windows; permite verificar contrato ciego.
class NullSurface {
 public:
  Status Init(void* /*hwnd*/, std::uint32_t width, std::uint32_t height);
  Status ClearAndPresent(const float rgba[4], bool present);
  Status Recover();
  void Shutdown();
  bool Ready() const { return ready_; }
  std::uint32_t Width() const { return width_; }
  std::uint32_t Height() const { return height_; }
  std::uint64_t PresentCount() const { return presents_; }
  std::uint64_t OmitCount() const { return omits_; }

 private:
  bool ready_ = false;
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
  float last_rgba_[4] = {0, 0, 0, 1};
  std::uint64_t presents_ = 0;
  std::uint64_t omits_ = 0;
};

}  // namespace render
}  // namespace epe
