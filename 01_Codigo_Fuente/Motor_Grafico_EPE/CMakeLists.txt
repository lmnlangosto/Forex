cmake_minimum_required(VERSION 3.20)
project(Motor_Grafico_EPE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(EPE_FORCE_NULL_RENDER "Usar NullSurface aunque exista D3D12" OFF)

set(EPE_CORE_SOURCES
  EPE/Canal/epe_canal_tipo5.cpp
  EPE/Render/epe_render_null.cpp
  EPE/Platform/epe_shm.cpp
  EPE/Core/epe_core.cpp
  EPE/Modulo/epe_module_continuo.cpp
  Contrato_Modulos/mod_telemetry_ring.cpp
  LABORATORIO/Laboratorios/EPE/lab_epe.cpp
)

if(WIN32 AND NOT EPE_FORCE_NULL_RENDER)
  list(APPEND EPE_CORE_SOURCES EPE/Render/epe_render_d3d12.cpp)
endif()

add_library(epe_core STATIC ${EPE_CORE_SOURCES})

target_include_directories(epe_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Public
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Interfaces
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Canal
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Render
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Core
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Modulo
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Platform
  ${CMAKE_CURRENT_SOURCE_DIR}/EPE/Blind
  ${CMAKE_CURRENT_SOURCE_DIR}/Contrato_Modulos
  ${CMAKE_CURRENT_SOURCE_DIR}/LABORATORIO/Contrato_Laboratorios
)

target_compile_definitions(epe_core PRIVATE NOMINMAX)
if(WIN32)
  target_compile_definitions(epe_core PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
  if(EPE_FORCE_NULL_RENDER)
    target_compile_definitions(epe_core PRIVATE EPE_FORCE_NULL_RENDER=1)
  endif()
  target_link_libraries(epe_core PUBLIC d3d12 dxgi)
else()
  target_compile_definitions(epe_core PRIVATE EPE_FORCE_NULL_RENDER=1)
  target_link_libraries(epe_core PUBLIC rt pthread)
endif()

add_executable(EPE_BlindRuntime EPE_BlindRuntime/main.cpp)
target_link_libraries(EPE_BlindRuntime PRIVATE epe_core)
target_include_directories(EPE_BlindRuntime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  target_link_libraries(EPE_BlindRuntime PRIVATE ws2_32)
  target_compile_definitions(EPE_BlindRuntime PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_executable(EPE_TestHost EPE_TestHost/main.cpp)
target_link_libraries(EPE_TestHost PRIVATE epe_core)
target_compile_definitions(EPE_TestHost PRIVATE NOMINMAX)
if(WIN32)
  target_compile_definitions(EPE_TestHost PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
endif()

# Entrega pública: copiar headers con el build
add_custom_command(TARGET EPE_BlindRuntime POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:EPE_BlindRuntime>/Public
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/Public
          $<TARGET_FILE_DIR:EPE_BlindRuntime>/Public
)
