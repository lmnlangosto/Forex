#pragma once
/* ABI de cuenta de módulos — C estable · entrega con el artefacto (sin fuentes internas).
 * Materializa el perfil Continuo de Doc_4 (ciclo) + anexo telemetría (bound/drop-oldest).
 * Orquestador / tercero: solo este header + binario. No abre el código del motor.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  MOD_OK = 0,
  MOD_FAIL = 1,
  MOD_BUSY = 2,
  MOD_NO_DATA = 3,
  MOD_INVALID = 4,
  MOD_FAIL_CLOSED = 5
};

enum {
  MOD_REGIME_CONTINUOUS = 1, /* EPE: hilo propio, buzón outbound, sin wait */
  MOD_REGIME_DISCRETE = 2    /* Bayes/Fuzzy/ráfaga · EPE no implementa */
};

typedef struct ModIdentity {
  const char* id;           /* "mod.epe" */
  const char* display_name; /* "EPE Core" */
  uint32_t version_major;
  uint32_t version_minor;
  uint32_t version_patch;
  uint32_t regime;          /* MOD_REGIME_* */
} ModIdentity;

typedef struct ModAttachArgs {
  void* host_hwnd;          /* HWND en Windows; NULL en headless/null-render */
  uint32_t width;
  uint32_t height;
  const wchar_t* channel_map_name; /* nombre MMF Doc_5; NULL = default */
  uint64_t channel_session_id;
  uint32_t channel_layout_version;
  uint32_t create_channel;  /* 1 = crear mapa si falta (Init productor/ensayo) */
  uint32_t telemetry_capacity; /* records; 0 = default del módulo */
} ModAttachArgs;

typedef struct ModHealth {
  uint32_t alive;             /* 1 = vivo */
  uint32_t fail_closed;       /* 1 = colapso Fail-Closed */
  uint64_t frames;
  uint64_t presents;
  uint64_t telemetry_pushed;
  uint64_t telemetry_dropped; /* drop-oldest count */
  int32_t last_status;        /* epe::Status como int32 */
} ModHealth;

enum { MOD_TELEMETRY_PAYLOAD = 64 };

typedef struct ModTelemetryRecord {
  uint64_t seq;
  uint64_t timestamp_qpc; /* QueryPerformanceCounter / reloj monotónico */
  uint32_t kind;          /* 1 = frame stats (payload = FramePayload) */
  uint32_t reserved;
  uint8_t payload[MOD_TELEMETRY_PAYLOAD];
} ModTelemetryRecord;

/* kind=1 payload (little-endian, empaquetado): */
typedef struct ModFrameTelemetryPayload {
  uint64_t frames;
  uint64_t presents;
  uint64_t omitted_presents;
  uint64_t last_generation;
} ModFrameTelemetryPayload;

typedef struct ModAccountingVTable {
  ModIdentity (*identity)(void);
  int (*attach)(void** out_ctx, const ModAttachArgs* args);
  int (*detach)(void* ctx);
  int (*session_start)(void* ctx, uint64_t session_id);
  int (*session_end)(void* ctx);
  int (*health)(void* ctx, ModHealth* out); /* no bloqueante */
  int (*telemetry_try_pop)(void* ctx, ModTelemetryRecord* out);
  int (*continuous_tick)(void* ctx);
  int (*discrete_burst)(void* ctx, const void* in_vec, uint32_t in_bytes, void* out_vec,
                        uint32_t out_cap, uint32_t* out_bytes);
} ModAccountingVTable;

/* Export del módulo EPE Continuo (DLL/SO o estático enlazado al runtime ciego). */
const ModAccountingVTable* Mod_EPE_Continuous_VTable(void);

#ifdef __cplusplus
}
#endif
