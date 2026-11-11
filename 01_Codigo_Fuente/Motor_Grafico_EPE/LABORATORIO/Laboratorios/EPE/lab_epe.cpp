#include "lab_contract.hpp"
#include "epe_host_api.hpp"
#include "epe_canal_tipo5.hpp"
#include <chrono>
#include <cstdio>

namespace {

struct Ctx {
  // Reservado: el TestHost posee ventana/motor; Lab EPE en alfa reutiliza callbacks locales.
};

lab::LabIdentity Identity() {
  return {"lab.epe", "Laboratorio EPE", 0, 1, 0};
}

lab::LabCapability caps[] = {
    {"init", "Inicialización EPE"},
    {"mmf", "Canal MMF tipo 5"},
    {"render", "Renderizado superficie base"},
    {"effects", "Efectos (aplazado alfa)"},
    {"recover", "Recuperación"},
};

std::uint32_t CapCount() { return static_cast<std::uint32_t>(sizeof(caps) / sizeof(caps[0])); }
const lab::LabCapability* CapAt(std::uint32_t i) { return i < CapCount() ? &caps[i] : nullptr; }

lab::LabStatus Init(void** out) {
  *out = new Ctx();
  return lab::LabStatus::Ok;
}
lab::LabStatus Shutdown(void* ctx) {
  delete static_cast<Ctx*>(ctx);
  return lab::LabStatus::Ok;
}

// Las pruebas A–I se ejecutan desde EPE_TestHost (anfitrión). Aquí se exponen IDs.
lab::LabStatus Stub(void*) { return lab::LabStatus::Skipped; }

lab::LabTestCase tests[] = {
    {"A", "Arranque e inicialización", Stub},
    {"B", "Apertura canal tipo 5", Stub},
    {"C", "Snapshots coherentes", Stub},
    {"D", "Superficie gráfica básica", Stub},
    {"E", "Present / omisión / frecuencia", Stub},
    {"F", "Recuperación", Stub},
    {"G", "Shutdown y recursos", Stub},
    {"H", "Ejecución desde Laboratorio", Stub},
    {"I", "Integración anfitrión de prueba", Stub},
};

std::uint32_t TestCount() { return static_cast<std::uint32_t>(sizeof(tests) / sizeof(tests[0])); }
const lab::LabTestCase* TestAt(std::uint32_t i) { return i < TestCount() ? &tests[i] : nullptr; }

}  // namespace

namespace lab {

const LabVTable* Lab_EPE_VTable() {
  static LabVTable vt{Identity, CapCount, CapAt, TestCount, TestAt, Init, Shutdown};
  return &vt;
}

}  // namespace lab
