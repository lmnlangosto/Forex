// EPE_TestHost — anfitrión de prueba ABI Continuo (headless portable + ventana Win opcional).
#include "mod_accounting_abi.h"
#include "epe_canal_tipo5.hpp"
#include "epe_shm.hpp"

#include <cstdio>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(EPE_FORCE_NULL_RENDER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
namespace {
HWND g_hwnd = nullptr;
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(h, m, w, l);
}
bool CreateHostWindow(std::uint32_t w, std::uint32_t h) {
  WNDCLASSW wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = L"EPE_TestHostWnd";
  RegisterClassW(&wc);
  g_hwnd = CreateWindowExW(0, wc.lpszClassName, L"EPE TestHost", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(w), static_cast<int>(h),
                           nullptr, nullptr, wc.hInstance, nullptr);
  return g_hwnd != nullptr;
}
void Pump() {
  MSG msg;
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}
}  // namespace
#else
namespace {
void* g_hwnd = nullptr;
bool CreateHostWindow(std::uint32_t, std::uint32_t) { return true; }
void Pump() {}
}  // namespace
#endif

namespace {
epe::canal::Channel g_producer;
constexpr std::uint64_t kSession = 1;
constexpr const char* kMap = "Local\\EPE_Canal_Tipo5_v1";

struct CaseResult {
  const char* id;
  bool ok;
  std::string detail;
};
}  // namespace

int main() {
  std::printf("EPE_TestHost · ABI cuenta Continuo (headless OK)\n");
  if (!CreateHostWindow(1280, 720)) return 2;

  const ModAccountingVTable* vt = Mod_EPE_Continuous_VTable();
  if (!vt) return 3;
  const ModIdentity id = vt->identity();
  if (id.regime != MOD_REGIME_CONTINUOUS) {
    std::printf("FAIL regime\n");
    return 4;
  }

  if (g_producer.OpenUtf8(epe::canal::Role::Producer, kMap, kSession, epe::canal::kLayoutVersion,
                          true) != epe::Status::Ok) {
    std::printf("FAIL producer\n");
    return 5;
  }

  std::wstring map_w = epe::plat::Utf8ToWide(kMap);
  ModAttachArgs args{};
  args.host_hwnd = g_hwnd;
  args.width = 1280;
  args.height = 720;
  args.channel_map_name = map_w.c_str();
  args.channel_session_id = kSession;
  args.channel_layout_version = epe::canal::kLayoutVersion;
  args.create_channel = 0;
  args.telemetry_capacity = 32;

  void* ctx = nullptr;
  std::vector<CaseResult> results;
  auto pass = [&](const char* cid, bool ok, const std::string& d) {
    results.push_back({cid, ok, d});
  };

  pass("ATTACH", vt->attach(&ctx, &args) == MOD_OK, "attach");
  pass("SESSION", ctx && vt->session_start(ctx, 1001) == MOD_OK, "session_start");

  g_producer.ProducerPublishMinimal(1);
  Pump();
  pass("TICK1", ctx && vt->continuous_tick(ctx) == MOD_OK, "tick+telem");

  ModHealth h{};
  pass("HEALTH", ctx && vt->health(ctx, &h) == MOD_OK && h.alive == 1, "alive");

  for (int i = 0; i < 64; ++i) {
    g_producer.ProducerPublishMinimal(static_cast<std::uint64_t>(2 + i));
    Pump();
    vt->continuous_tick(ctx);
  }
  vt->health(ctx, &h);
  pass("DROP", h.telemetry_dropped > 0, "dropped=" + std::to_string(h.telemetry_dropped));

  ModTelemetryRecord rec{};
  int pops = 0;
  while (vt->telemetry_try_pop(ctx, &rec) == MOD_OK) ++pops;
  pass("POP", pops > 0 && pops <= 32, "pops=" + std::to_string(pops));

  for (int i = 0; i < 40; ++i) {
    Pump();
    if (vt->continuous_tick(ctx) == MOD_FAIL_CLOSED) break;
  }
  pass("NOBLOCK", vt->health(ctx, &h) == MOD_OK && h.alive == 1, "ticks sin wait");
  pass("DISC_REJECT", vt->discrete_burst(ctx, nullptr, 0, nullptr, 0, nullptr) == MOD_INVALID,
       "continuo≠ráfaga");
  pass("END", vt->session_end(ctx) == MOD_OK, "session_end");
  pass("DETACH", vt->detach(ctx) == MOD_OK, "detach");
  ctx = nullptr;
  g_producer.Close();

  int fails = 0;
  for (const auto& r : results) {
    if (!r.ok) ++fails;
    std::printf("[%s] %s · %s\n", r.id, r.ok ? "PASS" : "FAIL", r.detail.c_str());
  }
  std::printf("TOTAL fail=%d · mod=%s v%u.%u.%u\n", fails, id.display_name, id.version_major,
              id.version_minor, id.version_patch);
  return fails == 0 ? 0 : 1;
}
