#pragma once
#include "epe_status.hpp"
#include "../../Public/epe_doc6_profile.h"
#include <cstdint>
#include <string>

namespace epe {

struct HostConfig {
  void* hwnd = nullptr;
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  const wchar_t* channel_map_name = nullptr;
  const char* channel_map_name_utf8 = nullptr;
  std::uint64_t session_id = 1;
  std::uint32_t layout_version = 1;
  bool create_channel = true;
  bool enable_present_throttle = true;
  bool headless = false;
  float clear_rgba[4] = {0.07f, 0.09f, 0.12f, 1.0f};
  EpeDoc6Profile doc6_profile{};  // MATERIALIZACIÓN Doc_6 §4.4
  bool doc6_profile_set = false;
  std::string proof_ppm_path;  // si no vacío, escribe PPM en cada Present (ensayo)
};

struct FrameStats {
  std::uint64_t frames = 0;
  std::uint64_t presents = 0;
  std::uint64_t omitted_presents = 0;
  std::uint64_t snapshots_ok = 0;
  std::uint64_t last_generation = 0;
  std::uint32_t paint_nonzero_pixels = 0;
  std::uint64_t paint_checksum = 0;
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
  virtual Status SetDoc6Profile(const EpeDoc6Profile& profile) = 0;
  virtual Status WriteProofPpm(const char* path) = 0;
};

IEngine* CreateEngine();
void DestroyEngine(IEngine* eng);

}  // namespace epe
