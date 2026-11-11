# Forex / Escáner Autónomo — árbol de código

## Motor Gráfico EPE

Código del Economy Painter Engine (módulo Continuo ciego):

`01_Codigo_Fuente/Motor_Grafico_EPE/`

Build y autoprueba:

```bash
cd 01_Codigo_Fuente/Motor_Grafico_EPE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
./build/EPE_BlindRuntime --self-test
./build/EPE_TestHost
```

La definición normativa del motor es Doc_1–Doc_6 (fuera de este repo / inviolables). Los pliegos de **uso** viven en el store del Project (`EPE_MANUAL_*`, `EPE_DCD/`).
