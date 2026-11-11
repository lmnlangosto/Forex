#include "../../Public/mod_accounting_abi.h"
#include "mod_telemetry_ring.hpp"
#include "epe_host_api.hpp"
#include "../Platform/epe_atomics.hpp"

#include <cstring>
#include <memory>
#include <string>

namespace {

constexpr std::uint32_t kDefaultTelemetryCap = 256;

struct FramePayload {
  std::uint64_t frames;
  std::uint64_t presents;
  std::uint64_t omitted;
  std::uint64_t generation;
};

struct Ctx {
  epe::IEngine* eng = nullptr;
  std::unique_ptr<mod::TelemetryRing> ring;
  std::uint64_t session_id = 0;
  bool session_on = false;
  bool fail_closed = false;
  std::uint64_t telem_seq = 0;
  ModAttachArgs args{};
  std::wstring map_name_storage;
  std::string map_name_utf8_storage;
};

ModIdentity Identity() {
  return {"mod.epe", "EPE Core", 0, 3, 0, MOD_REGIME_CONTINUOUS};
}

int Attach(void** out_ctx, const ModAttachArgs* args) {
  if (!out_ctx || !args) return MOD_INVALID;
  auto* c = new Ctx();
  c->args = *args;
  const std::uint32_t cap =
      args->telemetry_capacity ? args->telemetry_capacity : kDefaultTelemetryCap;
  c->ring = std::make_unique<mod::TelemetryRing>(cap);
  c->eng = epe::CreateEngine();
  if (!c->eng) {
    delete c;
    return MOD_FAIL;
  }
  epe::HostConfig cfg;
  cfg.hwnd = args->host_hwnd;
  cfg.width = args->width ? args->width : 1280;
  cfg.height = args->height ? args->height : 720;
  cfg.channel_map_name = args->channel_map_name;
  cfg.session_id = args->channel_session_id ? args->channel_session_id : 1;
  cfg.layout_version = args->channel_layout_version ? args->channel_layout_version : 1;
  cfg.create_channel = args->create_channel != 0;
  cfg.headless = (args->host_hwnd == nullptr);
  const auto st = c->eng->Init(cfg);
  if (st != epe::Status::Ok) {
    epe::DestroyEngine(c->eng);
    delete c;
    return MOD_FAIL_CLOSED;
  }
  *out_ctx = c;
  return MOD_OK;
}

int Detach(void* ctx) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c) return MOD_INVALID;
  if (c->eng) {
    c->eng->Shutdown();
    epe::DestroyEngine(c->eng);
    c->eng = nullptr;
  }
  c->session_on = false;
  delete c;
  return MOD_OK;
}

int SessionStart(void* ctx, std::uint64_t session_id) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c || !c->eng) return MOD_INVALID;
  if (c->fail_closed) return MOD_FAIL_CLOSED;
  c->session_id = session_id;
  const auto st = c->eng->OpenChannel();
  if (st != epe::Status::Ok) {
    c->fail_closed = true;
    return MOD_FAIL_CLOSED;
  }
  c->session_on = true;
  return MOD_OK;
}

int SessionEnd(void* ctx) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c || !c->eng) return MOD_INVALID;
  c->eng->Shutdown();
  c->session_on = false;
  return MOD_OK;
}

int Health(void* ctx, ModHealth* out) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c || !out) return MOD_INVALID;
  const auto s = c->eng ? c->eng->Stats() : epe::FrameStats{};
  out->alive = (!c->fail_closed && c->eng) ? 1u : 0u;
  out->fail_closed = c->fail_closed ? 1u : 0u;
  out->frames = s.frames;
  out->presents = s.presents;
  out->telemetry_pushed = c->ring ? c->ring->Pushed() : 0;
  out->telemetry_dropped = c->ring ? c->ring->Dropped() : 0;
  out->last_status = c->eng ? static_cast<std::int32_t>(c->eng->LastStatus()) : 0;
  return MOD_OK;
}

int TelemetryTryPop(void* ctx, ModTelemetryRecord* out) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c || !out || !c->ring) return MOD_INVALID;
  return c->ring->TryPop(*out) ? MOD_OK : MOD_NO_DATA;
}

void PushFrameTelemetry(Ctx* c) {
  if (!c->ring || !c->eng) return;
  const auto s = c->eng->Stats();
  ModTelemetryRecord rec{};
  rec.seq = ++c->telem_seq;
  rec.timestamp_qpc = epe::plat::MonoTime();
  rec.kind = 1;
  FramePayload p{s.frames, s.presents, s.omitted_presents, s.last_generation};
  static_assert(sizeof(FramePayload) <= MOD_TELEMETRY_PAYLOAD, "payload");
  std::memcpy(rec.payload, &p, sizeof(p));
  c->ring->PushDropOldest(rec);
}

int ContinuousTick(void* ctx) {
  auto* c = static_cast<Ctx*>(ctx);
  if (!c || !c->eng) return MOD_INVALID;
  if (c->fail_closed) return MOD_FAIL_CLOSED;
  if (!c->session_on) return MOD_BUSY;
  const auto st = c->eng->Frame();
  PushFrameTelemetry(c);
  if (st == epe::Status::FailClosed || st == epe::Status::DeviceLost ||
      st == epe::Status::ChannelInsane) {
    c->fail_closed = true;
    return MOD_FAIL_CLOSED;
  }
  return MOD_OK;
}

int DiscreteBurst(void*, const void*, uint32_t, void*, uint32_t, uint32_t*) {
  return MOD_INVALID;
}

}  // namespace

extern "C" const ModAccountingVTable* Mod_EPE_Continuous_VTable(void) {
  static ModAccountingVTable vt{Identity,     Attach,          Detach, SessionStart, SessionEnd,
                                Health,       TelemetryTryPop, ContinuousTick, DiscreteBurst};
  return &vt;
}
