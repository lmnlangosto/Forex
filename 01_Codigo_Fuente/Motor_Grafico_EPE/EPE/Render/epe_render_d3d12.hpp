#pragma once
#include "epe_status.hpp"
#include <cstdint>

struct HWND__;
typedef HWND__* HWND;

namespace epe {
namespace render {

class D3D12Surface {
 public:
  D3D12Surface() = default;
  ~D3D12Surface();

  Status Init(HWND hwnd, std::uint32_t width, std::uint32_t height);
  Status Resize(std::uint32_t width, std::uint32_t height);
  Status ClearAndPresent(const float rgba[4], bool present);
  Status Recover();
  void Shutdown();
  bool Ready() const { return ready_; }

 private:
  bool ready_ = false;
  void* impl_ = nullptr;  // detalle COM aislado en .cpp
};

}  // namespace render
}  // namespace epe
