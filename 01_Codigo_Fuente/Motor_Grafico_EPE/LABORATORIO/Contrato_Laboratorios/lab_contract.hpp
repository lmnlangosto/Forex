#pragma once
// Contrato común de laboratorios · la suite no conoce interiores.
#include <cstdint>

namespace lab {

enum class LabStatus : std::int32_t {
  Ok = 0,
  Failed = 1,
  Skipped = 2,
  NotFound = 3,
  Misconfigured = 4
};

struct LabIdentity {
  const char* id;           // p.ej. "lab.epe"
  const char* display_name; // "Laboratorio EPE"
  std::uint32_t version_major;
  std::uint32_t version_minor;
  std::uint32_t version_patch;
};

struct LabCapability {
  const char* name;
  const char* description;
};

struct LabTestCase {
  const char* id;           // "A", "B", ...
  const char* title;
  LabStatus (*run)(void* ctx);
};

struct LabResult {
  const char* test_id;
  LabStatus status;
  const char* message;
  std::int64_t elapsed_ms;
};

struct LabVTable {
  LabIdentity (*identity)();
  std::uint32_t (*capability_count)();
  const LabCapability* (*capability_at)(std::uint32_t index);
  std::uint32_t (*test_count)();
  const LabTestCase* (*test_at)(std::uint32_t index);
  LabStatus (*init)(void** out_ctx);
  LabStatus (*shutdown)(void* ctx);
};

// Registro estático (alfa): un laboratorio EPE enlazado.
const LabVTable* Lab_EPE_VTable();

}  // namespace lab
