#include "mod_telemetry_ring.hpp"
#include <cstring>

namespace mod {

TelemetryRing::TelemetryRing(std::uint32_t capacity_records) {
  cap_ = capacity_records < 16 ? 16 : capacity_records;
  slots_.assign(cap_, ModTelemetryRecord{});
}

void TelemetryRing::PushDropOldest(const ModTelemetryRecord& rec) {
  const std::uint64_t h = head_.load(std::memory_order_relaxed);
  const std::uint64_t t = tail_.load(std::memory_order_acquire);
  if (h - t >= cap_) {
    // drop oldest: avanzar tail
    tail_.store(t + 1, std::memory_order_release);
    dropped_.fetch_add(1, std::memory_order_relaxed);
  }
  slots_[static_cast<std::size_t>(h % cap_)] = rec;
  head_.store(h + 1, std::memory_order_release);
  pushed_.fetch_add(1, std::memory_order_relaxed);
}

bool TelemetryRing::TryPop(ModTelemetryRecord& out) {
  const std::uint64_t t = tail_.load(std::memory_order_relaxed);
  const std::uint64_t h = head_.load(std::memory_order_acquire);
  if (t >= h) return false;
  out = slots_[static_cast<std::size_t>(t % cap_)];
  tail_.store(t + 1, std::memory_order_release);
  return true;
}

}  // namespace mod
