cmake_minimum_required(VERSION 3.16)
project(AEGIS_SEAS VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific optimization flags (-O3 -mavx2 -fopenmp or /O2 /arch:AVX2 /openmp)
if(MSVC)
    add_compile_options(/O2 /arch:AVX2 /openmp /W4)
else()
    add_compile_options(-O3 -mavx2 -fopenmp -Wall -Wextra)
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Core Sources
set(CORE_SOURCES
    src/engine_sar/sar_reader.cpp
    src/engine_sar/cfar_detector.cpp
    src/engine_drift/weathering.cpp
    src/engine_drift/rk4_solver.cpp
    src/engine_kinematics/wake_inversion.cpp
    src/engine_dossier/statutory_dossier.cpp
    src/physics/calibrator.cpp
    src/physics/speckle_filter.cpp
    src/physics/polarimetry.cpp
    src/nn/layers.cpp
    src/nn/unet.cpp
    src/nn/sliding_window.cpp
    src/geo/slick_analyzer.cpp
    src/geo/geojson_writer.cpp
    src/simulation/synthetic_scene.cpp
    src/drift/lagrangian_drift.cpp
    src/attribution/cfar_detector.cpp
    src/attribution/ais_engine.cpp
    src/io/ais_parser.cpp
    src/pipeline/pipeline_orchestrator.cpp
)

add_library(aegis_core STATIC ${CORE_SOURCES})

# Executable Targets
add_executable(sar_oil_detector src/main.cpp)
target_link_libraries(sar_oil_detector PRIVATE aegis_core)

add_executable(test_stress_suite tests/red_team/test_stress_suite.cpp)
target_link_libraries(test_stress_suite PRIVATE aegis_core)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
