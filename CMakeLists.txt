cmake_minimum_required(VERSION 3.24)

project(SimpleSolid2
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

option(
    SS2_BUILD_DESKTOP
    "Build the Qt/OCCT desktop application and desktop/provider tests"
    ON
)

option(
    SS2_BUILD_KERNEL_NATIVE
    "Build the OCCT-backed kernel-native evidence lane without Qt"
    OFF
)

if(SS2_BUILD_DESKTOP AND SS2_BUILD_KERNEL_NATIVE)
    message(FATAL_ERROR
        "SS2_BUILD_DESKTOP and SS2_BUILD_KERNEL_NATIVE are mutually exclusive build modes")
endif()

option(
    SS2_ENABLE_C2_HISTORY_BENCHMARK
    "Build the C2 semantic history benchmark evidence target"
    OFF
)

option(
    SS2_ENABLE_E2_PRESENTATION_BENCHMARK
    "Build the E2 presentation measurement evidence target"
    OFF
)

option(
    SS2_ENABLE_SR02_LATENCY_BENCHMARK
    "Build the SR-02 current-stack interaction latency evidence target"
    OFF
)

add_subdirectory(src)
if(SS2_ENABLE_C2_HISTORY_BENCHMARK OR
   SS2_ENABLE_E2_PRESENTATION_BENCHMARK OR
   SS2_ENABLE_SR02_LATENCY_BENCHMARK)
    add_subdirectory(benchmarks)
endif()
if(SS2_BUILD_DESKTOP)
    add_subdirectory(tests)
elseif(SS2_BUILD_KERNEL_NATIVE)
    add_subdirectory(tests/core_only)
    add_subdirectory(tests/kernel_native)
else()
    add_subdirectory(tests/core_only)
endif()
