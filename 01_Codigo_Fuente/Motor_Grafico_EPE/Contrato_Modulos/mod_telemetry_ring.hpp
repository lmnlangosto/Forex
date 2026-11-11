#pragma once
#include "../Public/mod_accounting_abi.h"
#include <atomic>
#include <cstdint>
#include <vector>

namespace mod {

// Anillo SPSC fijo: productor (módulo Continuo) / consumidor (orquestador).
// Allocation bound en construct. Drop-oldest si lleno. Nunca bloquea al productor.
class TelemetryRing {
 public:
  explicit TelemetryRing(std::uint32_t capacity_records);
  void PushDropOldest(const ModTelemetryRecord& rec);
  bool TryPop(ModTelemetryRecord& out);
  std::uint64_t Pushed() const { return pushed_.load(std::memory_order_relaxed); }
  std::uint64_t Dropped() const { return dropped_.load(std::memory_order_relaxed); }
  std::uint32_t Capacity() const { return cap_; }

 private:
  std::uint32_t cap_ = 0;
  std::vector<ModTelemetryRecord> slots_;
  std::atomic<std::uint64_t> head_{0};
  std::atomic<std::uint64_t> tail_{0};
  std::atomic<std::uint64_t> pushed_{0};
  std::atomic<std::uint64_t> dropped_{0};
};

}  // namespace mod
