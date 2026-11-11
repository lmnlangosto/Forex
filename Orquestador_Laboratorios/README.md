# Orquestador de Laboratorios · v0

App mínima operativa (.NET 8). **Un laboratorio a la vez.** DEMO / no dominio.

## Arranque rápido

```bash
cd Orquestador_Laboratorios
dotnet restore
dotnet build -c Release
dotnet run --project Orquestador.App -c Release
```

Windows (mismo):

```powershell
cd Orquestador_Laboratorios
dotnet run --project Orquestador.App -c Release
```

Publicar binario autocontenido:

```bash
dotnet publish Orquestador.App -c Release -o ./artifacts/orquestador-app
./artifacts/orquestador-app/Orquestador.App
```

## Qué hace v0

1. Catálogo por **grupos temáticos** → labs → módulos (datos ejemplo).
2. Clic lab → pantalla completa del lab (módulos + acciones).
3. **Attach / Detach** de un solo lab; health stub; buzón stub (bound + drop-oldest).
4. Módulos mentira Continuo + Discreto.

## Qué NO hace

- **No** inventa API de pintado EPE. Integración = `[A DEFINIR]` hasta pliego cerrado.
- Probe opcional `EPE_BlindRuntime --self-test` solo si:
  - `ORQ_ALLOW_EPE_HARNESS_PROBE=1`
  - `EPE_BLIND_RUNTIME_PATH` apunta al exe
- Cero E: / cero MT5 / cero dominio Bayes-Fuzzy real.

## UI

Consola con paleta Escáner (`#000000` / `#00B4FF` / verde-rojo semántico).  
WPF/WinUI: pendiente máquina Windows (cloud Linux → consola).

## Docs internas

- `docs/PERFILES_Y_TELEMETRIA.md`
- `docs/PENDIENTE_ANALISIS_POR_LAB.md`
- `docs/EPE_INTEGRACION_TODO.md`
- `deposito/MANIFIESTO_DEPOSITO.md` → destino `G:\Mis DOcs\_MG_VIGENTE\…\Laboratorios\Orquestador`

## Norma

Orquestador = lanzar 1 lab, sesión, health, buzón, histórico.  
Lab = dominio + ritmo Continuo/Discreto + módulos.  
Anfitrión ≠ Lab. EPE = único pintor (fuera de este proceso como API).
