# Perfiles Continuo / Discreto + telemetría (v0)

Fuente: `ENCARGO_ORQUESTADOR_LABORATORIO.md` (escaner-entrega / 05_Motor_Grafico).

## Perfiles

| Perfil | Quién decide el ritmo | Buzón | Fallo |
|---|---|---|---|
| **Continuo** | El laboratorio (hilo/proceso propio) | Empuje ciego async; orquestador consume baja prio | Fail-Closed → health crítico + fin sesión |
| **Discreto** | Ráfagas Request/Response | Metadatos al terminar ráfaga | Error de ráfaga aislado; orquestador vivo |

El orquestador **no** impone Continuo/Discreto ni un bucle único para todos.

## Anexo telemetría Continuo (implementado stub)

1. Capacidad fija en construcción del buzón (`BoundMailbox`, default 64).
2. Cola llena → **drop-oldest**.
3. Contador `TelemetryDropped` expuesto en health.
4. Drain UI solo fuera del “Present” del módulo (hilo UI / consola).

## ABI de cuenta (stub C#)

`IAccountingModule`: Attach / Detach / SessionStart / SessionEnd / Health / TelemetryTryPop / ContinuousTick / DiscreteBurst.

- Espejo conceptual de `mod_accounting_abi.h` (**[PROVISIONAL]** en Uso_EPE).
- **No** es pliego de pintado EPE ni API de producto cerrada.
