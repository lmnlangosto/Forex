# Motor Gráfico EPE — Economy Painter Engine

Artefacto de módulo Continuo ciego. La definición normativa es **Doc_1–Doc_6** (inviolables). Este árbol **materializa** esa definición en código ejecutable + headers públicos.

## Entrega a terceros (sin fuentes internas)

Tras build Release, entregar:

1. `EPE_BlindRuntime` (y en Windows, además `EPE_Module`/`epe_core` según empaquetado)
2. Carpeta `Public/` (`mod_accounting_abi.h`, `epe_blind_control.h`, `epe_canal_layout.h`)
3. Pliegos de uso/DCD del store del Project (`EPE_MANUAL_*`, `EPE_DCD/`)

**Prohibido** entregar código fuente interno del motor como dependencia de integración.

## Árbol

- `Public/` — contrato binario/headers de entrega
- `EPE/` — Core, Canal Doc_5, Render (D3D12 Windows / Null headless), Blind, Platform
- `EPE_BlindRuntime/` — proceso operable por params / ficheros / socket
- `EPE_TestHost/` — anfitrión de prueba ABI
- `Contrato_Modulos/` — anillo telemetría + remisión al ABI público
- `LABORATORIO/` — contrato lab (no es el orquestador de producto)

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
./build/EPE_BlindRuntime --self-test   # fail=0
./build/EPE_TestHost                   # fail=0
```

Windows (MSVC + D3D12): mismo CMake; sin `EPE_FORCE_NULL_RENDER` usa superficie D3D12 si hay HWND.

## Arranque ciego (resumen)

```bash
./EPE_BlindRuntime --config EPE_BlindRuntime/ejemplos/epe_session.cfg
# socket 127.0.0.1:17900 — comandos HELLO/ATTACH/SESSION_START/TICK/HEALTH/...
```

Detalle normativo de uso: pliegos `EPE_MANUAL_USUARIO.md`, `EPE_MANUAL_MOTOR.md`, `EPE_INSTRUCCIONES_USO.md`, `EPE_DCD/`.
