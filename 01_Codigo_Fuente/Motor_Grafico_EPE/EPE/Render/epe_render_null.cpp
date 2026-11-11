#include "epe_render_null.hpp"
#include <cstring>

namespace epe {
namespace render {

Status NullSurface::Init(void*, std::uint32_t width, std::uint32_t height) {
  if (width == 0 || height == 0) return Status::InvalidArg;
  width_ = width;
  height_ = height;
  ready_ = true;
  presents_ = 0;
  omits_ = 0;
  return Status::Ok;
}

Status NullSurface::ClearAndPresent(const float rgba[4], bool present) {
  if (!ready_) return Status::NotReady;
  if (rgba) std::memcpy(last_rgba_, rgba, sizeof(last_rgba_));
  if (present) {
    ++presents_;
    return Status::Ok;
  }
  ++omits_;
  return Status::OmittedPresent;
}

Status NullSurface::Recover() {
  if (!ready_) return Status::NotReady;
  return Status::Ok;
}

void NullSurface::Shutdown() {
  ready_ = false;
  width_ = height_ = 0;
}

}  // namespace render
}  // namespace epe
