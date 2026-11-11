#pragma once
#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
namespace epe {
namespace plat {
inline std::uint64_t AtomicLoadU64(volatile std::uint64_t* p) {
  return static_cast<std::uint64_t>(
      InterlockedOr64(reinterpret_cast<volatile LONG64*>(p), 0));
}
inline void AtomicStoreU64(volatile std::uint64_t* p, std::uint64_t v) {
  InterlockedExchange64(reinterpret_cast<volatile LONG64*>(p), static_cast<LONG64>(v));
}
inline void FullBarrier() { MemoryBarrier(); }
inline std::uint64_t MonoTime() {
  LARGE_INTEGER qpc;
  QueryPerformanceCounter(&qpc);
  return static_cast<std::uint64_t>(qpc.QuadPart);
}
}  // namespace plat
}  // namespace epe
#else
#include <chrono>
namespace epe {
namespace plat {
inline std::uint64_t AtomicLoadU64(volatile std::uint64_t* p) {
  return __atomic_load_n(reinterpret_cast<volatile unsigned long long*>(p), __ATOMIC_ACQUIRE);
}
inline void AtomicStoreU64(volatile std::uint64_t* p, std::uint64_t v) {
  __atomic_store_n(reinterpret_cast<volatile unsigned long long*>(p),
                   static_cast<unsigned long long>(v), __ATOMIC_RELEASE);
}
inline void FullBarrier() { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
inline std::uint64_t MonoTime() {
  using clock = std::chrono::steady_clock;
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch())
          .count());
}
}  // namespace plat
}  // namespace epe
#endif
