#pragma once
// Interfaz de integración · mismo Core bajo TestHost, BlindRuntime o anfitrión de aplicación.
// Materializa Doc_4 (Init → OpenChannel → Frame → Recover → Shutdown).
#include "epe_status.hpp"
#include <cstdint>

namespace epe {

struct HostConfig {
  void* hwnd = nullptr;  // HWND Windows; nullptr permitido con headless=true
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  const wchar_t* channel_map_name = nullptr;
  const char* channel_map_name_utf8 = nullptr;  // preferido en runtime ciego
  std::uint64_t session_id = 1;
  std::uint32_t layout_version = 1;
  bool create_channel = true;
  bool enable_present_throttle = true;  // Doc_5 §5
  bool headless = false;                // fuerza NullSurface (Doc_4 ciclo sin GPU)
  float clear_rgba[4] = {0.07f, 0.09f, 0.12f, 1.0f};
};

struct FrameStats {
  std::uint64_t frames = 0;
  std::uint64_t presents = 0;
  std::uint64_t omitted_presents = 0;
  std::uint64_t snapshots_ok = 0;
  std::uint64_t last_generation = 0;
};

class IEngine {
 public:
  virtual ~IEngine() = default;
  virtual Status Init(const HostConfig& cfg) = 0;
  virtual Status OpenChannel() = 0;
  virtual Status Frame() = 0;
  virtual Status Recover() = 0;
  virtual Status Shutdown() = 0;
  virtual FrameStats Stats() const = 0;
  virtual Status LastStatus() const = 0;
};

IEngine* CreateEngine();
void DestroyEngine(IEngine* eng);

}  // namespace epe
