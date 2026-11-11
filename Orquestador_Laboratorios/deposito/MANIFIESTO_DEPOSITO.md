# Manifiesto depósito · Orquestador de Laboratorios v0

**Fecha:** 2026-11-11  
**Origen cloud:** repo `Forex` · carpeta `Orquestador_Laboratorios/` · rama `cursor/orquestador-labs-v0-147b`

## Destino canónico (espejo Mis DOcs)

```
G:\Mis DOcs\_MG_VIGENTE\documentación maestro\Laboratorios\Orquestador\
```

(Nota histórica de preferencias: también aparece `Orquestador_ Laboratorio` — unificar en `Orquestador\` al depositar.)

## Árbol a copiar

```
Orquestador\
  README.md                          ← arranque
  OrquestadorLaboratorios.sln
  Orquestador.Core\                  ← catálogo, sesión 1-lab, stubs, buzón, gate EPE
  Orquestador.App\                   ← UI consola operativa
  docs\
    PERFILES_Y_TELEMETRIA.md
    PENDIENTE_ANALISIS_POR_LAB.md
    EPE_INTEGRACION_TODO.md
  deposito\
    MANIFIESTO_DEPOSITO.md           ← este fichero
  artifacts\                         ← (opcional) salida de `dotnet publish`
```

## Procedimiento local (Windows)

1. `git fetch` + checkout de la rama/PR del Orquestador.
2. Crear destino si no existe: `…\Laboratorios\Orquestador\`.
3. Copiar árbol anterior (sin `bin/`/`obj/` salvo que se quiera el publish).
4. Build local: `dotnet publish Orquestador.App -c Release -o artifacts\orquestador-app`.
5. **No** depositar fuentes del Motor Gráfico / EPE aquí.
6. EPE BlindRuntime (si se prueba): ruta aparte bajo `Motor_Grafico\Entrega_Ejecutable\`; no inventar API.

## Qué queda fuera del depósito

- Código fuente EPE / Doc_1–6 editables.
- API de pintado inventada.
- Dependencias E: ni MT5.
