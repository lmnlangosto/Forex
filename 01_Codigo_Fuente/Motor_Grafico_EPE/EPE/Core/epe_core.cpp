#include "epe_host_api.hpp"
#include "epe_canal_tipo5.hpp"
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
    use_null_ = cfg_.headless || cfg_.hwnd == nullptr || !EPE_HAS_D3D12;
    if (!use_null_) {
#if EPE_HAS_D3D12
      last_ = d3d_.Init(static_cast<HWND>(cfg_.hwnd), cfg_.width, cfg_.height);
#else
      last_ = Status::Unsupported;
#endif
      if (last_ != Status::Ok) return last_;
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
    const bool surface_ready = use_null_ ? null_.Ready() :
#if EPE_HAS_D3D12
                                            d3d_.Ready();
#else
                                            false;
#endif
    if (!inited_ || !channel_.IsOpen() || !surface_ready) {
      last_ = Status::NotReady;
      return last_;
    }
    ++stats_.frames;

    canal::Snapshot snap;
    Status st = channel_.ConsumerTakeSnapshot(snap, payload_.data(),
                                              static_cast<std::uint32_t>(payload_.size()));
    bool present = true;
    if (st == Status::Ok) {
      ++stats_.snapshots_ok;
      stats_.last_generation = snap.generation;
      have_accepted_ = true;
    } else if (st == Status::NoNewData) {
      if (cfg_.enable_present_throttle && have_accepted_) {
        present = false;
        ++stats_.omitted_presents;
      }
    } else {
      last_ = st;
      return last_;
    }

    if (use_null_) {
      last_ = null_.ClearAndPresent(cfg_.clear_rgba, present);
    } else {
#if EPE_HAS_D3D12
      last_ = d3d_.ClearAndPresent(cfg_.clear_rgba, present);
#else
      last_ = Status::Unsupported;
#endif
    }
    if (last_ == Status::Ok && present) ++stats_.presents;
    if (last_ == Status::OmittedPresent) last_ = Status::Ok;
    return last_;
  }

  Status Recover() override {
    if (!inited_) {
      last_ = Status::NotReady;
      return last_;
    }
    if (use_null_) {
      last_ = null_.Recover();
      return last_;
    }
#if EPE_HAS_D3D12
    last_ = d3d_.Recover();
    if (last_ != Status::Ok) {
      last_ = d3d_.Init(static_cast<HWND>(cfg_.hwnd), cfg_.width, cfg_.height);
    }
#else
    last_ = Status::Unsupported;
#endif
    return last_;
  }

  Status Shutdown() override {
    channel_.Close();
    if (use_null_) null_.Shutdown();
#if EPE_HAS_D3D12
    else d3d_.Shutdown();
#endif
    inited_ = false;
    have_accepted_ = false;
    last_ = Status::Ok;
    return last_;
  }

  FrameStats Stats() const override { return stats_; }
  Status LastStatus() const override { return last_; }

 private:
  HostConfig cfg_{};
  bool use_null_ = true;
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
