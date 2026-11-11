#include "epe_host_api.hpp"
#include "epe_canal_tipo5.hpp"
#include "epe_paint_soft.hpp"
#include "epe_render_null.hpp"
#include <vector>

#if defined(_WIN32) && !defined(EPE_FORCE_NULL_RENDER)
#include "epe_render_d3d12.hpp"
#define EPE_HAS_D3D12 1
#else
#define EPE_HAS_D3D12 0
#endif

namespace epe {

class Engine final : public IEngine {
 public:
  Status Init(const HostConfig& cfg) override {
    last_ = Status::Ok;
    cfg_ = cfg;
    if (!cfg_.doc6_profile_set) EpeDoc6ProfileIdentity(&cfg_.doc6_profile);
    // SoftPainter materializa Doc_6 §1.4 + embellecedores (siempre en Núcleo v1 headless/ensayo).
    last_ = painter_.Init(cfg_.width ? cfg_.width : 1280, cfg_.height ? cfg_.height : 720,
                          cfg_.doc6_profile);
    if (last_ != Status::Ok) return last_;

    use_d3d_ = (!cfg_.headless && cfg_.hwnd != nullptr && EPE_HAS_D3D12);
    if (use_d3d_) {
#if EPE_HAS_D3D12
      last_ = d3d_.Init(static_cast<HWND>(cfg_.hwnd), cfg_.width, cfg_.height);
      if (last_ != Status::Ok) return last_;
#endif
    } else {
      last_ = null_.Init(cfg_.hwnd, cfg_.width ? cfg_.width : 1280, cfg_.height ? cfg_.height : 720);
      if (last_ != Status::Ok) return last_;
    }
    payload_.assign(canal::kSlotPayloadCapacity, 0);
    inited_ = true;
    return Status::Ok;
  }

  Status OpenChannel() override {
    if (!inited_) {
      last_ = Status::NotReady;
      return last_;
    }
    if (cfg_.channel_map_name_utf8) {
      last_ = channel_.OpenUtf8(canal::Role::Consumer, cfg_.channel_map_name_utf8, cfg_.session_id,
                               cfg_.layout_version, cfg_.create_channel);
    } else {
      last_ = channel_.Open(canal::Role::Consumer, cfg_.channel_map_name, cfg_.session_id,
                           cfg_.layout_version, cfg_.create_channel);
    }
    return last_;
  }

  Status Frame() override {
    if (!inited_ || !channel_.IsOpen() || !painter_.Ready()) {
      last_ = Status::NotReady;
      return last_;
    }
    ++stats_.frames;

    canal::Snapshot snap;
    Status st = channel_.ConsumerTakeSnapshot(snap, payload_.data(),
                                              static_cast<std::uint32_t>(payload_.size()));
    bool present = true;
    std::uint64_t gen = stats_.last_generation;
    std::uint32_t pay_n = 0;
    const std::uint8_t* pay = nullptr;
    if (st == Status::Ok) {
      ++stats_.snapshots_ok;
      stats_.last_generation = snap.generation;
      gen = snap.generation;
      pay_n = snap.payload_bytes;
      pay = payload_.data();
      have_accepted_ = true;
    } else if (st == Status::NoNewData) {
      if (cfg_.enable_present_throttle && have_accepted_) {
        present = false;
        ++stats_.omitted_presents;
      }
      gen = snap.generation ? snap.generation : stats_.last_generation;
    } else {
      last_ = st;
      return last_;
    }

    last_ = painter_.PaintAndPresent(gen, pay, pay_n, present);
    if (last_ == Status::Ok && present) {
      ++stats_.presents;
      stats_.paint_nonzero_pixels = painter_.Stats().nonzero_pixels;
      stats_.paint_checksum = painter_.Stats().rgba_checksum;
      if (!cfg_.proof_ppm_path.empty()) {
        painter_.WritePpm(cfg_.proof_ppm_path);
      }
      if (use_d3d_) {
#if EPE_HAS_D3D12
        d3d_.ClearAndPresent(cfg_.clear_rgba, true);
#endif
      } else {
        null_.ClearAndPresent(cfg_.clear_rgba, true);
      }
    }
    if (last_ == Status::OmittedPresent) {
      if (use_d3d_) {
#if EPE_HAS_D3D12
        d3d_.ClearAndPresent(cfg_.clear_rgba, false);
#endif
      } else {
        null_.ClearAndPresent(cfg_.clear_rgba, false);
      }
      last_ = Status::Ok;
    }
    return last_;
  }

  Status Recover() override {
    if (!inited_) return Status::NotReady;
    last_ = painter_.Init(cfg_.width, cfg_.height, cfg_.doc6_profile);
    return last_;
  }

  Status Shutdown() override {
    channel_.Close();
    painter_.Shutdown();
    if (use_d3d_) {
#if EPE_HAS_D3D12
      d3d_.Shutdown();
#endif
    } else {
      null_.Shutdown();
    }
    inited_ = false;
    have_accepted_ = false;
    last_ = Status::Ok;
    return last_;
  }

  FrameStats Stats() const override { return stats_; }
  Status LastStatus() const override { return last_; }

  Status SetDoc6Profile(const EpeDoc6Profile& profile) override {
    cfg_.doc6_profile = profile;
    cfg_.doc6_profile_set = true;
    painter_.SetProfile(profile);
    return Status::Ok;
  }

  Status WriteProofPpm(const char* path) override {
    if (!path) return Status::InvalidArg;
    return painter_.WritePpm(path);
  }

 private:
  HostConfig cfg_{};
  bool use_d3d_ = false;
  render::SoftPainter painter_{};
  render::NullSurface null_{};
#if EPE_HAS_D3D12
  render::D3D12Surface d3d_{};
#endif
  canal::Channel channel_{};
  std::vector<std::uint8_t> payload_;
  FrameStats stats_{};
  Status last_ = Status::NotReady;
  bool inited_ = false;
  bool have_accepted_ = false;
};

IEngine* CreateEngine() { return new Engine(); }
void DestroyEngine(IEngine* eng) { delete eng; }

}  // namespace epe
